/*
 * fileio.c - files, inodes and descriptors, one system call at a time.
 *
 * [CO-5] The Unix file abstraction. A regular file, a /proc entry, a device
 * and a pipe are all opened, read and closed with the same system calls. The
 * Virtual File System (VFS) layer in the kernel is what makes that possible:
 * it routes each call to whichever file system owns the path.
 *
 * The tour, in order:
 *   1. VFS: one read() call served by four different file systems, and the
 *      magic number statfs() reports for each mount.
 *   2. ext4: block size, capacity, inode count and journal mode of the root.
 *   3. Inodes and directory entries: hard links, symbolic links, unlink(),
 *      and a file that is deleted but still open.
 *   4. File descriptors: the per-process table, the shared open file
 *      description behind dup(), and /proc/self/fd.
 *   5. Buffered and unbuffered I/O: the same bytes through write() and
 *      through stdio, timed.
 *   6. Memory-mapped I/O: changing a file without calling write().
 *
 * Everything happens in a private directory under /tmp, removed at the end.
 *
 * Build: make demo
 * Run:   ./bin/fileio
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define WRITE_BYTES 200000

static char g_dir[] = "/tmp/km-fileio-XXXXXX";

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

static void path_in_dir(char *out, size_t size, const char *name) {
    snprintf(out, size, "%s/%s", g_dir, name);
}

static const char *fs_name(long type) {
    switch ((unsigned long)type) {
        case 0xEF53:     return "ext4";
        case 0x9FA0:     return "proc";
        case 0x62656572: return "sysfs";
        case 0x01021994: return "tmpfs";
        case 0x01021997: return "9p (Windows drive under WSL)";
        case 0x1CD1:     return "devpts";
        case 0x794C7630: return "overlayfs";
        case 0x9123683E: return "btrfs";
        case 0x58465342: return "xfs";
        case 0x65735546: return "fuse";
        case 0x27E0EB:   return "cgroup";
        case 0x63677270: return "cgroup2";
        default:         return "other";
    }
}

/* Reads up to size-1 bytes with the plain system calls. */
static ssize_t read_start(const char *path, char *buf, size_t size) {
    ssize_t n;
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        return -1;
    }
    n = read(fd, buf, size - 1);
    close(fd);
    if (n >= 0) {
        buf[n] = '\0';
    }
    return n;
}

static void part_vfs(void) {
    static const char *files[] = {
        "/etc/hostname", "/proc/loadavg", "/sys/kernel/mm/transparent_hugepage/enabled",
        "/dev/urandom",
    };
    static const char *mounts[] = { "/", "/proc", "/sys", "/tmp", "/dev/pts", "/mnt/c" };
    char buf[128];

    printf("Part 1. One interface, many file systems (VFS)\n");
    printf("  The same open(), read(), close() on each path:\n");
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        struct statfs sfs;
        struct stat st;
        ssize_t n = read_start(files[i], buf, 17);
        const char *fs = (statfs(files[i], &sfs) == 0) ? fs_name(sfs.f_type) : "unknown";

        /* A device node only names a driver, and the driver serves the read. */
        if (stat(files[i], &st) == 0 && S_ISCHR(st.st_mode)) {
            fs = "driver";
        }

        printf("  %-44s %-7s ", files[i], fs);
        if (n < 0) {
            printf("(%s)\n", strerror(errno));
        } else if (strcmp(files[i], "/dev/urandom") == 0) {
            for (ssize_t j = 0; j < 8; j++) {
                printf("%02x", (unsigned char)buf[j]);
            }
            printf(" (random bytes)\n");
        } else {
            buf[strcspn(buf, "\n")] = '\0';
            printf("\"%s\"\n", buf);
        }
    }

    printf("\n  statfs() names the file system behind each mount point:\n");
    for (size_t i = 0; i < sizeof(mounts) / sizeof(mounts[0]); i++) {
        struct statfs sfs;
        if (statfs(mounts[i], &sfs) == 0) {
            printf("  %-10s magic 0x%08lx  %s\n", mounts[i],
                   (unsigned long)sfs.f_type, fs_name(sfs.f_type));
        }
    }
    printf("\n");
}

/* Finds the data= journalling mode of the first ext4 file system listed in
   /proc/fs/ext4, the kernel's per-mount ext4 information. */
static void print_ext4_journal(void) {
    DIR *dir = opendir("/proc/fs/ext4");
    struct dirent *entry;

    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        char path[PATH_MAX], text[4096];
        const char *mode;

        if (entry->d_name[0] == '.') {
            continue;
        }
        snprintf(path, sizeof(path), "/proc/fs/ext4/%s/options", entry->d_name);
        if (read_start(path, text, sizeof(text)) <= 0) {
            continue;
        }
        mode = strstr(text, "data=");
        if (mode != NULL) {
            printf("  device %s journals with %.*s\n", entry->d_name,
                   (int)strcspn(mode, "\n"), mode);
            break;
        }
    }
    closedir(dir);
}

static void part_ext4(void) {
    struct statfs sfs;

    printf("Part 2. ext4, the file system under /\n");
    if (statfs("/", &sfs) != 0) {
        perror("  statfs");
        return;
    }
    if ((unsigned long)sfs.f_type != 0xEF53) {
        printf("  / is %s here, not ext4, so this part is skipped\n\n", fs_name(sfs.f_type));
        return;
    }
    printf("  block size   %ld bytes\n", (long)sfs.f_bsize);
    printf("  capacity     %.1f GiB, %.1f GiB free\n",
           (double)sfs.f_blocks * (double)sfs.f_bsize / (1 << 30),
           (double)sfs.f_bavail * (double)sfs.f_bsize / (1 << 30));
    printf("  inodes       %llu in total, %llu free\n",
           (unsigned long long)sfs.f_files, (unsigned long long)sfs.f_ffree);
    printf("  name length  up to %ld bytes per path component\n", (long)sfs.f_namelen);
    print_ext4_journal();
    printf("  ext4 fixes the number of inodes when it is formatted, so a disk can\n");
    printf("  run out of inodes while it still has free blocks. The journal records\n");
    printf("  each metadata change before it is applied, so a crash can be replayed\n");
    printf("  instead of needing a full disk check.\n\n");
}

static void show_name(const char *name) {
    char path[PATH_MAX];
    struct stat st, lst;

    path_in_dir(path, sizeof(path), name);
    if (lstat(path, &lst) == -1) {
        printf("  %-14s gone\n", name);
        return;
    }
    if (S_ISLNK(lst.st_mode)) {
        char target[PATH_MAX];
        ssize_t n = readlink(path, target, sizeof(target) - 1);
        target[n < 0 ? 0 : n] = '\0';
        printf("  %-14s inode %-9lu links %lu  symlink to \"%s\", %s\n", name,
               (unsigned long)lst.st_ino, (unsigned long)lst.st_nlink, target,
               stat(path, &st) == 0 ? "target reachable" : "target MISSING (dangling)");
    } else {
        printf("  %-14s inode %-9lu links %lu  regular file, %lld bytes\n", name,
               (unsigned long)lst.st_ino, (unsigned long)lst.st_nlink, (long long)lst.st_size);
    }
}

static void list_directory(void) {
    DIR *dir = opendir(g_dir);
    struct dirent *entry;

    if (dir == NULL) {
        return;
    }
    printf("  readdir() of the directory, each entry is only a name and an inode number:\n");
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            printf("      %-14s -> inode %lu\n", entry->d_name, (unsigned long)entry->d_ino);
        }
    }
    closedir(dir);
}

static void part_inodes(void) {
    char original[PATH_MAX], hard[PATH_MAX], soft[PATH_MAX], scratch[PATH_MAX];
    char buf[64];
    int fd;
    ssize_t n;

    path_in_dir(original, sizeof(original), "original.txt");
    path_in_dir(hard, sizeof(hard), "hardlink.txt");
    path_in_dir(soft, sizeof(soft), "symlink.txt");
    path_in_dir(scratch, sizeof(scratch), "scratch.txt");

    printf("Part 3. Inodes, directory entries and names, in %s\n", g_dir);
    fd = open(original, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("  create");
        return;
    }
    if (write(fd, "hello from an inode\n", 20) != 20) {
        perror("  write");
    }
    close(fd);

    /* [CO-5] A hard link is a second directory entry for the same inode. A
       symbolic link is a new, tiny inode whose content is a path. */
    if (link(original, hard) == -1 || symlink("original.txt", soft) == -1) {
        perror("  link");
        return;
    }
    show_name("original.txt");
    show_name("hardlink.txt");
    show_name("symlink.txt");
    list_directory();

    printf("\n  unlink(\"original.txt\") removes one name, not the file:\n");
    unlink(original);
    show_name("original.txt");
    show_name("hardlink.txt");
    show_name("symlink.txt");
    n = read_start(hard, buf, sizeof(buf));
    if (n > 0) {
        buf[strcspn(buf, "\n")] = '\0';
        printf("  hardlink.txt still reads \"%s\". The data is freed only when the\n", buf);
        printf("  link count reaches 0 and no process has the file open.\n");
    }

    /* The second half of that rule: an open descriptor keeps the inode. */
    fd = open(scratch, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        char link_path[64], target[PATH_MAX];
        ssize_t len;

        if (write(fd, "still here", 10) != 10) {
            perror("  write");
        }
        unlink(scratch);
        snprintf(link_path, sizeof(link_path), "/proc/self/fd/%d", fd);
        len = readlink(link_path, target, sizeof(target) - 1);
        target[len < 0 ? 0 : len] = '\0';
        n = pread(fd, buf, 10, 0);
        buf[n < 0 ? 0 : n] = '\0';
        printf("\n  created scratch.txt, kept fd %d open, then unlinked it.\n", fd);
        printf("  /proc/self/fd/%d -> %s\n", fd, target);
        printf("  reading through the descriptor still gives \"%s\"\n", buf);
        close(fd);
    }
    printf("\n");
}

static void part_descriptors(void) {
    char path[PATH_MAX], buf[8];
    int a, b, c;
    DIR *dir;
    struct dirent *entry;

    path_in_dir(path, sizeof(path), "hardlink.txt");
    printf("Part 4. File descriptors and open files\n");

    /* [CO-5] Three layers. The descriptor number indexes this process's fd
       table. Each open() creates a new open file description in the kernel,
       which holds the offset and flags. Several descriptions can point at
       the same inode. dup() makes a second descriptor for one description. */
    a = open(path, O_RDONLY);
    b = open(path, O_RDONLY);
    if (a == -1 || b == -1) {
        perror("  open");
        return;
    }
    c = dup(a);
    if (read(a, buf, 6) != 6) {
        perror("  read");
    }
    printf("  fd %d = open(hardlink.txt), fd %d = open(hardlink.txt) again, fd %d = dup(%d)\n",
           a, b, c, a);
    printf("  after read(fd %d, 6 bytes) the offsets are:\n", a);
    printf("      fd %d  offset %ld\n", a, (long)lseek(a, 0, SEEK_CUR));
    printf("      fd %d  offset %ld   its own open file, so its own offset\n",
           b, (long)lseek(b, 0, SEEK_CUR));
    printf("      fd %d  offset %ld   shares fd %d's open file, so it moved too\n",
           c, (long)lseek(c, 0, SEEK_CUR), a);

    printf("  /proc/self/fd right now:\n");
    dir = opendir("/proc/self/fd");
    if (dir != NULL) {
        int skip = dirfd(dir);
        while ((entry = readdir(dir)) != NULL) {
            char link_path[PATH_MAX], target[PATH_MAX];
            ssize_t len;
            int fd;

            if (entry->d_name[0] == '.') {
                continue;
            }
            fd = atoi(entry->d_name);
            if (fd == skip) {
                continue;   /* the descriptor opendir() itself is using */
            }
            snprintf(link_path, sizeof(link_path), "/proc/self/fd/%s", entry->d_name);
            len = readlink(link_path, target, sizeof(target) - 1);
            target[len < 0 ? 0 : len] = '\0';
            printf("      %d -> %s\n", fd, target);
        }
        closedir(dir);
    }
    printf("  0, 1 and 2 are stdin, stdout and stderr, inherited from the shell.\n\n");
    close(a);
    close(b);
    close(c);
}

static void part_buffering(void) {
    char raw_path[PATH_MAX], buffered_path[PATH_MAX];
    struct stat st;
    double t0, raw_ms, buffered_ms;
    long buffer_size, buffered_calls;
    FILE *out;
    int fd;

    path_in_dir(raw_path, sizeof(raw_path), "unbuffered.bin");
    path_in_dir(buffered_path, sizeof(buffered_path), "buffered.bin");
    printf("Part 5. Buffered and unbuffered I/O, %d bytes one at a time\n", WRITE_BYTES);

    fd = open(raw_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("  open");
        return;
    }
    t0 = now_ms();
    for (int i = 0; i < WRITE_BYTES; i++) {
        if (write(fd, "x", 1) != 1) {
            perror("  write");
            break;
        }
    }
    raw_ms = now_ms() - t0;
    close(fd);

    out = fopen(buffered_path, "w");
    if (out == NULL) {
        perror("  fopen");
        return;
    }
    /* [CO-5] stdio collects bytes in a buffer inside the process. Until it
       fills or is flushed, the kernel and the file know nothing about them. */
    fputs("abc", out);
    stat(buffered_path, &st);
    printf("  after fputs(\"abc\"), the file is %lld bytes: the data is still in the process\n",
           (long long)st.st_size);
    fflush(out);
    stat(buffered_path, &st);
    printf("  after fflush(), it is %lld bytes: one write() call delivered it\n",
           (long long)st.st_size);

    /* glibc sizes a file's buffer from the block size stat() reports. */
    buffer_size = (st.st_blksize > 0) ? (long)st.st_blksize : BUFSIZ;
    buffered_calls = WRITE_BYTES / buffer_size + 1;

    t0 = now_ms();
    for (int i = 0; i < WRITE_BYTES; i++) {
        fputc('x', out);
    }
    fclose(out);
    buffered_ms = now_ms() - t0;

    printf("  write(fd, 1 byte) x %d   %8.1f ms   %d system calls\n",
           WRITE_BYTES, raw_ms, WRITE_BYTES);
    printf("  fputc(1 byte)     x %d   %8.1f ms   about %ld system calls (%ld byte buffer)\n",
           WRITE_BYTES, buffered_ms, buffered_calls, buffer_size);
    if (buffered_ms > 0.0) {
        printf("  stdio was %.0fx faster. Every system call crosses into the kernel and\n",
               raw_ms / buffered_ms);
        printf("  back, and stdio made about %ld times fewer of them.\n",
               WRITE_BYTES / buffered_calls);
    }
    printf("  Run  strace -c ./bin/fileio  to count the calls yourself.\n\n");
}

static void part_mmap(void) {
    static const char text[] = "memory mapped files skip read and write\n";
    char path[PATH_MAX], buf[64];
    size_t len = sizeof(text) - 1;
    char *map;
    int fd;
    ssize_t n;

    path_in_dir(path, sizeof(path), "mapped.txt");
    printf("Part 6. Memory-mapped file I/O\n");

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1 || write(fd, text, len) != (ssize_t)len) {
        perror("  create");
        return;
    }
    printf("  file holds:      %s", text);

    /* [CO-5] mmap() maps the file's page cache pages into this address space.
       Plain stores to memory change the file. MAP_SHARED makes the change
       visible to everyone, and msync() forces it out to the disk. */
    map = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("  mmap");
        close(fd);
        return;
    }
    for (size_t i = 0; i < 20; i++) {
        map[i] = (char)toupper((unsigned char)map[i]);
    }
    msync(map, len, MS_SYNC);
    munmap(map, len);
    close(fd);

    n = read_start(path, buf, sizeof(buf));
    printf("  changed 20 bytes in memory, with no write() at all\n");
    printf("  read() now gives %s", n > 0 ? buf : "(nothing)\n");
    printf("\n");
}

static void cleanup(void) {
    static const char *names[] = {
        "original.txt", "hardlink.txt", "symlink.txt", "scratch.txt",
        "unbuffered.bin", "buffered.bin", "mapped.txt",
    };
    char path[PATH_MAX];

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        path_in_dir(path, sizeof(path), names[i]);
        unlink(path);
    }
    if (rmdir(g_dir) == 0) {
        printf("Removed %s and everything in it.\n", g_dir);
    }
}

int main(void) {
    if (mkdtemp(g_dir) == NULL) {
        perror("mkdtemp");
        return 1;
    }

    part_vfs();
    part_ext4();
    part_inodes();
    part_descriptors();
    part_buffering();
    part_mmap();
    cleanup();
    return 0;
}
