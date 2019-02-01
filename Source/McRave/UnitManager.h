#pragma once
#include <BWAPI.h>

namespace McRave::Units {
    
    std::shared_ptr<UnitInfo> getUnit(BWAPI::Unit);
    std::set<std::shared_ptr<UnitInfo>>& getUnits(PlayerState);
    std::set<BWAPI::Unit>& getSplashTargets();
    std::map<BWAPI::UnitSizeType, int>& getAllySizes();
    std::map<BWAPI::UnitSizeType, int>& getEnemySizes();
    std::map<BWAPI::UnitType, int>& getEnemyTypes();
    double getImmThreat();
    double getProxThreat();
    
    int getSupply();
    int getMyRoleCount(Role);
    int getMyVisible(BWAPI::UnitType);
    int getMyComplete(BWAPI::UnitType);
    int getEnemyCount(BWAPI::UnitType);
    int getNumberMelee();
    int getNumberRanged();

    void onFrame();
    void storeUnit(BWAPI::Unit);
    void removeUnit(BWAPI::Unit);
    void morphUnit(BWAPI::Unit);
}