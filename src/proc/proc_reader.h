#ifndef KM_PROC_READER_H
#define KM_PROC_READER_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/* Buffer sizes for whole-file reads. /proc/cpuinfo is the largest of these on a
 * many-core machine, at roughly 400 bytes per logical CPU. */
#define KM_PROC_BUF_SIZE      16384
#define KM_PROC_PID_BUF_SIZE   4096

/* [CO-1] User space and kernel space, system calls and kernel services.
 * This module is the only place the program crosses into the kernel. /proc is
 * not a disk filesystem: the kernel generates each file's contents at the
 * moment it is read, through the VFS. An ordinary open, read and close is
 * therefore enough to obtain live kernel state, with no driver, no kernel
 * module and no elevated privilege. */

/* A growable list of PIDs. Sized at runtime because the process count is not
 * known in advance and changes between samples. */
typedef struct {
    pid_t *items;
    size_t count;
    size_t capacity;
} km_pid_list;

void km_pid_list_init(km_pid_list *list);
void km_pid_list_free(km_pid_list *list);

/* Read a whole file into buf. Returns the byte count written, or -1 on failure.
 * NUL terminates on success. */
ssize_t km_proc_read_file(const char *path, char *buf, size_t bufsz);

/* Machine-wide files. */
ssize_t km_proc_read_stat(char *buf, size_t bufsz);
ssize_t km_proc_read_meminfo(char *buf, size_t bufsz);
ssize_t km_proc_read_uptime(char *buf, size_t bufsz);
ssize_t km_proc_read_loadavg(char *buf, size_t bufsz);
ssize_t km_proc_read_version(char *buf, size_t bufsz);
ssize_t km_proc_read_cpuinfo(char *buf, size_t bufsz);

/* Per-process files under /proc/<pid>/. Each returns -1 when the entry has
 * disappeared, which is routine: a process can exit between being listed and
 * being read. */
ssize_t km_proc_read_pid_stat(pid_t pid, char *buf, size_t bufsz);
ssize_t km_proc_read_pid_status(pid_t pid, char *buf, size_t bufsz);
ssize_t km_proc_read_pid_statm(pid_t pid, char *buf, size_t bufsz);
ssize_t km_proc_read_pid_cmdline(pid_t pid, char *buf, size_t bufsz);

/* Resolve the /proc/<pid>/exe symlink. Returns the length, or -1 if it cannot
 * be read, which is the normal outcome for a process owned by another user. */
ssize_t km_proc_read_pid_exe(pid_t pid, char *buf, size_t bufsz);

/* [CO-1] Linux architecture: the process table is exposed as directories.
 * Every numeric entry under /proc is a live process, so enumerating PIDs is a
 * directory walk with opendir, readdir and closedir. Returns false if /proc
 * itself cannot be opened. */
bool km_proc_list_pids(km_pid_list *list);

/* Count the entries in /proc/<pid>/fd. Returns -1 when it cannot be read. */
int km_proc_count_fds(pid_t pid);

/* [CO-1] Kernel services queried through the POSIX sysconf interface rather
 * than by parsing a file. */
int km_proc_cpu_count(void);

#endif /* KM_PROC_READER_H */
