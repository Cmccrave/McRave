#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TargetTrackerClass
	{
		Position getEngagePosition(UnitInfo&, UnitInfo&);
	public:
		void getTarget(UnitInfo&);
		void enemyTarget(UnitInfo&);
		void allyTarget(UnitInfo&);
	};
}

typedef Singleton<McRave::TargetTrackerClass> TargetTracker;