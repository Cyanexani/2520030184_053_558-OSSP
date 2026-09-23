#include "process_control.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

km_signal_result km_send_signal(pid_t pid, int signal) {
    km_signal_result result;
    int ret;

    result.success = false;
    result.pid = pid;
    result.signal = signal;
    result.error_message[0] = '\0';

    if (pid <= 0) {
        snprintf(result.error_message, sizeof(result.error_message), "Invalid PID");
        return result;
    }

    /* [CO-3] Signals as asynchronous notification. kill() does not kill: it asks
       the kernel to deliver a signal to another process, interrupting it wherever
       it happens to be executing. This is the project's only outbound IPC. */
    ret = kill(pid, signal);

    if (ret == 0) {
        result.success = true;
        snprintf(result.error_message, sizeof(result.error_message),
                 "Signal sent successfully");
        return result;
    }

    /* errno says why the kernel refused, and each case means something
       different to the user. */
    switch (errno) {
        case EINVAL:
            snprintf(result.error_message, sizeof(result.error_message),
                     "Invalid signal");
            break;
        case EPERM:
            snprintf(result.error_message, sizeof(result.error_message),
                     "Permission denied (insufficient privileges)");
            break;
        case ESRCH:
            snprintf(result.error_message, sizeof(result.error_message),
                     "Process does not exist");
            break;
        default:
            snprintf(result.error_message, sizeof(result.error_message),
                     "Error: %s", strerror(errno));
            break;
    }

    return result;
}

km_signal_result km_terminate_process(pid_t pid) {
    return km_send_signal(pid, SIGTERM);
}

km_signal_result km_kill_process(pid_t pid) {
    return km_send_signal(pid, SIGKILL);
}

/* [CO-3] Job control. SIGSTOP and SIGCONT suspend and resume a process without
   terminating it, which is the same mechanism a shell uses for Ctrl+Z, bg and
   fg. Like SIGKILL, SIGSTOP cannot be caught, blocked or ignored: the kernel
   acts on it without ever delivering it to the target. */
km_signal_result km_stop_process(pid_t pid) {
    return km_send_signal(pid, SIGSTOP);
}

km_signal_result km_continue_process(pid_t pid) {
    return km_send_signal(pid, SIGCONT);
}

const char *km_signal_name(int signal, char *buf, size_t bufsz) {
    const char *name = NULL;

    switch (signal) {
        case SIGTERM: name = "SIGTERM"; break;
        case SIGKILL: name = "SIGKILL"; break;
        case SIGSTOP: name = "SIGSTOP"; break;
        case SIGCONT: name = "SIGCONT"; break;
        case SIGHUP:  name = "SIGHUP";  break;
        case SIGINT:  name = "SIGINT";  break;
        case SIGQUIT: name = "SIGQUIT"; break;
        case SIGABRT: name = "SIGABRT"; break;
        case SIGUSR1: name = "SIGUSR1"; break;
        case SIGUSR2: name = "SIGUSR2"; break;
        default: break;
    }

    if (name) {
        snprintf(buf, bufsz, "%s", name);
    } else {
        snprintf(buf, bufsz, "Signal %d", signal);
    }

    return buf;
}

const char *km_signal_description(int signal) {
    switch (signal) {
        case SIGTERM:
            return "Terminate (allows graceful cleanup)";
        case SIGKILL:
            return "Kill immediately (cannot be caught or ignored)";
        case SIGSTOP:
            return "Stop process (pause execution)";
        case SIGCONT:
            return "Continue stopped process";
        case SIGHUP:
            return "Hangup (terminal disconnected)";
        case SIGINT:
            return "Interrupt (Ctrl+C)";
        case SIGQUIT:
            return "Quit (with core dump)";
        case SIGABRT:
            return "Abort (abnormal termination)";
        case SIGUSR1:
            return "User-defined signal 1";
        case SIGUSR2:
            return "User-defined signal 2";
        default:
            return "Unknown signal";
    }
}

bool km_signal_is_destructive(int signal) {
    return signal == SIGKILL || signal == SIGTERM ||
           signal == SIGQUIT || signal == SIGABRT;
}
