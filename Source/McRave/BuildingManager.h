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
		map <TilePosition, UnitType> buildingsQueued;
		map <Unit, BuildingInfo> myBuildings;
		set<Unit> returnValues;
		TilePosition currentExpansion;

		int poweredSmall, poweredMedium, poweredLarge;

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

		// Returns the minerals that are reserved for queued buildings
		int getQueuedMineral() { return queuedMineral; }

		// Returns the gas that is reserved for queued buildings
		int getQueuedGas() { return queuedGas; }

		// Returns the number of nukes available
		int getNukesAvailable() { return nukesAvailable; }
		
		// Get current expansion tile
		TilePosition getCurrentExpansion() { return currentExpansion; }
	};
}

typedef Singleton<McRave::BuildingManager> BuildingSingleton;