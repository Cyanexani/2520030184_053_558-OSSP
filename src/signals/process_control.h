#ifndef KM_PROCESS_CONTROL_H
#define KM_PROCESS_CONTROL_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define KM_SIGNAL_MSG_MAX 128

typedef struct {
    bool success;
    pid_t pid;
    int signal;
    char error_message[KM_SIGNAL_MSG_MAX];
} km_signal_result;

km_signal_result km_send_signal(pid_t pid, int signal);

km_signal_result km_terminate_process(pid_t pid); /* SIGTERM */
km_signal_result km_kill_process(pid_t pid);      /* SIGKILL */
km_signal_result km_stop_process(pid_t pid);      /* SIGSTOP */
km_signal_result km_continue_process(pid_t pid);  /* SIGCONT */

/* Writes the signal's name into buf and returns it, so that the unnamed
   signals can still be rendered as "Signal N". */
const char *km_signal_name(int signal, char *buf, size_t bufsz);

const char *km_signal_description(int signal);

bool km_signal_is_destructive(int signal);

#endif /* KM_PROCESS_CONTROL_H */
