/*
 * zombie.c - creates a zombie (defunct) process on purpose.
 *
 * A process becomes a zombie the instant it terminates, and it stays one until
 * its parent collects the exit status with wait(). The kernel cannot free the
 * process table entry before that, because the exit status still has to be
 * delivered to somebody. Everything else is already gone: the address space,
 * the open files, the threads. That is why a zombie shows 0% CPU and no memory
 * of its own but still occupies a PID.
 *
 * The bug this models is a parent that forks and never waits. Those PIDs are
 * never released, and a long-running server doing this will eventually exhaust
 * the process table.
 *
 * Build: make demo
 * Run:   ./bin/zombie [seconds_to_hold]
 *
 * While it holds, prove the state in another terminal:
 *     ps -eo pid,ppid,stat,comm | grep -w Z
 * or watch the STATE column in Kernel Monitor, where it reads "Zombie".
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>   /* pid_t */
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int hold = (argc > 1) ? atoi(argv[1]) : 30;
    if (hold < 1) {
        hold = 30;
    }

    printf("parent  : pid=%d\n", (int)getpid());
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Child. Exits at once and is a zombie from here until the parent waits. */
        printf("child   : pid=%d ppid=%d, exiting now with status 42\n",
               (int)getpid(), (int)getppid());
        fflush(stdout);
        _exit(42);
    }

    /*
     * Parent. Deliberately does NOT wait() yet. The child is finished but the
     * kernel keeps its entry alive holding the exit status, so it is a zombie
     * for exactly as long as we stall here.
     */
    printf("\n");
    printf("ZOMBIE CREATED. Child pid %d has exited but has not been reaped.\n", (int)pid);
    printf("Holding for %d seconds. In another terminal, run:\n", hold);
    printf("    ps -eo pid,ppid,stat,comm | grep -w Z\n");
    printf("The STAT column shows Z and the command appears as <defunct>.\n");
    printf("In Kernel Monitor, pid %d has STATE \"Zombie\".\n", (int)pid);
    printf("\n");
    fflush(stdout);

    sleep(hold);

    /* Reaping it. This is the single call that makes the zombie disappear. */
    int status = 0;
    pid_t reaped = waitpid(pid, &status, 0);

    printf("parent  : waitpid() returned %d\n", (int)reaped);
    if (WIFEXITED(status)) {
        printf("parent  : collected exit status %d\n", WEXITSTATUS(status));
    }
    printf("ZOMBIE REAPED. Pid %d is gone from the process table now.\n", (int)pid);
    printf("Re-run the ps command above and it returns nothing.\n");

    return 0;
}
