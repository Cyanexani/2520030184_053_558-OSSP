/*
 * fifo.c - a named pipe (FIFO) carrying messages between two processes.
 *
 * [CO-3] Named pipes. An anonymous pipe only works between relatives, because
 * the only way to reach it is to inherit its file descriptors across fork().
 * mkfifo() gives the same kind of kernel buffer a name in the file system, so
 * any two processes that can see the path can talk through it, even if they
 * were started from different terminals.
 *
 * The file on disk is only a meeting point. stat() reports its type as FIFO
 * and its size as 0 bytes, because the data never touches the disk. It lives
 * in a kernel buffer and is gone once it has been read.
 *
 * [CO-3] Common pitfall. open() on a FIFO blocks until the other end is also
 * opened. A writer with no reader simply waits, and a program that opens both
 * ends in the wrong order from one thread waits forever.
 *
 * Build: make demo
 * Run:   ./bin/fifo              one terminal, a child writes and the parent reads
 *        ./bin/fifo read         two terminals: start the reader first...
 *        ./bin/fifo write hi     ...then the writer, in another terminal
 *
 * The shell can use the same FIFO:  cat /tmp/km-demo.fifo  and
 * echo hello > /tmp/km-demo.fifo  in two terminals.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/km-demo.fifo"

/* Creates the FIFO if it is missing and prints what the file system sees. */
static int make_fifo(void) {
    struct stat st;

    if (mkfifo(FIFO_PATH, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }
    if (stat(FIFO_PATH, &st) == -1) {
        perror("stat");
        return -1;
    }
    if (!S_ISFIFO(st.st_mode)) {
        fprintf(stderr, "%s exists but is not a FIFO, remove it first\n", FIFO_PATH);
        return -1;
    }
    printf("fifo    : %s  type=FIFO  size=%lld bytes  inode=%lu\n",
           FIFO_PATH, (long long)st.st_size, (unsigned long)st.st_ino);
    printf("fifo    : ls -l shows it with a leading 'p', for pipe\n");
    return 0;
}

static int writer(int count, char **words) {
    char line[256];
    int fd;

    printf("writer  : pid=%d, open(O_WRONLY) blocks until a reader opens the FIFO\n",
           (int)getpid());
    fflush(stdout);

    fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open for writing");
        return 1;
    }
    printf("writer  : open() returned fd %d, a reader is connected\n", fd);
    fflush(stdout);

    if (count > 0) {
        /* Words from the command line, sent as one message. */
        size_t used = 0;
        line[0] = '\0';
        for (int i = 0; i < count && used < sizeof(line) - 2; i++) {
            used += (size_t)snprintf(line + used, sizeof(line) - used, "%s%s",
                                     i ? " " : "", words[i]);
        }
        if (used > sizeof(line) - 2) {
            used = sizeof(line) - 2;
        }
        line[used++] = '\n';
        line[used] = '\0';
        if (write(fd, line, used) == -1) {
            perror("write");
        }
    } else {
        for (int i = 1; i <= 5; i++) {
            int n = snprintf(line, sizeof(line), "message %d from pid %d\n", i, (int)getpid());
            if (write(fd, line, (size_t)n) == -1) {
                perror("write");
                break;
            }
            printf("writer  : wrote %d bytes\n", n);
            fflush(stdout);
            usleep(300000);
        }
    }

    /* Closing the last write end is what the reader sees as end-of-file. */
    close(fd);
    printf("writer  : closed the write end\n");
    /* The forked writer leaves through _exit(), which skips stdio's flush. */
    fflush(stdout);
    return 0;
}

static int reader(void) {
    char line[256];
    FILE *in;
    int fd;

    printf("reader  : pid=%d, open(O_RDONLY) blocks until a writer opens the FIFO\n",
           (int)getpid());
    fflush(stdout);

    fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open for reading");
        return 1;
    }
    printf("reader  : open() returned fd %d, a writer is connected\n", fd);
    fflush(stdout);

    in = fdopen(fd, "r");
    if (in == NULL) {
        perror("fdopen");
        close(fd);
        return 1;
    }
    while (fgets(line, sizeof(line), in) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        printf("reader  : got \"%s\"\n", line);
        fflush(stdout);
    }
    printf("reader  : read() returned 0, which means every writer has closed\n");
    fclose(in);
    return 0;
}

int main(int argc, char *argv[]) {
    pid_t child;
    int status;
    int rc;

    if (make_fifo() == -1) {
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "write") == 0) {
        return writer(argc - 2, argv + 2);
    }
    if (argc > 1 && strcmp(argv[1], "read") == 0) {
        rc = reader();
        unlink(FIFO_PATH);
        printf("fifo    : removed %s\n", FIFO_PATH);
        return rc;
    }
    if (argc > 1) {
        fprintf(stderr, "usage: %s [read | write [words...]]\n", argv[0]);
        return 2;
    }

    /* One-terminal version. The child writes, the parent reads. The two are
       related, but nothing is inherited: they meet only through the path. */
    fflush(stdout);
    child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        _exit(writer(0, NULL));
    }

    /* Give the writer a moment so its open() is visibly stuck waiting. */
    sleep(1);
    rc = reader();

    waitpid(child, &status, 0);
    unlink(FIFO_PATH);
    printf("fifo    : reaped the writer and removed %s\n", FIFO_PATH);
    return rc;
}
