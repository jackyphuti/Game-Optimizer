#pragma once

#if defined(_WIN32)

#include "optimizer/IPowerManager.hpp"
#include <string>

namespace optimizer {

class WinPowerManager : public IPowerManager {
public:
    WinPowerManager();
    ~WinPowerManager() override;

    bool switchHighPerformance() override;
    bool restoreOriginalScheme() override;

    std::string getActiveProfileName() override;
    std::string getOriginalScheme() const override;
    void setOriginalScheme(const std::string& scheme) override;

private:
    std::string guidToString(const void* pGuid);
    bool stringToGuid(const std::string& str, void* pGuid);

    std::string m_savedSchemeGuid;
    bool m_hasSwitched{false};
};

} // namespace optimizer

#endif // _WIN32
