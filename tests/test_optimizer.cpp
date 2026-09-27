#include "optimizer/Types.hpp"
#include "optimizer/ProfileManager.hpp"
#include "optimizer/RollbackJournal.hpp"
#include "optimizer/SystemMonitor.hpp"
#include "optimizer/Logger.hpp"

#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

void testJson() {
    std::cout << "[TEST] Running JSON serialization/deserialization tests..." << std::endl;

    optimizer::json::Value root(optimizer::json::Type::Object);
    root["appName"] = "GameOptimizer";
    root["version"] = 2.0;
    root["enabled"] = true;

    optimizer::json::Value arr(optimizer::json::Type::Array);
    arr.push_back("cs2.exe");
    arr.push_back("dota2.exe");
    root["games"] = arr;

    std::string serialized = root.serialize(2);
    assert(serialized.find("\"appName\": \"GameOptimizer\"") != std::string::npos);

    optimizer::json::Value parsed = optimizer::json::parse(serialized);
    assert(parsed.isObject());
    assert(parsed["appName"].asString() == "GameOptimizer");
    assert(parsed["enabled"].asBool() == true);
    assert(parsed["games"].isArray());
    assert(parsed["games"].arrVal.size() == 2);
    assert(parsed["games"][0].asString() == "cs2.exe");
    assert(parsed["games"][1].asString() == "dota2.exe");

    std::cout << "[PASS] JSON tests succeeded." << std::endl;
}

void testPriorityConversion() {
    std::cout << "[TEST] Running Priority string conversion tests..." << std::endl;

    assert(optimizer::priorityToString(optimizer::ProcessPriority::High) == "High");
    assert(optimizer::priorityFromString("High") == optimizer::ProcessPriority::High);
    assert(optimizer::priorityFromString("high") == optimizer::ProcessPriority::High);
    assert(optimizer::priorityFromString("idle") == optimizer::ProcessPriority::Idle);
    assert(optimizer::priorityFromString("belownormal") == optimizer::ProcessPriority::BelowNormal);
    assert(optimizer::priorityFromString("abovenormal") == optimizer::ProcessPriority::AboveNormal);
    assert(optimizer::priorityFromString("realtime") == optimizer::ProcessPriority::Realtime);

    std::cout << "[PASS] Priority conversion tests succeeded." << std::endl;
}

void testProfileManager() {
    std::cout << "[TEST] Running ProfileManager tests..." << std::endl;

    std::string tempConfig = (fs::temp_directory_path() / "test_profiles.json").string();
    if (fs::exists(tempConfig)) fs::remove(tempConfig);

    {
        optimizer::ProfileManager pm(tempConfig);
        assert(pm.getAllProfiles().size() > 0); // Defaults populated

        bool added = pm.addGameByPath("C:\\Games\\TestGame\\bin\\game.exe", "Test Title");
        assert(added);

        auto found = pm.findProfileForExecutable("game.exe");
        assert(found.has_value());
        assert(found->name == "Test Title");
        assert(found->exeName == "game.exe");

        pm.saveProfiles();
    }

    // Re-load from disk
    {
        optimizer::ProfileManager pm(tempConfig);
        auto found = pm.findProfileForExecutable("game.exe");
        assert(found.has_value());
        assert(found->name == "Test Title");

        bool removed = pm.removeGame("Test Title");
        assert(removed);

        auto notFound = pm.findProfileForExecutable("game.exe");
        assert(!notFound.has_value());
    }

    if (fs::exists(tempConfig)) fs::remove(tempConfig);
    std::cout << "[PASS] ProfileManager tests succeeded." << std::endl;
}

void testRollbackJournal() {
    std::cout << "[TEST] Running RollbackJournal tests..." << std::endl;

    std::string tempJournal = (fs::temp_directory_path() / "test_active_session.json").string();
    if (fs::exists(tempJournal)) fs::remove(tempJournal);

    optimizer::RollbackJournal journal(tempJournal);
    assert(!journal.hasPendingSession());

    optimizer::ActiveOptimizationSession session;
    session.active = true;
    session.gamePid = 9999;
    session.gameName = "Doom Eternal";
    session.gameExe = "DoomEternal.exe";
    session.originalPowerPlan = "Balanced (381b4222-f694-41f0-9685-ff5bb260df2e)";

    optimizer::ProcessRollbackEntry e;
    e.pid = 1234;
    e.name = "discord.exe";
    e.originalPriority = optimizer::ProcessPriority::Normal;
    session.modifiedProcesses.push_back(e);

    bool written = journal.writeSession(session);
    assert(written);
    assert(journal.hasPendingSession());

    auto recovered = journal.readSession();
    assert(recovered.has_value());
    assert(recovered->active == true);
    assert(recovered->gamePid == 9999);
    assert(recovered->gameName == "Doom Eternal");
    assert(recovered->modifiedProcesses.size() == 1);
    assert(recovered->modifiedProcesses[0].name == "discord.exe");

    journal.clearSession();
    assert(!journal.hasPendingSession());

    if (fs::exists(tempJournal)) fs::remove(tempJournal);
    std::cout << "[PASS] RollbackJournal tests succeeded." << std::endl;
}

void testSystemMonitor() {
    std::cout << "[TEST] Running SystemMonitor metrics test..." << std::endl;
    optimizer::SystemMonitor mon;
    auto m = mon.sampleMetrics();
    assert(m.totalRamBytes > 0);
    assert(m.usedRamBytes <= m.totalRamBytes);
    std::cout << "  Sampled RAM: " << (m.usedRamBytes / (1024 * 1024)) << " MB / "
              << (m.totalRamBytes / (1024 * 1024)) << " MB (" << m.ramUsagePercent << "%)" << std::endl;
    std::cout << "  Sampled CPU: " << m.cpuUsagePercent << "%" << std::endl;
    std::cout << "[PASS] SystemMonitor tests succeeded." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Game Optimizer Automated Unit Tests   " << std::endl;
    std::cout << "========================================" << std::endl;

    testJson();
    testPriorityConversion();
    testProfileManager();
    testRollbackJournal();
    testSystemMonitor();

    std::cout << "\nALL UNIT TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
