#include "process_monitor.hpp"
#include "../proc/proc_reader.hpp"
#include "../utils/parser.hpp"
#include <algorithm>
#include <sstream>
#include <sys/sysinfo.h>

namespace KernelMonitor {
namespace Process {

ProcessMonitor::ProcessMonitor() : pageSize_(0), cpuCount_(0), clockTicks_(0) {
    pageSize_ = sysconf(_SC_PAGESIZE);
    if (pageSize_ <= 0) {
        pageSize_ = 4096;
    }

    cpuCount_ = Proc::ProcReader::getCpuCount();
    if (cpuCount_ <= 0) {
        cpuCount_ = 1;
    }

    clockTicks_ = sysconf(_SC_CLK_TCK);
    if (clockTicks_ <= 0) {
        clockTicks_ = 100;  // Common default
    }

    // Get boot time for calculating process start time
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        bootTime_ = time(nullptr) - si.uptime;
    } else {
        bootTime_ = 0;
    }
}

void ProcessMonitor::update() {
    std::vector<pid_t> pids = Proc::ProcReader::listPids();
    std::map<pid_t, ProcessInfo> newProcesses;

    for (pid_t pid : pids) {
        ProcessInfo info;
        if (readProcessInfo(pid, info)) {
            // Calculate CPU usage based on previous sample
            if (processes_.find(pid) != processes_.end()) {
                calculateCpuUsage(info, processes_[pid]);
            } else {
                info.cpuPercent = 0.0;
            }

            newProcesses[pid] = info;
        }
    }

    // Detect new and terminated processes
    detectChanges(newProcesses);

    // Update process map
    processes_ = std::move(newProcesses);
}

bool ProcessMonitor::readProcessInfo(pid_t pid, ProcessInfo& info) {
    info.pid = pid;

    // Read /proc/[pid]/stat
    auto statContent = Proc::ProcReader::readProcPidStat(pid);
    if (!statContent) {
        return false;
    }

    int parsedPid;
    if (!Utils::Parser::parseProcStat(*statContent,
                                      parsedPid,
                                      info.name,
                                      info.state,
                                      info.ppid,
                                      info.utime,
                                      info.stime,
                                      info.priority,
                                      info.nice,
                                      info.numThreads)) {
        return false;
    }

    // Read /proc/[pid]/statm for memory
    auto statmContent = Proc::ProcReader::readProcPidStatm(pid);
    if (statmContent) {
        unsigned long size, resident, shared;
        if (Utils::Parser::parseProcStatm(*statmContent, size, resident, shared)) {
            info.vmSize = size * pageSize_;
            info.vmRss = resident * pageSize_;
        }
    }

    // Read /proc/[pid]/status for additional info
    auto statusContent = Proc::ProcReader::readProcPidStatus(pid);
    if (statusContent) {
        auto statusMap = Utils::Parser::parseProcStatus(*statusContent);

        // Extract UID
        if (statusMap.find("Uid") != statusMap.end()) {
            std::istringstream iss(statusMap["Uid"]);
            iss >> info.uid;
        }
    }

    // Read cmdline
    auto cmdline = Proc::ProcReader::readProcPidCmdline(pid);
    if (cmdline && !cmdline->empty()) {
        info.cmdline = *cmdline;
    } else {
        info.cmdline = "[" + info.name + "]";  // Kernel thread notation
    }

    // Read exe path
    auto exe = Proc::ProcReader::readProcPidExe(pid);
    if (exe) {
        info.exePath = *exe;
    }

    // Count file descriptors
    int fdCount = Proc::ProcReader::countFileDescriptors(pid);
    if (fdCount >= 0) {
        info.fdCount = fdCount;
    } else {
        info.fdCount = 0;
    }

    // Calculate start time
    info.startTime = bootTime_ + (info.utime + info.stime) / clockTicks_;

    return true;
}

void ProcessMonitor::calculateCpuUsage(ProcessInfo& current, const ProcessInfo& previous) {
    // Calculate CPU time difference
    unsigned long long prevTotal = previous.utime + previous.stime;
    unsigned long long currTotal = current.utime + current.stime;
    unsigned long long timeDiff = currTotal - prevTotal;

    // We need time elapsed between samples
    // For simplicity, assume 1 second between updates
    // In a real implementation, track actual time
    double timeElapsed = 1.0;  // seconds

    // CPU percentage = (time_diff / clock_ticks) / time_elapsed * 100
    if (timeElapsed > 0 && clockTicks_ > 0) {
        current.cpuPercent = (100.0 * timeDiff) / (clockTicks_ * timeElapsed);
    } else {
        current.cpuPercent = 0.0;
    }
}

void ProcessMonitor::detectChanges(const std::map<pid_t, ProcessInfo>& newProcesses) {
    // Detect new processes
    for (const auto& pair : newProcesses) {
        if (processes_.find(pair.first) == processes_.end()) {
            Event event;
            event.timestamp = time(nullptr);
            event.type = EventType::PROCESS_CREATED;
            event.pid = pair.first;
            event.processName = pair.second.name;
            event.description = "Process created";
            events_.push_back(event);

            // Keep only recent events
            if (events_.size() > 100) {
                events_.erase(events_.begin());
            }
        }
    }

    // Detect terminated processes
    for (const auto& pair : processes_) {
        if (newProcesses.find(pair.first) == newProcesses.end()) {
            Event event;
            event.timestamp = time(nullptr);
            event.type = EventType::PROCESS_TERMINATED;
            event.pid = pair.first;
            event.processName = pair.second.name;
            event.description = "Process terminated";
            events_.push_back(event);

            if (events_.size() > 100) {
                events_.erase(events_.begin());
            }
        }
    }
}

std::vector<ProcessInfo> ProcessMonitor::getProcessList() const {
    std::vector<ProcessInfo> list;
    list.reserve(processes_.size());

    for (const auto& pair : processes_) {
        list.push_back(pair.second);
    }

    return list;
}

std::optional<ProcessInfo> ProcessMonitor::getProcessInfo(pid_t pid) const {
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ProcessInfo> ProcessMonitor::getProcessChildren(pid_t ppid) const {
    std::vector<ProcessInfo> children;

    for (const auto& pair : processes_) {
        if (pair.second.ppid == ppid) {
            children.push_back(pair.second);
        }
    }

    return children;
}

ProcessTree ProcessMonitor::buildProcessTree() const {
    ProcessTree tree;
    tree.processes = processes_;

    // Build parent-child relationships
    for (const auto& pair : processes_) {
        pid_t pid = pair.first;
        pid_t ppid = pair.second.ppid;

        if (processes_.find(ppid) != processes_.end()) {
            tree.children[ppid].push_back(pid);
        } else {
            // Orphaned process or ppid is not in our list
            tree.roots.push_back(pid);
        }
    }

    // Find actual roots (ppid = 0 or ppid = self)
    for (const auto& pair : processes_) {
        if (pair.second.ppid == 0 || pair.second.ppid == pair.first) {
            auto it = std::find(tree.roots.begin(), tree.roots.end(), pair.first);
            if (it == tree.roots.end()) {
                tree.roots.push_back(pair.first);
            }
        }
    }

    // Sort roots (typically just PID 1)
    std::sort(tree.roots.begin(), tree.roots.end());

    return tree;
}

std::vector<Event> ProcessMonitor::getRecentEvents(size_t count) const {
    if (count >= events_.size()) {
        return events_;
    }

    return std::vector<Event>(events_.end() - count, events_.end());
}

std::string ProcessMonitor::getStateString(char state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk Sleep";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing Stop";
        case 'X': return "Dead";
        case 'x': return "Dead";
        case 'K': return "Wakekill";
        case 'W': return "Waking";
        case 'P': return "Parked";
        case 'I': return "Idle";
        default: return "Unknown";
    }
}

double ProcessMonitor::calculateMemoryPercent(const ProcessInfo& info) {
    // Get total system memory
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        return 0.0;
    }

    unsigned long long totalMem = si.totalram * si.mem_unit;
    if (totalMem == 0) {
        return 0.0;
    }

    return (100.0 * info.vmRss) / totalMem;
}

} // namespace Process
} // namespace KernelMonitor
