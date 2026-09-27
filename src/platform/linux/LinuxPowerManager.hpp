#pragma once

#if !defined(_WIN32)

#include "optimizer/IPowerManager.hpp"
#include <string>
#include <vector>

namespace optimizer {

class LinuxPowerManager : public IPowerManager {
public:
    LinuxPowerManager();
    ~LinuxPowerManager() override;

    bool switchHighPerformance() override;
    bool restoreOriginalScheme() override;

    std::string getActiveProfileName() override;
    std::string getOriginalScheme() const override;
    void setOriginalScheme(const std::string& scheme) override;

private:
    std::string executeCommand(const std::string& cmd);
    bool setCpuGovernor(const std::string& governor);
    std::string getCpuGovernor();

    std::string m_savedProfile;
    std::string m_savedGovernor;
    bool m_hasSwitched{false};
    bool m_hasPowerProfilesCtl{false};
};

} // namespace optimizer

#endif // !_WIN32
