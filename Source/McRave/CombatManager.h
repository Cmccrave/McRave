#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    void onFrame();
    std::multimap<double, BWAPI::Position>& getCombatClusters();
}