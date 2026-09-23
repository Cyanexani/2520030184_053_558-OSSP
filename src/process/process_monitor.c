#include "process_monitor.h"
#include "../proc/proc_reader.h"
#include "../utils/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

void km_process_monitor_init(km_process_monitor *m) {
    struct sysinfo si;

    memset(m, 0, sizeof(*m));
    km_pid_list_init(&m->pids);

    m->page_size = sysconf(_SC_PAGESIZE);
    if (m->page_size <= 0) {
        m->page_size = 4096;
    }

    m->cpu_count = km_proc_cpu_count();
    if (m->cpu_count <= 0) {
        m->cpu_count = 1;
    }

    m->clock_ticks = sysconf(_SC_CLK_TCK);
    if (m->clock_ticks <= 0) {
        m->clock_ticks = 100; /* Common default. */
    }

    /* Boot time anchors a process's start time, which /proc reports as an
       offset from boot rather than as a date. */
    if (sysinfo(&si) == 0) {
        m->boot_time = time(NULL) - si.uptime;
    } else {
        m->boot_time = 0;
    }
}

void km_process_monitor_destroy(km_process_monitor *m) {
    free(m->processes);
    free(m->incoming);
    km_pid_list_free(&m->pids);

    m->processes = NULL;
    m->incoming = NULL;
    m->count = 0;
    m->capacity = 0;
    m->incoming_count = 0;
    m->incoming_capacity = 0;
}

/* Binary search over a PID-sorted array. */
static const km_process_info *find_in(const km_process_info *items, size_t count, pid_t pid) {
    size_t low = 0;
    size_t high = count;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (items[mid].pid == pid) {
            return &items[mid];
        }
        if (items[mid].pid < pid) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return NULL;
}

const km_process_info *km_process_monitor_find(const km_process_monitor *m, pid_t pid) {
    return find_in(m->processes, m->count, pid);
}

static bool incoming_reserve(km_process_monitor *m) {
    if (m->incoming_count == m->incoming_capacity) {
        size_t new_cap = m->incoming_capacity ? m->incoming_capacity * 2 : 512;
        km_process_info *grown = realloc(m->incoming, new_cap * sizeof(*grown));
        if (!grown) {
            return false;
        }
        m->incoming = grown;
        m->incoming_capacity = new_cap;
    }
    return true;
}

static bool read_process_info(km_process_monitor *m, pid_t pid, km_process_info *info) {
    char buf[KM_PROC_PID_BUF_SIZE];
    int parsed_pid;
    int ppid = 0;
    long priority = 0;
    long nice = 0;
    long num_threads = 0;
    int fd_count;

    /* Start from a clean slate: any field whose /proc file is unreadable keeps
       a defined zero rather than whatever was on the stack. */
    memset(info, 0, sizeof(*info));
    info->pid = pid;

    if (km_proc_read_pid_stat(pid, buf, sizeof(buf)) < 0) {
        return false;
    }

    if (!km_parse_proc_stat(buf, &parsed_pid, info->name, sizeof(info->name),
                            &info->state, &ppid, &info->utime, &info->stime,
                            &priority, &nice, &num_threads)) {
        return false;
    }

    info->ppid = (pid_t)ppid;
    info->priority = (int)priority;
    info->nice = (int)nice;
    info->num_threads = num_threads;

    /* Memory, reported in pages. */
    if (km_proc_read_pid_statm(pid, buf, sizeof(buf)) >= 0) {
        unsigned long size, resident, shared;
        if (km_parse_proc_statm(buf, &size, &resident, &shared)) {
            info->vm_size = (unsigned long long)size * (unsigned long long)m->page_size;
            info->vm_rss = (unsigned long long)resident * (unsigned long long)m->page_size;
        }
    }

    if (km_proc_read_pid_status(pid, buf, sizeof(buf)) >= 0) {
        km_parse_status_uid(buf, &info->uid);
    }

    if (km_proc_read_pid_cmdline(pid, buf, sizeof(buf)) > 0) {
        /* An overlong command line is cut to fit on purpose: the detail view
           only ever shows the first few hundred columns of it. */
        snprintf(info->cmdline, sizeof(info->cmdline), "%.*s",
                 (int)(sizeof(info->cmdline) - 1), buf);
    } else {
        /* An empty cmdline marks a kernel thread, which top and ps show in
           square brackets. */
        snprintf(info->cmdline, sizeof(info->cmdline), "[%s]", info->name);
    }

    if (km_proc_read_pid_exe(pid, buf, sizeof(buf)) >= 0) {
        snprintf(info->exe_path, sizeof(info->exe_path), "%.*s",
                 (int)(sizeof(info->exe_path) - 1), buf);
    }

    fd_count = km_proc_count_fds(pid);
    info->fd_count = fd_count >= 0 ? fd_count : 0;

    info->start_time = m->boot_time +
        (time_t)((info->utime + info->stime) / (unsigned long long)m->clock_ticks);

    return true;
}

/* [CO-2] User-level scheduling concepts. The kernel does not publish a CPU
   percentage; it publishes a monotonically increasing count of clock ticks the
   scheduler has given this process. A percentage is therefore a rate, derived
   from two samples and the real time between them. One sample alone says
   nothing about current usage. */
static void calculate_cpu_usage(const km_process_monitor *m,
                                km_process_info *current,
                                const km_process_info *previous,
                                double seconds_elapsed) {
    /* Jiffies this process spent on a CPU between the two samples. */
    unsigned long long prev_total = previous->utime + previous->stime;
    unsigned long long curr_total = current->utime + current->stime;
    unsigned long long time_diff;

    /* A PID can be recycled onto a different process, whose counters start
       lower. Unsigned subtraction would wrap that into an enormous figure. */
    if (curr_total < prev_total) {
        current->cpu_percent = 0.0;
        return;
    }
    time_diff = curr_total - prev_total;

    /* Percentage of one CPU = (jiffies / ticks_per_second) / seconds * 100.
       seconds_elapsed is the measured interval; guard the first sample, where
       there is no previous timestamp to subtract, and a zero clock rate. */
    if (seconds_elapsed > 0.0 && m->clock_ticks > 0) {
        current->cpu_percent =
            (100.0 * (double)time_diff) / ((double)m->clock_ticks * seconds_elapsed);
    } else {
        current->cpu_percent = 0.0;
    }

    /* Left deliberately unclamped: a multi-threaded process can exceed 100%,
       which is the same convention top(1) uses. Anything above 100% for a
       single-threaded process means this arithmetic is wrong. */
}

static void push_event(km_process_monitor *m, km_event_type type, pid_t pid,
                       const char *name, const char *description) {
    km_event *slot;

    /* Full window: drop the oldest to make room, so the array always holds the
       most recent KM_MAX_EVENTS. */
    if (m->event_count == KM_MAX_EVENTS) {
        memmove(&m->events[0], &m->events[1], (KM_MAX_EVENTS - 1) * sizeof(m->events[0]));
        m->event_count = KM_MAX_EVENTS - 1;
    }

    slot = &m->events[m->event_count++];
    slot->timestamp = time(NULL);
    slot->type = type;
    slot->pid = pid;
    snprintf(slot->process_name, sizeof(slot->process_name), "%s", name);
    snprintf(slot->description, sizeof(slot->description), "%s", description);
}

/* [CO-2] Process creation and termination. Comparing this sample against the
   previous one detects both: a PID present now but not before was created, and
   one present before but not now has terminated. */
static void detect_changes(km_process_monitor *m) {
    size_t i;

    /* Both arrays are PID-sorted, so each lookup is a binary search. */
    for (i = 0; i < m->incoming_count; i++) {
        const km_process_info *proc = &m->incoming[i];
        if (!find_in(m->processes, m->count, proc->pid)) {
            push_event(m, KM_EVENT_PROCESS_CREATED, proc->pid, proc->name,
                       "Process created");
        }
    }

    for (i = 0; i < m->count; i++) {
        const km_process_info *proc = &m->processes[i];
        if (!find_in(m->incoming, m->incoming_count, proc->pid)) {
            push_event(m, KM_EVENT_PROCESS_TERMINATED, proc->pid, proc->name,
                       "Process terminated");
        }
    }
}

void km_process_monitor_update(km_process_monitor *m) {
    struct timespec now;
    double seconds_elapsed = 0.0;
    size_t i;

    km_proc_list_pids(&m->pids);
    m->incoming_count = 0;

    /* Measure the actual gap since the previous sample. The refresh interval is
       user-adjustable, and a missed or delayed tick stretches it further, so
       assuming a fixed one scales every percentage by the wrong factor. */
    km_clock_now(&now);
    if (m->have_last_sample) {
        seconds_elapsed = km_clock_diff_seconds(&m->last_sample, &now);
    }
    m->last_sample = now;
    m->have_last_sample = true;

    /* The PID list is sorted, so appending in order keeps the new sample sorted
       too, even though processes that vanish mid-walk are skipped. */
    for (i = 0; i < m->pids.count; i++) {
        pid_t pid = m->pids.items[i];
        km_process_info *slot;
        const km_process_info *previous;

        if (!incoming_reserve(m)) {
            break;
        }

        slot = &m->incoming[m->incoming_count];
        if (!read_process_info(m, pid, slot)) {
            continue;
        }

        previous = find_in(m->processes, m->count, pid);
        if (previous) {
            calculate_cpu_usage(m, slot, previous, seconds_elapsed);
        } else {
            slot->cpu_percent = 0.0;
        }

        m->incoming_count++;
    }

    detect_changes(m);

    /* Swap rather than copy: the old buffer becomes next tick's scratch. */
    {
        km_process_info *tmp_items = m->processes;
        size_t tmp_capacity = m->capacity;

        m->processes = m->incoming;
        m->count = m->incoming_count;
        m->capacity = m->incoming_capacity;

        m->incoming = tmp_items;
        m->incoming_capacity = tmp_capacity;
        m->incoming_count = 0;
    }
}

const km_event *km_process_monitor_recent_events(const km_process_monitor *m,
                                                 size_t count, size_t *out_count) {
    if (count >= m->event_count) {
        *out_count = m->event_count;
        return m->events;
    }

    *out_count = count;
    return &m->events[m->event_count - count];
}

/* [CO-2] Process creation and the parent/child relationship. Every process
   records the PID of its parent, so the whole hierarchy can be rebuilt from
   PPID links alone. A process whose parent has exited shows a PPID that is no
   longer its original creator, because the kernel re-parented it. */
bool km_process_tree_build(const km_process_monitor *m, km_process_tree *tree) {
    size_t i;
    size_t total_children = 0;
    size_t *fill = NULL;

    memset(tree, 0, sizeof(*tree));
    tree->processes = m->processes;
    tree->count = m->count;

    if (m->count == 0) {
        return true;
    }

    tree->child_start = calloc(m->count, sizeof(*tree->child_start));
    tree->child_count = calloc(m->count, sizeof(*tree->child_count));
    tree->roots = malloc(m->count * sizeof(*tree->roots));
    fill = calloc(m->count, sizeof(*fill));

    if (!tree->child_start || !tree->child_count || !tree->roots || !fill) {
        free(fill);
        km_process_tree_destroy(tree);
        return false;
    }

    /* First pass: count each parent's children, and collect the processes whose
       parent is not itself in the sample. Those are the tree's entry points. */
    for (i = 0; i < m->count; i++) {
        const km_process_info *parent =
            find_in(m->processes, m->count, m->processes[i].ppid);

        if (parent) {
            tree->child_count[parent - m->processes]++;
        } else {
            tree->roots[tree->root_count++] = m->processes[i].pid;
        }
    }

    /* Turn the counts into offsets into one shared array. */
    for (i = 0; i < m->count; i++) {
        tree->child_start[i] = total_children;
        total_children += tree->child_count[i];
    }

    if (total_children > 0) {
        tree->child_pids = malloc(total_children * sizeof(*tree->child_pids));
        if (!tree->child_pids) {
            free(fill);
            km_process_tree_destroy(tree);
            return false;
        }
    }

    /* Second pass: place each child. Walking parents in PID order means each
       parent's slice also comes out in PID order. */
    for (i = 0; i < m->count; i++) {
        const km_process_info *parent =
            find_in(m->processes, m->count, m->processes[i].ppid);

        if (parent) {
            size_t p = (size_t)(parent - m->processes);
            tree->child_pids[tree->child_start[p] + fill[p]++] = m->processes[i].pid;
        }
    }

    /* A process that reports PPID 0, or itself, is a root even if the first
       pass did not already record it. */
    for (i = 0; i < m->count; i++) {
        const km_process_info *proc = &m->processes[i];

        if (proc->ppid == 0 || proc->ppid == proc->pid) {
            bool already = false;
            size_t r;

            for (r = 0; r < tree->root_count; r++) {
                if (tree->roots[r] == proc->pid) {
                    already = true;
                    break;
                }
            }

            if (!already) {
                tree->roots[tree->root_count++] = proc->pid;
            }
        }
    }

    free(fill);
    return true;
}

void km_process_tree_destroy(km_process_tree *tree) {
    free(tree->child_start);
    free(tree->child_count);
    free(tree->child_pids);
    free(tree->roots);
    memset(tree, 0, sizeof(*tree));
}

const km_process_info *km_process_tree_find(const km_process_tree *tree, pid_t pid) {
    return find_in(tree->processes, tree->count, pid);
}

const pid_t *km_process_tree_children(const km_process_tree *tree, pid_t pid,
                                      size_t *out_count) {
    const km_process_info *proc = km_process_tree_find(tree, pid);
    size_t index;

    if (!proc) {
        *out_count = 0;
        return NULL;
    }

    index = (size_t)(proc - tree->processes);
    *out_count = tree->child_count[index];

    if (*out_count == 0) {
        return NULL;
    }

    return &tree->child_pids[tree->child_start[index]];
}

/* [CO-2] Process lifecycle and state transitions. The kernel stores the current
   state as a single character in field 3 of /proc/[pid]/stat. R and S are the
   common ones; Z is a terminated process not yet reaped by its parent, and T is
   a process suspended by SIGSTOP. See demo/zombie.c and demo/orphan.c. */
const char *km_process_state_string(char state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk Sleep";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing Stop";
        case 'X': return "Dead";
        case 'x': return "Dead";
        case 'K': return "Wakekill";
        case 'W': return "Waking";
        case 'P': return "Parked";
        case 'I': return "Idle";
        default:  return "Unknown";
    }
}

double km_process_memory_percent(const km_process_info *info) {
    struct sysinfo si;
    unsigned long long total_mem;

    if (sysinfo(&si) != 0) {
        return 0.0;
    }

    total_mem = (unsigned long long)si.totalram * si.mem_unit;
    if (total_mem == 0) {
        return 0.0;
    }

    return (100.0 * (double)info->vm_rss) / (double)total_mem;
}
