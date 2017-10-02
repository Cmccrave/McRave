#pragma once
#include <BWAPI.h>
#include "SpecialUnitInfo.h"

using namespace BWAPI;
using namespace std;

class SpecialUnitTrackerClass
{
public:
	void update();
	void updateArbiters();
	void updateDarchons();
	void updateDefilers();
	void updateDetectors();
	void updateQueens();
	void updateReavers();
	void updateVultures();
};

typedef Singleton<SpecialUnitTrackerClass> SpecialUnitTracker;