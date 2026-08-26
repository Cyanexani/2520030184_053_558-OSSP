#include "proc_reader.hpp"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>

namespace KernelMonitor {
namespace Proc {

std::optional<std::string> ProcReader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (file.bad()) {
        return std::nullopt;
    }

    return buffer.str();
}

std::optional<std::string> ProcReader::readProcStat() {
    return readFile("/proc/stat");
}

std::optional<std::string> ProcReader::readProcMeminfo() {
    return readFile("/proc/meminfo");
}

std::optional<std::string> ProcReader::readProcUptime() {
    return readFile("/proc/uptime");
}

std::optional<std::string> ProcReader::readProcLoadavg() {
    return readFile("/proc/loadavg");
}

std::optional<std::string> ProcReader::readProcVersion() {
    return readFile("/proc/version");
}

std::optional<std::string> ProcReader::readProcCpuinfo() {
    return readFile("/proc/cpuinfo");
}

std::optional<std::string> ProcReader::readProcPidStat(pid_t pid) {
    return readFile("/proc/" + std::to_string(pid) + "/stat");
}

std::optional<std::string> ProcReader::readProcPidStatus(pid_t pid) {
    return readFile("/proc/" + std::to_string(pid) + "/status");
}

std::optional<std::string> ProcReader::readProcPidStatm(pid_t pid) {
    return readFile("/proc/" + std::to_string(pid) + "/statm");
}

std::optional<std::string> ProcReader::readProcPidCmdline(pid_t pid) {
    auto content = readFile("/proc/" + std::to_string(pid) + "/cmdline");
    if (!content) {
        return std::nullopt;
    }

    // cmdline uses null bytes as delimiters; replace with spaces
    std::string result = *content;
    for (char& c : result) {
        if (c == '\0') {
            c = ' ';
        }
    }

    // Trim trailing spaces
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

std::optional<std::string> ProcReader::readProcPidExe(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/exe";
    char buffer[4096];

    ssize_t len = readlink(path.c_str(), buffer, sizeof(buffer) - 1);
    if (len == -1) {
        return std::nullopt;
    }

    buffer[len] = '\0';
    return std::string(buffer);
}

std::vector<pid_t> ProcReader::listPids() {
    std::vector<pid_t> pids;
    DIR* dir = opendir("/proc");

    if (!dir) {
        return pids;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Check if directory name is numeric (a PID)
        if (entry->d_type == DT_DIR) {
            const char* name = entry->d_name;
            bool isNumeric = true;

            for (const char* p = name; *p; ++p) {
                if (!isdigit(*p)) {
                    isNumeric = false;
                    break;
                }
            }

            if (isNumeric && name[0] != '\0') {
                pid_t pid = static_cast<pid_t>(std::atoi(name));
                if (pid > 0) {
                    pids.push_back(pid);
                }
            }
        }
    }

    closedir(dir);
    return pids;
}

int ProcReader::countFileDescriptors(pid_t pid) {
    std::string fdDir = "/proc/" + std::to_string(pid) + "/fd";
    DIR* dir = opendir(fdDir.c_str());

    if (!dir) {
        return -1;  // Permission denied or process doesn't exist
    }

    int count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

int ProcReader::getCpuCount() {
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs <= 0) {
        // Fallback: try to count from /proc/cpuinfo
        auto cpuinfo = readProcCpuinfo();
        if (!cpuinfo) {
            return 1;  // Assume at least 1 CPU
        }

        int count = 0;
        std::istringstream iss(*cpuinfo);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.find("processor") == 0) {
                count++;
            }
        }

        return count > 0 ? count : 1;
    }

    return static_cast<int>(nprocs);
}

} // namespace Proc
} // namespace KernelMonitor
