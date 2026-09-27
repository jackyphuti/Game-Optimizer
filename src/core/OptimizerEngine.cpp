#include "optimizer/OptimizerEngine.hpp"
#include "optimizer/Logger.hpp"

#if defined(_WIN32)
#include "../../src/platform/windows/WinProcessManager.hpp"
#include "../../src/platform/windows/WinPowerManager.hpp"
#include "../../src/platform/windows/WinGpuController.hpp"
#include "../../src/platform/windows/WinRamOptimizer.hpp"
#include "../../src/platform/windows/WinGameDetector.hpp"
#else
#include "../../src/platform/linux/LinuxProcessManager.hpp"
#include "../../src/platform/linux/LinuxPowerManager.hpp"
#include "../../src/platform/linux/LinuxGpuController.hpp"
#include "../../src/platform/linux/LinuxRamOptimizer.hpp"
#include "../../src/platform/linux/LinuxGameDetector.hpp"
#endif

namespace optimizer {

std::shared_ptr<OptimizerEngine> OptimizerEngine::createPlatformDefault(std::shared_ptr<ProfileManager> profileMgr) {
    if (!profileMgr) {
        profileMgr = std::make_shared<ProfileManager>();
    }

#if defined(_WIN32)
    auto procMgr = std::make_shared<WinProcessManager>(profileMgr->getSystemWhitelist());
    auto powerMgr = std::make_shared<WinPowerManager>();
    auto gpuCtrl = std::make_shared<WinGpuController>();
    auto ramOpt = std::make_shared<WinRamOptimizer>();
    auto detector = std::make_shared<WinGameDetector>();
#else
    auto procMgr = std::make_shared<LinuxProcessManager>(profileMgr->getSystemWhitelist());
    auto powerMgr = std::make_shared<LinuxPowerManager>();
    auto gpuCtrl = std::make_shared<LinuxGpuController>();
    auto ramOpt = std::make_shared<LinuxRamOptimizer>();
    auto detector = std::make_shared<LinuxGameDetector>();
#endif

    return std::make_shared<OptimizerEngine>(procMgr, powerMgr, gpuCtrl, ramOpt, detector, profileMgr);
}

OptimizerEngine::OptimizerEngine(std::shared_ptr<IProcessManager> procMgr,
                                 std::shared_ptr<IPowerManager> powerMgr,
                                 std::shared_ptr<IGpuController> gpuCtrl,
                                 std::shared_ptr<IRamOptimizer> ramOpt,
                                 std::shared_ptr<IGameDetector> detector,
                                 std::shared_ptr<ProfileManager> profileMgr)
    : m_procMgr(std::move(procMgr)),
      m_powerMgr(std::move(powerMgr)),
      m_gpuCtrl(std::move(gpuCtrl)),
      m_ramOpt(std::move(ramOpt)),
      m_detector(std::move(detector)),
      m_profileMgr(std::move(profileMgr)),
      m_journal(std::make_shared<RollbackJournal>()),
      m_monitor(std::make_unique<SystemMonitor>()) {
    m_session = std::make_shared<OptimizationSession>(m_procMgr, m_powerMgr, m_gpuCtrl, m_ramOpt, m_journal);

    // Check for crash recovery upon startup
    m_session->recoverPendingSession();
}

OptimizerEngine::~OptimizerEngine() {
    stopMonitoring();
    if (m_session && m_session->isActive()) {
        m_session->revert();
    }
}

void OptimizerEngine::startMonitoring(uint32_t pollIntervalMs) {
    if (m_running.exchange(true)) {
        return;
    }
    m_pollIntervalMs = pollIntervalMs;
    LOG_INFO("Started game optimization monitoring loop (interval: " + std::to_string(pollIntervalMs) + "ms)");

    m_workerThread = std::thread([this]() {
        while (m_running.load()) {
            pollCycle();
            std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalMs));
        }
    });
}

void OptimizerEngine::stopMonitoring() {
    if (!m_running.exchange(false)) {
        return;
    }
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    LOG_INFO("Stopped game optimization monitoring loop.");
}

bool OptimizerEngine::isMonitoring() const {
    return m_running.load();
}

void OptimizerEngine::pollCycle() {
    if (!m_detector || !m_profileMgr || !m_session) return;

    // Send metrics update if callback registered
    if (m_onMetricsUpdated) {
        SystemMetrics metrics = getLatestMetrics();
        m_onMetricsUpdated(metrics);
    }

    if (!m_session->isActive()) {
        // Look for registered games running
        auto registeredProfiles = m_profileMgr->getAllProfiles();
        auto detectedGame = m_detector->detectGame(registeredProfiles);

        if (detectedGame.has_value()) {
            const auto& gameProc = detectedGame.value();
            auto profileOpt = m_profileMgr->findProfileForExecutable(gameProc.name);

            if (profileOpt.has_value()) {
                LOG_INFO("[Engine] Detected game launch: " + profileOpt->name + " (PID " + std::to_string(gameProc.pid) + ")");
                m_activePid = gameProc.pid;
                m_activeGameName = profileOpt->name;

                m_session->apply(*profileOpt, gameProc);

                if (m_onGameStarted) {
                    m_onGameStarted(m_activeGameName, m_activePid);
                }
            }
        }
    } else {
        // A game is currently active - verify if it is still running
        if (!m_detector->isProcessRunning(m_activePid)) {
            LOG_INFO("[Engine] Game process terminated: " + m_activeGameName + " (PID " + std::to_string(m_activePid) + ")");

            if (m_onGameExited) {
                m_onGameExited(m_activeGameName, m_activePid);
            }

            m_session->revert();
            m_activePid = 0;
            m_activeGameName.clear();
        }
    }
}

bool OptimizerEngine::optimizePid(uint32_t pid, const std::string& customProfileName) {
    if (!m_detector || !m_detector->isProcessRunning(pid)) {
        LOG_WARN("Cannot optimize PID " + std::to_string(pid) + ": process not found.");
        return false;
    }

    std::string name = m_detector->getProcessName(pid);
    std::string path = m_detector->getProcessPath(pid);

    RunningProcessInfo proc;
    proc.pid = pid;
    proc.name = name;
    proc.exePath = path;

    auto profileOpt = m_profileMgr->findProfileForExecutable(name);
    GameProfile profile;
    if (profileOpt.has_value()) {
        profile = *profileOpt;
    } else {
        profile.name = customProfileName.empty() ? name : customProfileName;
        profile.exeName = name;
        profile.fullPath = path;
    }

    m_activePid = pid;
    m_activeGameName = profile.name;
    bool success = m_session->apply(profile, proc);
    if (success && m_onGameStarted) {
        m_onGameStarted(m_activeGameName, m_activePid);
    }
    return success;
}

bool OptimizerEngine::restoreDefaults() {
    LOG_INFO("Manual restore defaults requested.");
    bool res = m_session->revert();
    m_activePid = 0;
    m_activeGameName.clear();
    return res;
}

SystemMetrics OptimizerEngine::getLatestMetrics() {
    SystemMetrics m = m_monitor->sampleMetrics();
    if (m_powerMgr) {
        m.activePowerPlan = m_powerMgr->getActiveProfileName();
    }
    if (m_gpuCtrl) {
        m.gpuVendor = m_gpuCtrl->getVendorName();
        m.gpuName = m_gpuCtrl->getGpuName();
    }
    return m;
}

} // namespace optimizer
