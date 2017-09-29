#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "BaseInfo.h"

using namespace BWAPI;
using namespace std;

class BaseTrackerClass
{	
	map <Unit, BaseInfo> myBases;
	map <double, TilePosition> myOrderedBases;
public:
	map <Unit, BaseInfo>& getMyBases() { return myBases; }
	map <double, TilePosition>& getMyOrderedBases() { return myOrderedBases; }

	void update();
	void updateAlliedBases();
	//void updateEnemyBases();
	void storeBase(Unit);
	void removeBase(Unit);
	void updateProduction(BaseInfo&);
};

typedef Singleton<BaseTrackerClass> BaseTracker;