#pragma once
#include "Main/Common.h"

namespace McRave::Pathing {

    BWAPI::Position getPathPoint(UnitInfo &, BWAPI::Position);
    BWAPI::Position getNavPoint(UnitInfo &, BWEB::Path&);

    void onFrame();
}; // namespace McRave::Pathing