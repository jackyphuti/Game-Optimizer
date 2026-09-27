#pragma once

#if defined(_WIN32)

#include "optimizer/IGameDetector.hpp"

namespace optimizer {

class WinGameDetector : public IGameDetector {
public:
    WinGameDetector();

    std::vector<RunningProcessInfo> enumerateProcesses() override;
    std::optional<RunningProcessInfo> detectGame(const std::vector<GameProfile>& profiles) override;
    bool isProcessRunning(uint32_t pid) override;
    std::string getProcessName(uint32_t pid) override;
    std::string getProcessPath(uint32_t pid) override;

private:
    std::string queryProcessImagePath(uint32_t pid);
};

} // namespace optimizer

#endif // _WIN32
