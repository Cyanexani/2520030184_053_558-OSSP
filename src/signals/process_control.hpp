#ifndef PROCESS_CONTROL_HPP
#define PROCESS_CONTROL_HPP

#include <string>
#include <sys/types.h>

namespace KernelMonitor {
namespace Signals {

class ProcessControl {
public:
    struct Result {
        bool success;
        pid_t pid;
        int signal;
        std::string errorMessage;
    };

    ProcessControl();

    // Send arbitrary signal to process
    Result sendSignal(pid_t pid, int signal);

    // Common signal operations
    Result terminateProcess(pid_t pid);  // SIGTERM
    Result killProcess(pid_t pid);       // SIGKILL
    Result stopProcess(pid_t pid);       // SIGSTOP
    Result continueProcess(pid_t pid);   // SIGCONT

    // Utility functions
    static std::string getSignalName(int signal);
    static std::string getSignalDescription(int signal);
    static bool isDestructiveSignal(int signal);
};

} // namespace Signals
} // namespace KernelMonitor

#endif // PROCESS_CONTROL_HPP
