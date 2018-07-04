#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class StationManager
	{
		map <Unit, const BWEB::Station> myStations, enemyStations;
		map <TilePosition, int> lastVisible;
		void updateStations();
	public:
		map <Unit, const BWEB::Station>& getMyStations() { return myStations; };
		map <Unit, const BWEB::Station>& getEnemyStations() { return enemyStations; }

		map <TilePosition, int>& getLastVisible() { return lastVisible; }
		Position getClosestEnemyStation(Position);
		void onFrame();
		void storeStation(Unit);
		void removeStation(Unit);
	};
}

typedef Singleton<McRave::StationManager> StationSingleton;
