#include "collector.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "../utils/parser.h"

static void add_ms(struct timespec *ts, int ms) {
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000L;
    }
}

static void *collector_main(void *arg) {
    km_collector *c = arg;
    struct timespec last;

    km_clock_now(&last);

    pthread_mutex_lock(&c->lock);
    for (;;) {
        /* [CO-6] Condition variables. Sleep until the interval has elapsed, a
           refresh is asked for, or shutdown begins. The predicate is re-checked
           in a loop because pthread_cond_timedwait may return spuriously, and
           the deadline is recomputed after every wakeup so that changing the
           interval with + or - takes effect at once rather than after the old
           interval runs out. The wait releases the lock while asleep and takes
           it back before returning, which is the whole point of pairing a
           condition variable with a mutex. */
        while (!c->stopping && !c->refresh_requested) {
            struct timespec deadline = last;
            add_ms(&deadline, c->interval_ms);
            if (pthread_cond_timedwait(&c->wake, &c->lock, &deadline) == ETIMEDOUT) {
                break;
            }
        }
        if (c->stopping) {
            break;
        }
        c->refresh_requested = false;

        /* [CO-6] Keep the critical section small. The /proc sweep is over a
           thousand system calls and is the slow part, so it runs unlocked;
           only this thread touches the buffer it fills. The main thread can
           draw the previous sample the whole time. */
        pthread_mutex_unlock(&c->lock);
        km_clock_now(&last);
        km_process_monitor_collect(c->proc);
        pthread_mutex_lock(&c->lock);

        /* [CO-6] Mutual exclusion. The swap in publish() and the system
           figures are what the main thread reads, so they change only while
           the lock is held. Without it the UI could read a count from the new
           sample and a pointer from the old one. The system update is a
           handful of small reads, cheap enough to leave inside. */
        km_system_monitor_update(c->sys);
        km_process_monitor_publish(c->proc);
        c->generation++;
    }
    pthread_mutex_unlock(&c->lock);

    return NULL;
}

bool km_collector_start(km_collector *c, km_system_monitor *sys,
                        km_process_monitor *proc, int interval_ms) {
    pthread_condattr_t attr;
    sigset_t all;
    sigset_t previous;
    int rc;

    memset(c, 0, sizeof(*c));
    c->sys = sys;
    c->proc = proc;
    c->interval_ms = interval_ms;

    if (pthread_mutex_init(&c->lock, NULL) != 0) {
        return false;
    }

    /* Time the waits against CLOCK_MONOTONIC, the same clock the CPU% interval
       uses. The default, CLOCK_REALTIME, jumps when the wall clock is set. */
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    rc = pthread_cond_init(&c->wake, &attr);
    pthread_condattr_destroy(&attr);
    if (rc != 0) {
        pthread_mutex_destroy(&c->lock);
        return false;
    }

    /* [CO-3] and [CO-6] together: which thread receives a signal. A signal
       sent to a process is delivered to any one thread that does not block
       it. Blocking everything here, just around pthread_create, means the new
       thread inherits a full mask while the main thread keeps its own, so
       SIGINT, SIGTERM and the terminal's SIGWINCH always reach the main
       thread, where the handler and ncurses expect them. */
    sigfillset(&all);
    pthread_sigmask(SIG_SETMASK, &all, &previous);
    rc = pthread_create(&c->thread, NULL, collector_main, c);
    pthread_sigmask(SIG_SETMASK, &previous, NULL);

    if (rc != 0) {
        pthread_cond_destroy(&c->wake);
        pthread_mutex_destroy(&c->lock);
        return false;
    }

    c->running = true;
    return true;
}

void km_collector_stop(km_collector *c) {
    if (!c->running) {
        return;
    }

    pthread_mutex_lock(&c->lock);
    c->stopping = true;
    pthread_cond_signal(&c->wake);
    pthread_mutex_unlock(&c->lock);

    /* Waits for at most one sweep in progress to finish. */
    pthread_join(c->thread, NULL);
    c->running = false;

    pthread_cond_destroy(&c->wake);
    pthread_mutex_destroy(&c->lock);
}

void km_collector_lock(km_collector *c) {
    pthread_mutex_lock(&c->lock);
}

void km_collector_unlock(km_collector *c) {
    pthread_mutex_unlock(&c->lock);
}

void km_collector_set_interval(km_collector *c, int interval_ms) {
    pthread_mutex_lock(&c->lock);
    c->interval_ms = interval_ms;
    pthread_cond_signal(&c->wake);
    pthread_mutex_unlock(&c->lock);
}

void km_collector_request_refresh(km_collector *c) {
    pthread_mutex_lock(&c->lock);
    c->refresh_requested = true;
    pthread_cond_signal(&c->wake);
    pthread_mutex_unlock(&c->lock);
}
