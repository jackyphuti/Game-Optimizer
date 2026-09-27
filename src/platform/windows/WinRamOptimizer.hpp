#pragma once

#if defined(_WIN32)

#include "optimizer/IRamOptimizer.hpp"

namespace optimizer {

class WinRamOptimizer : public IRamOptimizer {
public:
    WinRamOptimizer();

    bool trimWorkingSets() override;
    bool purgeStandbyMemory() override;
    bool dropCaches(bool force = false) override;
    bool compactMemory() override;

private:
    bool enablePrivilege(const char* privName);
};

} // namespace optimizer

#endif // _WIN32
