#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class StationTrackerClass
	{		
		vector <const BWEB::Station> myStations;
		vector <const BWEB::Station> enemyStations;
	public:
		vector <const BWEB::Station>& getMyStations() {	return myStations; };
		vector <const BWEB::Station>& getEnemyStations() { return enemyStations; }

		Position getClosestEnemyStation(Position);
		void update();
		void updateStations();
		void storeStation(Unit);
		void removeStation(Unit);
	};
}

typedef Singleton<McRave::StationTrackerClass> StationTracker;