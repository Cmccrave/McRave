#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "Singleton.h"
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;

class ResourceTrackerClass
{
	map <Unit, ResourceInfo> myMinerals;
	map <Unit, ResourceInfo> myGas;
	map <Unit, ResourceInfo> myBoulders;
	bool saturated;
public:

	map <Unit, ResourceInfo>& getMyMinerals() { return myMinerals; }
	map <Unit, ResourceInfo>& getMyGas() { return myGas; }
	map <Unit, ResourceInfo>& getMyBoulders() { return myBoulders; }

	bool isSaturated() { return saturated; }

	void storeMineral(Unit);
	void storeGas(Unit);
	void storeBoulder(Unit);
	void removeResource(Unit);

	void update();
};

typedef Singleton<ResourceTrackerClass> ResourceTracker;