#pragma once

#include <cstdint>

struct WCETStats {
    uint64_t count{0};
    uint64_t totalNs{0};
    uint64_t maxNs{0};
    uint64_t minNs{UINT64_MAX};

    void update(uint64_t v) {
        ++count;
        totalNs += v;
        if (v > maxNs) maxNs = v;
        if (v < minNs) minNs = v;
    }
};
