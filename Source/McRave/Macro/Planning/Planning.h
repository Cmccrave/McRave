#pragma once
#include "Main/McRave.h"

namespace McRave::Planning {

    void onFrame();
    void onStart();
    void onUnitDestroy(BWAPI::Unit);
    void onUnitDiscover(BWAPI::Unit);

    bool isDefensiveType(BWAPI::UnitType);
    bool isProductionType(BWAPI::UnitType);
    bool isTechType(BWAPI::UnitType);
    bool isWallType(BWAPI::UnitType);

    BWAPI::UnitType whatPlannedHere(BWAPI::TilePosition);
    bool overlapsPlan(UnitInfo&, BWAPI::Position);

    std::map<BWAPI::TilePosition, BWAPI::UnitType>& getPlannedBuildings();
    int getPlannedMineral();
    int getPlannedGas();
    const BWEB::Station * getCurrentExpansion();
};
