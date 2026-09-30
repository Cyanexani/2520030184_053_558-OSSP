/*
 * pipeline.c - what a shell actually does when you type  a | b
 *
 * [CO-1] Command execution journey. A shell is an ordinary user-space program.
 * To run a command it asks the kernel for a new process with fork(), replaces
 * that process's program with execvp(), and waits for it with waitpid(). This
 * program performs those same steps, out loud, for a two-stage pipeline.
 *
 * [CO-3] Anonymous pipes. pipe() returns two file descriptors joined by a
 * buffer inside the kernel: bytes written to fds[1] come out of fds[0]. The
 * pipe has no name, so only processes that inherit the descriptors across
 * fork() can use it. dup2() then moves each end onto stdout or stdin, which is
 * why neither program in the pipeline needs to know a pipe is involved.
 *
 * [CO-3] Process groups. Both children are placed in one new process group,
 * the way a shell groups a pipeline into a single job. That is why Ctrl+C
 * stops every stage of  a | b  at once: the terminal signals the group.
 *
 * Build: make demo
 * Run:   ./bin/pipeline                      runs  ls /proc | grep -c ^[0-9]
 *        ./bin/pipeline ps -e -- wc -l       any two commands, split by --
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void report(const char *who, pid_t pid, int status) {
    if (WIFEXITED(status)) {
        printf("shell   : %s (pid %d) exited with status %d\n",
               who, (int)pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("shell   : %s (pid %d) was killed by signal %d\n",
               who, (int)pid, WTERMSIG(status));
    }
}

int main(int argc, char *argv[]) {
    static char *default_left[]  = { "ls", "/proc", NULL };
    static char *default_right[] = { "grep", "-c", "^[0-9]", NULL };
    char **left = default_left;
    char **right = default_right;
    int fds[2];
    pid_t first, second;
    int status;

    /* Split argv at "--" into the two commands. */
    if (argc > 1) {
        int split = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--") == 0) {
                split = i;
                break;
            }
        }
        if (split <= 1 || split == argc - 1) {
            fprintf(stderr, "usage: %s [cmd1 args... -- cmd2 args...]\n", argv[0]);
            return 2;
        }
        argv[split] = NULL;
        left = &argv[1];
        right = &argv[split + 1];
    }

    printf("shell   : pid=%d, running  %s ... | %s ...\n", (int)getpid(), left[0], right[0]);

    /* Step 1. Ask the kernel for a pipe. */
    if (pipe(fds) == -1) {
        perror("pipe");
        return 1;
    }
    printf("shell   : pipe() gave read end fd %d and write end fd %d\n", fds[0], fds[1]);

    /* [CO-2] Common pitfall. stdout is buffered inside this process. Anything
       still in the buffer at fork() is copied into the child and printed
       twice, once by each. Flushing before every fork() prevents that. */
    fflush(stdout);

    /* Step 2. First child: its stdout becomes the pipe's write end. */
    first = fork();
    if (first == -1) {
        perror("fork");
        return 1;
    }
    if (first == 0) {
        setpgid(0, 0);                  /* new group, led by this child */
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execvp(left[0], left);
        /* Only reached if exec failed: the program image was not replaced. */
        fprintf(stderr, "exec %s: %s\n", left[0], strerror(errno));
        _exit(127);
    }
    /* Also set it from the parent. Whichever of the two runs first wins, so
       the group exists before the second child tries to join it. */
    setpgid(first, first);

    fflush(stdout);

    /* Step 3. Second child: its stdin becomes the pipe's read end, and it
       joins the first child's process group. */
    second = fork();
    if (second == -1) {
        perror("fork");
        return 1;
    }
    if (second == 0) {
        setpgid(0, first);
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);
        execvp(right[0], right);
        fprintf(stderr, "exec %s: %s\n", right[0], strerror(errno));
        _exit(127);
    }
    setpgid(second, first);

    printf("shell   : forked %s as pid %d and %s as pid %d\n",
           left[0], (int)first, right[0], (int)second);
    printf("shell   : both are in process group %d, mine is %d\n",
           (int)getpgid(second), (int)getpgrp());

    /* [CO-3] Common pitfall. The reader sees end-of-file only when every copy
       of the write end is closed. This process still holds one from pipe(),
       and if it kept it, the second command would wait for input forever. */
    close(fds[0]);
    close(fds[1]);
    printf("shell   : closed my copies of both ends, so the reader can see EOF\n");
    printf("shell   : output of the pipeline follows\n\n");
    fflush(stdout);

    /* Step 4. Wait for both, reaping them so neither is left a zombie. */
    if (waitpid(first, &status, 0) == first) {
        putchar('\n');
        report(left[0], first, status);
    }
    if (waitpid(second, &status, 0) == second) {
        report(right[0], second, status);
    }

    return 0;
}
