#pragma once

#include "optimizer/Types.hpp"
#include <string>

namespace optimizer {

class SystemMonitor {
public:
    SystemMonitor();

    SystemMetrics sampleMetrics();

private:
    double calculateCpuUsage();

#if defined(_WIN32)
    uint64_t m_prevIdleTime{0};
    uint64_t m_prevKernelTime{0};
    uint64_t m_prevUserTime{0};
#else
    uint64_t m_prevTotalTime{0};
    uint64_t m_prevIdleTime{0};
#endif
};

} // namespace optimizer
