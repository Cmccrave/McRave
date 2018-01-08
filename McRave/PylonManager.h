#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class PylonTrackerClass
	{
		map<TilePosition, int> smallLocations;
		map<TilePosition, int> mediumLocations;
		map<TilePosition, int> largeLocations;
		set<Unit> myPylons;
	public:
		void storePylon(Unit);
		void updatePower(Unit);
		bool hasPower(TilePosition, UnitType);
	};
}

typedef Singleton<McRave::PylonTrackerClass> PylonTracker;