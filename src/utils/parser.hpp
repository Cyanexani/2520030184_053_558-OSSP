#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <map>

namespace KernelMonitor {
namespace Utils {

class Parser {
public:
    // Parse /proc/stat line for CPU statistics
    static bool parseCPULine(const std::string& line,
                            unsigned long long& user,
                            unsigned long long& nice,
                            unsigned long long& system,
                            unsigned long long& idle,
                            unsigned long long& iowait,
                            unsigned long long& irq,
                            unsigned long long& softirq);

    // Parse /proc/meminfo
    static std::map<std::string, unsigned long long> parseMeminfo(const std::string& content);

    // Parse /proc/stat for process information
    static bool parseProcStat(const std::string& content,
                             int& pid,
                             std::string& comm,
                             char& state,
                             int& ppid,
                             unsigned long long& utime,
                             unsigned long long& stime,
                             long& priority,
                             long& nice,
                             long& num_threads);

    // Parse /proc/[pid]/status
    static std::map<std::string, std::string> parseProcStatus(const std::string& content);

    // Parse /proc/[pid]/statm
    static bool parseProcStatm(const std::string& content,
                              unsigned long& size,
                              unsigned long& resident,
                              unsigned long& shared);

    // Parse /proc/uptime
    static bool parseUptime(const std::string& content, double& uptime, double& idle);

    // Parse /proc/loadavg
    static bool parseLoadavg(const std::string& content,
                            double& load1,
                            double& load5,
                            double& load15);

    // Utility: trim whitespace
    static std::string trim(const std::string& str);

    // Utility: split string
    static std::vector<std::string> split(const std::string& str, char delimiter);

    // Utility: parse key-value pair
    static bool parseKeyValue(const std::string& line, std::string& key, std::string& value);
};

} // namespace Utils
} // namespace KernelMonitor

#endif // PARSER_HPP
