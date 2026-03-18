#pragma once
#include "Main/Common.h"

namespace McRave::Units {

    std::shared_ptr<UnitInfo> getUnitInfo(BWAPI::Unit);
    std::set<std::shared_ptr<UnitInfo>>& getUnits(PlayerState);
    std::map<BWAPI::UnitSizeType, int>& getAllyGrdSizes();
    std::map<BWAPI::UnitSizeType, int>& getEnemyGrdSizes();
    std::map<BWAPI::UnitSizeType, int>& getAllyAirSizes();
    std::map<BWAPI::UnitSizeType, int>& getEnemyAirSizes();
    BWAPI::Position getEnemyArmyCenter();
    bool enemyThreatening();
    double getDamageRatioGrd(PlayerState, BWAPI::DamageType);
    double getDamageRatioAir(PlayerState, BWAPI::DamageType);
    double getDamageReductionGrd(PlayerState);
    double getDamageReductionAir(PlayerState);

    int getMyVisible(BWAPI::UnitType);
    int getMyComplete(BWAPI::UnitType);

    bool commandAllowed(UnitInfo&);

    bool inBoundUnit(UnitInfo& unit, int seconds = 30);

    void onFrame();
}