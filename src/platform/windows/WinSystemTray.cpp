#if defined(_WIN32)

#include "WinSystemTray.hpp"
#include "WinRamOptimizer.hpp"
#include "optimizer/Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <sstream>

namespace optimizer {

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_DASHBOARD  1001
#define ID_TRAY_RESTORE    1002
#define ID_TRAY_RAM_PURGE  1003
#define ID_TRAY_ADD_GAME   1004
#define ID_TRAY_STARTUP    1005
#define ID_TRAY_EXIT       1006

static WinSystemTray* s_trayInstance = nullptr;

WinSystemTray::WinSystemTray() {
    s_trayInstance = this;
}

WinSystemTray::~WinSystemTray() {
    if (m_initialized) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
    }
    if (m_hDashboardWnd) {
        DestroyWindow(m_hDashboardWnd);
    }
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
    }
    s_trayInstance = nullptr;
}

bool WinSystemTray::init(std::shared_ptr<OptimizerEngine> engine) {
    m_engine = std::move(engine);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WinSystemTray::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GameOptimizerTrayMessageWindow";

    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        0, wc.lpszClassName, L"GameOptimizerTray",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL);

    if (!m_hWnd) {
        LOG_ERROR("[WinSystemTray] Failed to create message window.");
        return false;
    }

    memset(&m_nid, 0, sizeof(NOTIFYICONDATAW));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"Game Optimizer - Background Monitor");

    if (!Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        LOG_ERROR("[WinSystemTray] Failed to add notification icon to system tray.");
        return false;
    }

    m_initialized = true;

    // Register engine callbacks for system notifications
    if (m_engine) {
        m_engine->setOnGameStarted([this](const std::string& name, uint32_t pid) {
            std::string msg = "Applied gaming tweaks for " + name + " (PID " + std::to_string(pid) + ")";
            showNotification("Game Optimizer Active", msg);
            updateTrayTooltip("Optimizing: " + name);
        });

        m_engine->setOnGameExited([this](const std::string& name, uint32_t /*pid*/) {
            std::string msg = "Restored system defaults after " + name + " closed.";
            showNotification("Game Optimizer Reverted", msg);
            updateTrayTooltip("Idle (Monitoring)");
        });
    }

    LOG_INFO("[WinSystemTray] System tray initialized successfully.");
    return true;
}

void WinSystemTray::updateTrayTooltip(const std::string& statusText) {
    if (!m_initialized) return;
    std::wstring wText(statusText.begin(), statusText.end());
    wcsncpy_s(m_nid.szTip, wText.c_str(), 127);
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void WinSystemTray::showNotification(const std::string& title, const std::string& message) {
    if (!m_initialized) return;
    std::wstring wTitle(title.begin(), title.end());
    std::wstring wMsg(message.begin(), message.end());

    m_nid.uFlags |= NIF_INFO;
    m_nid.dwInfoFlags = NIIF_INFO;
    wcsncpy_s(m_nid.szInfoTitle, wTitle.c_str(), 63);
    wcsncpy_s(m_nid.szInfo, wMsg.c_str(), 255);
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void WinSystemTray::showContextMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    bool isOptimizing = (m_engine && m_engine->getSession() && m_engine->getSession()->isActive());

    std::string header = isOptimizing ?
        ("Active: " + m_engine->getSession()->getCurrentSession().gameName) : "Status: Monitoring (Idle)";
    std::wstring wHeader(header.begin(), header.end());

    AppendMenuW(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED, 0, wHeader.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_DASHBOARD, L"Open Dashboard");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_ADD_GAME, L"Add Game Executable...");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RAM_PURGE, L"Clear Standby RAM Now");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"Restore Defaults");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    UINT startupFlags = isRegisteredForStartup() ? (MF_STRING | MF_CHECKED) : (MF_STRING | MF_UNCHECKED);
    AppendMenuW(hMenu, startupFlags, ID_TRAY_STARTUP, L"Run at Windows Startup");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit Game Optimizer");

    SetForegroundWindow(m_hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);
}

bool WinSystemTray::isRegisteredForStartup() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    char buf[MAX_PATH];
    DWORD cbData = sizeof(buf);
    LONG res = RegQueryValueExA(hKey, "GameOptimizer", NULL, NULL, reinterpret_cast<LPBYTE>(buf), &cbData);
    RegCloseKey(hKey);
    return (res == ERROR_SUCCESS);
}

void WinSystemTray::toggleStartupRun() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE | KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }

    if (isRegisteredForStartup()) {
        RegDeleteValueA(hKey, "GameOptimizer");
        LOG_INFO("Disabled run at Windows startup.");
    } else {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string cmd = std::string("\"") + exePath + "\" --tray";
        RegSetValueExA(hKey, "GameOptimizer", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(cmd.c_str()), static_cast<DWORD>(cmd.length() + 1));
        LOG_INFO("Enabled run at Windows startup: " + cmd);
    }
    RegCloseKey(hKey);
}

void WinSystemTray::promptAddGame() {
    char filename[MAX_PATH] = {0};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = "Game Executables (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        if (m_engine && m_engine->getProfileManager()) {
            m_engine->getProfileManager()->addGameByPath(filename);
            showNotification("Game Profile Added", std::string("Registered: ") + filename);
        }
    }
}

void WinSystemTray::openDashboard() {
    if (m_hDashboardWnd) {
        SetForegroundWindow(m_hDashboardWnd);
        ShowWindow(m_hDashboardWnd, SW_SHOW);
        return;
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WinSystemTray::DashboardWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GameOptimizerDashboardClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassExW(&wc);

    m_hDashboardWnd = CreateWindowExW(
        0, wc.lpszClassName, L"Game Optimizer - Dashboard",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 480,
        NULL, NULL, hInstance, NULL);

    if (m_hDashboardWnd) {
        ShowWindow(m_hDashboardWnd, SW_SHOW);
        UpdateWindow(m_hDashboardWnd);
    }
}

LRESULT CALLBACK WinSystemTray::DashboardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Dashboard text display
            CreateWindowExW(0, L"STATIC", L"Game Optimizer Control Center\n\n- Active Background Monitoring\n- Auto GPU/CPU/RAM Tuning\n- Zero Permanent System Changes",
                            WS_CHILD | WS_VISIBLE, 20, 20, 560, 80, hwnd, NULL, GetModuleHandle(NULL), NULL);

            CreateWindowExW(0, L"BUTTON", L"Add Game Executable...",
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 110, 180, 32,
                            hwnd, reinterpret_cast<HMENU>(ID_TRAY_ADD_GAME), GetModuleHandle(NULL), NULL);

            CreateWindowExW(0, L"BUTTON", L"Restore Defaults",
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 210, 110, 160, 32,
                            hwnd, reinterpret_cast<HMENU>(ID_TRAY_RESTORE), GetModuleHandle(NULL), NULL);

            CreateWindowExW(0, L"BUTTON", L"Clear Standby RAM",
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 380, 110, 160, 32,
                            hwnd, reinterpret_cast<HMENU>(ID_TRAY_RAM_PURGE), GetModuleHandle(NULL), NULL);

            // Log / Status Text Box
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Initializing monitor...",
                            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                            20, 160, 560, 260, hwnd, reinterpret_cast<HMENU>(2001), GetModuleHandle(NULL), NULL);
            return 0;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == ID_TRAY_ADD_GAME && s_trayInstance) {
                s_trayInstance->promptAddGame();
            } else if (id == ID_TRAY_RESTORE && s_trayInstance && s_trayInstance->m_engine) {
                s_trayInstance->m_engine->restoreDefaults();
            } else if (id == ID_TRAY_RAM_PURGE && s_trayInstance && s_trayInstance->m_engine) {
                // Manually trigger RAM purge
                WinRamOptimizer ram;
                ram.purgeStandbyMemory();
                ram.trimWorkingSets();
            }
            return 0;
        }
        case WM_CLOSE: {
            ShowWindow(hwnd, SW_HIDE);
            return 0; // Hide to tray rather than destroy
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WinSystemTray::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            if (s_trayInstance) s_trayInstance->showContextMenu();
            return 0;
        } else if (lParam == WM_LBUTTONDBLCLK) {
            if (s_trayInstance) s_trayInstance->openDashboard();
            return 0;
        }
    } else if (msg == WM_COMMAND) {
        WORD id = LOWORD(wParam);
        switch (id) {
            case ID_TRAY_DASHBOARD:
                if (s_trayInstance) s_trayInstance->openDashboard();
                break;
            case ID_TRAY_ADD_GAME:
                if (s_trayInstance) s_trayInstance->promptAddGame();
                break;
            case ID_TRAY_RAM_PURGE: {
                WinRamOptimizer ram;
                ram.purgeStandbyMemory();
                ram.trimWorkingSets();
                break;
            }
            case ID_TRAY_RESTORE:
                if (s_trayInstance && s_trayInstance->m_engine) {
                    s_trayInstance->m_engine->restoreDefaults();
                }
                break;
            case ID_TRAY_STARTUP:
                if (s_trayInstance) s_trayInstance->toggleStartupRun();
                break;
            case ID_TRAY_EXIT:
                if (s_trayInstance) s_trayInstance->quit();
                break;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void WinSystemTray::runMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void WinSystemTray::quit() {
    if (m_engine) {
        m_engine->stopMonitoring();
        m_engine->restoreDefaults();
    }
    PostQuitMessage(0);
}

std::unique_ptr<SystemTray> SystemTray::create(std::shared_ptr<OptimizerEngine> engine) {
    auto tray = std::make_unique<WinSystemTray>();
    tray->init(engine);
    return tray;
}

} // namespace optimizer

#endif // _WIN32
