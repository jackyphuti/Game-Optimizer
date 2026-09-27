#if defined(_WIN32)

#include "WinRamOptimizer.hpp"
#include "optimizer/Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

namespace optimizer {

#define SystemMemoryListInformation 80
#define STATUS_SUCCESS ((LONG)0x00000000L)

enum SYSTEM_MEMORY_LIST_COMMAND {
    MemoryCaptureAccessedBits,
    MemoryCaptureAndResetAccessedBits,
    MemoryEmptyWorkingSets,
    MemoryFlushModifiedList,
    MemoryPurgeStandbyList,
    MemoryPurgeLowPriorityStandbyList,
    MemoryCommandMax
};

typedef LONG (WINAPI *NtSetSystemInformation_t)(
    INT SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength
);

WinRamOptimizer::WinRamOptimizer() = default;

bool WinRamOptimizer::enablePrivilege(const char* privName) {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValueA(NULL, privName, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        CloseHandle(hToken);
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

bool WinRamOptimizer::trimWorkingSets() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        EmptyWorkingSet(GetCurrentProcess());
        return false;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    size_t count = 0;

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID <= 4) continue;
            HANDLE hProc = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (hProc) {
                if (EmptyWorkingSet(hProc)) {
                    count++;
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    EmptyWorkingSet(GetCurrentProcess());
    LOG_INFO("[WinRamOptimizer] Trimmed working set for " + std::to_string(count) + " processes.");
    return true;
}

bool WinRamOptimizer::purgeStandbyMemory() {
    if (!enablePrivilege("SeProfileSingleProcessPrivilege")) {
        LOG_WARN("[WinRamOptimizer] Cannot purge standby RAM: Administrator rights (SeProfileSingleProcessPrivilege) required.");
        return false;
    }

    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (!hNtDll) return false;

    auto pNtSetSystemInformation = reinterpret_cast<NtSetSystemInformation_t>(
        GetProcAddress(hNtDll, "NtSetSystemInformation"));
    if (!pNtSetSystemInformation) return false;

    SYSTEM_MEMORY_LIST_COMMAND command = MemoryPurgeStandbyList;
    LONG status = pNtSetSystemInformation(
        SystemMemoryListInformation,
        &command,
        sizeof(command)
    );

    if (status == STATUS_SUCCESS) {
        LOG_INFO("[WinRamOptimizer] Standby memory list purged successfully.");
        return true;
    } else {
        LOG_WARN("[WinRamOptimizer] NtSetSystemInformation failed with status: 0x" + std::to_string(status));
        return false;
    }
}

bool WinRamOptimizer::dropCaches(bool /*force*/) {
    // On Windows, dropping cache corresponds to standby list purge
    return purgeStandbyMemory();
}

bool WinRamOptimizer::compactMemory() {
    return trimWorkingSets();
}

} // namespace optimizer

#endif // _WIN32
