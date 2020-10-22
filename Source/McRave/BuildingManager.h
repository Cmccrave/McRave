#pragma once
#include "McRave.h"

namespace McRave::Buildings {

    void onFrame();
    void onStart();

    bool isBuildable(BWAPI::UnitType, BWAPI::TilePosition);
    bool isPlannable(BWAPI::UnitType, BWAPI::TilePosition);
    bool overlapsPlan(UnitInfo&, BWAPI::Position);
    bool overlapsUnit(UnitInfo&, BWAPI::TilePosition, BWAPI::UnitType);
    bool hasPoweredPositions();
    int getQueuedMineral();
    int getQueuedGas();
    BWAPI::TilePosition getCurrentExpansion();
};