#ifndef SYSTEM_MONITOR_HPP
#define SYSTEM_MONITOR_HPP

#include <string>
#include <vector>
#include <unistd.h>

namespace KernelMonitor {
namespace System {

class SystemMonitor {
public:
    SystemMonitor();

    // Update all system statistics
    bool update();

    // Individual update methods
    bool updateSystemInfo();
    bool updateCPUStats();
    bool updateMemoryStats();
    bool updateLoadAverage();

    // Getters for system information
    std::string getKernelVersion() const;
    int getCpuCount() const;
    double getCpuUsagePercent() const;
    const std::vector<double>& getPerCoreCpuUsage() const;

    double getLoadAvg1() const;
    double getLoadAvg5() const;
    double getLoadAvg15() const;

    unsigned long long getMemTotal() const;
    unsigned long long getMemUsed() const;
    unsigned long long getMemFree() const;
    unsigned long long getMemAvailable() const;
    double getMemUsagePercent() const;

    unsigned long long getSwapTotal() const;
    unsigned long long getSwapUsed() const;
    unsigned long long getSwapFree() const;
    double getSwapUsagePercent() const;

    double getUptime() const;
    std::string formatUptime() const;

    // Utility
    static std::string formatBytes(unsigned long long bytes);

private:
    // System info
    std::string kernelVersion_;
    int cpuCount_;
    long pageSize_;
    double uptime_;

    // CPU stats
    double cpuUsagePercent_;
    std::vector<double> perCoreCpuUsage_;
    unsigned long long prevTotalCpu_;
    unsigned long long prevIdleCpu_;

    // Load average
    double loadAvg1_;
    double loadAvg5_;
    double loadAvg15_;

    // Memory stats
    unsigned long long memTotal_;
    unsigned long long memUsed_;
    unsigned long long memFree_;
    unsigned long long memAvailable_;
    double memUsagePercent_;

    // Swap stats
    unsigned long long swapTotal_;
    unsigned long long swapUsed_;
    unsigned long long swapFree_;
    double swapUsagePercent_;
};

} // namespace System
} // namespace KernelMonitor

#endif // SYSTEM_MONITOR_HPP
