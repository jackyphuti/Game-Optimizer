#pragma once

#include <cstdint>

namespace optimizer {

class IRamOptimizer {
public:
    virtual ~IRamOptimizer() = default;

    // Trims working sets of non-essential applications
    virtual bool trimWorkingSets() = 0;

    // Windows: purges standby memory list via NtSetSystemInformation
    virtual bool purgeStandbyMemory() = 0;

    // Linux: Drops pagecache, dentries, and inodes via /proc/sys/vm/drop_caches (strictly opt-in)
    virtual bool dropCaches(bool force = false) = 0;

    // Compacts system memory to reduce fragmentation
    virtual bool compactMemory() = 0;
};

} // namespace optimizer
