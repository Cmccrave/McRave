#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "BaseInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class BaseTrackerClass
	{
		map <Unit, BaseInfo> myBases;
		map <Unit, BaseInfo> enemyBases;
		map <double, TilePosition> myOrderedBases;

		vector <const BWEB::Station> myStations;
		vector <const BWEB::Station> enemyStations;

	public:
		map <Unit, BaseInfo>& getMyBases() { return myBases; }
		map <Unit, BaseInfo>& getEnemyBases() { return enemyBases; }
		map <double, TilePosition>& getMyOrderedBases() { return myOrderedBases; }
		Position getClosestEnemyBase(Position);

		void update();
		void updateBases();
		void storeBase(Unit);
		void removeBase(Unit);
		void updateProduction(BaseInfo&);
	};
}

typedef Singleton<McRave::BaseTrackerClass> BaseTracker;