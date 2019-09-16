#pragma once
#include <BWAPI.h>

namespace McRave::Command {

    bool misc(UnitInfo& unit);
    bool move(UnitInfo& unit);
    bool approach(UnitInfo& unit);
    bool defend(UnitInfo& unit);
    bool kite(UnitInfo& unit);
    bool attack(UnitInfo& unit);
    bool hunt(UnitInfo& unit);
    bool escort(UnitInfo& unit);
    bool special(UnitInfo& unit);
    bool retreat(UnitInfo& unit);
    bool transport(UnitInfo& unit);
}