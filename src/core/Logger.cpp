#include "optimizer/Logger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace optimizer {

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

Logger::Logger() {
#if defined(_WIN32)
    // Enable ANSI escape sequences in Windows console if possible
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

Logger::~Logger() {
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return m_minLevel;
}

void Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(filePath, std::ios::out | std::ios::app);
}

std::string Logger::formatCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream ss;
    ss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        default: return "INFO ";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_minLevel) return;

    std::string timeStr = formatCurrentTime();
    std::string lvlStr = levelToString(level);

    std::lock_guard<std::mutex> lock(m_mutex);

    // Save to in-memory history
    m_history.push_back({timeStr, level, message});
    if (m_history.size() > MAX_HISTORY) {
        m_history.pop_front();
    }

    // Format ANSI color
    const char* colorCode = "";
    const char* resetCode = "\033[0m";
    switch (level) {
        case LogLevel::Debug: colorCode = "\033[90m"; break; // Gray
        case LogLevel::Info:  colorCode = "\033[32m"; break; // Green
        case LogLevel::Warn:  colorCode = "\033[33m"; break; // Yellow
        case LogLevel::Error: colorCode = "\033[31m"; break; // Red
    }

    std::string formattedConsole = std::string(colorCode) + "[" + timeStr + "] [" + lvlStr + "] " + message + resetCode;
    std::string formattedFile = "[" + timeStr + "] [" + lvlStr + "] " + message;

    if (level == LogLevel::Error) {
        std::cerr << formattedConsole << std::endl;
    } else {
        std::cout << formattedConsole << std::endl;
    }

    if (m_fileStream.is_open()) {
        m_fileStream << formattedFile << std::endl;
    }
}

void Logger::debug(const std::string& message) { log(LogLevel::Debug, message); }
void Logger::info(const std::string& message)  { log(LogLevel::Info, message); }
void Logger::warn(const std::string& message)  { log(LogLevel::Warn, message); }
void Logger::error(const std::string& message) { log(LogLevel::Error, message); }

std::vector<LogEntry> Logger::getRecentLogs(size_t maxCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LogEntry> result;
    size_t count = std::min(maxCount, m_history.size());
    auto startIt = m_history.end() - count;
    result.assign(startIt, m_history.end());
    return result;
}

} // namespace optimizer
