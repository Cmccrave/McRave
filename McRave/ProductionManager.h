#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class ProductionTrackerClass
	{
		map <Unit, UnitType> idleProduction;
		map <Unit, TechType> idleTech;
		map <Unit, UpgradeType> idleUpgrade;
		int reservedMineral, reservedGas;
		bool idle = false;

		bool isAffordable(UnitType), isAffordable(UpgradeType), isAffordable(TechType);
		bool isCreateable(Unit, UnitType), isCreateable(UpgradeType), isCreateable(TechType);
		bool isSuitable(UnitType), isSuitable(UpgradeType), isSuitable(TechType);
		void updateProduction(), updateReservedResources();
	public:
		map <Unit, UnitType>& getIdleProduction() { return idleProduction; }
		map <Unit, TechType>& getIdleTech() { return idleTech; }
		map <Unit, UpgradeType>& getIdleUpgrade() { return idleUpgrade; }

		int getReservedMineral() { return reservedMineral; }
		int getReservedGas() { return reservedGas; }
		bool hasIdleProduction() { return idle; }

		void update();
	};
}

typedef Singleton<McRave::ProductionTrackerClass> ProductionTracker;