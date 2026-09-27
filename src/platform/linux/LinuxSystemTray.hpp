#pragma once

#if !defined(_WIN32)

#include "optimizer/SystemTray.hpp"
#include <memory>
#include <atomic>

namespace optimizer {

class LinuxSystemTray : public SystemTray {
public:
    LinuxSystemTray();
    ~LinuxSystemTray() override;

    bool init(std::shared_ptr<OptimizerEngine> engine) override;
    void showNotification(const std::string& title, const std::string& message) override;
    void runMessageLoop() override;
    void quit() override;

private:
    std::shared_ptr<OptimizerEngine> m_engine;
    std::atomic<bool> m_running{false};
};

} // namespace optimizer

#endif // !_WIN32
