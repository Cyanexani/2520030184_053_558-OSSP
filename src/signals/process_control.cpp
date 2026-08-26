#include "process_control.hpp"
#include <signal.h>
#include <cerrno>
#include <cstring>

namespace KernelMonitor {
namespace Signals {

ProcessControl::ProcessControl() {}

ProcessControl::Result ProcessControl::sendSignal(pid_t pid, int signal) {
    Result result;
    result.success = false;
    result.pid = pid;
    result.signal = signal;

    // Validate PID
    if (pid <= 0) {
        result.errorMessage = "Invalid PID";
        return result;
    }

    // Send signal using POSIX kill()
    int ret = kill(pid, signal);

    if (ret == 0) {
        result.success = true;
        result.errorMessage = "Signal sent successfully";
    } else {
        // Signal failed, get error
        int err = errno;
        result.errorMessage = std::string("Failed to send signal: ") + strerror(err);

        // Provide specific error messages
        switch (err) {
            case EINVAL:
                result.errorMessage = "Invalid signal";
                break;
            case EPERM:
                result.errorMessage = "Permission denied (insufficient privileges)";
                break;
            case ESRCH:
                result.errorMessage = "Process does not exist";
                break;
            default:
                result.errorMessage = std::string("Error: ") + strerror(err);
                break;
        }
    }

    return result;
}

ProcessControl::Result ProcessControl::terminateProcess(pid_t pid) {
    return sendSignal(pid, SIGTERM);
}

ProcessControl::Result ProcessControl::killProcess(pid_t pid) {
    return sendSignal(pid, SIGKILL);
}

ProcessControl::Result ProcessControl::stopProcess(pid_t pid) {
    return sendSignal(pid, SIGSTOP);
}

ProcessControl::Result ProcessControl::continueProcess(pid_t pid) {
    return sendSignal(pid, SIGCONT);
}

std::string ProcessControl::getSignalName(int signal) {
    switch (signal) {
        case SIGTERM: return "SIGTERM";
        case SIGKILL: return "SIGKILL";
        case SIGSTOP: return "SIGSTOP";
        case SIGCONT: return "SIGCONT";
        case SIGHUP: return "SIGHUP";
        case SIGINT: return "SIGINT";
        case SIGQUIT: return "SIGQUIT";
        case SIGABRT: return "SIGABRT";
        case SIGUSR1: return "SIGUSR1";
        case SIGUSR2: return "SIGUSR2";
        default: return "Signal " + std::to_string(signal);
    }
}

std::string ProcessControl::getSignalDescription(int signal) {
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

bool ProcessControl::isDestructiveSignal(int signal) {
    return (signal == SIGKILL || signal == SIGTERM ||
            signal == SIGQUIT || signal == SIGABRT);
}

} // namespace Signals
} // namespace KernelMonitor
