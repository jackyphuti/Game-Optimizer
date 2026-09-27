#if defined(_WIN32)

#include "WinPowerManager.hpp"
#include "optimizer/Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <powrprof.h>
#include <objbase.h>
#include <sstream>
#include <iomanip>

namespace optimizer {

// High Performance scheme GUID: 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c
static const GUID GUID_HIGH_PERFORMANCE = {
    0x8c5e7fda, 0xe8bf, 0x4a96, { 0x9a, 0x85, 0xa6, 0xe2, 0x3a, 0x8c, 0x63, 0x5c }
};

// Ultimate Performance scheme GUID: e9a42b02-d5df-448d-aa00-03f14749eb61
static const GUID GUID_ULTIMATE_PERFORMANCE = {
    0xe9a42b02, 0xd5df, 0x448d, { 0xaa, 0x00, 0x03, 0xf1, 0x47, 0x49, 0xeb, 0x61 }
};

WinPowerManager::WinPowerManager() = default;

WinPowerManager::~WinPowerManager() {
    if (m_hasSwitched) {
        restoreOriginalScheme();
    }
}

std::string WinPowerManager::guidToString(const void* pGuid) {
    if (!pGuid) return "";
    const GUID* g = static_cast<const GUID*>(pGuid);
    char buf[64];
    snprintf(buf, sizeof(buf), "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             g->Data1, g->Data2, g->Data3,
             g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
             g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
    return std::string(buf);
}

bool WinPowerManager::stringToGuid(const std::string& str, void* pGuid) {
    if (str.empty() || !pGuid) return false;
    GUID* g = static_cast<GUID*>(pGuid);
    unsigned long p0;
    unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;
    int err = sscanf(str.c_str(), "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    if (err == 11) {
        g->Data1 = p0;
        g->Data2 = static_cast<unsigned short>(p1);
        g->Data3 = static_cast<unsigned short>(p2);
        g->Data4[0] = static_cast<unsigned char>(p3);
        g->Data4[1] = static_cast<unsigned char>(p4);
        g->Data4[2] = static_cast<unsigned char>(p5);
        g->Data4[3] = static_cast<unsigned char>(p6);
        g->Data4[4] = static_cast<unsigned char>(p7);
        g->Data4[5] = static_cast<unsigned char>(p8);
        g->Data4[6] = static_cast<unsigned char>(p9);
        g->Data4[7] = static_cast<unsigned char>(p10);
        return true;
    }
    return false;
}

std::string WinPowerManager::getActiveProfileName() {
    GUID* activeScheme = nullptr;
    if (PowerGetActiveScheme(NULL, &activeScheme) != ERROR_SUCCESS || !activeScheme) {
        return "Unknown";
    }

    std::string guidStr = guidToString(activeScheme);

    // Try reading friendly name
    UCHAR buffer[512] = {0};
    DWORD bufSize = sizeof(buffer);
    if (PowerReadFriendlyName(NULL, activeScheme, NULL, NULL, buffer, &bufSize) == ERROR_SUCCESS) {
        wchar_t* wName = reinterpret_cast<wchar_t*>(buffer);
        char cName[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, wName, -1, cName, sizeof(cName), NULL, NULL);
        LocalFree(activeScheme);
        if (strlen(cName) > 0) {
            return std::string(cName) + " (" + guidStr + ")";
        }
    }

    LocalFree(activeScheme);
    return guidStr;
}

bool WinPowerManager::switchHighPerformance() {
    GUID* activeScheme = nullptr;
    if (PowerGetActiveScheme(NULL, &activeScheme) == ERROR_SUCCESS && activeScheme) {
        m_savedSchemeGuid = guidToString(activeScheme);
        LocalFree(activeScheme);
    }

    // Attempt Ultimate Performance first, then High Performance
    DWORD res = PowerSetActiveScheme(NULL, &GUID_ULTIMATE_PERFORMANCE);
    if (res != ERROR_SUCCESS) {
        res = PowerSetActiveScheme(NULL, &GUID_HIGH_PERFORMANCE);
    }

    if (res == ERROR_SUCCESS) {
        m_hasSwitched = true;
        return true;
    }

    LOG_WARN("[WinPowerManager] PowerSetActiveScheme failed with error: " + std::to_string(res));
    return false;
}

bool WinPowerManager::restoreOriginalScheme() {
    if (m_savedSchemeGuid.empty()) {
        return true;
    }

    GUID origGuid{};
    if (!stringToGuid(m_savedSchemeGuid, &origGuid)) {
        LOG_WARN("[WinPowerManager] Could not parse saved GUID: " + m_savedSchemeGuid);
        return false;
    }

    DWORD res = PowerSetActiveScheme(NULL, &origGuid);
    if (res == ERROR_SUCCESS) {
        m_hasSwitched = false;
        m_savedSchemeGuid.clear();
        return true;
    }

    LOG_WARN("[WinPowerManager] Failed to restore power scheme. Error: " + std::to_string(res));
    return false;
}

std::string WinPowerManager::getOriginalScheme() const {
    return m_savedSchemeGuid;
}

void WinPowerManager::setOriginalScheme(const std::string& scheme) {
    m_savedSchemeGuid = scheme;
    m_hasSwitched = !scheme.empty();
}

} // namespace optimizer

#endif // _WIN32
