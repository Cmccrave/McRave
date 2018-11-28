#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class StationManager
	{
		map <Unit, const BWEB::Stations::Station *> myStations, enemyStations;
		void updateStations();
		map<const BWEB::Stations::Station *, map<const BWEB::Stations::Station *, BWEB::PathFinding::Path>> stationNetwork;
	public:
		map <Unit, const BWEB::Stations::Station *>& getMyStations() { return myStations; };
		map <Unit, const BWEB::Stations::Station *>& getEnemyStations() { return enemyStations; }
		Position getClosestEnemyStation(Position);
		void onFrame();
		void onStart();
		void storeStation(Unit);
		void removeStation(Unit);
		bool needDefenses(const BWEB::Stations::Station&);
		bool stationNetworkExists(const BWEB::Stations::Station *, const BWEB::Stations::Station *);
		BWEB::PathFinding::Path* pathStationToStation(const BWEB::Stations::Station *, const BWEB::Stations::Station *);
	};
}

typedef Singleton<McRave::StationManager> StationSingleton;
