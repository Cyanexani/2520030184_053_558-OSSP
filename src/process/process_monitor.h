#ifndef KM_PROCESS_MONITOR_H
#define KM_PROCESS_MONITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#include "../proc/proc_reader.h"

/* The kernel caps comm at 16 bytes; the extra room absorbs an unusual one. */
#define KM_COMM_MAX 64
#define KM_CMDLINE_MAX 512
#define KM_EXE_PATH_MAX 512
#define KM_EVENT_DESC_MAX 64

/* Events are a rolling window, oldest dropped once it is full. */
#define KM_MAX_EVENTS 100

/* [CO-2] The process abstraction. Each field below mirrors something the kernel
   tracks in its own task_struct and publishes through /proc/[pid]/: identity
   (pid, ppid), scheduling state, CPU time consumed, memory footprint and thread
   count. This struct is the user-space view of one entry in the process table. */
typedef struct {
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    char name[KM_COMM_MAX];
    char state;
    int priority;
    int nice;
    long num_threads;
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long vm_size;
    unsigned long long vm_rss;
    double cpu_percent;
    char cmdline[KM_CMDLINE_MAX];
    char exe_path[KM_EXE_PATH_MAX];
    int fd_count;
    time_t start_time;
} km_process_info;

typedef enum {
    KM_EVENT_PROCESS_CREATED,
    KM_EVENT_PROCESS_TERMINATED,
    KM_EVENT_PROCESS_STATE_CHANGED,
    KM_EVENT_HIGH_CPU_USAGE,
    KM_EVENT_HIGH_MEMORY_USAGE
} km_event_type;

typedef struct {
    time_t timestamp;
    km_event_type type;
    pid_t pid;
    char process_name[KM_COMM_MAX];
    char description[KM_EVENT_DESC_MAX];
} km_event;

/* A snapshot of the PPID hierarchy.

   Children are held in one flat array, with each parent owning the slice that
   starts at child_start[i] and runs for child_count[i] entries. That keeps the
   whole tree in three allocations instead of one per parent. */
typedef struct {
    const km_process_info *processes; /* Borrowed from the monitor, pid order. */
    size_t count;

    size_t *child_start;
    size_t *child_count;
    pid_t *child_pids;

    pid_t *roots;
    size_t root_count;
} km_process_tree;

typedef struct {
    /* The current sample, kept sorted by PID so a lookup is a binary search
       and iteration is in PID order. */
    km_process_info *processes;
    size_t count;
    size_t capacity;

    /* The sample being built, swapped in at the end of an update. */
    km_process_info *incoming;
    size_t incoming_count;
    size_t incoming_capacity;

    km_event events[KM_MAX_EVENTS];
    size_t event_count;

    km_pid_list pids;

    long page_size;
    int cpu_count;
    long clock_ticks;
    time_t boot_time;

    /* When update() last sampled. CPU percentage is a rate, so it needs the
       real interval between the two samples being compared, not an assumed
       one. */
    struct timespec last_sample;
    bool have_last_sample;
} km_process_monitor;

void km_process_monitor_init(km_process_monitor *m);
void km_process_monitor_destroy(km_process_monitor *m);

void km_process_monitor_update(km_process_monitor *m);

/* Returns NULL when no such process is in the current sample. The pointer is
   invalidated by the next update. */
const km_process_info *km_process_monitor_find(const km_process_monitor *m, pid_t pid);

/* Points at the most recent events, oldest first, and reports how many are
   available (never more than requested). */
const km_event *km_process_monitor_recent_events(const km_process_monitor *m,
                                                 size_t count, size_t *out_count);

bool km_process_tree_build(const km_process_monitor *m, km_process_tree *tree);
void km_process_tree_destroy(km_process_tree *tree);
const km_process_info *km_process_tree_find(const km_process_tree *tree, pid_t pid);
const pid_t *km_process_tree_children(const km_process_tree *tree, pid_t pid,
                                      size_t *out_count);

const char *km_process_state_string(char state);
double km_process_memory_percent(const km_process_info *info);

#endif /* KM_PROCESS_MONITOR_H */
