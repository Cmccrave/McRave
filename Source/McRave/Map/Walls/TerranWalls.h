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

    // TvP Ground
    int TvP_Opener(BWEB::Wall &wall) { return 0; }

    int TvP_Transition(BWEB::Wall &wall) { return 0; }

    int TvP_GroundDefenses(BWEB::Wall &wall) { return 1; }

    // TvP Air
    int TvP_AirDefenses(BWEB::Wall &wall) { return 0; }

    // TvT Ground
    int TvT_Opener(BWEB::Wall &wall) { return 0; }

    int TvT_Transition(BWEB::Wall &wall) { return 0; }

    int TvT_GroundDefenses(BWEB::Wall &wall) { return 1; }

    // TvT Air
    int TvT_AirDefenses(BWEB::Wall &wall) { return 0; }

    // TvZ Ground
    int TvZ_Opener(BWEB::Wall &wall) { return 0; }

    int TvZ_Transition(BWEB::Wall &wall) { return 0; }

    int TvZ_GroundDefenses(BWEB::Wall &wall)
    {
        if (Spy::getEnemyTransition() == Z_1HatchLurker || Spy::getEnemyTransition() == Z_2HatchLurker || Players::getTotalCount(PlayerState::Enemy, Zerg_Lurker) > 0)
            return 2;
        if (Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 6 || Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) >= 2)
            return 1;
        return 0;
    }

    // TvZ Air
    int TvZ_AirDefenses(BWEB::Wall &wall)
    {
        if (Spy::getEnemyTransition() == Z_1HatchLurker || Spy::getEnemyTransition() == Z_2HatchLurker || Players::getTotalCount(PlayerState::Enemy, Zerg_Lurker) > 0)
            return 1;
        return 0;
    }

    // TvFFA Ground
    int TvFFA_GroundDefenses(BWEB::Wall &wall) { return 1; }

    // TvFFA Air
    int TvFFA_AirDefenses(BWEB::Wall &wall) { return 0; }

} // namespace McRave::Walls::Terran
