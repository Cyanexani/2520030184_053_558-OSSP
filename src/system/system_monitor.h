#ifndef KM_SYSTEM_MONITOR_H
#define KM_SYSTEM_MONITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

/* System-wide statistics, refreshed as a block by km_system_monitor_update.
   Fields are read directly rather than through accessors. */
typedef struct {
    char kernel_version[64];
    int cpu_count;
    long page_size;
    double uptime;

    double cpu_usage_percent;
    double *per_core_cpu_usage;
    size_t per_core_count;
    size_t per_core_capacity;
    unsigned long long prev_total_cpu;
    unsigned long long prev_idle_cpu;

    double load_avg_1;
    double load_avg_5;
    double load_avg_15;

    /* Memory and swap figures are in bytes, converted from the kB that
       /proc/meminfo publishes. */
    unsigned long long mem_total;
    unsigned long long mem_used;
    unsigned long long mem_free;
    unsigned long long mem_available;
    double mem_usage_percent;

    unsigned long long swap_total;
    unsigned long long swap_used;
    unsigned long long swap_free;
    double swap_usage_percent;
} km_system_monitor;

void km_system_monitor_init(km_system_monitor *m);
void km_system_monitor_destroy(km_system_monitor *m);

bool km_system_monitor_update(km_system_monitor *m);

bool km_system_monitor_update_system_info(km_system_monitor *m);
bool km_system_monitor_update_cpu_stats(km_system_monitor *m);
bool km_system_monitor_update_memory_stats(km_system_monitor *m);
bool km_system_monitor_update_load_average(km_system_monitor *m);

/* Format seconds of uptime as HH:MM:SS. */
void km_format_uptime(char *buf, size_t bufsz, double uptime);

/* Format a byte count with the largest unit that keeps it above 1. */
void km_format_bytes(char *buf, size_t bufsz, unsigned long long bytes);

#endif /* KM_SYSTEM_MONITOR_H */
