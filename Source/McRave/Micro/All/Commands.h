#pragma once
#include "Main/Common.h"

namespace McRave::Command {

    bool misc(UnitInfo&);
    bool move(UnitInfo&);
    bool approach(UnitInfo&);
    bool defend(UnitInfo&);
    bool kite(UnitInfo&);
    bool attack(UnitInfo&);
    bool poke(UnitInfo&);
    bool explore(UnitInfo&);
    bool escort(UnitInfo&);
    bool special(UnitInfo&);
    bool retreat(UnitInfo&);
    bool transport(UnitInfo&);

    bool click(UnitInfo&);
    bool siege(UnitInfo&);
    bool repair(UnitInfo&);
    bool burrow(UnitInfo&);
    bool castSelf(UnitInfo&);
    bool castTarget(UnitInfo&);
    bool morph(UnitInfo&);
    bool train(UnitInfo&);
    bool returnResource(UnitInfo&);
    bool clearNeutral(UnitInfo&);
    bool build(UnitInfo&);
    bool gather(UnitInfo&);
}