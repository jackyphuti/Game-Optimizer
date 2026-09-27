#include "optimizer/OptimizerEngine.hpp"
#include "optimizer/Logger.hpp"
#include "optimizer/SystemTray.hpp"
#include "optimizer/Types.hpp"

#include <iostream>
#include <csignal>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static std::shared_ptr<optimizer::OptimizerEngine> g_engine = nullptr;

static void globalCleanup() {
    if (g_engine) {
        LOG_INFO("Global cleanup triggered. Restoring default system states...");
        g_engine->restoreDefaults();
        g_engine->stopMonitoring();
    }
}

static void signalHandler(int sig) {
    LOG_WARN("Caught signal " + std::to_string(sig) + ". Initiating emergency rollback...");
    globalCleanup();
    std::exit(sig);
}

#if defined(_WIN32)
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            LOG_WARN("Received console control event " + std::to_string(ctrlType) + ". Restoring system...");
            globalCleanup();
            return TRUE;
        default:
            return FALSE;
    }
}

static LONG WINAPI unhandledExceptionHandler(EXCEPTION_POINTERS* /*pExceptionInfo*/) {
    LOG_ERROR("FATAL: Unhandled exception caught! Executing emergency state rollback...");
    globalCleanup();
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static void printUsage(const char* progName) {
    std::cout << "========================================================\n"
              << "       Cross-Platform Game Optimizer (v2.0)            \n"
              << "========================================================\n"
              << "Usage: " << progName << " [command / options]\n\n"
              << "Commands:\n"
              << "  --monitor               Run background game monitoring loop in console\n"
              << "  --tray, --gui           Launch lightweight desktop system tray application\n"
              << "  --daemon                Run as background service/daemon\n"
              << "  --status                Display live CPU, RAM, GPU, and Power stats\n"
              << "  --list, --list-games    Show registered game profiles and configurations\n"
              << "  --add-game <path> [name] Register a game by executable path\n"
              << "  --remove-game <name>    Remove a registered game profile\n"
              << "  --optimize-now <pid>    Manually apply game tweaks to a running process\n"
              << "  --restore               Restore system defaults & revert all active tweaks\n"
              << "  --version, -v           Show version information\n"
              << "  --help, -h              Show this help message\n\n"
              << "Examples:\n"
              << "  " << progName << " --tray\n"
              << "  " << progName << " --add-game \"C:\\Games\\Cyberpunk2077\\bin\\x64\\Cyberpunk2077.exe\"\n"
              << "  " << progName << " --optimize-now 1234\n"
              << "  " << progName << " --restore\n";
}

int main(int argc, char* argv[]) {
    std::atexit(globalCleanup);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
#if defined(SIGSEGV)
    std::signal(SIGSEGV, signalHandler);
#endif

#if defined(_WIN32)
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    SetUnhandledExceptionFilter(unhandledExceptionHandler);
#endif

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    auto profileMgr = std::make_shared<optimizer::ProfileManager>();
    g_engine = optimizer::OptimizerEngine::createPlatformDefault(profileMgr);

    if (args.empty()) {
#if defined(_WIN32)
        // Default to tray mode on Windows
        args.emplace_back("--tray");
#else
        // Default to monitor on Linux
        args.emplace_back("--monitor");
#endif
    }

    const std::string& cmd = args[0];

    if (cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    if (cmd == "--version" || cmd == "-v") {
        std::cout << "Game Optimizer v2.0.0 (Cross-Platform C++ Native)\n";
        return 0;
    }

    if (cmd == "--status") {
        auto metrics = g_engine->getLatestMetrics();
        std::cout << "=== System Status ===\n"
                  << "CPU Usage:      " << metrics.cpuUsagePercent << "%\n"
                  << "RAM Used:       " << (metrics.usedRamBytes / (1024 * 1024)) << " MB / "
                                      << (metrics.totalRamBytes / (1024 * 1024)) << " MB ("
                                      << metrics.ramUsagePercent << "%)\n"
                  << "Power Plan:     " << metrics.activePowerPlan << "\n"
                  << "GPU Hardware:   " << metrics.gpuVendor << " - " << metrics.gpuName << "\n";
        if (g_engine->getSession() && g_engine->getSession()->isActive()) {
            const auto& s = g_engine->getSession()->getCurrentSession();
            std::cout << "Active Game:    " << s.gameName << " (PID " << s.gamePid << ")\n";
        } else {
            std::cout << "Active Game:    None (Idle)\n";
        }
        return 0;
    }

    if (cmd == "--list" || cmd == "--list-games") {
        auto profiles = profileMgr->getAllProfiles();
        std::cout << "=== Registered Game Profiles (" << profiles.size() << ") ===\n";
        for (const auto& p : profiles) {
            std::cout << " - " << p.name << " [" << p.exeName << "]"
                      << " | Priority: " << priorityToString(p.targetPriority)
                      << " | GPU: " << (p.tuneGpu ? "Yes" : "No")
                      << " | PowerPlan: " << (p.switchPowerPlan ? "Yes" : "No")
                      << " | Throttling: " << (p.lowerBackgroundProcesses ? "Yes" : "No")
                      << "\n";
        }
        return 0;
    }

    if (cmd == "--add-game") {
        if (args.size() < 2) {
            std::cerr << "Error: --add-game requires an executable path.\n";
            std::cerr << "Usage: " << argv[0] << " --add-game <path_to_game_exe> [optional_name]\n";
            return 1;
        }
        std::string path = args[1];
        std::string name = (args.size() >= 3) ? args[2] : "";
        if (profileMgr->addGameByPath(path, name)) {
            std::cout << "Successfully added game profile: " << path << "\n";
            return 0;
        } else {
            std::cerr << "Failed to add game profile.\n";
            return 1;
        }
    }

    if (cmd == "--remove-game") {
        if (args.size() < 2) {
            std::cerr << "Error: --remove-game requires an executable name or title.\n";
            return 1;
        }
        if (profileMgr->removeGame(args[1])) {
            std::cout << "Successfully removed game profile: " << args[1] << "\n";
            return 0;
        } else {
            std::cerr << "Profile not found: " << args[1] << "\n";
            return 1;
        }
    }

    if (cmd == "--optimize-now") {
        if (args.size() < 2) {
            std::cerr << "Error: --optimize-now requires a target PID.\n";
            return 1;
        }
        uint32_t pid = static_cast<uint32_t>(std::stoul(args[1]));
        if (g_engine->optimizePid(pid)) {
            std::cout << "Optimizations applied to PID " << pid << ".\n";
            std::cout << "Press Enter to restore defaults and exit...";
            std::cin.get();
            g_engine->restoreDefaults();
            return 0;
        } else {
            std::cerr << "Failed to optimize PID " << pid << ".\n";
            return 1;
        }
    }

    if (cmd == "--restore") {
        g_engine->restoreDefaults();
        std::cout << "System defaults restored successfully.\n";
        return 0;
    }

    if (cmd == "--tray" || cmd == "--gui") {
        g_engine->startMonitoring(2000);
        auto tray = optimizer::SystemTray::create(g_engine);
        if (tray) {
            tray->showNotification("Game Optimizer", "Monitoring running in background.");
            tray->runMessageLoop();
        }
        return 0;
    }

    if (cmd == "--monitor" || cmd == "--daemon") {
        g_engine->startMonitoring(2000);
        std::cout << "Game Optimizer active monitoring loop started. Press Ctrl+C to terminate.\n";
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            auto metrics = g_engine->getLatestMetrics();
            std::cout << "[Heartbeat] CPU: " << static_cast<int>(metrics.cpuUsagePercent)
                      << "% | RAM: " << static_cast<int>(metrics.ramUsagePercent)
                      << "% | Plan: " << metrics.activePowerPlan << "\n";
        }
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    printUsage(argv[0]);
    return 1;
}
