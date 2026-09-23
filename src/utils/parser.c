#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool km_read_line(const char **cursor, char *out, size_t out_size) {
    const char *p = *cursor;
    const char *nl;
    size_t len, copy;

    if (out_size == 0 || *p == '\0') {
        return false;
    }

    nl = strchr(p, '\n');
    len = nl ? (size_t)(nl - p) : strlen(p);

    copy = len < out_size - 1 ? len : out_size - 1;
    memcpy(out, p, copy);
    out[copy] = '\0';

    /* Advance past the whole line even when the copy was truncated. */
    *cursor = nl ? nl + 1 : p + len;
    return true;
}

static bool is_space_char(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char *km_trim(char *s) {
    char *end;

    while (is_space_char(*s)) {
        s++;
    }
    if (*s == '\0') {
        return s;
    }

    end = s + strlen(s) - 1;
    while (end > s && is_space_char(*end)) {
        end--;
    }
    end[1] = '\0';
    return s;
}

bool km_parse_key_value(char *line, char **key, char **value) {
    char *colon = strchr(line, ':');
    if (!colon) {
        return false;
    }

    *colon = '\0';
    *key = km_trim(line);
    *value = km_trim(colon + 1);
    return true;
}

void km_clock_now(struct timespec *ts) {
    if (clock_gettime(CLOCK_MONOTONIC, ts) != 0) {
        ts->tv_sec = 0;
        ts->tv_nsec = 0;
    }
}

double km_clock_diff_seconds(const struct timespec *start, const struct timespec *end) {
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / 1e9;
}

long km_clock_diff_ms(const struct timespec *start, const struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000L +
           (end->tv_nsec - start->tv_nsec) / 1000000L;
}

bool km_parse_cpu_line(const char *line,
                       unsigned long long *user,
                       unsigned long long *nice,
                       unsigned long long *system,
                       unsigned long long *idle,
                       unsigned long long *iowait,
                       unsigned long long *irq,
                       unsigned long long *softirq) {
    char label[32];

    if (sscanf(line, "%31s %llu %llu %llu %llu %llu %llu %llu",
               label, user, nice, system, idle, iowait, irq, softirq) != 8) {
        return false;
    }

    return strncmp(label, "cpu", 3) == 0;
}

void km_parse_meminfo(const char *content, km_meminfo *out) {
    const char *cursor = content;
    char line[512];

    /* Anything /proc/meminfo does not report stays zero, so a caller reading
       an absent field gets 0 rather than a stale value. */
    memset(out, 0, sizeof(*out));

    while (km_read_line(&cursor, line, sizeof(line))) {
        char *key;
        char *value;
        unsigned long long v;

        if (!km_parse_key_value(line, &key, &value)) {
            continue;
        }

        /* Values carry a "kB" suffix, which strtoull stops at. */
        v = strtoull(value, NULL, 10);

        if (strcmp(key, "MemTotal") == 0) {
            out->mem_total = v;
        } else if (strcmp(key, "MemFree") == 0) {
            out->mem_free = v;
        } else if (strcmp(key, "MemAvailable") == 0) {
            out->mem_available = v;
        } else if (strcmp(key, "Buffers") == 0) {
            out->buffers = v;
        } else if (strcmp(key, "Cached") == 0) {
            out->cached = v;
        } else if (strcmp(key, "SwapTotal") == 0) {
            out->swap_total = v;
        } else if (strcmp(key, "SwapFree") == 0) {
            out->swap_free = v;
        }
    }
}

bool km_parse_proc_stat(const char *content,
                        int *pid,
                        char *comm, size_t comm_size,
                        char *state,
                        int *ppid,
                        unsigned long long *utime,
                        unsigned long long *stime,
                        long *priority,
                        long *nice,
                        long *num_threads) {
    const char *open_paren;
    const char *close_paren;
    size_t comm_len;

    /* The comm field is wrapped in parentheses and may itself contain spaces
       and parentheses, so it cannot be found by counting whitespace. Bracket
       it with the first '(' and the last ')' instead. */
    open_paren = strchr(content, '(');
    close_paren = strrchr(content, ')');

    if (!open_paren || !close_paren || close_paren < open_paren || comm_size == 0) {
        return false;
    }

    if (sscanf(content, "%d", pid) != 1) {
        *pid = 0;
    }

    comm_len = (size_t)(close_paren - open_paren - 1);
    if (comm_len >= comm_size) {
        comm_len = comm_size - 1;
    }
    memcpy(comm, open_paren + 1, comm_len);
    comm[comm_len] = '\0';

    /* Everything after the closing parenthesis is plain whitespace-separated
       fields, numbered from 3. Fields 5-13 and 16-17 are skipped with %*s,
       which steps over a token whatever its sign or width. */
    return sscanf(close_paren + 1,
                  " %c %d"                      /* 3 state, 4 ppid            */
                  " %*s %*s %*s %*s %*s"        /* 5-9                        */
                  " %*s %*s %*s %*s"            /* 10-13                      */
                  " %llu %llu"                  /* 14 utime, 15 stime         */
                  " %*s %*s"                    /* 16 cutime, 17 cstime       */
                  " %ld %ld %ld",               /* 18 prio, 19 nice, 20 thrds */
                  state, ppid, utime, stime, priority, nice, num_threads) == 7;
}

bool km_parse_status_uid(const char *content, uid_t *out_uid) {
    const char *cursor = content;
    char line[512];

    while (km_read_line(&cursor, line, sizeof(line))) {
        char *key;
        char *value;

        if (!km_parse_key_value(line, &key, &value)) {
            continue;
        }

        if (strcmp(key, "Uid") == 0) {
            /* "Uid:" lists real, effective, saved and filesystem UIDs; the
               first is the one shown. */
            *out_uid = (uid_t)strtoul(value, NULL, 10);
            return true;
        }
    }

    return false;
}

bool km_parse_proc_statm(const char *content,
                         unsigned long *size,
                         unsigned long *resident,
                         unsigned long *shared) {
    return sscanf(content, "%lu %lu %lu", size, resident, shared) == 3;
}

bool km_parse_uptime(const char *content, double *uptime, double *idle) {
    return sscanf(content, "%lf %lf", uptime, idle) == 2;
}

bool km_parse_loadavg(const char *content, double *load1, double *load5, double *load15) {
    return sscanf(content, "%lf %lf %lf", load1, load5, load15) == 3;
}
