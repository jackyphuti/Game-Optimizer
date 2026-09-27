#pragma once

#if !defined(_WIN32)

#include "optimizer/IGameDetector.hpp"

namespace optimizer {

class LinuxGameDetector : public IGameDetector {
public:
    LinuxGameDetector();

    std::vector<RunningProcessInfo> enumerateProcesses() override;
    std::optional<RunningProcessInfo> detectGame(const std::vector<GameProfile>& profiles) override;
    bool isProcessRunning(uint32_t pid) override;
    std::string getProcessName(uint32_t pid) override;
    std::string getProcessPath(uint32_t pid) override;

private:
    std::string readProcessComm(uint32_t pid);
    std::string readProcessExe(uint32_t pid);
};

} // namespace optimizer

#endif // !_WIN32
