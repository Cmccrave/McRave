#pragma once
#include <BWAPI.h>
#include "Main/Common.h"
#include "Time.h"

namespace McRave {
    inline BWAPI::GameWrapper &operator<<(BWAPI::GameWrapper &bw, const Time &t)
    {
        bw << t.minutes << ":";
        t.seconds < 10 ? bw << "0" << t.seconds : bw << t.seconds;
        bw << std::endl;
        return bw;
    }
} // namespace McRave