#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class ProductionTrackerClass
{
	map <Unit, UnitType> idleProduction;
	map <Unit, TechType> idleTech;
	map <Unit, UpgradeType> idleUpgrade;
	int reservedMineral, reservedGas;	
	bool gateSat = false;
	bool partiallySat = false;
	bool barracksSat = false;
	bool hatchSat = false;
	bool productionSat = false;
public:
	map <Unit, UnitType>& getIdleProduction() { return idleProduction; }
	map <Unit, TechType>& getIdleTech() { return idleTech; }
	map <Unit, UpgradeType>& getIdleUpgrade() { return idleUpgrade; }

	int getReservedMineral() { return reservedMineral; }
	int getReservedGas() { return reservedGas; }
	bool isGateSat() { return gateSat; }
	bool isBarracksSat() { return barracksSat; }
	bool isProductionSat() { return productionSat; }

	void update();
	void updateProduction();
	void updateReservedResources();

	bool isAffordable(UnitType);
	bool isAffordable(UpgradeType);
	bool isAffordable(TechType);
	bool isCreateable(Unit, UnitType);
	bool isCreateable(UpgradeType);
	bool isCreateable(TechType);
	bool isSuitable(UnitType);
	bool isSuitable(UpgradeType);
	bool isSuitable(TechType);
};

typedef Singleton<ProductionTrackerClass> ProductionTracker;