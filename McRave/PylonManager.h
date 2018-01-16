#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

// Reference: https://docs.google.com/spreadsheets/d/188VI8uHoV8xzoQusGg7hDO5JeFLGO1i2b_5P3jc_-88/edit#gid=1858976685
// Note that small and medium buildings have identical requirements

namespace McRave
{
	class PylonTrackerClass
	{
		map<TilePosition, int> smallMediumLocations, largeLocations;
		set<Unit> myPylons;
		void updatePower(Unit);
	public:
		void storePylon(Unit);		
		bool hasPower(TilePosition, UnitType);
	};
}

typedef Singleton<McRave::PylonTrackerClass> PylonTracker;