#include "uti.h"

namespace uti 
{
    uint64_t getCurrentTimestamp()
    {
        return static_cast<uint64_t>(std::time(nullptr));
    }
}