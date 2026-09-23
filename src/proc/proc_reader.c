#include "proc_reader.h"
#include "../utils/parser.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void km_pid_list_init(km_pid_list *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void km_pid_list_free(km_pid_list *list) {
    free(list->items);
    km_pid_list_init(list);
}

static bool pid_list_push(km_pid_list *list, pid_t pid) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 256;
        pid_t *grown = realloc(list->items, new_cap * sizeof(*grown));
        if (!grown) {
            return false;
        }
        list->items = grown;
        list->capacity = new_cap;
    }

    list->items[list->count++] = pid;
    return true;
}

/* [CO-1] User space and kernel space, system calls and kernel services.
   This is the single point where the program crosses into the kernel. /proc is
   not a disk filesystem: the kernel generates each file's contents on read,
   through the VFS. So an ordinary open/read/close is enough to obtain live
   kernel state, with no driver, no kernel module and no elevated privilege. */
ssize_t km_proc_read_file(const char *path, char *buf, size_t bufsz) {
    int fd;
    size_t total = 0;

    if (bufsz == 0) {
        return -1;
    }

    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    /* A /proc file reports a size of zero, so there is nothing to seek on or
       preallocate from: read until it stops producing bytes. */
    while (total < bufsz - 1) {
        ssize_t n = read(fd, buf + total, bufsz - 1 - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        if (n == 0) {
            break;
        }
        total += (size_t)n;
    }

    close(fd);
    buf[total] = '\0';
    return (ssize_t)total;
}

ssize_t km_proc_read_stat(char *buf, size_t bufsz) {
    return km_proc_read_file("/proc/stat", buf, bufsz);
}

ssize_t km_proc_read_meminfo(char *buf, size_t bufsz) {
    return km_proc_read_file("/proc/meminfo", buf, bufsz);
}

ssize_t km_proc_read_uptime(char *buf, size_t bufsz) {
    return km_proc_read_file("/proc/uptime", buf, bufsz);
}

ssize_t km_proc_read_loadavg(char *buf, size_t bufsz) {
    return km_proc_read_file("/proc/loadavg", buf, bufsz);
}

ssize_t km_proc_read_version(char *buf, size_t bufsz) {
    return km_proc_read_file("/proc/version", buf, bufsz);
}

ssize_t km_proc_read_cpuinfo(char *buf, size_t bufsz) {
    return km_proc_read_file("/proc/cpuinfo", buf, bufsz);
}

static ssize_t read_pid_file(pid_t pid, const char *leaf, char *buf, size_t bufsz) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/%s", (int)pid, leaf);
    return km_proc_read_file(path, buf, bufsz);
}

ssize_t km_proc_read_pid_stat(pid_t pid, char *buf, size_t bufsz) {
    return read_pid_file(pid, "stat", buf, bufsz);
}

ssize_t km_proc_read_pid_status(pid_t pid, char *buf, size_t bufsz) {
    return read_pid_file(pid, "status", buf, bufsz);
}

ssize_t km_proc_read_pid_statm(pid_t pid, char *buf, size_t bufsz) {
    return read_pid_file(pid, "statm", buf, bufsz);
}

ssize_t km_proc_read_pid_cmdline(pid_t pid, char *buf, size_t bufsz) {
    ssize_t len = read_pid_file(pid, "cmdline", buf, bufsz);
    ssize_t i;

    if (len < 0) {
        return -1;
    }

    /* cmdline separates arguments with NUL bytes, so it has to be walked by
       length rather than treated as one string. */
    for (i = 0; i < len; i++) {
        if (buf[i] == '\0') {
            buf[i] = ' ';
        }
    }

    while (len > 0 && buf[len - 1] == ' ') {
        len--;
    }
    buf[len] = '\0';

    return len;
}

ssize_t km_proc_read_pid_exe(pid_t pid, char *buf, size_t bufsz) {
    char path[64];
    ssize_t len;

    if (bufsz == 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "/proc/%d/exe", (int)pid);

    /* readlink does not NUL-terminate, and silently truncates if the target
       does not fit, so leave room and terminate by hand. */
    len = readlink(path, buf, bufsz - 1);
    if (len < 0) {
        return -1;
    }

    buf[len] = '\0';
    return len;
}

static int compare_pid(const void *a, const void *b) {
    pid_t pa = *(const pid_t *)a;
    pid_t pb = *(const pid_t *)b;

    if (pa < pb) {
        return -1;
    }
    return pa > pb;
}

bool km_proc_list_pids(km_pid_list *list) {
    DIR *dir;
    struct dirent *entry;

    list->count = 0;

    /* [CO-1] Linux architecture: the process table is exposed as directories.
       Every numeric entry under /proc is a live process, so enumerating PIDs is
       just a directory walk with opendir, readdir and closedir. */
    dir = opendir("/proc");
    if (!dir) {
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        bool is_numeric = true;
        const char *p;
        long value;

        if (entry->d_type != DT_DIR) {
            continue;
        }

        for (p = name; *p; ++p) {
            if (!isdigit((unsigned char)*p)) {
                is_numeric = false;
                break;
            }
        }

        if (!is_numeric || name[0] == '\0') {
            continue;
        }

        value = strtol(name, NULL, 10);
        if (value > 0) {
            pid_list_push(list, (pid_t)value);
        }
    }

    closedir(dir);

    /* readdir order is whatever the filesystem hands back. Sorting here is what
       lets the caller keep its sample as a sorted array and look a PID up by
       binary search. */
    qsort(list->items, list->count, sizeof(*list->items), compare_pid);

    return true;
}

int km_proc_count_fds(pid_t pid) {
    char path[64];
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    snprintf(path, sizeof(path), "/proc/%d/fd", (int)pid);

    dir = opendir(path);
    if (!dir) {
        return -1; /* Permission denied, or the process has gone. */
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

int km_proc_cpu_count(void) {
    /* [CO-1] Kernel services queried through the POSIX sysconf interface rather
       than by parsing a file. */
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);

    if (nprocs <= 0) {
        /* Fallback: count the "processor" lines in /proc/cpuinfo. */
        static char buf[KM_PROC_BUF_SIZE];
        const char *cursor = buf;
        char line[512];
        int count = 0;

        if (km_proc_read_cpuinfo(buf, sizeof(buf)) < 0) {
            return 1; /* Assume at least one CPU. */
        }

        while (km_read_line(&cursor, line, sizeof(line))) {
            if (strncmp(line, "processor", 9) == 0) {
                count++;
            }
        }

        return count > 0 ? count : 1;
    }

    return (int)nprocs;
}
