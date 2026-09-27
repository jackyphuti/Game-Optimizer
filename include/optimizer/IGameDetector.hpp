#pragma once

#include "optimizer/Types.hpp"
#include <vector>
#include <optional>
#include <string>
#include <cstdint>

namespace optimizer {

class IGameDetector {
public:
    virtual ~IGameDetector() = default;

    // Enumerates all currently running processes
    virtual std::vector<RunningProcessInfo> enumerateProcesses() = 0;

    // Checks running processes against the registered game profiles
    // Returns the first detected active game, or nullopt if no game is running
    virtual std::optional<RunningProcessInfo> detectGame(const std::vector<GameProfile>& profiles) = 0;

    // Checks if a specific process ID is still alive
    virtual bool isProcessRunning(uint32_t pid) = 0;

    // Retrieves process name by PID
    virtual std::string getProcessName(uint32_t pid) = 0;

    // Retrieves executable path by PID
    virtual std::string getProcessPath(uint32_t pid) = 0;
};

} // namespace optimizer
