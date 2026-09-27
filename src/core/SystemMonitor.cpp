#include "optimizer/SystemMonitor.hpp"
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

namespace optimizer {

SystemMonitor::SystemMonitor() {
    // Initial prime sample
    calculateCpuUsage();
}

double SystemMonitor::calculateCpuUsage() {
#if defined(_WIN32)
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0;
    }

    auto toUInt64 = [](const FILETIME& ft) -> uint64_t {
        return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    uint64_t idle = toUInt64(idleTime);
    uint64_t kernel = toUInt64(kernelTime);
    uint64_t user = toUInt64(userTime);

    uint64_t usrDiff = user - m_prevUserTime;
    uint64_t kerDiff = kernel - m_prevKernelTime;
    uint64_t idlDiff = idle - m_prevIdleTime;

    uint64_t totalSys = usrDiff + kerDiff;

    m_prevIdleTime = idle;
    m_prevKernelTime = kernel;
    m_prevUserTime = user;

    if (totalSys == 0) return 0.0;
    double cpuUsage = (static_cast<double>(totalSys - idlDiff) * 100.0) / totalSys;
    if (cpuUsage < 0.0) cpuUsage = 0.0;
    if (cpuUsage > 100.0) cpuUsage = 100.0;
    return cpuUsage;
#else
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) return 0.0;

    std::string line;
    if (!std::getline(statFile, line)) return 0.0;

    std::istringstream ss(line);
    std::string cpuLabel;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    uint64_t totalIdle = idle + iowait;
    uint64_t totalNonIdle = user + nice + system + irq + softirq + steal;
    uint64_t total = totalIdle + totalNonIdle;

    uint64_t totalDiff = total - m_prevTotalTime;
    uint64_t idleDiff = totalIdle - m_prevIdleTime;

    m_prevTotalTime = total;
    m_prevIdleTime = totalIdle;

    if (totalDiff == 0) return 0.0;
    double cpuUsage = (static_cast<double>(totalDiff - idleDiff) * 100.0) / totalDiff;
    if (cpuUsage < 0.0) cpuUsage = 0.0;
    if (cpuUsage > 100.0) cpuUsage = 100.0;
    return cpuUsage;
#endif
}

SystemMetrics SystemMonitor::sampleMetrics() {
    SystemMetrics m;
    m.cpuUsagePercent = calculateCpuUsage();

#if defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m.totalRamBytes = memInfo.ullTotalPhys;
        m.availableRamBytes = memInfo.ullAvailPhys;
        m.usedRamBytes = m.totalRamBytes - m.availableRamBytes;
        m.ramUsagePercent = static_cast<double>(memInfo.dwMemoryLoad);
    }
#else
    struct sysinfo si{};
    if (sysinfo(&si) == 0) {
        m.totalRamBytes = static_cast<uint64_t>(si.totalram) * si.mem_unit;
        m.availableRamBytes = static_cast<uint64_t>(si.freeram + si.bufferram) * si.mem_unit;
        m.usedRamBytes = m.totalRamBytes - m.availableRamBytes;
        if (m.totalRamBytes > 0) {
            m.ramUsagePercent = (static_cast<double>(m.usedRamBytes) * 100.0) / m.totalRamBytes;
        }
    }
#endif

    return m;
}

} // namespace optimizer
