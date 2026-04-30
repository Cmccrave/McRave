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

namespace McRave::Walls::Terran {

    // TvP
    int TvP_Opener(BWEB::Wall &wall) { return 0; }

    int TvP_Transition(BWEB::Wall &wall) { return 0; }

    int TvP_Defenses(BWEB::Wall &wall) { return 1; }

    // TvT
    int TvT_Opener(BWEB::Wall &wall) { return 0; }

    int TvT_Transition(BWEB::Wall &wall) { return 0; }

    int TvT_Defenses(BWEB::Wall &wall) { return 0; }

    // TvZ
    int TvZ_Opener(BWEB::Wall &wall) { return 0; }

    int TvZ_Transition(BWEB::Wall &wall) { return 0; }

    int TvZ_Defenses(BWEB::Wall &wall)
    {
        if (Spy::getEnemyTransition() == Z_1HatchLurker || Spy::getEnemyTransition() == Z_2HatchLurker)
            return 2;
        return 0;
    }
} // namespace McRave::Walls::Terran
