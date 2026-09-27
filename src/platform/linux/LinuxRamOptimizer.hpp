#pragma once

#if !defined(_WIN32)

#include "optimizer/IRamOptimizer.hpp"

namespace optimizer {

class LinuxRamOptimizer : public IRamOptimizer {
public:
    LinuxRamOptimizer();

    bool trimWorkingSets() override;
    bool purgeStandbyMemory() override;
    bool dropCaches(bool force = false) override;
    bool compactMemory() override;
};

} // namespace optimizer

#endif // !_WIN32
