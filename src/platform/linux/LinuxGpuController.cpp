#if !defined(_WIN32)

#include "LinuxGpuController.hpp"
#include "optimizer/Logger.hpp"

#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace optimizer {

typedef int (*gamemode_request_start_t)();
typedef int (*gamemode_request_end_t)();

LinuxGpuController::LinuxGpuController() {
    detectGpu();

    // Check for Feral GameMode client library
    m_gameModeLib = dlopen("libgamemode.so.0", RTLD_NOW);
    if (!m_gameModeLib) {
        m_gameModeLib = dlopen("libgamemode.so", RTLD_NOW);
    }
    if (m_gameModeLib) {
        LOG_INFO("[LinuxGpuController] Found Feral Interactive GameMode client library.");
    }
}

LinuxGpuController::~LinuxGpuController() {
    if (m_gameModeActive) {
        tryGameModeEnd();
    }
    if (m_gameModeLib) {
        dlclose(m_gameModeLib);
    }
}

void LinuxGpuController::detectGpu() {
    m_vendor = "Generic";
    m_gpuName = "Generic Linux Graphics";

    // Check /sys/class/drm
    if (fs::exists("/sys/class/drm/card0/device/vendor")) {
        std::ifstream vFile("/sys/class/drm/card0/device/vendor");
        if (vFile.is_open()) {
            std::string venId;
            vFile >> venId;
            if (venId == "0x10de") {
                m_vendor = "NVIDIA";
                m_gpuName = "NVIDIA GeForce / Quadro";
            } else if (venId == "0x1002") {
                m_vendor = "AMD";
                m_gpuName = "AMD Radeon Graphics";
            } else if (venId == "0x8086") {
                m_vendor = "Intel";
                m_gpuName = "Intel Arc / Xe Graphics";
            }
        }
    }

    // Check lspci if still generic
    if (m_vendor == "Generic") {
        std::ifstream lspci("/proc/bus/pci/devices");
        // Fallback default
    }

    LOG_INFO("[LinuxGpuController] Detected GPU vendor: " + m_vendor + " (" + m_gpuName + ")");
}

bool LinuxGpuController::tryGameModeStart() {
    if (!m_gameModeLib) return false;
    auto gm_start = reinterpret_cast<gamemode_request_start_t>(dlsym(m_gameModeLib, "gamemode_request_start"));
    if (gm_start) {
        int status = gm_start();
        if (status == 0) {
            LOG_INFO("[LinuxGpuController] GameMode successfully activated via libgamemode.");
            m_gameModeActive = true;
            return true;
        } else {
            LOG_WARN("[LinuxGpuController] gamemode_request_start returned status: " + std::to_string(status));
        }
    }
    return false;
}

bool LinuxGpuController::tryGameModeEnd() {
    if (!m_gameModeLib || !m_gameModeActive) return false;
    auto gm_end = reinterpret_cast<gamemode_request_end_t>(dlsym(m_gameModeLib, "gamemode_request_end"));
    if (gm_end) {
        int status = gm_end();
        m_gameModeActive = false;
        LOG_INFO("[LinuxGpuController] GameMode deactivated.");
        return (status == 0);
    }
    return false;
}

bool LinuxGpuController::applyMaxPerformance(const std::string& /*gameExe*/) {
    bool tuned = false;

    // First try GameMode
    if (tryGameModeStart()) {
        tuned = true;
    }

    // AMD sysfs performance level
    if (m_vendor == "AMD" && fs::exists("/sys/class/drm/card0/device/power_dpm_force_performance_level")) {
        std::ifstream curFile("/sys/class/drm/card0/device/power_dpm_force_performance_level");
        if (curFile.is_open()) {
            curFile >> m_savedDpmLevel;
            curFile.close();
        }

        std::ofstream setFile("/sys/class/drm/card0/device/power_dpm_force_performance_level");
        if (setFile.is_open()) {
            setFile << "high";
            setFile.close();
            LOG_INFO("[LinuxGpuController] AMD GPU DPM set to high performance.");
            tuned = true;
        }
    } else if (m_vendor == "NVIDIA") {
        // Run nvidia-smi persistence mode if available
        int res = std::system("nvidia-smi -pm 1 >/dev/null 2>&1");
        if (res == 0) {
            LOG_INFO("[LinuxGpuController] NVIDIA GPU persistence mode enabled.");
            tuned = true;
        }
    }

    m_savedState = tuned ? "Performance" : "Default";
    return tuned;
}

bool LinuxGpuController::restoreDefaultState(const std::string& /*gameExe*/) {
    if (m_gameModeActive) {
        tryGameModeEnd();
    }

    if (m_vendor == "AMD" && !m_savedDpmLevel.empty()) {
        std::ofstream setFile("/sys/class/drm/card0/device/power_dpm_force_performance_level");
        if (setFile.is_open()) {
            setFile << m_savedDpmLevel;
            setFile.close();
            LOG_INFO("[LinuxGpuController] AMD GPU DPM restored to " + m_savedDpmLevel);
        }
        m_savedDpmLevel.clear();
    }

    m_savedState.clear();
    return true;
}

std::string LinuxGpuController::getVendorName() { return m_vendor; }
std::string LinuxGpuController::getGpuName() { return m_gpuName; }
bool LinuxGpuController::isSupported() { return true; }

std::string LinuxGpuController::getSavedState() const { return m_savedState; }
void LinuxGpuController::setSavedState(const std::string& state) { m_savedState = state; }

} // namespace optimizer

#endif // !_WIN32
