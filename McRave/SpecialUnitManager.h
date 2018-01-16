#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class SpecialUnitTrackerClass
	{
		void updateArbiters(), updateDarchons(), updateDefilers(), updateDetectors(), updateQueens(), updateReavers(), updateVultures();
	public:
		void update();
	};
}

typedef Singleton<McRave::SpecialUnitTrackerClass> SpecialUnitTracker;