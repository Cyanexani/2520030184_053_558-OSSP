/*
 * threadsync.c - POSIX threads, and what goes wrong without synchronization.
 *
 * [CO-6] Threads and processes. A thread is a second flow of execution inside
 * the same process: same pid, same memory, same open files, but its own
 * stack, registers and thread id. A forked process gets a copy of everything
 * instead, so its writes are invisible to the parent.
 *
 * The tour, in order:
 *   1. Threads versus processes: who sees whose writes.
 *   2. A race condition on a shared counter, then two fixes: a mutex and an
 *      atomic operation.
 *   3. Condition variables: a bounded buffer between a producer and two
 *      consumers.
 *   4. A counting semaphore that admits two threads at a time.
 *   5. A deadlock, caught with a timed lock, then fixed by lock ordering.
 *   6. Advanced tools: a read-write lock and a barrier.
 *
 * Kernel Monitor itself uses the same ideas: a collector thread and the UI
 * thread share process data under one mutex, and a condition variable wakes
 * the collector early when you press R.
 *
 * Build: make demo
 * Run:   ./bin/threadsync
 */

#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define THREADS 4
#define INCREMENTS 1000000L

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

static void start_all(pthread_t *threads, int count, void *(*fn)(void *), void *args, size_t arg_size) {
    for (int i = 0; i < count; i++) {
        void *arg = args ? (char *)args + (size_t)i * arg_size : NULL;
        if (pthread_create(&threads[i], NULL, fn, arg) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            exit(1);
        }
    }
}

static void join_all(pthread_t *threads, int count) {
    for (int i = 0; i < count; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* Part 1 ------------------------------------------------------------------ */

static int g_shared = 0;

static void *set_shared(void *arg) {
    (void)arg;
    g_shared = 1;
    printf("  thread   pid %d  tid %d  set the shared variable to 1\n",
           (int)getpid(), (int)gettid());
    return NULL;
}

static void part_threads_vs_processes(void) {
    pthread_t thread;
    pid_t child;

    printf("Part 1. Threads versus processes\n");
    printf("  main     pid %d  tid %d  (the first thread's tid equals the pid)\n",
           (int)getpid(), (int)gettid());

    pthread_create(&thread, NULL, set_shared, NULL);
    pthread_join(thread, NULL);
    printf("  main     sees %d: a thread shares the process's memory\n", g_shared);

    fflush(stdout);
    child = fork();
    if (child == 0) {
        g_shared = 100;
        printf("  child    pid %d  set its copy to 100\n", (int)getpid());
        fflush(stdout);
        _exit(0);
    }
    waitpid(child, NULL, 0);
    printf("  main     still sees %d: a forked child wrote to its own copy\n\n", g_shared);
}

/* Part 2 ------------------------------------------------------------------ */

/* [CO-6] volatile makes the compiler load and store the counter on every
   iteration, so the race is visible. It does NOT make ++ atomic: ++ is a
   load, an add and a store, and two threads can interleave between them. */
static volatile long g_unsafe = 0;
static long g_locked = 0;
static atomic_long g_atomic = 0;
static pthread_mutex_t g_counter_lock = PTHREAD_MUTEX_INITIALIZER;

static void *add_unsafe(void *arg) {
    (void)arg;
    for (long i = 0; i < INCREMENTS; i++) {
        g_unsafe++;
    }
    return NULL;
}

static void *add_locked(void *arg) {
    (void)arg;
    for (long i = 0; i < INCREMENTS; i++) {
        pthread_mutex_lock(&g_counter_lock);
        g_locked++;                      /* the critical section */
        pthread_mutex_unlock(&g_counter_lock);
    }
    return NULL;
}

static void *add_atomic(void *arg) {
    (void)arg;
    for (long i = 0; i < INCREMENTS; i++) {
        atomic_fetch_add(&g_atomic, 1);  /* one indivisible CPU instruction */
    }
    return NULL;
}

static double run_counter(void *(*fn)(void *)) {
    pthread_t threads[THREADS];
    double t0 = now_ms();
    start_all(threads, THREADS, fn, NULL, 0);
    join_all(threads, THREADS);
    return now_ms() - t0;
}

static void part_race(void) {
    const long expected = THREADS * INCREMENTS;
    double ms;

    printf("Part 2. A race condition, %d threads each adding 1 to a shared counter %ld times\n",
           THREADS, INCREMENTS);
    printf("  expected total %ld\n", expected);

    ms = run_counter(add_unsafe);
    printf("  no lock    %9ld  lost %8ld updates  %7.1f ms\n",
           (long)g_unsafe, expected - (long)g_unsafe, ms);
    ms = run_counter(add_locked);
    printf("  mutex      %9ld  lost %8ld updates  %7.1f ms\n",
           g_locked, expected - g_locked, ms);
    ms = run_counter(add_atomic);
    printf("  atomic     %9ld  lost %8ld updates  %7.1f ms\n",
           (long)atomic_load(&g_atomic), expected - (long)atomic_load(&g_atomic), ms);
    printf("  The mutex lets one thread at a time into the critical section. The\n");
    printf("  atomic add needs no lock at all, but only works for single operations.\n\n");
}

/* Part 3 ------------------------------------------------------------------ */

#define SLOTS 4
#define ITEMS 10
#define CONSUMERS 2

static int g_buffer[SLOTS];
static int g_count = 0, g_head = 0, g_tail = 0;
static int g_producer_waits = 0;
static pthread_mutex_t g_buffer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t g_not_empty = PTHREAD_COND_INITIALIZER;

static void buffer_put(int item) {
    pthread_mutex_lock(&g_buffer_lock);
    /* [CO-6] Always re-check the condition in a loop. A wakeup only means
       "look again": another thread may have taken the slot first, and
       spurious wakeups are allowed. */
    while (g_count == SLOTS) {
        g_producer_waits++;
        pthread_cond_wait(&g_not_full, &g_buffer_lock);
    }
    g_buffer[g_tail] = item;
    g_tail = (g_tail + 1) % SLOTS;
    g_count++;
    pthread_cond_signal(&g_not_empty);
    pthread_mutex_unlock(&g_buffer_lock);
}

static int buffer_take(int *left) {
    int item;
    pthread_mutex_lock(&g_buffer_lock);
    while (g_count == 0) {
        pthread_cond_wait(&g_not_empty, &g_buffer_lock);
    }
    item = g_buffer[g_head];
    g_head = (g_head + 1) % SLOTS;
    g_count--;
    *left = g_count;
    pthread_cond_signal(&g_not_full);
    pthread_mutex_unlock(&g_buffer_lock);
    return item;
}

static void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= ITEMS; i++) {
        buffer_put(i);
    }
    for (int i = 0; i < CONSUMERS; i++) {
        buffer_put(-1);                  /* one stop marker per consumer */
    }
    return NULL;
}

static void *consumer(void *arg) {
    int id = *(int *)arg;
    int taken = 0, left;

    for (;;) {
        int item = buffer_take(&left);
        if (item == -1) {
            break;
        }
        taken++;
        printf("  consumer %d took item %2d, %d left in the buffer\n", id, item, left);
        usleep(20000);                   /* slow consumers make the buffer fill */
    }
    printf("  consumer %d stops after %d items\n", id, taken);
    return NULL;
}

static void part_bounded_buffer(void) {
    pthread_t prod, cons[CONSUMERS];
    int ids[CONSUMERS];

    printf("Part 3. Condition variables: a %d slot buffer, 1 producer, %d consumers\n",
           SLOTS, CONSUMERS);
    for (int i = 0; i < CONSUMERS; i++) {
        ids[i] = i + 1;
    }
    start_all(cons, CONSUMERS, consumer, ids, sizeof(ids[0]));
    pthread_create(&prod, NULL, producer, NULL);
    pthread_join(prod, NULL);
    join_all(cons, CONSUMERS);
    printf("  the producer found the buffer full and slept %d times instead of spinning\n\n",
           g_producer_waits);
}

/* Part 4 ------------------------------------------------------------------ */

#define VISITORS 6

static sem_t g_room;
static atomic_int g_inside = 0;
static atomic_int g_most_inside = 0;

static void *visitor(void *arg) {
    int id = *(int *)arg;
    int inside, most;

    sem_wait(&g_room);                   /* takes a permit, or blocks at 0 */
    inside = atomic_fetch_add(&g_inside, 1) + 1;
    most = atomic_load(&g_most_inside);
    while (inside > most && !atomic_compare_exchange_weak(&g_most_inside, &most, inside)) {
    }
    printf("  thread %d enters, %d inside\n", id, inside);
    usleep(50000);
    atomic_fetch_sub(&g_inside, 1);
    sem_post(&g_room);                   /* returns the permit */
    return NULL;
}

static void part_semaphore(void) {
    pthread_t threads[VISITORS];
    int ids[VISITORS];

    printf("Part 4. A counting semaphore with 2 permits, %d threads\n", VISITORS);
    sem_init(&g_room, 0, 2);
    for (int i = 0; i < VISITORS; i++) {
        ids[i] = i + 1;
    }
    start_all(threads, VISITORS, visitor, ids, sizeof(ids[0]));
    join_all(threads, VISITORS);
    sem_destroy(&g_room);
    printf("  never more than %d inside at once. A mutex is the special case of 1 permit.\n\n",
           atomic_load(&g_most_inside));
}

/* Part 5 ------------------------------------------------------------------ */

static pthread_mutex_t g_lock_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_lock_b = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t g_both_hold_one;

struct transfer {
    const char *name;
    pthread_mutex_t *first;
    pthread_mutex_t *second;
    const char *first_name;
    const char *second_name;
    int use_barrier;
};

static void *do_transfer(void *arg) {
    struct transfer *t = arg;
    struct timespec deadline;
    int rc;

    pthread_mutex_lock(t->first);
    printf("  %s holds lock %s, wants lock %s\n", t->name, t->first_name, t->second_name);
    if (t->use_barrier) {
        /* Make sure both threads hold their first lock before either asks
           for its second, so the deadlock happens every time. */
        pthread_barrier_wait(&g_both_hold_one);
    }

    /* A plain pthread_mutex_lock() here would wait forever. The timed
       version gives up after one second so the demo can report it. */
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;
    rc = pthread_mutex_timedlock(t->second, &deadline);
    if (rc == ETIMEDOUT) {
        printf("  %s waited 1 s for lock %s and gave up\n", t->name, t->second_name);
    } else {
        printf("  %s got lock %s and finished\n", t->name, t->second_name);
        pthread_mutex_unlock(t->second);
    }
    if (t->use_barrier) {
        /* Neither lets go until both have given up, so neither one's
           release lets the other through by luck. */
        pthread_barrier_wait(&g_both_hold_one);
    }
    pthread_mutex_unlock(t->first);
    return NULL;
}

static void part_deadlock(void) {
    pthread_t one, two;
    struct transfer bad1 = { "thread 1", &g_lock_a, &g_lock_b, "A", "B", 1 };
    struct transfer bad2 = { "thread 2", &g_lock_b, &g_lock_a, "B", "A", 1 };
    struct transfer good1 = { "thread 1", &g_lock_a, &g_lock_b, "A", "B", 0 };
    struct transfer good2 = { "thread 2", &g_lock_a, &g_lock_b, "A", "B", 0 };

    printf("Part 5. Deadlock\n");
    printf("  Opposite order: thread 1 locks A then B, thread 2 locks B then A.\n");
    pthread_barrier_init(&g_both_hold_one, NULL, 2);
    pthread_create(&one, NULL, do_transfer, &bad1);
    pthread_create(&two, NULL, do_transfer, &bad2);
    pthread_join(one, NULL);
    pthread_join(two, NULL);
    pthread_barrier_destroy(&g_both_hold_one);
    printf("  Each held what the other needed: circular wait, one of the four\n");
    printf("  conditions for deadlock (with mutual exclusion, hold and wait,\n");
    printf("  and no preemption).\n\n");

    printf("  Same order: both threads lock A, then B.\n");
    pthread_create(&one, NULL, do_transfer, &good1);
    pthread_create(&two, NULL, do_transfer, &good2);
    pthread_join(one, NULL);
    pthread_join(two, NULL);
    printf("  A global lock order makes a cycle impossible, so no deadlock.\n\n");
}

/* Part 6 ------------------------------------------------------------------ */

#define READERS 3

static pthread_rwlock_t g_table_lock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_barrier_t g_start_line;
static atomic_int g_readers_now = 0;
static atomic_int g_readers_most = 0;
static atomic_int g_readers_during_write = -1;

static void *reader_thread(void *arg) {
    (void)arg;
    pthread_barrier_wait(&g_start_line);
    pthread_rwlock_rdlock(&g_table_lock);
    int now = atomic_fetch_add(&g_readers_now, 1) + 1;
    int most = atomic_load(&g_readers_most);
    while (now > most && !atomic_compare_exchange_weak(&g_readers_most, &most, now)) {
    }
    usleep(100000);
    atomic_fetch_sub(&g_readers_now, 1);
    pthread_rwlock_unlock(&g_table_lock);
    return NULL;
}

static void *writer_thread(void *arg) {
    (void)arg;
    pthread_barrier_wait(&g_start_line);
    usleep(20000);                       /* let the readers in first */
    pthread_rwlock_wrlock(&g_table_lock);
    atomic_store(&g_readers_during_write, atomic_load(&g_readers_now));
    pthread_rwlock_unlock(&g_table_lock);
    return NULL;
}

static void part_advanced(void) {
    pthread_t readers[READERS], writer;

    printf("Part 6. Advanced tools: a read-write lock and a barrier\n");
    /* [CO-6] A barrier holds every thread until all of them arrive, so they
       start the race together. */
    pthread_barrier_init(&g_start_line, NULL, READERS + 1);
    start_all(readers, READERS, reader_thread, NULL, 0);
    pthread_create(&writer, NULL, writer_thread, NULL);
    join_all(readers, READERS);
    pthread_join(writer, NULL);
    pthread_barrier_destroy(&g_start_line);

    printf("  %d readers held the read lock at the same time\n", atomic_load(&g_readers_most));
    printf("  the writer waited, and while it held the lock %d readers were inside\n",
           atomic_load(&g_readers_during_write));
    printf("  Readers share, a writer is exclusive: good for data read far more\n");
    printf("  often than it is changed.\n");
}

int main(void) {
    part_threads_vs_processes();
    part_race();
    part_bounded_buffer();
    part_semaphore();
    part_deadlock();
    part_advanced();
    return 0;
}
