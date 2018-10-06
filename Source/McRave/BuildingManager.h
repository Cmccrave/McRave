#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "BuildingInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class BuildingManager
	{
		int queuedMineral, queuedGas, nukesAvailable;
		int poweredSmall, poweredMedium, poweredLarge;
		int lairsMorphing, hivesMorphing;

		map <TilePosition, UnitType> buildingsQueued;
		map <Unit, BuildingInfo> myBuildings;
		TilePosition currentExpansion;

		void updateBuildings(), queueBuildings();
		TilePosition findProdLocation(UnitType, Position), findDefLocation(UnitType, Position), findTechLocation(UnitType, Position), findWallLocation(UnitType, Position), findExpoLocation();
		TilePosition getBuildLocation(UnitType);

		void updateCommands(BuildingInfo&);

	public:
		map <Unit, BuildingInfo>& getMyBuildings() { return myBuildings; }
		bool isBuildable(UnitType, TilePosition);
		bool isQueueable(UnitType, TilePosition);
		bool overlapsQueuedBuilding(UnitType, TilePosition);	
		bool hasPoweredPositions() { return (poweredLarge > 0 && poweredMedium > 0); }
		void onFrame();
		void storeBuilding(Unit);
		void removeBuilding(Unit);
		int getQueuedMineral() { return queuedMineral; }
		int getQueuedGas() { return queuedGas; }
		int getNukesAvailable() { return nukesAvailable; }		
		TilePosition getCurrentExpansion() { return currentExpansion; }
	};
}

typedef Singleton<McRave::BuildingManager> BuildingSingleton;