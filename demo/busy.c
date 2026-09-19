/*
 * busy.c - a process that stays in the Running (R) state and says so, until
 * you stop it.
 *
 * Useful for demonstrating three things in Kernel Monitor at once: a non-zero
 * CPU% column, a process whose STATE reads Running rather than Sleeping, and a
 * live target for the signal keys.
 *
 * The throttling matters. A loop that calls printf() as fast as it can spends
 * most of its life blocked on write(), so the kernel reports it as S (sleeping
 * on I/O) and the demonstration shows the opposite of what it means to. This
 * program therefore burns a fixed slice of real CPU between prints, so it is
 * almost always caught in R while the output stays readable.
 *
 * It also installs a handler for SIGINT and SIGTERM, so Ctrl+C or the K key in
 * Kernel Monitor makes it exit cleanly with a summary. SIGKILL, on the X key,
 * cannot be caught, so it dies instantly with no summary. Running it once each
 * way is the clearest demonstration of the difference between those two.
 *
 * Build: make demo
 * Run:   ./bin/busy                 runs until stopped
 *        ./bin/busy 30              stops itself after 30 seconds
 *        ./bin/busy 0 10            runs until stopped, 10 prints per second
 *
 * Confirm the state from another terminal:
 *     ps -o pid,stat,comm -p $(pgrep -x busy)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Written from a signal handler, so it must be written atomically and never
 * cached in a register. The handler does nothing else; the loop does the work. */
static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_signal  = 0;

static void on_signal(int sig) {
    g_signal = sig;
    g_running = 0;
}

static void install_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Monotonic milliseconds; unaffected by wall-clock adjustments. */
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(int argc, char* argv[]) {
    int seconds = (argc > 1) ? atoi(argv[1]) : 0;   /* 0 means run until stopped */
    int rate    = (argc > 2) ? atoi(argv[2]) : 2;

    if (seconds < 0) {
        seconds = 0;
    }
    if (rate < 1) {
        rate = 1;
    }
    if (rate > 50) {
        rate = 50;      /* beyond this the terminal, not the CPU, is the load */
    }

    install_handlers();

    const double slice_ms = 1000.0 / rate;
    const double started  = now_ms();
    const double deadline = seconds > 0 ? started + seconds * 1000.0 : 0.0;

    printf("busy    : pid=%d ppid=%d\n", (int)getpid(), (int)getppid());
    if (seconds > 0) {
        printf("busy    : running for %ds at %d prints/sec\n", seconds, rate);
    } else {
        printf("busy    : running until stopped, %d prints/sec\n", rate);
        printf("busy    : Ctrl+C, or K in Kernel Monitor, exits cleanly\n");
        printf("busy    : X in Kernel Monitor sends SIGKILL and it dies at once\n");
    }
    printf("busy    : STATE should read Running, CPU%% near 100%% of one core\n\n");
    fflush(stdout);

    unsigned long long iterations = 0;
    volatile unsigned long long sink = 0;   /* volatile so the loop is not optimised away */
    long ticks = 0;

    while (g_running) {
        if (deadline > 0.0 && now_ms() >= deadline) {
            break;
        }

        /* Burn one slice of real CPU. This is what holds the state at R. */
        double until = now_ms() + slice_ms;
        while (now_ms() < until) {
            for (int i = 0; i < 4096; ++i) {
                sink += (unsigned long long)i * 2654435761ULL;
            }
            iterations++;
            if (!g_running) {
                break;      /* react to a signal without finishing the slice */
            }
        }

        if (!g_running) {
            break;
        }

        ticks++;
        printf("running   tick=%-6ld  iterations=%-12llu  pid=%d\n",
               ticks, iterations, (int)getpid());
        fflush(stdout);
    }

    double elapsed = (now_ms() - started) / 1000.0;
    printf("\n");
    if (g_signal == SIGINT) {
        printf("busy    : caught SIGINT, shutting down cleanly\n");
    } else if (g_signal == SIGTERM) {
        printf("busy    : caught SIGTERM, shutting down cleanly\n");
    } else {
        printf("busy    : reached its time limit\n");
    }
    printf("busy    : ran %.1fs, %ld ticks, %llu iterations\n",
           elapsed, ticks, iterations);
    return 0;
}
