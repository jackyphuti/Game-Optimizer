#if defined(_WIN32)

#include "WinGameDetector.hpp"
#include "optimizer/Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>

namespace optimizer {

static std::string wideToUtf8(const wchar_t* wstr) {
    if (!wstr || !*wstr) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, NULL, NULL);
    return result;
}

WinGameDetector::WinGameDetector() = default;

std::string WinGameDetector::queryProcessImagePath(uint32_t pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return "";

    char pathBuf[MAX_PATH];
    DWORD size = MAX_PATH;
    std::string result;
    if (QueryFullProcessImageNameA(hProc, 0, pathBuf, &size)) {
        result = pathBuf;
    }
    CloseHandle(hProc);
    return result;
}

std::vector<RunningProcessInfo> WinGameDetector::enumerateProcesses() {
    std::vector<RunningProcessInfo> list;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return list;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == 0) continue;
            RunningProcessInfo info;
            info.pid = pe.th32ProcessID;
            info.name = wideToUtf8(pe.szExeFile);
            list.push_back(std::move(info));
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return list;
}

std::optional<RunningProcessInfo> WinGameDetector::detectGame(const std::vector<GameProfile>& profiles) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID <= 4) continue;

            std::string procName = wideToUtf8(pe.szExeFile);
            std::string lowerProcName = procName;
            std::transform(lowerProcName.begin(), lowerProcName.end(), lowerProcName.begin(), [](unsigned char c) { return std::tolower(c); });

            for (const auto& profile : profiles) {
                std::string targetExe = profile.exeName;
                std::transform(targetExe.begin(), targetExe.end(), targetExe.begin(), [](unsigned char c) { return std::tolower(c); });

                if (lowerProcName == targetExe) {
                    RunningProcessInfo info;
                    info.pid = pe.th32ProcessID;
                    info.name = procName;
                    info.exePath = queryProcessImagePath(pe.th32ProcessID);

                    CloseHandle(hSnapshot);
                    return info;
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return std::nullopt;
}

bool WinGameDetector::isProcessRunning(uint32_t pid) {
    if (pid == 0) return false;
    HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) {
        return false;
    }
    DWORD exitCode = 0;
    if (GetExitCodeProcess(hProc, &exitCode)) {
        CloseHandle(hProc);
        return (exitCode == STILL_ACTIVE);
    }
    DWORD waitRes = WaitForSingleObject(hProc, 0);
    CloseHandle(hProc);
    return (waitRes == WAIT_TIMEOUT);
}

std::string WinGameDetector::getProcessName(uint32_t pid) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return "";

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    std::string name;

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                name = wideToUtf8(pe.szExeFile);
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return name;
}

std::string WinGameDetector::getProcessPath(uint32_t pid) {
    return queryProcessImagePath(pid);
}

} // namespace optimizer

#endif // _WIN32
