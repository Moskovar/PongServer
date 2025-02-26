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

    uint32_t getElapsedTimeMs(uint32_t start_time)
	{
		// Obtenir le temps actuel en millisecondes depuis l'époque UNIX
		uint32_t current_time = static_cast<uint32_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()
			).count()
			);

		// Calculer le temps écoulé depuis start_time
		if (current_time >= start_time) {
			return current_time - start_time;
		}
		else {
			// Si current_time est inférieur à start_time (cas improbable mais possible si il y a un dépassement)
			return 0;
		}
	}
}