#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UtilTrackerClass
	{
	public:
		double getPercentHealth(UnitInfo&);
		double getMaxGroundStrength(UnitInfo&);
		double getVisibleGroundStrength(UnitInfo&);
		double getMaxAirStrength(UnitInfo&);
		double getVisibleAirStrength(UnitInfo&);
		double getPriority(UnitInfo&);
		double getTrueGroundRange(UnitInfo&);
		double getTrueAirRange(UnitInfo&);
		double getTrueGroundDamage(UnitInfo&);
		double getTrueAirDamage(UnitInfo&);
		double getTrueSpeed(UnitInfo&);

		// Returns the minimum number of frames for the given unit type to wait before having another command issued to it
		int getMinStopFrame(UnitType);

		// Returns the WalkPosition of the unit
		WalkPosition getWalkPosition(Unit);

		// Returns the set of WalkPositions under the unit
		set<WalkPosition> getWalkPositionsUnderUnit(Unit);

		// Returns 1 if the tiles at the finish that would be under the unit meet the criteria of the options chosen
		// If groundcheck/aircheck, then this function checks if every WalkPosition around finish has no ground/air threat
		bool isSafe(WalkPosition finish, UnitType, bool groundCheck, bool airCheck);

		// Returns 1 if the tiles at the finish are all walkable tiles and checks for overlap with this unit
		bool isMobile(WalkPosition start, WalkPosition finish, UnitType);

		// Returns 1 if the unit is in range of a position
		bool unitInRange(UnitInfo& unit);
		bool targetInRange(UnitInfo& unit);

		// Returns 1 if the worker should fight
		bool shouldPullWorker(Unit unit);

		bool isWalkable(TilePosition here);
	};
}

typedef Singleton<McRave::UtilTrackerClass> UtilTracker;