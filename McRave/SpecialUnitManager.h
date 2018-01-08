#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
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
}

typedef Singleton<McRave::SpecialUnitTrackerClass> SpecialUnitTracker;