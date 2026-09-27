#if !defined(_WIN32)

#include "LinuxSystemTray.hpp"
#include "optimizer/Logger.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

namespace optimizer {

static LinuxSystemTray* s_linuxTray = nullptr;

LinuxSystemTray::LinuxSystemTray() {
    s_linuxTray = this;
}

LinuxSystemTray::~LinuxSystemTray() {
    quit();
    s_linuxTray = nullptr;
}

bool LinuxSystemTray::init(std::shared_ptr<OptimizerEngine> engine) {
    m_engine = std::move(engine);

    if (m_engine) {
        m_engine->setOnGameStarted([this](const std::string& name, uint32_t pid) {
            std::string msg = "Optimizations applied for " + name + " (PID " + std::to_string(pid) + ")";
            showNotification("Game Optimizer Active", msg);
        });

        m_engine->setOnGameExited([this](const std::string& name, uint32_t /*pid*/) {
            std::string msg = "Restored system defaults after " + name + " exited.";
            showNotification("Game Optimizer Reverted", msg);
        });
    }

    LOG_INFO("[LinuxSystemTray] Background daemon runner initialized.");
    return true;
}

void LinuxSystemTray::showNotification(const std::string& title, const std::string& message) {
    std::string cmd = "notify-send -a \"Game Optimizer\" -i applications-games \"" + title + "\" \"" + message + "\" 2>/dev/null &";
    int res = std::system(cmd.c_str());
    (void)res;
}

void LinuxSystemTray::runMessageLoop() {
    m_running = true;
    LOG_INFO("Game Optimizer background daemon is running. Press Ctrl+C to terminate.");

    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void LinuxSystemTray::quit() {
    m_running = false;
    if (m_engine) {
        m_engine->stopMonitoring();
        m_engine->restoreDefaults();
    }
}

std::unique_ptr<SystemTray> SystemTray::create(std::shared_ptr<OptimizerEngine> engine) {
    auto tray = std::make_unique<LinuxSystemTray>();
    tray->init(engine);
    return tray;
}

} // namespace optimizer

#endif // !_WIN32
