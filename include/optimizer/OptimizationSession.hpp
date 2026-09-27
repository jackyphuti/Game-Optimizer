#pragma once

#include "optimizer/Types.hpp"
#include "optimizer/IProcessManager.hpp"
#include "optimizer/IPowerManager.hpp"
#include "optimizer/IGpuController.hpp"
#include "optimizer/IRamOptimizer.hpp"
#include "optimizer/RollbackJournal.hpp"
#include <memory>
#include <mutex>

namespace optimizer {

class OptimizationSession {
public:
    OptimizationSession(std::shared_ptr<IProcessManager> procMgr,
                        std::shared_ptr<IPowerManager> powerMgr,
                        std::shared_ptr<IGpuController> gpuCtrl,
                        std::shared_ptr<IRamOptimizer> ramOpt,
                        std::shared_ptr<RollbackJournal> journal);

    ~OptimizationSession();

    // Applies all requested tweaks for the target game
    bool apply(const GameProfile& profile, const RunningProcessInfo& gameProc);

    // Reverts all tweaks back to the saved original states
    bool revert();

    // Recovers and reverts a previous un-reverted session (e.g. after crash)
    bool recoverPendingSession();

    bool isActive() const;
    const ActiveOptimizationSession& getCurrentSession() const;

private:
    std::shared_ptr<IProcessManager> m_procMgr;
    std::shared_ptr<IPowerManager> m_powerMgr;
    std::shared_ptr<IGpuController> m_gpuCtrl;
    std::shared_ptr<IRamOptimizer> m_ramOpt;
    std::shared_ptr<RollbackJournal> m_journal;

    ActiveOptimizationSession m_session;
    mutable std::recursive_mutex m_mutex;
};

// RAII Guard object ensuring rollback on abnormal exit / scope exit
class OptimizationGuard {
public:
    explicit OptimizationGuard(OptimizationSession& session)
        : m_session(session), m_dismissed(false) {}

    ~OptimizationGuard() {
        if (!m_dismissed && m_session.isActive()) {
            m_session.revert();
        }
    }

    void dismiss() {
        m_dismissed = true;
    }

private:
    OptimizationSession& m_session;
    bool m_dismissed;
};

} // namespace optimizer
