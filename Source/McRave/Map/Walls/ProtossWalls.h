#pragma once
#include "BWEB.h"
#include "Builds/All/BuildOrder.h"
#include "Builds/All/Learning.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls::Protoss {

    // PvP
    int PvP_Defenses(BWEB::Wall &wall)
    {
        if (BuildOrder::getCurrentTransition() == P_DT)
            return 3;
        return 0;
    }

    // PvT
    int PvT_Defenses(BWEB::Wall &wall) { return 0; }

    // PvZ
    int PvZ_Opener(BWEB::Wall &wall)
    {
        if (Spy::getEnemyOpener() == Z_4Pool)
            return 2 + (Players::getSupply(PlayerState::Self, Races::Protoss) >= 24);
        if (Spy::getEnemyOpener() == Z_9Pool || Spy::getEnemyOpener() == Z_Overpool)
            return 2;
        return total(Protoss_Gateway) > 0;
    }

    int PvZ_Transition(BWEB::Wall &wall)
    {
        auto cannonCount = 1 + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 6) + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 12) +
                           (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 24) + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

        if (Spy::getEnemyTransition() == Z_2HatchHydra && Util::getTime() > Time(4, 00)) {
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) > 0)
                return 5;
            return 2;
        }
        else if (Spy::getEnemyTransition() == Z_3HatchHydra && Util::getTime() > Time(5, 00)) {
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) > 0)
                return 4;
            return 2;
        }
        else if (Spy::getEnemyTransition() == Z_2HatchMuta && Util::getTime() > Time(5, 30))
            return 2;
        else if (Spy::getEnemyTransition() == Z_3HatchMuta && Util::getTime() > Time(6, 00))
            return 2;
        return cannonCount;
    }

    int PvZ_Defenses(BWEB::Wall &wall)
    {
        // Determine how much we have traded
        auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Zerg_Hydralisk) / 2 + Players::getDeadCount(PlayerState::Enemy, Zerg_Zergling) / 4;

        auto minimum   = 0;
        auto expected  = max(PvZ_Opener(wall), PvZ_Transition(wall));
        auto reduction = max(0, unitsKilled / 8);

        if (expected > 0)
            minimum = 1;

        return max(minimum, expected - reduction);
    }

    // PvFFA
    int PvFFA_Defenses(BWEB::Wall &wall) { return 1 + (Util::getTime() > Time(5, 20)) + (Util::getTime() > Time(5, 40)); }
} // namespace McRave::Walls::Protoss
