#pragma once

#include "optimizer/Types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <mutex>

namespace optimizer {

class ProfileManager {
public:
    explicit ProfileManager(std::string configPath = "");

    bool loadProfiles();
    bool saveProfiles();

    bool addGame(const GameProfile& profile);
    bool addGameByPath(const std::string& exePath, const std::string& customName = "");
    bool removeGame(const std::string& exeNameOrTitle);

    std::optional<GameProfile> findProfileForExecutable(const std::string& exeName) const;
    std::vector<GameProfile> getAllProfiles() const;

    const std::vector<std::string>& getBackgroundBlacklist() const;
    void setBackgroundBlacklist(const std::vector<std::string>& blacklist);

    const std::vector<std::string>& getSystemWhitelist() const;

    std::string getConfigFilePath() const { return m_configPath; }

    static std::string getDefaultConfigPath();

private:
    void populateDefaultProfiles();
    void populateDefaultLists();

    std::string m_configPath;
    std::vector<GameProfile> m_profiles;
    std::vector<std::string> m_backgroundBlacklist;
    std::vector<std::string> m_systemWhitelist;
    mutable std::recursive_mutex m_mutex;
};

} // namespace optimizer
