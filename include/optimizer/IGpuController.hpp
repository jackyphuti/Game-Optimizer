#pragma once

#include <string>

namespace optimizer {

class IGpuController {
public:
    virtual ~IGpuController() = default;

    // Engages maximum performance power state (NVIDIA prefer maximum performance / AMD overdrive / D3D high-performance)
    virtual bool applyMaxPerformance(const std::string& gameExe = "") = 0;

    // Restores default power and clock state
    virtual bool restoreDefaultState(const std::string& gameExe = "") = 0;

    virtual std::string getVendorName() = 0;
    virtual std::string getGpuName() = 0;
    virtual bool isSupported() = 0;

    virtual std::string getSavedState() const = 0;
    virtual void setSavedState(const std::string& state) = 0;
};

} // namespace optimizer
