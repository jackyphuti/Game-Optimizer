#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <fstream>
#include <deque>

namespace optimizer {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3
};

struct LogEntry {
    std::string timestamp;
    LogLevel level;
    std::string message;
};

class Logger {
public:
    static Logger& instance();

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    void setLogFile(const std::string& filePath);
    void log(LogLevel level, const std::string& message);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    std::vector<LogEntry> getRecentLogs(size_t maxCount = 100);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string formatCurrentTime();
    std::string levelToString(LogLevel level);

    LogLevel m_minLevel{LogLevel::Info};
    std::ofstream m_fileStream;
    std::mutex m_mutex;
    std::deque<LogEntry> m_history;
    static constexpr size_t MAX_HISTORY = 500;
};

// Convenience macros / helpers
#define LOG_DEBUG(msg) ::optimizer::Logger::instance().debug(msg)
#define LOG_INFO(msg)  ::optimizer::Logger::instance().info(msg)
#define LOG_WARN(msg)  ::optimizer::Logger::instance().warn(msg)
#define LOG_ERROR(msg) ::optimizer::Logger::instance().error(msg)

} // namespace optimizer
