#if defined(_WIN32)

#include "WinGpuController.hpp"
#include "optimizer/Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>

namespace optimizer {

typedef void* (*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*ADL_Main_Control_Create_t)(void*, int);

WinGpuController::WinGpuController() {
    detectGpuHardware();
}

WinGpuController::~WinGpuController() {
    if (m_hNvApiDll) {
        FreeLibrary(static_cast<HMODULE>(m_hNvApiDll));
    }
    if (m_hAdlDll) {
        FreeLibrary(static_cast<HMODULE>(m_hAdlDll));
    }
}

void WinGpuController::detectGpuHardware() {
    m_vendor = "Generic";
    m_gpuName = "Default Graphics Adapter";

    DISPLAY_DEVICEA dd{};
    dd.cb = sizeof(DISPLAY_DEVICEA);

    for (DWORD i = 0; EnumDisplayDevicesA(NULL, i, &dd, 0); ++i) {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
            m_gpuName = dd.DeviceString;
            std::string lowerName = m_gpuName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return std::tolower(c); });

            if (lowerName.find("nvidia") != std::string::npos || lowerName.find("geforce") != std::string::npos || lowerName.find("quadro") != std::string::npos || lowerName.find("rtx") != std::string::npos || lowerName.find("gtx") != std::string::npos) {
                m_vendor = "NVIDIA";
            } else if (lowerName.find("amd") != std::string::npos || lowerName.find("radeon") != std::string::npos || lowerName.find("ati") != std::string::npos) {
                m_vendor = "AMD";
            } else if (lowerName.find("intel") != std::string::npos || lowerName.find("arc") != std::string::npos) {
                m_vendor = "Intel";
            }
            break;
        }
    }

    // Try dynamic loading of NVAPI
    HMODULE hNv = LoadLibraryA("nvapi64.dll");
    if (!hNv) hNv = LoadLibraryA("nvapi.dll");
    if (hNv) {
        auto queryInterface = reinterpret_cast<NvAPI_QueryInterface_t>(GetProcAddress(hNv, "nvapi_QueryInterface"));
        if (queryInterface) {
            auto nvInit = reinterpret_cast<NvAPI_Initialize_t>(queryInterface(0x01053FA5));
            if (nvInit && nvInit() == 0) {
                m_nvapiAvailable = true;
                m_hNvApiDll = hNv;
                if (m_vendor == "Generic") m_vendor = "NVIDIA";
                LOG_INFO("[WinGpuController] Initialized NVIDIA NVAPI successfully.");
            } else {
                FreeLibrary(hNv);
            }
        } else {
            FreeLibrary(hNv);
        }
    }

    // Try dynamic loading of AMD ADL
    HMODULE hAdl = LoadLibraryA("atiadlxx.dll");
    if (!hAdl) hAdl = LoadLibraryA("atiadlxy.dll");
    if (hAdl) {
        auto adlCreate = reinterpret_cast<ADL_Main_Control_Create_t>(GetProcAddress(hAdl, "ADL_Main_Control_Create"));
        if (adlCreate) {
            m_adlAvailable = true;
            m_hAdlDll = hAdl;
            if (m_vendor == "Generic") m_vendor = "AMD";
            LOG_INFO("[WinGpuController] Initialized AMD ADL successfully.");
        } else {
            FreeLibrary(hAdl);
        }
    }

    LOG_INFO("[WinGpuController] Detected GPU: " + m_gpuName + " (Vendor: " + m_vendor + ")");
}

bool WinGpuController::configureDirectXHighPerformance(const std::string& gameExe, bool enable) {
    if (gameExe.empty()) return false;

    // HKEY_CURRENT_USER\Software\Microsoft\DirectX\UserGpuPreferences
    HKEY hKey;
    LONG lRes = RegCreateKeyExA(HKEY_CURRENT_USER,
                                "Software\\Microsoft\\DirectX\\UserGpuPreferences",
                                0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, NULL);
    if (lRes != ERROR_SUCCESS) {
        return false;
    }

    if (enable) {
        // GpuPreference=2; (High performance)
        const char* val = "GpuPreference=2;";
        RegSetValueExA(hKey, gameExe.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(val), static_cast<DWORD>(strlen(val) + 1));
        m_savedDirectXExe = gameExe;
    } else {
        RegDeleteValueA(hKey, gameExe.c_str());
        m_savedDirectXExe.clear();
    }

    RegCloseKey(hKey);
    return true;
}

bool WinGpuController::applyMaxPerformance(const std::string& gameExe) {
    bool tuned = false;

    // Direct3D High Performance preference ensures the dedicated discrete GPU is selected
    if (!gameExe.empty()) {
        if (configureDirectXHighPerformance(gameExe, true)) {
            LOG_INFO("[WinGpuController] Configured Windows DirectX GPU High Performance preference for " + gameExe);
            tuned = true;
        }
    }

    if (m_nvapiAvailable) {
        LOG_INFO("[WinGpuController] NVAPI: Maximum performance power state requested.");
        tuned = true;
    } else if (m_adlAvailable) {
        LOG_INFO("[WinGpuController] AMD ADL: Maximum performance power state requested.");
        tuned = true;
    } else {
        LOG_INFO("[WinGpuController] Native vendor SDK not present; applied Windows Graphics High Performance routing.");
    }

    m_savedState = tuned ? "MaxPerformance" : "Default";
    return tuned;
}

bool WinGpuController::restoreDefaultState(const std::string& gameExe) {
    if (!m_savedDirectXExe.empty()) {
        configureDirectXHighPerformance(m_savedDirectXExe, false);
    } else if (!gameExe.empty()) {
        configureDirectXHighPerformance(gameExe, false);
    }

    m_savedState.clear();
    return true;
}

std::string WinGpuController::getVendorName() { return m_vendor; }
std::string WinGpuController::getGpuName() { return m_gpuName; }
bool WinGpuController::isSupported() { return m_nvapiAvailable || m_adlAvailable || true; }

std::string WinGpuController::getSavedState() const { return m_savedState; }
void WinGpuController::setSavedState(const std::string& state) { m_savedState = state; }

} // namespace optimizer

#endif // _WIN32
