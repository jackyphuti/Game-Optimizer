#pragma once

#include "optimizer/Types.hpp"
#include <string>
#include <optional>

namespace optimizer {

class RollbackJournal {
public:
    explicit RollbackJournal(std::string journalPath = "");

    bool writeSession(const ActiveOptimizationSession& session);
    std::optional<ActiveOptimizationSession> readSession();
    bool clearSession();
    bool hasPendingSession() const;

    std::string getJournalPath() const { return m_journalPath; }

    static std::string getDefaultJournalPath();

private:
    std::string m_journalPath;
};

} // namespace optimizer
