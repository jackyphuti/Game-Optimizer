#if !defined(_WIN32)

#include "LinuxPowerManager.hpp"
#include "optimizer/Logger.hpp"

#include <fstream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace optimizer {

LinuxPowerManager::LinuxPowerManager() {
    // Check if powerprofilesctl is available
    std::string test = executeCommand("which powerprofilesctl 2>/dev/null");
    m_hasPowerProfilesCtl = !test.empty();
}

LinuxPowerManager::~LinuxPowerManager() {
    if (m_hasSwitched) {
        restoreOriginalScheme();
    }
}

std::string LinuxPowerManager::executeCommand(const std::string& cmd) {
    std::array<char, 256> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    // Trim trailing newline
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

std::string LinuxPowerManager::getCpuGovernor() {
    std::ifstream file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    if (!file.is_open()) return "";
    std::string gov;
    std::getline(file, gov);
    return gov;
}

bool LinuxPowerManager::setCpuGovernor(const std::string& governor) {
    bool success = false;
    for (const auto& entry : fs::directory_iterator("/sys/devices/system/cpu")) {
        std::string filename = entry.path().filename().string();
        if (filename.rfind("cpu", 0) == 0 && std::isdigit(filename.back())) {
            fs::path govPath = entry.path() / "cpufreq" / "scaling_governor";
            if (fs::exists(govPath)) {
                std::ofstream ofs(govPath);
                if (ofs.is_open()) {
                    ofs << governor;
                    ofs.close();
                    success = true;
                }
            }
        }
    }
    return success;
}

std::string LinuxPowerManager::getActiveProfileName() {
    if (m_hasPowerProfilesCtl) {
        std::string profile = executeCommand("powerprofilesctl get 2>/dev/null");
        if (!profile.empty()) return profile;
    }
    std::string gov = getCpuGovernor();
    if (!gov.empty()) return "Governor: " + gov;
    return "Default";
}

bool LinuxPowerManager::switchHighPerformance() {
    m_savedProfile = getActiveProfileName();
    m_savedGovernor = getCpuGovernor();

    bool applied = false;
    if (m_hasPowerProfilesCtl) {
        std::string out = executeCommand("powerprofilesctl set performance 2>&1");
        applied = out.empty() || out.find("Error") == std::string::npos;
    }

    if (!applied) {
        applied = setCpuGovernor("performance");
        if (!applied) {
            // Attempt with cpupower if installed
            std::string out = executeCommand("cpupower frequency-set -g performance 2>&1");
            applied = out.find("error") == std::string::npos;
        }
    }

    if (applied) {
        m_hasSwitched = true;
        LOG_INFO("[LinuxPowerManager] Switched to performance power profile.");
        return true;
    }

    LOG_WARN("[LinuxPowerManager] Could not switch to performance profile (may require sudo/powerprofilesctl).");
    return false;
}

bool LinuxPowerManager::restoreOriginalScheme() {
    if (!m_hasSwitched && m_savedProfile.empty() && m_savedGovernor.empty()) {
        return true;
    }

    if (m_hasPowerProfilesCtl && !m_savedProfile.empty() && m_savedProfile.find("Governor:") == std::string::npos) {
        executeCommand("powerprofilesctl set " + m_savedProfile + " 2>/dev/null");
    }

    if (!m_savedGovernor.empty()) {
        setCpuGovernor(m_savedGovernor);
    }

    m_hasSwitched = false;
    m_savedProfile.clear();
    m_savedGovernor.clear();
    return true;
}

std::string LinuxPowerManager::getOriginalScheme() const {
    return m_savedProfile;
}

void LinuxPowerManager::setOriginalScheme(const std::string& scheme) {
    m_savedProfile = scheme;
    m_hasSwitched = !scheme.empty();
}

} // namespace optimizer

#endif // !_WIN32
