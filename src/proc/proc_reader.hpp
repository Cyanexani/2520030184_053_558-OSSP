#ifndef PROC_READER_HPP
#define PROC_READER_HPP

#include <string>
#include <vector>
#include <optional>
#include <sys/types.h>

namespace KernelMonitor {
namespace Proc {

class ProcReader {
public:
    // Read entire file content from /proc
    static std::optional<std::string> readFile(const std::string& path);

    // Read /proc/stat
    static std::optional<std::string> readProcStat();

    // Read /proc/meminfo
    static std::optional<std::string> readProcMeminfo();

    // Read /proc/uptime
    static std::optional<std::string> readProcUptime();

    // Read /proc/loadavg
    static std::optional<std::string> readProcLoadavg();

    // Read /proc/version
    static std::optional<std::string> readProcVersion();

    // Read /proc/cpuinfo
    static std::optional<std::string> readProcCpuinfo();

    // Read /proc/[pid]/stat
    static std::optional<std::string> readProcPidStat(pid_t pid);

    // Read /proc/[pid]/status
    static std::optional<std::string> readProcPidStatus(pid_t pid);

    // Read /proc/[pid]/statm
    static std::optional<std::string> readProcPidStatm(pid_t pid);

    // Read /proc/[pid]/cmdline
    static std::optional<std::string> readProcPidCmdline(pid_t pid);

    // Read /proc/[pid]/exe (symlink)
    static std::optional<std::string> readProcPidExe(pid_t pid);

    // List all PIDs in /proc
    static std::vector<pid_t> listPids();

    // Count file descriptors for a process
    static int countFileDescriptors(pid_t pid);

    // Get number of CPU cores
    static int getCpuCount();
};

} // namespace Proc
} // namespace KernelMonitor

#endif // PROC_READER_HPP
