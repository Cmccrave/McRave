#pragma once
#include "McRave.h"

namespace McRave::Buildings {

    void onFrame();
    void onStart();

    bool overlapsPlan(UnitInfo&, BWAPI::Position);
    bool overlapsUnit(UnitInfo&, BWAPI::TilePosition, BWAPI::UnitType);
    bool hasPoweredPositions();
    int getQueuedMineral();
    int getQueuedGas();
    BWEB::Station * getCurrentExpansion();
    BWEB::Station * getLastExpansion();
    bool expansionBlockersExists();
};