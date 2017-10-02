#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class UtilTrackerClass
{
public:
	double getPercentHealth(UnitInfo&);
	double getMaxGroundStrength(UnitInfo&, Player);
	double getVisibleGroundStrength(UnitInfo&, Player);
	double getMaxAirStrength(UnitInfo&, Player);
	double getVisibleAirStrength(UnitInfo&, Player);
	double getPriority(UnitInfo&, Player);
	double getTrueGroundRange(UnitType, Player);
	double getTrueAirRange(UnitType, Player);
	double getTrueGroundDamage(UnitType, Player);
	double getTrueAirDamage(UnitType, Player);
	double getTrueSpeed(UnitType, Player);

	// Returns the minimum number of frames for the given unit type to wait before having another command issued to it
	int getMinStopFrame(UnitType);

	// Returns the WalkPosition of the unit
	WalkPosition getWalkPosition(Unit);

	// Returns the set of WalkPositions under the unit
	set<WalkPosition> getWalkPositionsUnderUnit(Unit);

	// Returns 1 if the tiles at the finish that would be under the unit meet the criteria of the options chosen (can be any combination or all three)
	// If groundcheck/aircheck, then this function checks if every WalkPosition around finish has no ground/air threat
	// If mobilitycheck, then this function checks if every WalkPosition around finish has suitable mobility
	bool isSafe(WalkPosition start, WalkPosition finish, UnitType, bool groundCheck, bool airCheck, bool mobilityCheck);
	
};

typedef Singleton<UtilTrackerClass> UtilTracker;