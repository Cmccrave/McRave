#pragma once
#include <BWAPI.h>
#include <set>

namespace McRave::Units {

    BWAPI::Position getArmyCenter();
    std::set<BWAPI::Unit>& getSplashTargets();
    std::map<BWAPI::Unit, UnitInfo>& getMyUnits();
    std::map<BWAPI::Unit, UnitInfo>& getEnemyUnits();
    std::map<BWAPI::Unit, UnitInfo>& getNeutralUnits();
    std::map<BWAPI::UnitSizeType, int>& getAllySizes();
    std::map<BWAPI::UnitSizeType, int>& getEnemySizes();
    std::map<BWAPI::UnitType, int>& getenemyTypes();
    double getImmThreat();
    double getProxThreat();
    double getGlobalAllyGroundStrength();
    double getGlobalEnemyGroundStrength();
    double getGlobalAllyAirStrength();
    double getGlobalEnemyAirStrength();
    double getAllyDefense();
    int getSupply();
    int getMyRoleCount(Role role);
    int getMyTypeCount(BWAPI::UnitType type);
    bool isThreatening(UnitInfo&);
    int getEnemyCount(BWAPI::UnitType);

    void onFrame();
    void storeUnit(BWAPI::Unit);
    void removeUnit(BWAPI::Unit);
}