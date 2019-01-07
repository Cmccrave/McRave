#pragma once
#include <BWAPI.h>

namespace McRave::Units {

    BWAPI::Position getArmyCenter();
    std::set<BWAPI::Unit>& getSplashTargets();
    std::map<BWAPI::Unit, UnitInfo>& getMyUnits();
    std::map<BWAPI::Unit, UnitInfo>& getEnemyUnits();
    std::map<BWAPI::Unit, UnitInfo>& getNeutralUnits();
    std::map<BWAPI::UnitSizeType, int>& getAllySizes();
    std::map<BWAPI::UnitSizeType, int>& getEnemySizes();
    std::map<BWAPI::UnitType, int>& getEnemyTypes();
    double getImmThreat();
    double getProxThreat();
    double getGlobalAllyGroundStrength();
    double getGlobalEnemyGroundStrength();
    double getGlobalAllyAirStrength();
    double getGlobalEnemyAirStrength();
    double getAllyDefense();
    int getSupply();
    int getMyRoleCount(Role);
    int getMyVisible(BWAPI::UnitType);
    int getMyComplete(BWAPI::UnitType);
    int getEnemyCount(BWAPI::UnitType);

    void onFrame();
    void storeUnit(BWAPI::Unit);
    void removeUnit(BWAPI::Unit);
    void morphUnit(BWAPI::Unit);
}