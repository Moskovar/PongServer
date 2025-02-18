#include "uti.h"

namespace uti 
{
    uint64_t getCurrentTimestamp()
    {
        return static_cast<uint64_t>(std::time(nullptr));
    }
    long long getCurrentTimestampMs()
    {
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        return now_ms.time_since_epoch().count();
    }
}