#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class ProductionManager
	{
		map <Unit, UnitType> idleProduction;
		map <Unit, TechType> idleTech;
		map <Unit, UpgradeType> idleUpgrade;
		int reservedMineral, reservedGas;
		int idleFrame = 0;

		bool isAffordable(UnitType), isAffordable(UpgradeType), isAffordable(TechType);
		bool isCreateable(Unit, UnitType), isCreateable(UpgradeType), isCreateable(TechType);
		bool isSuitable(UnitType), isSuitable(UpgradeType), isSuitable(TechType);
		void updateProduction(), updateReservedResources();

		void produce(UnitInfo&), research(UnitInfo&), upgrade(UnitInfo&);

		void MadMix(UnitInfo&);
	public:
		int getReservedMineral() { return reservedMineral; }
		int getReservedGas() { return reservedGas; }
		bool hasIdleProduction() { return Broodwar->getFrameCount() == idleFrame; }

		void onFrame();
	};
}

typedef Singleton<McRave::ProductionManager> ProductionSingleton;