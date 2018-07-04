#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitInfo;
	class InterfaceManager
	{
		chrono::steady_clock::time_point start;
		int screenOffset = 0;
		map <string, double> myTest;

		map<int, Unit> dings;

		int color, textColor;

		bool targets = false;
		bool builds = true;
		bool bweb = false;
		bool sim = true;
		bool paths = true;


		void drawAllyInfo(), drawEnemyInfo(), drawInformation();	
		void dingEffect();
	public:
		void onFrame(), startClock(), performanceTest(string), onSendText(string);
		void storeDing(Unit unit) { dings.insert(make_pair(Broodwar->getFrameCount(), unit)); }
		void displayPath(UnitInfo&, vector<TilePosition>);
		void displaySim(UnitInfo&, double);
	};
}

typedef Singleton<McRave::InterfaceManager> InterfaceSingleton;