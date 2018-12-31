#pragma once
#include <BWAPI.h>
#include "UnitInfo.h"
#include <set>

namespace McRave::Units
{
    Position getArmyCenter();
    set<Unit>& getSplashTargets();
    map<Unit, UnitInfo>& getMyUnits();
    map<Unit, UnitInfo>& getEnemyUnits();
    map<Unit, UnitInfo>& getNeutralUnits();
    map<UnitSizeType, int>& getAllySizes();
    map<UnitSizeType, int>& getEnemySizes();
    map<UnitType, int>& getenemyTypes();
    double getImmThreat();
    double getProxThreat();
    double getGlobalAllyGroundStrength();
    double getGlobalEnemyGroundStrength();
    double getGlobalAllyAirStrength();
    double getGlobalEnemyAirStrength();
    double getAllyDefense();
    int getSupply();
    int getMyRoleCount(Role role);
    int getMyTypeCount(UnitType type);
    bool isThreatening(UnitInfo&);
    int getEnemyCount(UnitType);

    void onFrame();
    void storeUnit(Unit);
    void removeUnit(Unit);
}