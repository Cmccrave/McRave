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
		void updateScoutTargets();
		void updateAssignment(UnitInfo&);
		void updateDecision(UnitInfo&);

		bool search(UnitInfo&);
		bool scout(UnitInfo&);
		bool hide(UnitInfo&);
		bool harass(UnitInfo&);

		set<Position> scoutTargets;
		set<Position> scoutAssignments;
		int scoutCount;
		bool proxyCheck = false;

		typedef bool (ScoutManager::*Command)(UnitInfo&);
		vector<Command> commands{ &ScoutManager::search, &ScoutManager::scout, &ScoutManager::hide, &ScoutManager::harass };
	public:
		void onFrame();	
	};
}

typedef Singleton<McRave::ScoutManager> ScoutSingleton;