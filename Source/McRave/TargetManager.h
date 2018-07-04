#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TargetManager
	{
		Position getEngagePosition(UnitInfo&);
	public:
		void getTarget(UnitInfo&);
		void enemyTarget(UnitInfo&);
		void allyTarget(UnitInfo&);

		void getPathToTarget(UnitInfo&);
	};
}

typedef Singleton<McRave::TargetManager> TargetSingleton;
