#include "system_monitor.hpp"
#include "../proc/proc_reader.hpp"
#include "../utils/parser.hpp"
#include <sstream>
#include <cstring>

namespace KernelMonitor {
namespace System {

SystemMonitor::SystemMonitor()
    : cpuCount_(0), pageSize_(0), prevTotalCpu_(0), prevIdleCpu_(0) {
    cpuCount_ = Proc::ProcReader::getCpuCount();
    pageSize_ = sysconf(_SC_PAGESIZE);
    if (pageSize_ <= 0) {
        pageSize_ = 4096;  // Default fallback
    }

    // Initialize previous CPU stats
    updateCPUStats();
}

bool SystemMonitor::update() {
    bool success = true;

    success &= updateSystemInfo();
    success &= updateCPUStats();
    success &= updateMemoryStats();
    success &= updateLoadAverage();

    return success;
}

bool SystemMonitor::updateSystemInfo() {
    auto versionContent = Proc::ProcReader::readProcVersion();
    if (!versionContent) {
        return false;
    }

    // Extract kernel version from /proc/version
    // Format: "Linux version X.X.X ..."
    std::istringstream iss(*versionContent);
    std::string linux, version;
    iss >> linux >> version >> kernelVersion_;

    // Get uptime
    auto uptimeContent = Proc::ProcReader::readProcUptime();
    if (uptimeContent) {
        double idleTime;
        Utils::Parser::parseUptime(*uptimeContent, uptime_, idleTime);
    }

    return true;
}

bool SystemMonitor::updateCPUStats() {
    auto statContent = Proc::ProcReader::readProcStat();
    if (!statContent) {
        return false;
    }

    std::istringstream iss(*statContent);
    std::string line;

    // Read first line (aggregate CPU stats)
    if (!std::getline(iss, line)) {
        return false;
    }

    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    if (!Utils::Parser::parseCPULine(line, user, nice, system, idle,
                                     iowait, irq, softirq)) {
        return false;
    }

    // Calculate total and idle
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq;
    unsigned long long totalIdle = idle + iowait;

    // Calculate CPU usage percentage
    if (prevTotalCpu_ > 0) {
        unsigned long long totalDiff = total - prevTotalCpu_;
        unsigned long long idleDiff = totalIdle - prevIdleCpu_;

        if (totalDiff > 0) {
            cpuUsagePercent_ = 100.0 * (totalDiff - idleDiff) / totalDiff;
        } else {
            cpuUsagePercent_ = 0.0;
        }
    } else {
        cpuUsagePercent_ = 0.0;
    }

    prevTotalCpu_ = total;
    prevIdleCpu_ = totalIdle;

    // Parse per-core stats
    perCoreCpuUsage_.clear();
    while (std::getline(iss, line)) {
        if (line.find("cpu") != 0 || line.find("cpu") == std::string::npos) {
            break;
        }

        // Skip aggregate line
        if (line.find("cpu ") == 0) {
            continue;
        }

        if (Utils::Parser::parseCPULine(line, user, nice, system, idle,
                                       iowait, irq, softirq)) {
            unsigned long long coreTotal = user + nice + system + idle + iowait + irq + softirq;
            unsigned long long coreIdle = idle + iowait;

            // For simplicity, we calculate usage differently for per-core
            // In a real implementation, we'd store previous values per core
            if (coreTotal > 0) {
                double coreUsage = 100.0 * (coreTotal - coreIdle) / coreTotal;
                perCoreCpuUsage_.push_back(coreUsage);
            }
        }
    }

    return true;
}

bool SystemMonitor::updateMemoryStats() {
    auto meminfoContent = Proc::ProcReader::readProcMeminfo();
    if (!meminfoContent) {
        return false;
    }

    auto meminfo = Utils::Parser::parseMeminfo(*meminfoContent);

    // Values in /proc/meminfo are in kB
    memTotal_ = meminfo["MemTotal"] * 1024;
    memFree_ = meminfo["MemFree"] * 1024;
    memAvailable_ = meminfo["MemAvailable"] * 1024;

    // Calculate used memory
    // Used = Total - Free - Buffers - Cached
    unsigned long long buffers = meminfo["Buffers"] * 1024;
    unsigned long long cached = meminfo["Cached"] * 1024;
    memUsed_ = memTotal_ - memFree_ - buffers - cached;

    // Swap
    swapTotal_ = meminfo["SwapTotal"] * 1024;
    swapFree_ = meminfo["SwapFree"] * 1024;
    swapUsed_ = swapTotal_ - swapFree_;

    // Calculate percentages
    if (memTotal_ > 0) {
        memUsagePercent_ = 100.0 * memUsed_ / memTotal_;
    }

    if (swapTotal_ > 0) {
        swapUsagePercent_ = 100.0 * swapUsed_ / swapTotal_;
    } else {
        swapUsagePercent_ = 0.0;
    }

    return true;
}

bool SystemMonitor::updateLoadAverage() {
    auto loadavgContent = Proc::ProcReader::readProcLoadavg();
    if (!loadavgContent) {
        return false;
    }

    return Utils::Parser::parseLoadavg(*loadavgContent,
                                       loadAvg1_, loadAvg5_, loadAvg15_);
}

// Getters
std::string SystemMonitor::getKernelVersion() const {
    return kernelVersion_;
}

int SystemMonitor::getCpuCount() const {
    return cpuCount_;
}

double SystemMonitor::getCpuUsagePercent() const {
    return cpuUsagePercent_;
}

const std::vector<double>& SystemMonitor::getPerCoreCpuUsage() const {
    return perCoreCpuUsage_;
}

double SystemMonitor::getLoadAvg1() const {
    return loadAvg1_;
}

double SystemMonitor::getLoadAvg5() const {
    return loadAvg5_;
}

double SystemMonitor::getLoadAvg15() const {
    return loadAvg15_;
}

unsigned long long SystemMonitor::getMemTotal() const {
    return memTotal_;
}

unsigned long long SystemMonitor::getMemUsed() const {
    return memUsed_;
}

unsigned long long SystemMonitor::getMemFree() const {
    return memFree_;
}

unsigned long long SystemMonitor::getMemAvailable() const {
    return memAvailable_;
}

double SystemMonitor::getMemUsagePercent() const {
    return memUsagePercent_;
}

unsigned long long SystemMonitor::getSwapTotal() const {
    return swapTotal_;
}

unsigned long long SystemMonitor::getSwapUsed() const {
    return swapUsed_;
}

unsigned long long SystemMonitor::getSwapFree() const {
    return swapFree_;
}

double SystemMonitor::getSwapUsagePercent() const {
    return swapUsagePercent_;
}

double SystemMonitor::getUptime() const {
    return uptime_;
}

std::string SystemMonitor::formatUptime() const {
    int totalSeconds = static_cast<int>(uptime_);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
    return std::string(buffer);
}

std::string SystemMonitor::formatBytes(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double value = static_cast<double>(bytes);

    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        unitIndex++;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unitIndex]);
    return std::string(buffer);
}

} // namespace System
} // namespace KernelMonitor
