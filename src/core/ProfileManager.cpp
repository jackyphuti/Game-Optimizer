#include "optimizer/ProfileManager.hpp"
#include "optimizer/Logger.hpp"
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>

namespace fs = std::filesystem;

namespace optimizer {

std::string ProfileManager::getDefaultConfigPath() {
#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        return (fs::path(appData) / "GameOptimizer" / "profiles.json").string();
    }
    return "profiles.json";
#else
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig && std::string(xdgConfig).length() > 0) {
        return (fs::path(xdgConfig) / "gameoptimizer" / "profiles.json").string();
    }
    const char* home = std::getenv("HOME");
    if (home) {
        return (fs::path(home) / ".config" / "gameoptimizer" / "profiles.json").string();
    }
    return "profiles.json";
#endif
}

ProfileManager::ProfileManager(std::string configPath)
    : m_configPath(std::move(configPath)) {
    if (m_configPath.empty()) {
        m_configPath = getDefaultConfigPath();
    }
    populateDefaultLists();
    if (!loadProfiles()) {
        populateDefaultProfiles();
        saveProfiles();
    }
}

void ProfileManager::populateDefaultLists() {
    // Non-essential processes to lower or suspend during gaming
    m_backgroundBlacklist = {
        "discord.exe", "discord",
        "slack.exe", "slack",
        "spotify.exe", "spotify",
        "chrome.exe", "chrome",
        "msedge.exe", "msedge",
        "firefox.exe", "firefox",
        "epicgameslauncher.exe", "epicgameslauncher",
        "galaxyclient.exe", "galaxyclient",
        "steamwebhelper.exe", "steamwebhelper",
        "onedrive.exe", "onedrive",
        "dropbox.exe", "dropbox",
        "bittorrent.exe", "bittorrent",
        "utorrent.exe", "utorrent",
        "qbittorrent.exe", "qbittorrent",
        "cortana.exe", "searchapp.exe",
        "obs64.exe", "obs" // Note: users can remove OBS from list if streaming
    };

    // Critical system processes that must NEVER be altered
    m_systemWhitelist = {
        "system", "system idle process", "registry",
        "smss.exe", "csrss.exe", "wininit.exe", "services.exe",
        "lsass.exe", "winlogon.exe", "explorer.exe", "dwm.exe",
        "fontdrvhost.exe", "svchost.exe", "taskmgr.exe", "spoolsv.exe",
        "conhost.exe", "audiodg.exe", "ctfmon.exe", "shellexperiencehost.exe",
        "searchui.exe", "startmenuexperiencehost.exe", "securityhealthservice.exe",
        "game-optimizer.exe", "game-optimizer", "gameoptimizer",
        // Linux core daemons
        "init", "systemd", "kthreadd", "dbus-daemon", "Xorg", "wayland",
        "pipewire", "wireplumber", "pulseaudio", "gnome-shell", "plasma-desktop"
    };
}

void ProfileManager::populateDefaultProfiles() {
    m_profiles.clear();

    auto add = [this](const std::string& name, const std::string& exe, bool affinity = false, uint64_t mask = 0) {
        GameProfile p;
        p.name = name;
        p.exeName = exe;
        p.tuneCpuPriority = true;
        p.targetPriority = ProcessPriority::High;
        p.tuneAffinity = affinity;
        p.affinityMask = mask;
        p.tuneGpu = true;
        p.switchPowerPlan = true;
        p.lowerBackgroundProcesses = true;
        p.trimRamOnLaunch = true;
        p.dropCachesOnLaunch = false;
        m_profiles.push_back(p);
    };

    // Windows & cross-platform titles
    add("Counter-Strike 2", "cs2.exe");
    add("Counter-Strike 2 (Linux)", "cs2");
    add("Cyberpunk 2077", "cyberpunk2077.exe");
    add("Dota 2", "dota2.exe");
    add("Dota 2 (Linux)", "dota2");
    add("Valorant", "valorant-win64-shipping.exe");
    add("The Witcher 3", "witcher3.exe");
    add("Elden Ring", "eldenring.exe");
    add("Half-Life 2", "hl2.exe");
    add("Half-Life 2 (Linux)", "hl2_linux");
    add("Grand Theft Auto V", "gta5.exe");
    add("Apex Legends", "r5apex.exe");
    add("Overwatch 2", "overwatch.exe");
    add("Starfield", "starfield.exe");
    add("Rocket League", "rocketleague.exe");
    add("Call of Duty", "cod.exe");
    add("Fortnite", "fortniteclient-win64-shipping.exe");
}

bool ProfileManager::loadProfiles() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!fs::exists(m_configPath)) {
        LOG_INFO("Config file not found at " + m_configPath + ", creating default profiles.");
        return false;
    }

    try {
        std::ifstream file(m_configPath);
        if (!file.is_open()) {
            LOG_WARN("Could not open config file: " + m_configPath);
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();

        json::Value root = json::parse(content);
        if (!root.isObject()) {
            LOG_WARN("Malformed JSON configuration in " + m_configPath);
            return false;
        }

        m_profiles.clear();
        const json::Value& gamesArr = root["profiles"];
        if (gamesArr.isArray()) {
            for (size_t i = 0; i < gamesArr.arrVal.size(); ++i) {
                const auto& item = gamesArr[i];
                if (!item.isObject()) continue;

                GameProfile p;
                p.name = item["name"].asString("Unknown Game");
                p.exeName = item["exeName"].asString();
                p.fullPath = item["fullPath"].asString();
                p.tuneCpuPriority = item["tuneCpuPriority"].asBool(true);
                p.targetPriority = priorityFromString(item["targetPriority"].asString("High"));
                p.tuneAffinity = item["tuneAffinity"].asBool(false);
                p.affinityMask = item["affinityMask"].asUInt64(0);
                p.tuneGpu = item["tuneGpu"].asBool(true);
                p.switchPowerPlan = item["switchPowerPlan"].asBool(true);
                p.lowerBackgroundProcesses = item["lowerBackgroundProcesses"].asBool(true);
                p.trimRamOnLaunch = item["trimRamOnLaunch"].asBool(true);
                p.dropCachesOnLaunch = item["dropCachesOnLaunch"].asBool(false);

                if (!p.exeName.empty()) {
                    m_profiles.push_back(p);
                }
            }
        }

        const json::Value& blacklistArr = root["backgroundBlacklist"];
        if (blacklistArr.isArray() && !blacklistArr.arrVal.empty()) {
            m_backgroundBlacklist.clear();
            for (size_t i = 0; i < blacklistArr.arrVal.size(); ++i) {
                std::string s = blacklistArr[i].asString();
                if (!s.empty()) m_backgroundBlacklist.push_back(s);
            }
        }

        LOG_INFO("Loaded " + std::to_string(m_profiles.size()) + " game profiles from " + m_configPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading profiles: " + std::string(e.what()));
        return false;
    }
}

bool ProfileManager::saveProfiles() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    try {
        fs::path p(m_configPath);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        json::Value root(json::Type::Object);
        json::Value profilesArr(json::Type::Array);

        for (const auto& gp : m_profiles) {
            json::Value item(json::Type::Object);
            item["name"] = gp.name;
            item["exeName"] = gp.exeName;
            item["fullPath"] = gp.fullPath;
            item["tuneCpuPriority"] = gp.tuneCpuPriority;
            item["targetPriority"] = priorityToString(gp.targetPriority);
            item["tuneAffinity"] = gp.tuneAffinity;
            item["affinityMask"] = gp.affinityMask;
            item["tuneGpu"] = gp.tuneGpu;
            item["switchPowerPlan"] = gp.switchPowerPlan;
            item["lowerBackgroundProcesses"] = gp.lowerBackgroundProcesses;
            item["trimRamOnLaunch"] = gp.trimRamOnLaunch;
            item["dropCachesOnLaunch"] = gp.dropCachesOnLaunch;
            profilesArr.push_back(item);
        }
        root["profiles"] = profilesArr;

        json::Value blacklistArr(json::Type::Array);
        for (const auto& bl : m_backgroundBlacklist) {
            blacklistArr.push_back(bl);
        }
        root["backgroundBlacklist"] = blacklistArr;

        std::ofstream file(m_configPath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to write config file to " + m_configPath);
            return false;
        }

        file << root.serialize(2);
        file.close();
        LOG_INFO("Saved " + std::to_string(m_profiles.size()) + " profiles to " + m_configPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception saving profiles: " + std::string(e.what()));
        return false;
    }
}

bool ProfileManager::addGame(const GameProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& existing : m_profiles) {
        if (existing.exeName == profile.exeName) {
            existing = profile; // Update existing
            LOG_INFO("Updated existing profile for " + profile.exeName);
            saveProfiles();
            return true;
        }
    }
    m_profiles.push_back(profile);
    LOG_INFO("Added new game profile for " + profile.name + " (" + profile.exeName + ")");
    saveProfiles();
    return true;
}

bool ProfileManager::addGameByPath(const std::string& exePath, const std::string& customName) {
    std::string normalizedPath = exePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    fs::path p(normalizedPath);
    std::string exeName = p.filename().string();
    if (exeName.empty()) {
        LOG_ERROR("Invalid executable path: " + exePath);
        return false;
    }

    GameProfile gp;
    gp.name = customName.empty() ? p.stem().string() : customName;
    gp.exeName = exeName;
    gp.fullPath = exePath;
    gp.tuneCpuPriority = true;
    gp.targetPriority = ProcessPriority::High;
    gp.tuneAffinity = false;
    gp.affinityMask = 0;
    gp.tuneGpu = true;
    gp.switchPowerPlan = true;
    gp.lowerBackgroundProcesses = true;
    gp.trimRamOnLaunch = true;
    gp.dropCachesOnLaunch = false;

    return addGame(gp);
}

bool ProfileManager::removeGame(const std::string& exeNameOrTitle) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string lowerTarget = exeNameOrTitle;
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), [](unsigned char c) { return std::tolower(c); });

    auto it = std::remove_if(m_profiles.begin(), m_profiles.end(), [&](const GameProfile& p) {
        std::string pName = p.name;
        std::string pExe = p.exeName;
        std::transform(pName.begin(), pName.end(), pName.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(pExe.begin(), pExe.end(), pExe.begin(), [](unsigned char c) { return std::tolower(c); });
        return (pName == lowerTarget || pExe == lowerTarget);
    });

    if (it != m_profiles.end()) {
        m_profiles.erase(it, m_profiles.end());
        LOG_INFO("Removed profile: " + exeNameOrTitle);
        saveProfiles();
        return true;
    }
    LOG_WARN("Profile not found: " + exeNameOrTitle);
    return false;
}

std::optional<GameProfile> ProfileManager::findProfileForExecutable(const std::string& exeName) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string target = exeName;
    std::transform(target.begin(), target.end(), target.begin(), [](unsigned char c) { return std::tolower(c); });

    for (const auto& p : m_profiles) {
        std::string current = p.exeName;
        std::transform(current.begin(), current.end(), current.begin(), [](unsigned char c) { return std::tolower(c); });
        if (current == target) {
            return p;
        }
    }
    return std::nullopt;
}

std::vector<GameProfile> ProfileManager::getAllProfiles() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_profiles;
}

const std::vector<std::string>& ProfileManager::getBackgroundBlacklist() const {
    return m_backgroundBlacklist;
}

void ProfileManager::setBackgroundBlacklist(const std::vector<std::string>& blacklist) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_backgroundBlacklist = blacklist;
    saveProfiles();
}

const std::vector<std::string>& ProfileManager::getSystemWhitelist() const {
    return m_systemWhitelist;
}

} // namespace optimizer
