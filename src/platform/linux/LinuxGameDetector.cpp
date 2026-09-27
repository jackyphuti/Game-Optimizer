#if !defined(_WIN32)

#include "LinuxGameDetector.hpp"
#include "optimizer/Logger.hpp"

#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace optimizer {

LinuxGameDetector::LinuxGameDetector() = default;

std::string LinuxGameDetector::readProcessComm(uint32_t pid) {
    std::string commPath = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream f(commPath);
    if (!f.is_open()) return "";
    std::string comm;
    std::getline(f, comm);
    return comm;
}

std::string LinuxGameDetector::readProcessExe(uint32_t pid) {
    std::string exeLink = "/proc/" + std::to_string(pid) + "/exe";
    char buf[1024];
    ssize_t len = readlink(exeLink.c_str(), buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        return std::string(buf);
    }
    return "";
}

std::vector<RunningProcessInfo> LinuxGameDetector::enumerateProcesses() {
    std::vector<RunningProcessInfo> list;

    DIR* dir = opendir("/proc");
    if (!dir) return list;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        char* endPtr = nullptr;
        long pid = strtol(entry->d_name, &endPtr, 10);
        if (*endPtr != '\0' || pid <= 0) continue;

        std::string comm = readProcessComm(static_cast<uint32_t>(pid));
        if (!comm.empty()) {
            RunningProcessInfo info;
            info.pid = static_cast<uint32_t>(pid);
            info.name = comm;
            info.exePath = readProcessExe(static_cast<uint32_t>(pid));
            list.push_back(std::move(info));
        }
    }
    closedir(dir);
    return list;
}

std::optional<RunningProcessInfo> LinuxGameDetector::detectGame(const std::vector<GameProfile>& profiles) {
    DIR* dir = opendir("/proc");
    if (!dir) return std::nullopt;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        char* endPtr = nullptr;
        long pid = strtol(entry->d_name, &endPtr, 10);
        if (*endPtr != '\0' || pid <= 1) continue;

        uint32_t uPid = static_cast<uint32_t>(pid);
        std::string comm = readProcessComm(uPid);
        if (comm.empty()) continue;

        std::string exePath = readProcessExe(uPid);
        std::string exeName = fs::path(exePath).filename().string();
        if (exeName.empty()) exeName = comm;

        std::string lowerComm = comm;
        std::string lowerExe = exeName;
        std::transform(lowerComm.begin(), lowerComm.end(), lowerComm.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), [](unsigned char c) { return std::tolower(c); });

        for (const auto& profile : profiles) {
            std::string target = profile.exeName;
            std::transform(target.begin(), target.end(), target.begin(), [](unsigned char c) { return std::tolower(c); });

            if (lowerComm == target || lowerExe == target) {
                RunningProcessInfo info;
                info.pid = uPid;
                info.name = exeName;
                info.exePath = exePath;

                closedir(dir);
                return info;
            }
        }
    }

    closedir(dir);
    return std::nullopt;
}

bool LinuxGameDetector::isProcessRunning(uint32_t pid) {
    if (pid <= 0) return false;
    return (kill(static_cast<pid_t>(pid), 0) == 0);
}

std::string LinuxGameDetector::getProcessName(uint32_t pid) {
    return readProcessComm(pid);
}

std::string LinuxGameDetector::getProcessPath(uint32_t pid) {
    return readProcessExe(pid);
}

} // namespace optimizer

#endif // !_WIN32
