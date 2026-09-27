#include "optimizer/OptimizationSession.hpp"
#include "optimizer/Logger.hpp"
#include <chrono>
#include <ctime>
#include <sstream>

namespace optimizer {

OptimizationSession::OptimizationSession(std::shared_ptr<IProcessManager> procMgr,
                                         std::shared_ptr<IPowerManager> powerMgr,
                                         std::shared_ptr<IGpuController> gpuCtrl,
                                         std::shared_ptr<IRamOptimizer> ramOpt,
                                         std::shared_ptr<RollbackJournal> journal)
    : m_procMgr(std::move(procMgr)),
      m_powerMgr(std::move(powerMgr)),
      m_gpuCtrl(std::move(gpuCtrl)),
      m_ramOpt(std::move(ramOpt)),
      m_journal(std::move(journal)) {}

OptimizationSession::~OptimizationSession() {
    if (m_session.active) {
        LOG_WARN("OptimizationSession destroyed while active. Reverting tweaks...");
        revert();
    }
}

bool OptimizationSession::isActive() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_session.active;
}

const ActiveOptimizationSession& OptimizationSession::getCurrentSession() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_session;
}

bool OptimizationSession::apply(const GameProfile& profile, const RunningProcessInfo& gameProc) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_session.active) {
        LOG_WARN("An optimization session is already active for PID " + std::to_string(m_session.gamePid) + ". Reverting previous session first.");
        revert();
    }

    LOG_INFO(">>> Initiating optimization session for game: " + profile.name + " (PID " + std::to_string(gameProc.pid) + ") <<<");

    m_session.active = true;
    m_session.gamePid = gameProc.pid;
    m_session.gameName = profile.name;
    m_session.gameExe = gameProc.name;

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char timeBuf[64];
#if defined(_WIN32)
    ctime_s(timeBuf, sizeof(timeBuf), &now);
#else
    ctime_r(&now, timeBuf);
#endif
    m_session.startTime = timeBuf;

    // 1. Power Management
    if (profile.switchPowerPlan && m_powerMgr) {
        m_session.originalPowerPlan = m_powerMgr->getActiveProfileName();
        LOG_INFO("[Power] Stored original scheme: " + m_session.originalPowerPlan);
        if (m_powerMgr->switchHighPerformance()) {
            LOG_INFO("[Power] Switched to High Performance plan.");
        } else {
            LOG_WARN("[Power] Failed or skipped switching power plan (graceful degradation).");
        }
    }

    // 2. GPU Tuning
    if (profile.tuneGpu && m_gpuCtrl) {
        m_session.originalGpuState = m_gpuCtrl->getSavedState();
        if (m_gpuCtrl->applyMaxPerformance(gameProc.name)) {
            LOG_INFO("[GPU] Engaged maximum performance profile for " + m_gpuCtrl->getVendorName());
        } else {
            LOG_WARN("[GPU] Could not apply maximum performance state (graceful degradation).");
        }
    }

    // 3. Process CPU Priority
    if (profile.tuneCpuPriority && m_procMgr) {
        ProcessPriority origPrio = m_procMgr->getProcessPriority(gameProc.pid);
        LOG_INFO("[CPU Priority] Original game priority: " + priorityToString(origPrio));
        if (m_procMgr->setProcessPriority(gameProc.pid, profile.targetPriority)) {
            LOG_INFO("[CPU Priority] Set game priority to " + priorityToString(profile.targetPriority));
        } else {
            LOG_WARN("[CPU Priority] Failed to set game process priority (insufficient rights?).");
        }
    }

    // 4. Process CPU Affinity
    if (profile.tuneAffinity && profile.affinityMask != 0 && m_procMgr) {
        uint64_t origAff = m_procMgr->getProcessAffinity(gameProc.pid);
        LOG_INFO("[CPU Affinity] Original mask: 0x" + std::to_string(origAff));
        if (m_procMgr->setProcessAffinity(gameProc.pid, profile.affinityMask)) {
            LOG_INFO("[CPU Affinity] Applied affinity mask: 0x" + std::to_string(profile.affinityMask));
        } else {
            LOG_WARN("[CPU Affinity] Failed to set CPU affinity mask.");
        }
    }

    // 5. Background Process Suppression
    if (profile.lowerBackgroundProcesses && m_procMgr) {
        m_session.modifiedProcesses = m_procMgr->lowerNonEssentialProcesses(
            gameProc.name, profile.customBackgroundBlacklist);
        LOG_INFO("[Background] Throttled " + std::to_string(m_session.modifiedProcesses.size()) + " background processes.");
    }

    // 6. RAM Optimization
    if (profile.trimRamOnLaunch && m_ramOpt) {
        LOG_INFO("[RAM] Trimming background working sets...");
        m_ramOpt->trimWorkingSets();
        LOG_INFO("[RAM] Purging standby memory list...");
        if (!m_ramOpt->purgeStandbyMemory()) {
            LOG_WARN("[RAM] Purging standby memory was skipped or lacks SeProfileSingleProcessPrivilege (graceful degradation).");
        }
    }

    // 7. Linux Pagecache Dropping (Opt-in)
    if (profile.dropCachesOnLaunch && m_ramOpt) {
        LOG_INFO("[RAM] Opt-in drop_caches requested. Executing sync & drop_caches...");
        m_ramOpt->dropCaches(true);
    }

    // 8. Persist to rollback journal for crash safety
    if (m_journal) {
        m_journal->writeSession(m_session);
    }

    LOG_INFO(">>> Optimizations successfully applied for " + profile.name + " <<<");
    return true;
}

bool OptimizationSession::revert() {
    if (!m_session.active) {
        return true;
    }

    LOG_INFO("<<< Reverting optimizations for game PID " + std::to_string(m_session.gamePid) + " (" + m_session.gameName + ") >>>");

    // 1. Restore Background Processes
    if (m_procMgr && !m_session.modifiedProcesses.empty()) {
        m_procMgr->restoreProcesses(m_session.modifiedProcesses);
        LOG_INFO("[Background] Restored " + std::to_string(m_session.modifiedProcesses.size()) + " process priorities.");
        m_session.modifiedProcesses.clear();
    }

    // 2. Restore Power Plan
    if (m_powerMgr && !m_session.originalPowerPlan.empty()) {
        m_powerMgr->setOriginalScheme(m_session.originalPowerPlan);
        if (m_powerMgr->restoreOriginalScheme()) {
            LOG_INFO("[Power] Restored original power plan: " + m_session.originalPowerPlan);
        } else {
            LOG_WARN("[Power] Failed to restore original power plan.");
        }
        m_session.originalPowerPlan.clear();
    }

    // 3. Restore GPU State
    if (m_gpuCtrl) {
        if (!m_session.originalGpuState.empty()) {
            m_gpuCtrl->setSavedState(m_session.originalGpuState);
        }
        if (m_gpuCtrl->restoreDefaultState(m_session.gameExe)) {
            LOG_INFO("[GPU] Restored default GPU power/clock state.");
        }
        m_session.originalGpuState.clear();
    }

    // 4. Clear Journal
    if (m_journal) {
        m_journal->clearSession();
    }

    m_session.active = false;
    m_session.gamePid = 0;
    m_session.gameName.clear();
    m_session.gameExe.clear();

    LOG_INFO("<<< System restored to default state cleanly >>>");
    return true;
}

bool OptimizationSession::recoverPendingSession() {
    if (!m_journal || !m_journal->hasPendingSession()) {
        return false;
    }

    auto pending = m_journal->readSession();
    if (!pending || !pending->active) {
        m_journal->clearSession();
        return false;
    }

    LOG_WARN("=== Detected un-reverted optimization session from previous crash! Initiating automatic recovery... ===");
    m_session = *pending;

    bool result = revert();
    if (result) {
        LOG_INFO("=== Crash recovery completed successfully. System state restored. ===");
    } else {
        LOG_ERROR("=== Crash recovery encountered issues while reverting tweaks. ===");
    }
    return result;
}

} // namespace optimizer
