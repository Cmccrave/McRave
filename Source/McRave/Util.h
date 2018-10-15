#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UtilManager
	{
	public:

		UnitInfo * getClosestUnit(Position, Player, UnitType t = UnitTypes::None);
		UnitInfo * getClosestThreat(UnitInfo&);
		UnitInfo * getClosestBuilder(Position);

		// Returns the width of the choke in pixels
		int chokeWidth(const BWEM::ChokePoint *);
		const BWEM::ChokePoint * getClosestChokepoint(Position);

		double getMaxGroundStrength(UnitInfo&);
		double getVisibleGroundStrength(UnitInfo&);
		double getMaxAirStrength(UnitInfo&);
		double getVisibleAirStrength(UnitInfo&);
		double getPriority(UnitInfo&);

		// Unit statistics
		double groundRange(UnitInfo&);
		double airRange(UnitInfo&);
		double groundDamage(UnitInfo&);
		double airDamage(UnitInfo&);
		double speed(UnitInfo&);
		double splashModifier(UnitInfo&);
		double groundDPS(UnitInfo&);
		double airDPS(UnitInfo&);
		double effectiveness(UnitInfo&);
		double survivability(UnitInfo&);
		double gWeaponCooldown(UnitInfo&);
		double aWeaponCooldown(UnitInfo&);

		// Returns the highest threat in a grid the size of the unit
		double getHighestThreat(WalkPosition, UnitInfo&);

		// Returns the minimum number of frames for the given unit type to wait before having another command issued to it
		int getMinStopFrame(UnitType);

		// Returns the WalkPosition of the unit
		WalkPosition getWalkPosition(Unit);

		// Returns 1 if the tiles at the finish that would be under the unit meet the criteria of the options chosen
		// If groundcheck/aircheck, then this function checks if every WalkPosition around finish has no ground/air threat
		bool isSafe(WalkPosition finish, UnitType, bool groundCheck, bool airCheck);

		bool isMobile(WalkPosition start, WalkPosition finish, UnitType);
		bool unitInRange(UnitInfo& unit);
		
		bool reactivePullWorker(UnitInfo& unit);
		bool proactivePullWorker(UnitInfo& unit);
		bool pullRepairWorker(UnitInfo& unit);

		template<class T>
		bool isWalkable(T here)
		{
			WalkPosition start(here);
			for (int x = start.x; x < start.x + 4; x++) {
				for (int y = start.y; y < start.y + 4; y++) {
					if (Grids().getMobility(WalkPosition(x, y)) == -1)
						return false;
				}
			}
			return true;
		}

		
		Position getConcavePosition(UnitInfo&, BWEM::Area const * area = nullptr, Position here = Positions::Invalid);
	};
}

typedef Singleton<McRave::UtilManager> UtilSingleton;
