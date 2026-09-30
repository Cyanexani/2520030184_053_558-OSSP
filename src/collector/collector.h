#ifndef KM_COLLECTOR_H
#define KM_COLLECTOR_H

#include <pthread.h>
#include <stdbool.h>

#include "../process/process_monitor.h"
#include "../system/system_monitor.h"

/* [CO-6] Threads and processes. The monitor runs two threads inside one
   process: the main thread owns the terminal, and a collector thread sweeps
   /proc on a timer. Both see the same address space, which is what makes it
   possible to hand a sample from one to the other with no copying, and also
   what makes a lock necessary.

   Everything below the lock is shared. The two monitors are written only by
   the collector thread and read only by the main thread, and every such read
   or write happens with the lock held. */
typedef struct {
    pthread_t thread;
    bool running;

    pthread_mutex_t lock;
    pthread_cond_t wake;

    km_system_monitor *sys;
    km_process_monitor *proc;

    int interval_ms;
    bool refresh_requested;
    bool stopping;

    /* Bumped once per published sample, so the main thread can tell whether
       there is anything new to draw without comparing the data itself. */
    unsigned long generation;
} km_collector;

/* Start the collector thread. The monitors must already hold one sample and
   must not be touched outside km_collector_lock() from here on. */
bool km_collector_start(km_collector *c, km_system_monitor *sys,
                        km_process_monitor *proc, int interval_ms);

/* Ask the thread to finish, and wait for it. Safe to call if start failed. */
void km_collector_stop(km_collector *c);

/* Hold the lock for as short a time as possible, and never across anything
   that waits on the user: the collector cannot publish while it is held. */
void km_collector_lock(km_collector *c);
void km_collector_unlock(km_collector *c);

/* These take the lock themselves; do not call them while holding it. */
void km_collector_set_interval(km_collector *c, int interval_ms);
void km_collector_request_refresh(km_collector *c);

#endif /* KM_COLLECTOR_H */
