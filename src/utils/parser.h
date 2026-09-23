#ifndef KM_PARSER_H
#define KM_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

/* Text helpers ----------------------------------------------------------- */

/* Copy the next line out of a NUL-terminated buffer and advance the cursor
   past its newline, returning false at the end of the text. A line longer
   than out_size is truncated, but the cursor still skips all of it, so the
   following line is unaffected. */
bool km_read_line(const char **cursor, char *out, size_t out_size);

/* Trim spaces, tabs and line endings in place. Returns a pointer into s. */
char *km_trim(char *s);

/* Split "Key:  value" at the first colon, trimming both halves. Rewrites line
   in place, so key and value point into it. */
bool km_parse_key_value(char *line, char **key, char **value);

/* Monotonic clock helpers. CLOCK_MONOTONIC, not the wall clock, so that an
   NTP step or a daylight-saving change cannot make an interval negative. */
void km_clock_now(struct timespec *ts);
double km_clock_diff_seconds(const struct timespec *start, const struct timespec *end);
long km_clock_diff_ms(const struct timespec *start, const struct timespec *end);

/* /proc parsers ---------------------------------------------------------- */

/* Parse one "cpu"/"cpuN" line of /proc/stat. */
bool km_parse_cpu_line(const char *line,
                       unsigned long long *user,
                       unsigned long long *nice,
                       unsigned long long *system,
                       unsigned long long *idle,
                       unsigned long long *iowait,
                       unsigned long long *irq,
                       unsigned long long *softirq);

/* The /proc/meminfo entries the monitor uses, in kB exactly as published. */
typedef struct {
    unsigned long long mem_total;
    unsigned long long mem_free;
    unsigned long long mem_available;
    unsigned long long buffers;
    unsigned long long cached;
    unsigned long long swap_total;
    unsigned long long swap_free;
} km_meminfo;

/* Fields absent from /proc/meminfo are left at zero. */
void km_parse_meminfo(const char *content, km_meminfo *out);

/* Parse /proc/[pid]/stat. */
bool km_parse_proc_stat(const char *content,
                        int *pid,
                        char *comm, size_t comm_size,
                        char *state,
                        int *ppid,
                        unsigned long long *utime,
                        unsigned long long *stime,
                        long *priority,
                        long *nice,
                        long *num_threads);

/* Pull the real UID out of /proc/[pid]/status. */
bool km_parse_status_uid(const char *content, uid_t *out_uid);

/* Parse /proc/[pid]/statm. Values are in pages, not bytes. */
bool km_parse_proc_statm(const char *content,
                         unsigned long *size,
                         unsigned long *resident,
                         unsigned long *shared);

/* Parse /proc/uptime. */
bool km_parse_uptime(const char *content, double *uptime, double *idle);

/* Parse /proc/loadavg. */
bool km_parse_loadavg(const char *content, double *load1, double *load5, double *load15);

#endif /* KM_PARSER_H */
