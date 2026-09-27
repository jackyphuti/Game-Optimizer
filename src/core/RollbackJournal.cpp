#include "optimizer/RollbackJournal.hpp"
#include "optimizer/Logger.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;

namespace optimizer {

std::string RollbackJournal::getDefaultJournalPath() {
#if defined(_WIN32)
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) {
        return (fs::path(localAppData) / "GameOptimizer" / "active_session.json").string();
    }
    return "active_session.json";
#else
    const char* xdgState = std::getenv("XDG_STATE_HOME");
    if (xdgState && std::string(xdgState).length() > 0) {
        return (fs::path(xdgState) / "gameoptimizer" / "active_session.json").string();
    }
    const char* home = std::getenv("HOME");
    if (home) {
        return (fs::path(home) / ".local" / "state" / "gameoptimizer" / "active_session.json").string();
    }
    return "active_session.json";
#endif
}

RollbackJournal::RollbackJournal(std::string journalPath)
    : m_journalPath(std::move(journalPath)) {
    if (m_journalPath.empty()) {
        m_journalPath = getDefaultJournalPath();
    }
}

bool RollbackJournal::hasPendingSession() const {
    return fs::exists(m_journalPath);
}

bool RollbackJournal::writeSession(const ActiveOptimizationSession& session) {
    try {
        fs::path p(m_journalPath);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        json::Value root(json::Type::Object);
        root["active"] = session.active;
        root["gamePid"] = session.gamePid;
        root["gameName"] = session.gameName;
        root["gameExe"] = session.gameExe;
        root["startTime"] = session.startTime;
        root["originalPowerPlan"] = session.originalPowerPlan;
        root["originalGpuState"] = session.originalGpuState;
        root["originalGameModeActive"] = session.originalGameModeActive;

        json::Value procsArr(json::Type::Array);
        for (const auto& proc : session.modifiedProcesses) {
            json::Value pVal(json::Type::Object);
            pVal["pid"] = proc.pid;
            pVal["name"] = proc.name;
            pVal["originalPriority"] = priorityToString(proc.originalPriority);
            pVal["originalAffinity"] = proc.originalAffinity;
            pVal["wasSuspended"] = proc.wasSuspended;
            procsArr.push_back(pVal);
        }
        root["modifiedProcesses"] = procsArr;

        std::ofstream file(m_journalPath);
        if (!file.is_open()) {
            LOG_ERROR("Could not open rollback journal file for writing: " + m_journalPath);
            return false;
        }

        file << root.serialize(2);
        file.flush();
        file.close();

        LOG_DEBUG("Wrote active optimization journal to " + m_journalPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception writing rollback journal: " + std::string(e.what()));
        return false;
    }
}

std::optional<ActiveOptimizationSession> RollbackJournal::readSession() {
    if (!fs::exists(m_journalPath)) {
        return std::nullopt;
    }

    try {
        std::ifstream file(m_journalPath);
        if (!file.is_open()) return std::nullopt;

        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();

        json::Value root = json::parse(content);
        if (!root.isObject()) return std::nullopt;

        ActiveOptimizationSession s;
        s.active = root["active"].asBool(false);
        s.gamePid = root["gamePid"].asUInt(0);
        s.gameName = root["gameName"].asString();
        s.gameExe = root["gameExe"].asString();
        s.startTime = root["startTime"].asString();
        s.originalPowerPlan = root["originalPowerPlan"].asString();
        s.originalGpuState = root["originalGpuState"].asString();
        s.originalGameModeActive = root["originalGameModeActive"].asBool(false);

        const auto& procs = root["modifiedProcesses"];
        if (procs.isArray()) {
            for (size_t i = 0; i < procs.arrVal.size(); ++i) {
                const auto& item = procs[i];
                if (!item.isObject()) continue;

                ProcessRollbackEntry e;
                e.pid = item["pid"].asUInt(0);
                e.name = item["name"].asString();
                e.originalPriority = priorityFromString(item["originalPriority"].asString("Normal"));
                e.originalAffinity = item["originalAffinity"].asUInt64(0);
                e.wasSuspended = item["wasSuspended"].asBool(false);
                s.modifiedProcesses.push_back(e);
            }
        }

        return s;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception reading rollback journal: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool RollbackJournal::clearSession() {
    try {
        if (fs::exists(m_journalPath)) {
            fs::remove(m_journalPath);
            LOG_DEBUG("Cleared rollback journal: " + m_journalPath);
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception clearing rollback journal: " + std::string(e.what()));
        return false;
    }
}

} // namespace optimizer
