#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class CommandTrackerClass
{
public:
	void update();
	void updateAlliedUnits();

	void engage(UnitInfo&);
	void move(UnitInfo&);
	void approach(UnitInfo&);
	void defend(UnitInfo&);
	void exploreArea(UnitInfo&);
	void flee(UnitInfo&);
	void attack(UnitInfo&);

	bool shouldAttack(UnitInfo&);
	bool shouldKite(UnitInfo&);
	bool shouldApproach(UnitInfo&);
	bool isLastCommand(UnitInfo&, UnitCommandType, Position);
};

typedef Singleton<CommandTrackerClass> CommandTracker;