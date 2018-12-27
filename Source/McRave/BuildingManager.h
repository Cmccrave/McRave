#pragma once
#include "McRave.h"

namespace McRave::Buildings
{
    void onFrame();
    bool isBuildable(BWAPI::UnitType, BWAPI::TilePosition);
    bool isQueueable(BWAPI::UnitType, BWAPI::TilePosition);
    bool overlapsQueuedBuilding(BWAPI::UnitType, BWAPI::TilePosition);
    bool hasPoweredPositions();
    int getQueuedMineral();
    int getQueuedGas();
    int getNukesAvailable();
    BWAPI::TilePosition getCurrentExpansion();
};