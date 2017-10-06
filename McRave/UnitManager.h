#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "UnitInfo.h"

using namespace BWAPI;
using namespace std;

class UnitTrackerClass
{
	map <Unit, UnitInfo> enemyUnits;
	map <Unit, UnitInfo> enemyDefenses;
	map <Unit, UnitInfo> allyUnits;
	map <Unit, UnitInfo> allyDefenses;
	map <UnitSizeType, int> allySizes;
	map <UnitSizeType, int> enemySizes;
	map <UnitType, int> enemyComposition;

	double globalAllyStrength, globalEnemyStrength;
	double allyDefense, enemyDefense;
	double groundLatch, airLatch;
	int supply, globalStrategy;
public:

	map<Unit, UnitInfo>& getAllyUnits() { return allyUnits; }
	map<Unit, UnitInfo>& getEnemyUnits() { return enemyUnits; }
	map<UnitSizeType, int>& getAllySizes() { return allySizes; }
	map<UnitSizeType, int>& getEnemySizes() { return enemySizes; }
	map<UnitType, int>& getEnemyComposition() { return enemyComposition; }

	map<Unit, UnitInfo>& getAllyUnitsFilter(UnitType);

	UnitInfo& getAllyUnit(Unit);
	UnitInfo& getEnemyUnit(Unit);

	double getGlobalAllyStrength() { return globalAllyStrength; }
	double getGlobalEnemyStrength() { return globalEnemyStrength; }
	double getAllyDefense() { return allyDefense; }
	double getEnemyDefense() { return enemyDefense; }
	int getGlobalStrategy() { return globalStrategy; }
	int getSupply() { return supply; }

	// Updating
	void update();
	void updateAliveUnits();
	void updateDeadUnits();
	void updateEnemy(UnitInfo&);
	void updateAlly(UnitInfo&);	
	void getLocalCalculation(UnitInfo&);
	void updateGlobalCalculations();

	// Storage
	void onUnitDiscover(Unit);
	void onUnitCreate(Unit);
	void onUnitDestroy(Unit);
	void onUnitMorph(Unit);
	void onUnitComplete(Unit);
	void storeAlly(Unit);
	void storeEnemy(Unit);

	// Manual increase of global strength (worker pulling)
	void increaseGlobalAlly(int increase) { globalAllyStrength += increase; }
	void increaseGlobalEnemy(int increase) { globalEnemyStrength += increase; }
};

typedef Singleton<UnitTrackerClass> UnitTracker;