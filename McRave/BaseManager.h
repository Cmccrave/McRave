#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "BaseInfo.h"

using namespace BWAPI;
using namespace std;

class BaseTrackerClass
{	
	map <Unit, BaseInfo> myBases;
	map <Unit, BaseInfo> enemyBases;
	map <double, TilePosition> myOrderedBases;
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

typedef Singleton<BaseTrackerClass> BaseTracker;