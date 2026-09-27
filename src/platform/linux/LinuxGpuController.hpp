#pragma once

#if !defined(_WIN32)

#include "optimizer/IGpuController.hpp"
#include <string>

namespace optimizer {

class LinuxGpuController : public IGpuController {
public:
    LinuxGpuController();
    ~LinuxGpuController() override;

    bool applyMaxPerformance(const std::string& gameExe = "") override;
    bool restoreDefaultState(const std::string& gameExe = "") override;

    std::string getVendorName() override;
    std::string getGpuName() override;
    bool isSupported() override;

    std::string getSavedState() const override;
    void setSavedState(const std::string& state) override;

private:
    void detectGpu();
    bool tryGameModeStart();
    bool tryGameModeEnd();

    std::string m_vendor;
    std::string m_gpuName;
    std::string m_savedDpmLevel;
    std::string m_savedState;

    void* m_gameModeLib{nullptr};
    bool m_gameModeActive{false};
};

} // namespace optimizer

#endif // !_WIN32
