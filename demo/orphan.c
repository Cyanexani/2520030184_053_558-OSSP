/*
 * orphan.c - creates an orphan process on purpose.
 *
 * An orphan is a process whose parent died while it was still running. It is
 * not an error and nothing is leaked: the kernel re-parents it immediately so
 * that somebody is still responsible for reaping it when it eventually exits.
 * That new parent is pid 1 (init, or systemd), or the nearest ancestor that
 * marked itself a subreaper.
 *
 * Note how this is the opposite of a zombie. A zombie has exited and its parent
 * is alive but negligent. An orphan is alive and its parent has exited. The
 * kernel handles the orphan for you; the zombie is the one that needs fixing.
 *
 * Build: make demo
 * Run:   ./bin/orphan [seconds_to_live]
 *
 * Watch the PPID change in the output, or from another terminal:
 *     ps -eo pid,ppid,stat,comm | grep orphan
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>   /* pid_t */
#include <unistd.h>

int main(int argc, char* argv[]) {
    int lifetime = (argc > 1) ? atoi(argv[1]) : 25;
    if (lifetime < 4) {
        lifetime = 25;
    }

    pid_t parent_pid = getpid();
    printf("parent  : pid=%d, about to fork\n", (int)parent_pid);
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid > 0) {
        /* Parent. Exits first, which is what orphans the child. */
        printf("parent  : forked child pid=%d\n", (int)pid);
        printf("parent  : exiting in 3 seconds, which will orphan the child\n");
        fflush(stdout);
        sleep(3);
        printf("parent  : exiting now\n");
        fflush(stdout);
        return 0;
    }

    /* Child. Outlives its parent and reports who its parent is each second. */
    pid_t original_ppid = getppid();
    printf("child   : pid=%d ppid=%d (parent still alive)\n",
           (int)getpid(), (int)original_ppid);
    fflush(stdout);

    int reported = 0;
    for (int i = 0; i < lifetime; ++i) {
        sleep(1);
        pid_t now = getppid();

        if (now != original_ppid && !reported) {
            printf("\n");
            printf("ORPHANED. Parent %d is gone.\n", (int)original_ppid);
            printf("child   : ppid changed %d -> %d\n", (int)original_ppid, (int)now);
            printf("The kernel re-parented this process. Pid %d is init or systemd,\n", (int)now);
            printf("or the nearest ancestor registered as a subreaper.\n");
            printf("This process is still running normally, with %d seconds left.\n",
                   lifetime - i);
            printf("\n");
            fflush(stdout);
            reported = 1;
        } else {
            printf("child   : alive, pid=%d ppid=%d\n", (int)getpid(), (int)now);
            fflush(stdout);
        }
    }

    printf("child   : exiting. Its new parent reaps it, so no zombie is left behind.\n");
    return 0;
}
