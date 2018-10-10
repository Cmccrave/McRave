#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "UnitInfo.h"

using namespace McRave;
class CombatUnitManager {
	map<Unit, CombatUnit> myCombatUnits;
	map<Unit, CombatUnit> enemyCombatUnits;
	set<Unit> splashTargets;
public:
	void onFrame();
};