#pragma once

#if defined(_WIN32)

#include "optimizer/SystemTray.hpp"
#include <windows.h>
#include <string>
#include <memory>

namespace optimizer {

class WinSystemTray : public SystemTray {
public:
    WinSystemTray();
    ~WinSystemTray() override;

    bool init(std::shared_ptr<OptimizerEngine> engine) override;
    void showNotification(const std::string& title, const std::string& message) override;
    void runMessageLoop() override;
    void quit() override;

    void openDashboard();
    void toggleStartupRun();
    bool isRegisteredForStartup();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DashboardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void showContextMenu();
    void updateTrayTooltip(const std::string& statusText);
    void promptAddGame();

    std::shared_ptr<OptimizerEngine> m_engine;
    HWND m_hWnd{NULL};
    HWND m_hDashboardWnd{NULL};
    NOTIFYICONDATAW m_nid{};
    bool m_initialized{false};
};

} // namespace optimizer

#endif // _WIN32
