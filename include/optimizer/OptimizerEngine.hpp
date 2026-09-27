#pragma once

#include "optimizer/Types.hpp"
#include "optimizer/IProcessManager.hpp"
#include "optimizer/IPowerManager.hpp"
#include "optimizer/IGpuController.hpp"
#include "optimizer/IRamOptimizer.hpp"
#include "optimizer/IGameDetector.hpp"
#include "optimizer/ProfileManager.hpp"
#include "optimizer/RollbackJournal.hpp"
#include "optimizer/OptimizationSession.hpp"
#include "optimizer/SystemMonitor.hpp"

#include <memory>
#include <atomic>
#include <thread>
#include <functional>

namespace optimizer {

class OptimizerEngine {
public:
    using GameEventCallback = std::function<void(const std::string& gameName, uint32_t pid)>;
    using MetricsCallback = std::function<void(const SystemMetrics& metrics)>;

    OptimizerEngine(std::shared_ptr<IProcessManager> procMgr,
                    std::shared_ptr<IPowerManager> powerMgr,
                    std::shared_ptr<IGpuController> gpuCtrl,
                    std::shared_ptr<IRamOptimizer> ramOpt,
                    std::shared_ptr<IGameDetector> detector,
                    std::shared_ptr<ProfileManager> profileMgr);

    ~OptimizerEngine();

    void startMonitoring(uint32_t pollIntervalMs = 2000);
    void stopMonitoring();
    bool isMonitoring() const;

    // Single evaluation step (can be called manually or in loop)
    void pollCycle();

    // Manual triggers
    bool optimizePid(uint32_t pid, const std::string& customProfileName = "");
    bool restoreDefaults();

    // Callbacks
    void setOnGameStarted(GameEventCallback cb) { m_onGameStarted = std::move(cb); }
    void setOnGameExited(GameEventCallback cb)  { m_onGameExited = std::move(cb); }
    void setOnMetricsUpdated(MetricsCallback cb) { m_onMetricsUpdated = std::move(cb); }

    std::shared_ptr<OptimizationSession> getSession() const { return m_session; }
    std::shared_ptr<ProfileManager> getProfileManager() const { return m_profileMgr; }
    SystemMetrics getLatestMetrics();

    static std::shared_ptr<OptimizerEngine> createPlatformDefault(std::shared_ptr<ProfileManager> profileMgr = nullptr);

private:
    std::shared_ptr<IProcessManager> m_procMgr;
    std::shared_ptr<IPowerManager> m_powerMgr;
    std::shared_ptr<IGpuController> m_gpuCtrl;
    std::shared_ptr<IRamOptimizer> m_ramOpt;
    std::shared_ptr<IGameDetector> m_detector;
    std::shared_ptr<ProfileManager> m_profileMgr;
    std::shared_ptr<RollbackJournal> m_journal;
    std::shared_ptr<OptimizationSession> m_session;
    std::unique_ptr<SystemMonitor> m_monitor;

    std::atomic<bool> m_running{false};
    std::thread m_workerThread;
    uint32_t m_pollIntervalMs{2000};

    uint32_t m_activePid{0};
    std::string m_activeGameName;

    GameEventCallback m_onGameStarted;
    GameEventCallback m_onGameExited;
    MetricsCallback m_onMetricsUpdated;
};

} // namespace optimizer
