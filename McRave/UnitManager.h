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

	set<Unit> returnValues;

	double globalAllyGroundStrength, globalEnemyGroundStrength;
	double globalAllyAirStrength, globalEnemyAirStrength;
	double allyDefense, enemyDefense;
	int supply, globalGroundStrategy, globalAirStrategy;
public:

	map<Unit, UnitInfo>& getAllyUnits() { return allyUnits; }
	map<Unit, UnitInfo>& getEnemyUnits() { return enemyUnits; }
	map<UnitSizeType, int>& getAllySizes() { return allySizes; }
	map<UnitSizeType, int>& getEnemySizes() { return enemySizes; }
	map<UnitType, int>& getEnemyComposition() { return enemyComposition; }

	set<Unit> getAllyUnitsFilter(UnitType);

	UnitInfo& getAllyUnit(Unit);
	UnitInfo& getEnemyUnit(Unit);

	double getGlobalAllyGroundStrength() { return globalAllyGroundStrength; }
	double getGlobalEnemyGroundStrength() { return globalEnemyGroundStrength; }
	double getGlobalAllyAirStrength() { return globalAllyAirStrength; }
	double getGlobalEnemyAirStrength() { return globalEnemyAirStrength; }
	double getAllyDefense() { return allyDefense; }
	double getEnemyDefense() { return enemyDefense; }
	int getGlobalGroundStrategy() { return globalGroundStrategy; }
	int getGlobalAirStrategy() { return globalAirStrategy; }
	int getSupply() { return supply; }

	bool shouldAttack(UnitInfo&);
	bool shouldDefend(UnitInfo&);
	bool shouldRetreat(UnitInfo&);

	// Updating
	void update();
	void updateUnits();
	void updateEnemy(UnitInfo&);
	void updateAlly(UnitInfo&);
	void updateLocalSimulation(UnitInfo&);
	void updateStrategy(UnitInfo&);
	void updateGlobalSimulation();

	// Storage
	void onUnitDiscover(Unit);
	void onUnitCreate(Unit);
	void onUnitDestroy(Unit);
	void onUnitMorph(Unit);
	void onUnitComplete(Unit);
	void storeAlly(Unit);
	void storeEnemy(Unit);
	void removeUnit(Unit);
};

typedef Singleton<UnitTrackerClass> UnitTracker;