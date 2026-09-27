#pragma once

#include "optimizer/Types.hpp"
#include <vector>
#include <cstdint>
#include <string>

namespace optimizer {

class IProcessManager {
public:
    virtual ~IProcessManager() = default;

    virtual bool setProcessPriority(uint32_t pid, ProcessPriority priority) = 0;
    virtual ProcessPriority getProcessPriority(uint32_t pid) = 0;

    virtual bool setProcessAffinity(uint32_t pid, uint64_t mask) = 0;
    virtual uint64_t getProcessAffinity(uint32_t pid) = 0;

    // Suppresses or lowers priority of background non-essential processes (Discord, browsers, updaters, etc.)
    // Returns list of modified processes with original states for safe rollback
    virtual std::vector<ProcessRollbackEntry> lowerNonEssentialProcesses(
        const std::string& currentGameExe,
        const std::vector<std::string>& customBlacklist = {}) = 0;

    // Restores original priorities and states of modified processes
    virtual bool restoreProcesses(const std::vector<ProcessRollbackEntry>& entries) = 0;

    // Checks if running with administrator / root rights
    virtual bool isElevated() = 0;
};

} // namespace optimizer
