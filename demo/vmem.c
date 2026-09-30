/*
 * vmem.c - virtual memory, observed from inside one process.
 *
 * [CO-4] Virtual memory as an abstraction. Every address this program prints
 * is virtual. The process believes it owns a huge private address space. The
 * kernel builds that illusion out of page tables, which translate each 4 KiB
 * virtual page to a physical frame, and fills them in lazily.
 *
 * The tour, in order:
 *   1. The layout of the address space: code, data, heap, libraries, stack,
 *      and where malloc() gets small and large blocks from.
 *   2. Demand paging: mmap() reserves 64 MiB instantly, and the memory only
 *      becomes real one page fault at a time as it is touched.
 *   3. Page tables: /proc/self/pagemap shows which pages have a frame.
 *   4. Copy-on-write after fork(): the child shares every page until it
 *      writes one.
 *   5. Memory errors: how the kernel reports a bad access with SIGSEGV, and
 *      the difference between an unmapped address and a protected one.
 *
 * Two extra modes plant real bugs for the debugging tools to find. The kernel
 * never notices either one, because neither touches an unmapped page.
 *
 * Build: make demo
 * Run:   ./bin/vmem                 the full tour
 *        ./bin/vmem leak            leaks memory on purpose
 *        ./bin/vmem overflow [n]    writes one element past a heap array
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_initialized = 42;   /* lives in .data, stored in the file */
static int g_uninitialized;      /* lives in .bss, zero-filled at start */

static long page_size;

/* Reads land here so the compiler cannot drop them as unused. */
static volatile unsigned g_sink;

/* Minor page faults so far: faults the kernel resolved without disk I/O. */
static long minor_faults(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_minflt;
}

/* Resident set size in KiB, the second field of /proc/self/statm. */
static long rss_kib(void) {
    char buf[128];
    long size = 0, resident = -1;
    int fd = open("/proc/self/statm", O_RDONLY);
    ssize_t n;

    if (fd == -1) {
        return -1;
    }
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        return -1;
    }
    buf[n] = '\0';
    if (sscanf(buf, "%ld %ld", &size, &resident) != 2) {
        return -1;
    }
    return resident * (page_size / 1024);
}

static void *map_anonymous(size_t len) {
    void *p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    /* Keep to 4 KiB pages. With transparent huge pages one fault could map
       2 MiB at once, which would hide the one-fault-per-page pattern. */
    madvise(p, len, MADV_NOHUGEPAGE);
    return p;
}

static void part_layout(void) {
    int local = 0;
    void *brk_before = sbrk(0);
    char *small = malloc(64);
    char *large = malloc(4 << 20);
    void *brk_after = sbrk(0);
    char line[512];
    FILE *maps;

    printf("Part 1. The address space of pid %d, low addresses first\n", (int)getpid());
    printf("  %-38s %p\n", "code, a function in this program", (void *)part_layout);
    printf("  %-38s %p\n", "initialized global (.data)", (void *)&g_initialized);
    printf("  %-38s %p\n", "uninitialized global (.bss)", (void *)&g_uninitialized);
    printf("  %-38s %p\n", "malloc(64), from the brk heap", (void *)small);
    printf("  %-38s %p\n", "malloc(4 MiB), its own mmap", (void *)large);
    printf("  %-38s %p\n", "printf(), inside libc", (void *)printf);
    printf("  %-38s %p\n", "local variable, on the stack", (void *)&local);

    /* [CO-4] Dynamic allocation. glibc serves small requests by growing the
       heap with brk(), and requests above 128 KiB with a private mmap() that
       free() hands straight back to the kernel. */
    printf("  program break moved from %p to %p for the small block\n", brk_before, brk_after);

    printf("\n  /proc/self/maps, the kernel's list of this process's regions:\n");
    maps = fopen("/proc/self/maps", "r");
    if (maps != NULL) {
        while (fgets(line, sizeof(line), maps) != NULL) {
            unsigned long start, end;
            char perms[8];
            int path_at = -1;
            const char *path;

            if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %n", &start, &end, perms, &path_at) < 3) {
                continue;
            }
            path = (path_at >= 0) ? line + path_at : "";
            line[strcspn(line, "\n")] = '\0';
            printf("  %12lx-%-12lx %s %9lu KiB  %s\n", start, end, perms,
                   (end - start) / 1024, *path ? path : "(anonymous)");
        }
        fclose(maps);
    }
    printf("  r w x are the page permissions, p means private (copy-on-write).\n\n");

    free(small);
    free(large);
}

static void part_demand_paging(void) {
    const size_t len = (size_t)64 << 20;
    const long pages = (long)(len / (size_t)page_size);
    volatile char *mem;
    long f0, f1, f2, f3, r0, r1, r2, r3;
    unsigned sum = 0;

    f0 = minor_faults();
    r0 = rss_kib();
    mem = map_anonymous(len);
    f1 = minor_faults();
    r1 = rss_kib();

    /* Read every page once. */
    for (long i = 0; i < pages; i++) {
        sum += (unsigned char)mem[i * page_size];
    }
    f2 = minor_faults();
    r2 = rss_kib();

    /* Write every page once. */
    for (long i = 0; i < pages; i++) {
        mem[i * page_size] = 1;
    }
    f3 = minor_faults();
    r3 = rss_kib();

    printf("Part 2. Demand paging, 64 MiB = %ld pages of %ld bytes\n", pages, page_size);
    printf("  %-30s %8s %12s\n", "step", "faults", "RSS change");
    printf("  %-30s %8ld %+9ld KiB\n", "mmap(64 MiB)", f1 - f0, r1 - r0);
    printf("  %-30s %8ld %+9ld KiB\n", "read one byte of every page", f2 - f1, r2 - r1);
    printf("  %-30s %8ld %+9ld KiB\n", "write one byte of every page", f3 - f2, r3 - r2);
    printf("  mmap() only records the range, so it costs no faults and no memory.\n");
    printf("  The first read of each page faults, and the kernel maps its single\n");
    printf("  shared zero page read-only, so RSS stays flat.\n");
    printf("  The first write faults again, and only now does each page get its\n");
    printf("  own physical frame. That is one fault and 4 KiB per page.\n\n");
    g_sink = sum;

    munmap((void *)mem, len);
}

static void part_page_tables(void) {
    const int count = 8;
    char *mem = map_anonymous((size_t)count * (size_t)page_size);
    int fd;

    /* Touch only some of the pages. */
    mem[0 * page_size] = 1;
    mem[3 * page_size] = 1;
    mem[4 * page_size] = 1;
    mem[7 * page_size] = 1;

    printf("Part 3. Page tables, read through /proc/self/pagemap\n");
    printf("  Wrote to pages 0, 3, 4 and 7 of an 8 page mapping.\n");

    /* [CO-4] Address translation. pagemap holds one 64 bit entry per virtual
       page. Bit 63 says a frame is present, bits 0 to 54 are the frame
       number. Without root the kernel zeroes the frame number but still
       reports the present bit. */
    fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd == -1) {
        perror("  open /proc/self/pagemap");
        munmap(mem, (size_t)count * (size_t)page_size);
        return;
    }
    for (int i = 0; i < count; i++) {
        uint64_t entry = 0;
        uintptr_t vaddr = (uintptr_t)(mem + (long)i * page_size);
        off_t offset = (off_t)(vaddr / (uintptr_t)page_size) * (off_t)sizeof(entry);
        uint64_t pfn;

        if (pread(fd, &entry, sizeof(entry), offset) != (ssize_t)sizeof(entry)) {
            perror("  pread pagemap");
            break;
        }
        pfn = entry & ((UINT64_C(1) << 55) - 1);
        if (entry >> 63 & 1) {
            if (pfn != 0) {
                printf("  page %d  virtual 0x%lx  present, physical frame 0x%llx\n",
                       i, (unsigned long)vaddr, (unsigned long long)pfn);
            } else {
                printf("  page %d  virtual 0x%lx  present, frame number hidden without root\n",
                       i, (unsigned long)vaddr);
            }
        } else {
            printf("  page %d  virtual 0x%lx  not present, no frame yet\n",
                   i, (unsigned long)vaddr);
        }
    }
    close(fd);
    printf("  Run with sudo to see the physical frame numbers.\n\n");
    munmap(mem, (size_t)count * (size_t)page_size);
}

static void part_copy_on_write(void) {
    const size_t len = (size_t)16 << 20;
    const long pages = (long)(len / (size_t)page_size);
    char *mem = map_anonymous(len);
    pid_t child;
    long intact = 0;

    memset(mem, 'A', len);

    printf("Part 4. Copy-on-write after fork(), %ld pages filled with 'A'\n", pages);
    fflush(stdout);

    child = fork();
    if (child == -1) {
        perror("fork");
        munmap(mem, len);
        return;
    }
    if (child == 0) {
        volatile char *v = mem;
        unsigned sum = 0;
        long f0 = minor_faults();
        for (long i = 0; i < pages; i++) {
            sum += (unsigned char)v[i * page_size];
        }
        long f1 = minor_faults();
        for (long i = 0; i < pages; i++) {
            v[i * page_size] = 'B';
        }
        long f2 = minor_faults();

        printf("  child   read every page:  %6ld faults, the pages are shared\n", f1 - f0);
        printf("  child   wrote every page: %6ld faults, each write copied one page\n", f2 - f1);
        g_sink = sum;
        fflush(stdout);
        _exit(0);
    }
    waitpid(child, NULL, 0);

    for (long i = 0; i < pages; i++) {
        if (mem[i * page_size] == 'A') {
            intact++;
        }
    }
    printf("  parent  still sees 'A' in %ld of %ld pages. The child's writes went\n", intact, pages);
    printf("          to its own private copies.\n");
    printf("  fork() copies the page table, not the memory. Both processes point at\n");
    printf("  the same frames, marked read-only, and a write fault makes the copy.\n\n");
    munmap(mem, len);
}

static sigjmp_buf g_recover;
static volatile sig_atomic_t g_si_code;
static void *volatile g_si_addr;

/* [CO-4] Memory errors. On an invalid access the MMU traps into the kernel,
   the kernel finds no valid mapping or permission for the address, and it
   sends SIGSEGV. si_code says which of the two failed. Jumping out of the
   handler is only safe here because the faulting code is ours and simple. */
static void on_segv(int sig, siginfo_t *info, void *context) {
    (void)sig;
    (void)context;
    g_si_code = info->si_code;
    g_si_addr = info->si_addr;
    siglongjmp(g_recover, 1);
}

static const char *segv_code_name(int code) {
    switch (code) {
        case SEGV_MAPERR: return "SEGV_MAPERR, no mapping at that address";
        case SEGV_ACCERR: return "SEGV_ACCERR, mapped but not writable";
        default:          return "another code";
    }
}

static void part_memory_errors(void) {
    struct sigaction sa, old;
    char *page = map_anonymous((size_t)page_size);
    int *volatile bad = NULL;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = on_segv;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old);

    printf("Part 5. Memory errors and how the kernel answers them\n");

    page[0] = 'x';
    mprotect(page, (size_t)page_size, PROT_READ);
    printf("  made page %p read-only with mprotect(), reading it gives '%c'\n",
           (void *)page, page[0]);
    if (sigsetjmp(g_recover, 1) == 0) {
        ((volatile char *)page)[0] = 'y';
        printf("  the write went through, which should not happen\n");
    } else {
        printf("  writing it:  SIGSEGV at %p, %s\n", g_si_addr, segv_code_name(g_si_code));
    }

    if (sigsetjmp(g_recover, 1) == 0) {
        *bad = 1;
        printf("  the write went through, which should not happen\n");
    } else {
        printf("  writing *NULL: SIGSEGV at %p, %s\n", g_si_addr, segv_code_name(g_si_code));
    }

    printf("  Without a handler, SIGSEGV kills the process: the familiar\n");
    printf("  \"Segmentation fault (core dumped)\".\n\n");

    sigaction(SIGSEGV, &old, NULL);
    munmap(page, (size_t)page_size);
}

/* [CO-4] Memory debugging. A leak never faults: the pages stay mapped and
   valid, the program just loses every pointer to them. Only a tool that
   tracks each allocation can tell. */
static int bug_leak(void) {
    size_t total = 0;

    for (int i = 0; i < 10; i++) {
        char *record = malloc(100);
        if (record == NULL) {
            return 1;
        }
        snprintf(record, 100, "record %d", i);
        total += strlen(record);
        /* BUG: record is never freed, and the pointer dies here. */
    }
    printf("leaked 10 blocks of 100 bytes (%zu bytes of text in them)\n", total);
    printf("nothing crashed and nothing looks wrong. Find the leak with:\n");
    printf("    valgrind --leak-check=full ./bin/vmem leak\n");
    printf("or with AddressSanitizer:\n");
    printf("    gcc -g -fsanitize=address demo/vmem.c -o /tmp/vmem-asan && /tmp/vmem-asan leak\n");
    return 0;
}

/* [CO-4] A heap overflow. The array ends in the middle of a page that is
   mapped and writable, so the extra write lands in malloc's bookkeeping or
   a neighbouring block and the kernel sees nothing wrong. */
static int bug_overflow(int n) {
    int *scores;

    if (n < 1 || n > 1024) {
        n = 8;
    }
    scores = malloc((size_t)n * sizeof(*scores));
    if (scores == NULL) {
        return 1;
    }
    for (int i = 0; i <= n; i++) {        /* BUG: <= writes scores[n] */
        scores[i] = i * 10;
    }
    printf("wrote %d ints into an array of %d, one past the end\n", n + 1, n);
    printf("no SIGSEGV, because the stray write stayed inside a mapped page.\n");
    printf("Catch it with:\n");
    printf("    valgrind ./bin/vmem overflow\n");
    printf("    gcc -g -fsanitize=address demo/vmem.c -o /tmp/vmem-asan && /tmp/vmem-asan overflow\n");
    free(scores);
    return 0;
}

int main(int argc, char *argv[]) {
    page_size = sysconf(_SC_PAGESIZE);

    if (argc > 1 && strcmp(argv[1], "leak") == 0) {
        return bug_leak();
    }
    if (argc > 1 && strcmp(argv[1], "overflow") == 0) {
        return bug_overflow(argc > 2 ? atoi(argv[2]) : 8);
    }
    if (argc > 1) {
        fprintf(stderr, "usage: %s [leak | overflow [n]]\n", argv[0]);
        return 2;
    }

    part_layout();
    part_demand_paging();
    part_page_tables();
    part_copy_on_write();
    part_memory_errors();
    return 0;
}
