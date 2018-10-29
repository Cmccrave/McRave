#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class StationManager
	{
		map <Unit, const BWEB::Station *> myStations, enemyStations;
		void updateStations();
	public:
		map <Unit, const BWEB::Station *>& getMyStations() { return myStations; };
		map <Unit, const BWEB::Station *>& getEnemyStations() { return enemyStations; }
		Position getClosestEnemyStation(Position);
		void onFrame();
		void storeStation(Unit);
		void removeStation(Unit);
		bool needDefenses(const Station);
	};
}

typedef Singleton<McRave::StationManager> StationSingleton;
