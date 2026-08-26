#include "parser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace KernelMonitor {
namespace Utils {

std::string Parser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> Parser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool Parser::parseKeyValue(const std::string& line, std::string& key, std::string& value) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    key = trim(line.substr(0, colonPos));
    value = trim(line.substr(colonPos + 1));
    return true;
}

bool Parser::parseCPULine(const std::string& line,
                         unsigned long long& user,
                         unsigned long long& nice,
                         unsigned long long& system,
                         unsigned long long& idle,
                         unsigned long long& iowait,
                         unsigned long long& irq,
                         unsigned long long& softirq) {
    std::istringstream iss(line);
    std::string cpu;
    iss >> cpu;

    if (cpu.find("cpu") != 0) {
        return false;
    }

    iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
    return !iss.fail();
}

std::map<std::string, unsigned long long> Parser::parseMeminfo(const std::string& content) {
    std::map<std::string, unsigned long long> result;
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        std::string key, valueStr;
        if (parseKeyValue(line, key, valueStr)) {
            // Remove "kB" or other units
            std::istringstream valueStream(valueStr);
            unsigned long long value;
            valueStream >> value;
            result[key] = value;
        }
    }

    return result;
}

bool Parser::parseProcStat(const std::string& content,
                          int& pid,
                          std::string& comm,
                          char& state,
                          int& ppid,
                          unsigned long long& utime,
                          unsigned long long& stime,
                          long& priority,
                          long& nice,
                          long& num_threads) {
    // /proc/[pid]/stat format is complex due to comm field containing spaces and parentheses
    size_t commStart = content.find('(');
    size_t commEnd = content.rfind(')');

    if (commStart == std::string::npos || commEnd == std::string::npos) {
        return false;
    }

    // Parse PID before comm
    std::istringstream pidStream(content.substr(0, commStart));
    pidStream >> pid;

    // Extract comm (process name)
    comm = content.substr(commStart + 1, commEnd - commStart - 1);

    // Parse fields after comm
    std::istringstream iss(content.substr(commEnd + 1));

    // Field indices after comm (starting at index 3 in /proc/stat)
    int dummy_int __attribute__((unused));
    unsigned long dummy_ulong;

    iss >> state        // 3: state
        >> ppid;        // 4: ppid

    // Skip fields 5-13
    for (int i = 0; i < 9; i++) {
        iss >> dummy_ulong;
    }

    iss >> utime        // 14: utime
        >> stime;       // 15: stime

    // Skip cutime (16), cstime (17)
    iss >> dummy_ulong >> dummy_ulong;

    iss >> priority     // 18: priority
        >> nice         // 19: nice
        >> num_threads; // 20: num_threads

    return !iss.fail();
}

std::map<std::string, std::string> Parser::parseProcStatus(const std::string& content) {
    std::map<std::string, std::string> result;
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        std::string key, value;
        if (parseKeyValue(line, key, value)) {
            result[key] = value;
        }
    }

    return result;
}

bool Parser::parseProcStatm(const std::string& content,
                           unsigned long& size,
                           unsigned long& resident,
                           unsigned long& shared) {
    std::istringstream iss(content);
    iss >> size >> resident >> shared;
    return !iss.fail();
}

bool Parser::parseUptime(const std::string& content, double& uptime, double& idle) {
    std::istringstream iss(content);
    iss >> uptime >> idle;
    return !iss.fail();
}

bool Parser::parseLoadavg(const std::string& content,
                         double& load1,
                         double& load5,
                         double& load15) {
    std::istringstream iss(content);
    iss >> load1 >> load5 >> load15;
    return !iss.fail();
}

} // namespace Utils
} // namespace KernelMonitor
