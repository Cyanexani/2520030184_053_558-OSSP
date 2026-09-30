/*
 * jobctl.c - process groups, sessions and job control, driven by hand.
 *
 * [CO-3] Process groups. Every process belongs to one group, named by the pid
 * of its leader. kill() with a negative pid signals the whole group in a
 * single call. That is how a shell stops, continues or kills a whole job,
 * and how Ctrl+C reaches every stage of a pipeline at once.
 *
 * [CO-3] Sessions. A session is a set of groups that share one controlling
 * terminal, normally everything started from one login shell. Exactly one
 * group in the session is the foreground group: the one that receives the
 * keyboard's Ctrl+C and Ctrl+Z. tcgetpgrp() asks the terminal which group
 * that is. setsid() starts a brand new session with no terminal at all,
 * which is the first step every daemon takes.
 *
 * [CO-3] Job control. Ctrl+Z is SIGTSTP, bg and fg are SIGCONT, and a stopped
 * job has state T. This program sends SIGSTOP, SIGCONT and SIGTERM to a group
 * of three workers and reads each worker's state back from /proc to prove the
 * signal reached all of them.
 *
 * Build: make demo
 * Run:   ./bin/jobctl [seconds_per_phase]
 *
 * With a longer phase, say ./bin/jobctl 6, open Kernel Monitor alongside it.
 * The three workers switch between Sleeping and Stopped together, and the
 * detail view of any worker shows the same Process Group number.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define WORKERS 3

static volatile sig_atomic_t g_group = 0;

/* [CO-3] Pressing Ctrl+C signals the foreground group, which is this process
   alone. The workers are in their own group, so the terminal never signals
   them. Forwarding it by hand is exactly what a shell does for its jobs.
   kill() and _exit() are async-signal-safe, so both are allowed here. */
static void on_interrupt(int sig) {
    (void)sig;
    if (g_group > 0) {
        kill(-g_group, SIGTERM);
    }
    _exit(130);
}

/* Reads the one-letter state from /proc/<pid>/stat, or '?' if unreadable. */
static char proc_state(pid_t pid) {
    char path[64], buf[512];
    char *close_paren;
    ssize_t n;
    int fd;

    snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return '?';
    }
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        return '?';
    }
    buf[n] = '\0';
    close_paren = strrchr(buf, ')');
    return (close_paren && close_paren[1] == ' ') ? close_paren[2] : '?';
}

static const char *state_name(char s) {
    switch (s) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'T': return "Stopped";
        case 'Z': return "Zombie";
        default:  return "Unknown";
    }
}

/* Waits up to a second for every worker to reach the expected state. */
static void show_states(const pid_t *workers, char expect) {
    for (int tries = 0; tries < 50; tries++) {
        int ready = 1;
        for (int i = 0; i < WORKERS; i++) {
            char s = proc_state(workers[i]);
            if (s != expect && !(expect == 'S' && s == 'R')) {
                ready = 0;
            }
        }
        if (ready) {
            break;
        }
        usleep(20000);
    }
    for (int i = 0; i < WORKERS; i++) {
        char s = proc_state(workers[i]);
        printf("          worker pid %d  pgid %d  state %c (%s)\n",
               (int)workers[i], (int)getpgid(workers[i]), s, state_name(s));
    }
    fflush(stdout);
}

static void show_identity(const char *who) {
    printf("%-8s: pid=%d ppid=%d pgid=%d sid=%d\n", who, (int)getpid(),
           (int)getppid(), (int)getpgrp(), (int)getsid(0));
}

/* Reads tty_nr, field 7 of /proc/self/stat. 0 means no controlling terminal. */
static int own_tty_nr(void) {
    char buf[512];
    char state;
    int ppid, pgrp, session, tty_nr = -1;
    ssize_t n;
    char *close_paren;
    int fd = open("/proc/self/stat", O_RDONLY);

    if (fd == -1) {
        return -1;
    }
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        return -1;
    }
    buf[n] = '\0';
    close_paren = strrchr(buf, ')');
    if (close_paren == NULL ||
        sscanf(close_paren + 1, " %c %d %d %d %d", &state, &ppid, &pgrp, &session, &tty_nr) != 5) {
        return -1;
    }
    return tty_nr;
}

/* Decodes tty_nr the way ps does. Pseudo terminals use majors 136 to 143. */
static void describe_tty(int tty_nr, char *out, size_t size) {
    int major = (tty_nr >> 8) & 0xfff;
    int minor = (tty_nr & 0xff) | ((tty_nr >> 12) & 0xfff00);

    if (tty_nr == 0) {
        snprintf(out, size, "no controlling terminal");
    } else if (major >= 136 && major <= 143) {
        snprintf(out, size, "terminal pts/%d", (major - 136) * 256 + minor);
    } else {
        snprintf(out, size, "terminal %d:%d", major, minor);
    }
}

static void worker_loop(pid_t parent) {
    /* Sleep in short naps so the worker shows state S, and leave if the
       parent disappears so no worker is ever left behind. */
    for (;;) {
        if (getppid() != parent) {
            _exit(0);
        }
        usleep(100000);
    }
}

int main(int argc, char *argv[]) {
    int phase = (argc > 1) ? atoi(argv[1]) : 1;
    pid_t workers[WORKERS];
    pid_t parent = getpid();
    struct sigaction sa;
    char tty_text[64];
    int tty_fd;

    if (phase < 1) {
        phase = 1;
    }

    /* Part 1. Where this program sits. */
    printf("Part 1. Who am I\n");
    show_identity("jobctl");
    describe_tty(own_tty_nr(), tty_text, sizeof(tty_text));
    printf("jobctl  : controlling terminal from /proc/self/stat: %s\n", tty_text);

    /* /dev/tty always means "my controlling terminal", even when stdin is
       redirected, so it is the reliable thing to ask. */
    tty_fd = open("/dev/tty", O_RDONLY);
    if (tty_fd != -1) {
        pid_t fg = tcgetpgrp(tty_fd);
        printf("jobctl  : that terminal's foreground group is %d, so this program is %s\n",
               (int)fg, fg == getpgrp() ? "in the foreground" : "in the background");
        close(tty_fd);
    } else {
        printf("jobctl  : no controlling terminal, so there is no foreground group\n");
    }
    if (getpgrp() == getpid()) {
        printf("jobctl  : pgid == pid, so the shell made this program leader of its own group\n");
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_interrupt;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    /* Part 2. Three workers in one new process group. */
    printf("\nPart 2. A job of %d workers in one process group\n", WORKERS);
    fflush(stdout);
    for (int i = 0; i < WORKERS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (g_group > 0) {
                kill(-g_group, SIGTERM);
            }
            return 1;
        }
        if (pid == 0) {
            /* Workers take the default Ctrl+C action, not the forwarding one. */
            signal(SIGINT, SIG_DFL);
            /* The first worker leads a new group, the others join it. */
            setpgid(0, i == 0 ? 0 : workers[0]);
            worker_loop(parent);
        }
        workers[i] = pid;
        /* Set it from the parent as well, so the group exists before the next
           fork no matter which process the scheduler runs first. */
        setpgid(pid, workers[0]);
        if (i == 0) {
            g_group = workers[0];
        }
    }
    printf("jobctl  : process group %d holds all three workers\n", (int)g_group);
    show_states(workers, 'S');
    sleep((unsigned)phase);

    printf("\njobctl  : kill(-%d, SIGSTOP), one call for the whole group, like Ctrl+Z\n",
           (int)g_group);
    kill(-g_group, SIGSTOP);
    show_states(workers, 'T');
    sleep((unsigned)phase);

    printf("\njobctl  : kill(-%d, SIGCONT), like typing fg or bg\n", (int)g_group);
    kill(-g_group, SIGCONT);
    show_states(workers, 'S');
    sleep((unsigned)phase);

    printf("\njobctl  : kill(-%d, SIGTERM), ending the whole job\n", (int)g_group);
    kill(-g_group, SIGTERM);
    for (int i = 0; i < WORKERS; i++) {
        int status;
        if (waitpid(workers[i], &status, 0) == workers[i] && WIFSIGNALED(status)) {
            printf("          reaped worker %d, killed by signal %d (%s)\n",
                   (int)workers[i], WTERMSIG(status), strsignal(WTERMSIG(status)));
        }
    }
    g_group = 0;

    /* Part 3. A new session. */
    printf("\nPart 3. setsid() and a session with no terminal\n");
    printf("jobctl  : my tty_nr in /proc/self/stat is %d, that is %s\n", own_tty_nr(), tty_text);
    fflush(stdout);
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        /* A fresh child is never a group leader, so setsid() is allowed. */
        if (setsid() == -1) {
            perror("setsid");
            _exit(1);
        }
        show_identity("child");
        printf("child   : new session, so sid == pgid == my pid\n");
        describe_tty(own_tty_nr(), tty_text, sizeof(tty_text));
        printf("child   : my tty_nr is now %d, that is %s\n", own_tty_nr(), tty_text);
        int fd = open("/dev/tty", O_RDWR);
        if (fd == -1) {
            printf("child   : open(\"/dev/tty\") fails with \"%s\", as expected\n",
                   strerror(errno));
        } else {
            printf("child   : open(\"/dev/tty\") unexpectedly worked\n");
            close(fd);
        }
        fflush(stdout);
        _exit(0);
    }
    waitpid(child, NULL, 0);
    printf("jobctl  : done, every child reaped\n");
    return 0;
}
