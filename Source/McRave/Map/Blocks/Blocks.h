#pragma once
#include "BWEB.h"
#include "Main/Common.h"

namespace McRave::Blocks {
    int needGroundDefenses(BWEB::Block *);
    int needAirDefenses(BWEB::Block *);
    int getGroundDefenseCount(BWEB::Block *);
    int getAirDefenseCount(BWEB::Block *);
} // namespace McRave::Blocks