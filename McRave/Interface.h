#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class InterfaceTrackerClass
	{
		chrono::steady_clock::time_point start;
		int screenOffset = 0;
		map <string, double> myTest;
		bool debugging = false;
		void drawAllyInfo(), drawEnemyInfo(), drawInformation();
	public:
		void update(), startClock(), performanceTest(string), sendText(string);
	};
}

typedef Singleton<McRave::InterfaceTrackerClass> InterfaceTracker;