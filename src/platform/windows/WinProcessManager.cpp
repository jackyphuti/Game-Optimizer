#if defined(_WIN32)

#include "WinProcessManager.hpp"
#include "optimizer/Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>

namespace optimizer {

static DWORD toWinPriorityClass(ProcessPriority priority) {
    switch (priority) {
        case ProcessPriority::Idle:        return IDLE_PRIORITY_CLASS;
        case ProcessPriority::BelowNormal: return BELOW_NORMAL_PRIORITY_CLASS;
        case ProcessPriority::Normal:      return NORMAL_PRIORITY_CLASS;
        case ProcessPriority::AboveNormal: return ABOVE_NORMAL_PRIORITY_CLASS;
        case ProcessPriority::High:        return HIGH_PRIORITY_CLASS;
        case ProcessPriority::Realtime:    return REALTIME_PRIORITY_CLASS;
        default:                           return NORMAL_PRIORITY_CLASS;
    }
}

static ProcessPriority fromWinPriorityClass(DWORD dwClass) {
    switch (dwClass) {
        case IDLE_PRIORITY_CLASS:         return ProcessPriority::Idle;
        case BELOW_NORMAL_PRIORITY_CLASS: return ProcessPriority::BelowNormal;
        case NORMAL_PRIORITY_CLASS:       return ProcessPriority::Normal;
        case ABOVE_NORMAL_PRIORITY_CLASS: return ProcessPriority::AboveNormal;
        case HIGH_PRIORITY_CLASS:         return ProcessPriority::High;
        case REALTIME_PRIORITY_CLASS:     return ProcessPriority::Realtime;
        default:                          return ProcessPriority::Normal;
    }
}

WinProcessManager::WinProcessManager(std::vector<std::string> systemWhitelist) {
    for (auto& s : systemWhitelist) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        m_systemWhitelist.insert(s);
    }
}

bool WinProcessManager::isElevated() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }
    TOKEN_ELEVATION elevation;
    DWORD cbSize = sizeof(TOKEN_ELEVATION);
    BOOL bSuccess = GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize);
    CloseHandle(hToken);
    return bSuccess && elevation.TokenIsElevated;
}

bool WinProcessManager::setProcessPriority(uint32_t pid, ProcessPriority priority) {
    HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) {
        LOG_WARN("[WinProcessManager] OpenProcess failed for PID " + std::to_string(pid) + " (Error: " + std::to_string(GetLastError()) + ")");
        return false;
    }

    DWORD dwClass = toWinPriorityClass(priority);
    BOOL success = SetPriorityClass(hProc, dwClass);
    if (!success) {
        LOG_WARN("[WinProcessManager] SetPriorityClass failed for PID " + std::to_string(pid) + " (Error: " + std::to_string(GetLastError()) + ")");
    }
    CloseHandle(hProc);
    return success != 0;
}

ProcessPriority WinProcessManager::getProcessPriority(uint32_t pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) {
        return ProcessPriority::Normal;
    }
    DWORD dwClass = GetPriorityClass(hProc);
    CloseHandle(hProc);
    if (dwClass == 0) {
        return ProcessPriority::Normal;
    }
    return fromWinPriorityClass(dwClass);
}

bool WinProcessManager::setProcessAffinity(uint32_t pid, uint64_t mask) {
    HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) {
        LOG_WARN("[WinProcessManager] Could not open PID " + std::to_string(pid) + " for affinity setting.");
        return false;
    }

    BOOL success = SetProcessAffinityMask(hProc, static_cast<DWORD_PTR>(mask));
    if (!success) {
        LOG_WARN("[WinProcessManager] SetProcessAffinityMask failed for PID " + std::to_string(pid) + " (Error: " + std::to_string(GetLastError()) + ")");
    }
    CloseHandle(hProc);
    return success != 0;
}

uint64_t WinProcessManager::getProcessAffinity(uint32_t pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) {
        return 0;
    }
    DWORD_PTR procMask = 0;
    DWORD_PTR sysMask = 0;
    if (!GetProcessAffinityMask(hProc, &procMask, &sysMask)) {
        procMask = 0;
    }
    CloseHandle(hProc);
    return static_cast<uint64_t>(procMask);
}

std::vector<ProcessRollbackEntry> WinProcessManager::lowerNonEssentialProcesses(
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

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LOG_ERROR("[WinProcessManager] Failed to snapshot processes.");
        return modified;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            int len = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, NULL, 0, NULL, NULL);
            std::string procName;
            if (len > 0) {
                procName.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, &procName[0], len, NULL, NULL);
            }
            std::string lowerProcName = procName;
            std::transform(lowerProcName.begin(), lowerProcName.end(), lowerProcName.begin(), [](unsigned char c) { return std::tolower(c); });

            // Never touch current game or optimizer itself
            if (lowerProcName == lowerGameExe || pe32.th32ProcessID == GetCurrentProcessId()) {
                continue;
            }

            // Never touch whitelist system processes
            if (m_systemWhitelist.find(lowerProcName) != m_systemWhitelist.end()) {
                continue;
            }

            // Check if matches blacklist
            bool match = false;
            if (!blacklist.empty()) {
                match = (blacklist.find(lowerProcName) != blacklist.end());
            } else {
                // Default heuristic matching common background hogs
                if (lowerProcName.find("discord") != std::string::npos ||
                    lowerProcName.find("slack") != std::string::npos ||
                    lowerProcName.find("spotify") != std::string::npos ||
                    lowerProcName.find("chrome") != std::string::npos ||
                    lowerProcName.find("msedge") != std::string::npos ||
                    lowerProcName.find("firefox") != std::string::npos ||
                    lowerProcName.find("epicgames") != std::string::npos ||
                    lowerProcName.find("galaxyclient") != std::string::npos ||
                    lowerProcName.find("steamwebhelper") != std::string::npos ||
                    lowerProcName.find("onedrive") != std::string::npos ||
                    lowerProcName.find("dropbox") != std::string::npos ||
                    lowerProcName.find("torrent") != std::string::npos) {
                    match = true;
                }
            }

            if (match) {
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION, FALSE, pe32.th32ProcessID);
                if (hProc) {
                    DWORD origClass = GetPriorityClass(hProc);
                    if (origClass != 0 && origClass != IDLE_PRIORITY_CLASS) {
                        if (SetPriorityClass(hProc, IDLE_PRIORITY_CLASS)) {
                            ProcessRollbackEntry entry;
                            entry.pid = pe32.th32ProcessID;
                            entry.name = procName;
                            entry.originalPriority = fromWinPriorityClass(origClass);
                            modified.push_back(entry);
                        }
                    }
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return modified;
}

bool WinProcessManager::restoreProcesses(const std::vector<ProcessRollbackEntry>& entries) {
    bool allSuccess = true;
    for (const auto& entry : entries) {
        HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, entry.pid);
        if (hProc) {
            DWORD dwClass = toWinPriorityClass(entry.originalPriority);
            if (!SetPriorityClass(hProc, dwClass)) {
                allSuccess = false;
            }
            CloseHandle(hProc);
        }
    }
    return allSuccess;
}

} // namespace optimizer

#endif // _WIN32
