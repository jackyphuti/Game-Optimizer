#if !defined(_WIN32)

#include "LinuxProcessManager.hpp"
#include "optimizer/Logger.hpp"

#include <unistd.h>
#include <sys/resource.h>
#include <sched.h>
#include <dirent.h>
#include <fstream>
#include <algorithm>
#include <cerrno>
#include <cstring>

namespace optimizer {

static int toLinuxNice(ProcessPriority prio) {
    switch (prio) {
        case ProcessPriority::Idle:        return 19;
        case ProcessPriority::BelowNormal: return 5;
        case ProcessPriority::Normal:      return 0;
        case ProcessPriority::AboveNormal: return -5;
        case ProcessPriority::High:        return -10;
        case ProcessPriority::Realtime:    return -20;
        default:                           return 0;
    }
}

static ProcessPriority fromLinuxNice(int niceVal) {
    if (niceVal >= 15) return ProcessPriority::Idle;
    if (niceVal > 0)   return ProcessPriority::BelowNormal;
    if (niceVal == 0)  return ProcessPriority::Normal;
    if (niceVal > -10) return ProcessPriority::AboveNormal;
    if (niceVal > -20) return ProcessPriority::High;
    return ProcessPriority::Realtime;
}

LinuxProcessManager::LinuxProcessManager(std::vector<std::string> systemWhitelist) {
    for (auto& s : systemWhitelist) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        m_systemWhitelist.insert(s);
    }
}

bool LinuxProcessManager::isElevated() {
    return geteuid() == 0;
}

bool LinuxProcessManager::setProcessPriority(uint32_t pid, ProcessPriority priority) {
    int nice = toLinuxNice(priority);
    errno = 0;
    int res = setpriority(PRIO_PROCESS, pid, nice);
    if (res != 0) {
        LOG_WARN("[LinuxProcessManager] setpriority failed for PID " + std::to_string(pid) + " to nice " + std::to_string(nice) + " (Error: " + std::strerror(errno) + "). Elevate with CAP_SYS_NICE or sudo for negative nice values.");
        return false;
    }
    return true;
}

ProcessPriority LinuxProcessManager::getProcessPriority(uint32_t pid) {
    errno = 0;
    int val = getpriority(PRIO_PROCESS, pid);
    if (errno != 0) {
        return ProcessPriority::Normal;
    }
    return fromLinuxNice(val);
}

bool LinuxProcessManager::setProcessAffinity(uint32_t pid, uint64_t mask) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int i = 0; i < 64; ++i) {
        if ((mask >> i) & 1ULL) {
            CPU_SET(i, &cpuset);
        }
    }

    if (sched_setaffinity(static_cast<pid_t>(pid), sizeof(cpu_set_t), &cpuset) != 0) {
        LOG_WARN("[LinuxProcessManager] sched_setaffinity failed for PID " + std::to_string(pid) + " (Error: " + std::strerror(errno) + ")");
        return false;
    }
    return true;
}

uint64_t LinuxProcessManager::getProcessAffinity(uint32_t pid) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(static_cast<pid_t>(pid), sizeof(cpu_set_t), &cpuset) != 0) {
        return 0;
    }
    uint64_t mask = 0;
    for (int i = 0; i < 64; ++i) {
        if (CPU_ISSET(i, &cpuset)) {
            mask |= (1ULL << i);
        }
    }
    return mask;
}

std::vector<ProcessRollbackEntry> LinuxProcessManager::lowerNonEssentialProcesses(
    const std::string& currentGameExe,
    const std::vector<std::string>& customBlacklist) {

    std::vector<ProcessRollbackEntry> modified;

    std::string lowerGameExe = currentGameExe;
    std::transform(lowerGameExe.begin(), lowerGameExe.end(), lowerGameExe.begin(), [](unsigned char c) { return std::tolower(c); });

    std::unordered_set<std::string> blacklist;
    for (auto& s : customBlacklist) {
        std::string lowerS = s;
        std::transform(lowerS.begin(), lowerS.end(), lowerS.begin(), [](unsigned char c) { return std::tolower(c); });
        blacklist.insert(lowerS);
    }

    DIR* dir = opendir("/proc");
    if (!dir) return modified;

    struct dirent* entry;
    pid_t selfPid = getpid();

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        char* endPtr = nullptr;
        long pid = strtol(entry->d_name, &endPtr, 10);
        if (*endPtr != '\0' || pid <= 1 || pid == selfPid) continue;

        std::string commPath = std::string("/proc/") + entry->d_name + "/comm";
        std::ifstream commFile(commPath);
        if (!commFile.is_open()) continue;

        std::string procName;
        std::getline(commFile, procName);
        commFile.close();

        std::string lowerProc = procName;
        std::transform(lowerProc.begin(), lowerProc.end(), lowerProc.begin(), [](unsigned char c) { return std::tolower(c); });

        if (lowerProc == lowerGameExe) continue;
        if (m_systemWhitelist.find(lowerProc) != m_systemWhitelist.end()) continue;

        bool match = false;
        if (!blacklist.empty()) {
            match = (blacklist.find(lowerProc) != blacklist.end());
        } else {
            if (lowerProc.find("discord") != std::string::npos ||
                lowerProc.find("slack") != std::string::npos ||
                lowerProc.find("spotify") != std::string::npos ||
                lowerProc.find("chrome") != std::string::npos ||
                lowerProc.find("firefox") != std::string::npos ||
                lowerProc.find("torrent") != std::string::npos) {
                match = true;
            }
        }

        if (match) {
            errno = 0;
            int curNice = getpriority(PRIO_PROCESS, pid);
            if (errno == 0 && curNice < 15) {
                if (setpriority(PRIO_PROCESS, pid, 15) == 0) {
                    ProcessRollbackEntry r;
                    r.pid = static_cast<uint32_t>(pid);
                    r.name = procName;
                    r.originalPriority = fromLinuxNice(curNice);
                    modified.push_back(r);
                }
            }
        }
    }

    closedir(dir);
    return modified;
}

bool LinuxProcessManager::restoreProcesses(const std::vector<ProcessRollbackEntry>& entries) {
    bool allSuccess = true;
    for (const auto& e : entries) {
        int nice = toLinuxNice(e.originalPriority);
        if (setpriority(PRIO_PROCESS, e.pid, nice) != 0) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

} // namespace optimizer

#endif // !_WIN32
