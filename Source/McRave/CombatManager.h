#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    void onStart();
    void onFrame();
    std::multimap<double, BWAPI::Position>& getCombatClusters();
    BWAPI::Position getClosestRetreatPosition(BWAPI::Position);
    //BWAPI::Position getClosestAttackPosition(BWAPI::Position);
}