#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitInfo;
	class CommandTrackerClass
	{
		void updateAlliedUnits();
		void engage(UnitInfo&), move(UnitInfo&), approach(UnitInfo&), defend(UnitInfo&), flee(UnitInfo&), attack(UnitInfo&);
		bool shouldAttack(UnitInfo&), shouldKite(UnitInfo&), shouldApproach(UnitInfo&);
		bool isLastCommand(UnitInfo&, UnitCommandType, Position);
	public:
		void update();
	};
}

typedef Singleton<McRave::CommandTrackerClass> CommandTracker;