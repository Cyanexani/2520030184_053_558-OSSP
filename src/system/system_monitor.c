#include "system_monitor.h"
#include "../proc/proc_reader.h"
#include "../utils/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void km_system_monitor_init(km_system_monitor *m) {
    /* Zeroing the whole struct up front means every field has a defined value
       before the first update, including the ones no constructor reached. */
    memset(m, 0, sizeof(*m));

    m->cpu_count = km_proc_cpu_count();

    m->page_size = sysconf(_SC_PAGESIZE);
    if (m->page_size <= 0) {
        m->page_size = 4096; /* Default fallback. */
    }

    /* Seed the previous CPU counters, so the first real update has something
       to take a difference against. */
    km_system_monitor_update_cpu_stats(m);
}

void km_system_monitor_destroy(km_system_monitor *m) {
    free(m->per_core_cpu_usage);
    m->per_core_cpu_usage = NULL;
    m->per_core_count = 0;
    m->per_core_capacity = 0;
}

static void per_core_push(km_system_monitor *m, double usage) {
    if (m->per_core_count == m->per_core_capacity) {
        size_t new_cap = m->per_core_capacity ? m->per_core_capacity * 2 : 16;
        double *grown = realloc(m->per_core_cpu_usage, new_cap * sizeof(*grown));
        if (!grown) {
            return;
        }
        m->per_core_cpu_usage = grown;
        m->per_core_capacity = new_cap;
    }

    m->per_core_cpu_usage[m->per_core_count++] = usage;
}

bool km_system_monitor_update(km_system_monitor *m) {
    bool success = true;

    success &= km_system_monitor_update_system_info(m);
    success &= km_system_monitor_update_cpu_stats(m);
    success &= km_system_monitor_update_memory_stats(m);
    success &= km_system_monitor_update_load_average(m);

    return success;
}

bool km_system_monitor_update_system_info(km_system_monitor *m) {
    char buf[KM_PROC_BUF_SIZE];

    if (km_proc_read_version(buf, sizeof(buf)) < 0) {
        return false;
    }

    /* /proc/version reads "Linux version X.Y.Z ...", so the release string is
       the third whitespace-separated token. */
    if (sscanf(buf, "%*s %*s %63s", m->kernel_version) != 1) {
        m->kernel_version[0] = '\0';
    }

    if (km_proc_read_uptime(buf, sizeof(buf)) >= 0) {
        double idle_time;
        km_parse_uptime(buf, &m->uptime, &idle_time);
    }

    return true;
}

bool km_system_monitor_update_cpu_stats(km_system_monitor *m) {
    char buf[KM_PROC_BUF_SIZE];
    const char *cursor;
    char line[512];
    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    unsigned long long total, total_idle;

    if (km_proc_read_stat(buf, sizeof(buf)) < 0) {
        return false;
    }

    cursor = buf;

    /* The first line aggregates every core. */
    if (!km_read_line(&cursor, line, sizeof(line))) {
        return false;
    }

    if (!km_parse_cpu_line(line, &user, &nice, &system, &idle, &iowait, &irq, &softirq)) {
        return false;
    }

    total = user + nice + system + idle + iowait + irq + softirq;
    total_idle = idle + iowait;

    /* These counters only ever climb, so usage is the share of the growth that
       was not idle time. */
    if (m->prev_total_cpu > 0) {
        unsigned long long total_diff = total - m->prev_total_cpu;
        unsigned long long idle_diff = total_idle - m->prev_idle_cpu;

        if (total_diff > 0) {
            m->cpu_usage_percent = 100.0 * (double)(total_diff - idle_diff) / (double)total_diff;
        } else {
            m->cpu_usage_percent = 0.0;
        }
    } else {
        m->cpu_usage_percent = 0.0;
    }

    m->prev_total_cpu = total;
    m->prev_idle_cpu = total_idle;

    /* Per-core lines follow the aggregate, and end where the "intr" line
       begins. */
    m->per_core_count = 0;
    while (km_read_line(&cursor, line, sizeof(line))) {
        if (strncmp(line, "cpu", 3) != 0) {
            break;
        }

        if (km_parse_cpu_line(line, &user, &nice, &system, &idle, &iowait, &irq, &softirq)) {
            unsigned long long core_total =
                user + nice + system + idle + iowait + irq + softirq;
            unsigned long long core_idle = idle + iowait;

            /* Computed from the cumulative totals rather than a difference, so
               this is the average since boot, not the current rate. Matching
               the aggregate would need a stored previous value per core. */
            if (core_total > 0) {
                per_core_push(m,
                    100.0 * (double)(core_total - core_idle) / (double)core_total);
            }
        }
    }

    return true;
}

bool km_system_monitor_update_memory_stats(km_system_monitor *m) {
    char buf[KM_PROC_BUF_SIZE];
    km_meminfo info;
    unsigned long long buffers, cached;

    if (km_proc_read_meminfo(buf, sizeof(buf)) < 0) {
        return false;
    }

    km_parse_meminfo(buf, &info);

    /* /proc/meminfo publishes kB. */
    m->mem_total = info.mem_total * 1024;
    m->mem_free = info.mem_free * 1024;
    m->mem_available = info.mem_available * 1024;

    /* Buffers and page cache are reclaimable, so they do not count as used. */
    buffers = info.buffers * 1024;
    cached = info.cached * 1024;
    m->mem_used = m->mem_total - m->mem_free - buffers - cached;

    m->swap_total = info.swap_total * 1024;
    m->swap_free = info.swap_free * 1024;
    m->swap_used = m->swap_total - m->swap_free;

    if (m->mem_total > 0) {
        m->mem_usage_percent = 100.0 * (double)m->mem_used / (double)m->mem_total;
    }

    if (m->swap_total > 0) {
        m->swap_usage_percent = 100.0 * (double)m->swap_used / (double)m->swap_total;
    } else {
        m->swap_usage_percent = 0.0;
    }

    return true;
}

bool km_system_monitor_update_load_average(km_system_monitor *m) {
    char buf[KM_PROC_BUF_SIZE];

    if (km_proc_read_loadavg(buf, sizeof(buf)) < 0) {
        return false;
    }

    return km_parse_loadavg(buf, &m->load_avg_1, &m->load_avg_5, &m->load_avg_15);
}

void km_format_uptime(char *buf, size_t bufsz, double uptime) {
    int total_seconds = (int)uptime;
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;

    snprintf(buf, bufsz, "%02d:%02d:%02d", hours, minutes, seconds);
}

void km_format_bytes(char *buf, size_t bufsz, unsigned long long bytes) {
    static const char *const units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double value = (double)bytes;

    while (value >= 1024.0 && unit_index < 4) {
        value /= 1024.0;
        unit_index++;
    }

    snprintf(buf, bufsz, "%.2f %s", value, units[unit_index]);
}
