#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TargetManager
	{
		void getEngagePosition(UnitInfo&);
		void getPathToTarget(UnitInfo&);
	public:
		void getTarget(UnitInfo&);
		void enemyTarget(UnitInfo&);
		void allyTarget(UnitInfo&);
	};
}

typedef Singleton<McRave::TargetManager> TargetSingleton;
