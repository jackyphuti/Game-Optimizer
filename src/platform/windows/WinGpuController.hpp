#pragma once

#if defined(_WIN32)

#include "optimizer/IGpuController.hpp"
#include <string>

namespace optimizer {

class WinGpuController : public IGpuController {
public:
    WinGpuController();
    ~WinGpuController() override;

    bool applyMaxPerformance(const std::string& gameExe = "") override;
    bool restoreDefaultState(const std::string& gameExe = "") override;

    std::string getVendorName() override;
    std::string getGpuName() override;
    bool isSupported() override;

    std::string getSavedState() const override;
    void setSavedState(const std::string& state) override;

private:
    void detectGpuHardware();
    bool configureDirectXHighPerformance(const std::string& gameExe, bool enable);

    std::string m_vendor;
    std::string m_gpuName;
    bool m_nvapiAvailable{false};
    bool m_adlAvailable{false};
    void* m_hNvApiDll{nullptr};
    void* m_hAdlDll{nullptr};

    std::string m_savedDirectXExe;
    std::string m_savedState;
};

} // namespace optimizer

#endif // _WIN32
