#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{	
	class ScoutManager
	{
		void updateScouts();

		void scout(UnitInfo&);

		set<Position> scoutAssignments;
		int scoutCount;
		bool proxyCheck = false;
	public:
		void onFrame();	
	};
}

typedef Singleton<McRave::ScoutManager> ScoutSingleton;