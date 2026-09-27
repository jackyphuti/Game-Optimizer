#pragma once

#if defined(_WIN32)

#include "optimizer/IProcessManager.hpp"
#include <vector>
#include <string>
#include <unordered_set>

namespace optimizer {

class WinProcessManager : public IProcessManager {
public:
    explicit WinProcessManager(std::vector<std::string> systemWhitelist);

    bool setProcessPriority(uint32_t pid, ProcessPriority priority) override;
    ProcessPriority getProcessPriority(uint32_t pid) override;

    bool setProcessAffinity(uint32_t pid, uint64_t mask) override;
    uint64_t getProcessAffinity(uint32_t pid) override;

    std::vector<ProcessRollbackEntry> lowerNonEssentialProcesses(
        const std::string& currentGameExe,
        const std::vector<std::string>& customBlacklist = {}) override;

    bool restoreProcesses(const std::vector<ProcessRollbackEntry>& entries) override;

    bool isElevated() override;

private:
    std::unordered_set<std::string> m_systemWhitelist;
};

} // namespace optimizer

#endif // _WIN32
