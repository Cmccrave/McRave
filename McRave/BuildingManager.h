#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "BuildingInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class BuildingTrackerClass
	{
		int queuedMineral, queuedGas;
		map <TilePosition, UnitType> buildingsQueued;
		map <Unit, BuildingInfo> myBuildings;
		set<TilePosition> usedTiles;
		set<Unit> returnValues;
		TilePosition currentExpansion;
		UnitType lastBadBuilding;
		int errorTime = 0, buildingOffset = 0;

		void updateBuildings(), queueBuildings(), constructBuildings();
		TilePosition findProdLocation(UnitType, Position), findDefLocation(UnitType, Position);

	public:
		map <Unit, BuildingInfo>& getMyBuildings() { return myBuildings; }
		map <TilePosition, UnitType>& getBuildingsQueued() { return buildingsQueued; }
		TilePosition getBuildLocation(UnitType);

		bool isBuildable(UnitType, TilePosition);
		bool isSuitable(UnitType, TilePosition);
		bool isQueueable(UnitType, TilePosition);

		void update();
		void storeBuilding(Unit);
		void removeBuilding(Unit);

		// Returns the minerals that are reserved for queued buildings
		int getQueuedMineral() { return queuedMineral; }

		// Returns the gas that is reserved for queued buildings
		int getQueuedGas() { return queuedGas; }

		// Returns a set of ally buildings of a certain type
		set<Unit> getAllyBuildingsFilter(UnitType);
		BuildingInfo& getAllyBuilding(Unit);

		// Get current expansion tile
		TilePosition getCurrentExpansion() { return currentExpansion; }


	};
}

typedef Singleton<McRave::BuildingTrackerClass> BuildingTracker;