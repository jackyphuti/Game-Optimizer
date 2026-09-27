#if !defined(_WIN32)

#include "LinuxRamOptimizer.hpp"
#include "optimizer/Logger.hpp"

#include <unistd.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace optimizer {

LinuxRamOptimizer::LinuxRamOptimizer() = default;

bool LinuxRamOptimizer::compactMemory() {
    if (fs::exists("/proc/sys/vm/compact_memory")) {
        std::ofstream ofs("/proc/sys/vm/compact_memory");
        if (ofs.is_open()) {
            ofs << "1";
            ofs.close();
            LOG_INFO("[LinuxRamOptimizer] Triggered Linux memory compaction.");
            return true;
        }
    }
    return false;
}

bool LinuxRamOptimizer::trimWorkingSets() {
    // Memory compaction defragments RAM for large game allocations
    return compactMemory();
}

bool LinuxRamOptimizer::purgeStandbyMemory() {
    // Standby memory list is a Windows-specific NT memory manager concept
    return false;
}

bool LinuxRamOptimizer::dropCaches(bool force) {
    if (!force) {
        LOG_INFO("[LinuxRamOptimizer] drop_caches skipped. (Destructive to disk cache; requires explicit opt-in in game profile)");
        return false;
    }

    LOG_WARN("[LinuxRamOptimizer] Flushing file buffers (sync) before dropping caches...");
    sync();

    if (fs::exists("/proc/sys/vm/drop_caches")) {
        std::ofstream ofs("/proc/sys/vm/drop_caches");
        if (ofs.is_open()) {
            ofs << "3\n"; // Drop pagecache, dentries, and inodes
            ofs.close();
            LOG_INFO("[LinuxRamOptimizer] Caches dropped successfully.");
            return true;
        } else {
            LOG_WARN("[LinuxRamOptimizer] Permission denied writing to /proc/sys/vm/drop_caches (requires root/sudo).");
            return false;
        }
    }
    return false;
}

} // namespace optimizer

#endif // !_WIN32
