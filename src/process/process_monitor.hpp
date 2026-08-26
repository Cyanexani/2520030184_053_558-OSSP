#ifndef PROCESS_MONITOR_HPP
#define PROCESS_MONITOR_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <sys/types.h>
#include <ctime>

namespace KernelMonitor {
namespace Process {

struct ProcessInfo {
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    std::string name;
    char state;
    int priority;
    int nice;
    long numThreads;
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long vmSize;
    unsigned long long vmRss;
    double cpuPercent;
    std::string cmdline;
    std::string exePath;
    int fdCount;
    time_t startTime;
};

enum class EventType {
    PROCESS_CREATED,
    PROCESS_TERMINATED,
    PROCESS_STATE_CHANGED,
    HIGH_CPU_USAGE,
    HIGH_MEMORY_USAGE
};

struct Event {
    time_t timestamp;
    EventType type;
    pid_t pid;
    std::string processName;
    std::string description;
};

struct ProcessTree {
    std::map<pid_t, ProcessInfo> processes;
    std::map<pid_t, std::vector<pid_t>> children;  // ppid -> [child pids]
    std::vector<pid_t> roots;  // Top-level processes
};

class ProcessMonitor {
public:
    ProcessMonitor();

    // Update process information
    void update();

    // Get all processes
    std::vector<ProcessInfo> getProcessList() const;

    // Get specific process info
    std::optional<ProcessInfo> getProcessInfo(pid_t pid) const;

    // Get children of a process
    std::vector<ProcessInfo> getProcessChildren(pid_t ppid) const;

    // Build process tree
    ProcessTree buildProcessTree() const;

    // Get recent events
    std::vector<Event> getRecentEvents(size_t count = 10) const;

    // Utility functions
    static std::string getStateString(char state);
    static double calculateMemoryPercent(const ProcessInfo& info);

private:
    bool readProcessInfo(pid_t pid, ProcessInfo& info);
    void calculateCpuUsage(ProcessInfo& current, const ProcessInfo& previous);
    void detectChanges(const std::map<pid_t, ProcessInfo>& newProcesses);

    std::map<pid_t, ProcessInfo> processes_;
    std::vector<Event> events_;
    long pageSize_;
    int cpuCount_;
    long clockTicks_;
    time_t bootTime_;
};

} // namespace Process
} // namespace KernelMonitor

#endif // PROCESS_MONITOR_HPP
