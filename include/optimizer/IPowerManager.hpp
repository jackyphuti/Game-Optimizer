#pragma once

#include <string>

namespace optimizer {

class IPowerManager {
public:
    virtual ~IPowerManager() = default;

    // Switches the system to maximum/high performance power scheme
    // Returns true on success or if already on high performance
    virtual bool switchHighPerformance() = 0;

    // Restores the previous power scheme/profile before optimization was applied
    virtual bool restoreOriginalScheme() = 0;

    // Retrieves current power plan / profile name (e.g. "High performance", "Balanced", etc.)
    virtual std::string getActiveProfileName() = 0;

    // Gets the original power plan stored before tweak
    virtual std::string getOriginalScheme() const = 0;

    // Force sets the original scheme string (used for journal rollback recovery)
    virtual void setOriginalScheme(const std::string& scheme) = 0;
};

} // namespace optimizer
