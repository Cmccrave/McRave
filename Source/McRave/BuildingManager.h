#pragma once
#include "McRave.h"

namespace McRave::Buildings {

    void onFrame();
    void onStart();

    bool isBuildable(BWAPI::UnitType, BWAPI::TilePosition);
    bool isQueueable(BWAPI::UnitType, BWAPI::TilePosition);
    bool overlapsQueue(UnitInfo&, BWAPI::Position);
    bool overlapsUnit(UnitInfo&, BWAPI::TilePosition, BWAPI::UnitType);
    bool hasPoweredPositions();
    int getQueuedMineral();
    int getQueuedGas();
    BWAPI::TilePosition getCurrentExpansion();
};