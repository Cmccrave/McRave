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

		// New stuff
		UnitInfo * getClosestUnit(Position, Player, UnitType t = UnitTypes::None);
		UnitInfo * getClosestThreat(UnitInfo&);
		UnitInfo * getClosestBuilder(Position);

		// Returns the width of the choke in pixels
		int chokeWidth(const BWEM::ChokePoint *);

		double getPercentHealth(UnitInfo&);
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

		//
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

		template <class T>
		const Position getConcavePosition(T &unit, BWEM::Area const * area = nullptr, Position here = Positions::Invalid) {

			// Setup parameters
			int min = int(unit.getGroundRange());
			double distBest = DBL_MAX;
			WalkPosition center = WalkPositions::None;
			Position bestPosition = Positions::None;

			// HACK: I found my reavers getting picked off too often when they held too close
			if (unit.getType() == UnitTypes::Protoss_Reaver)
				min += 64;

			// Finds which position we are forming the concave at
			const auto getConcaveCenter = [&]() {
				if (here.isValid())
					center = (WalkPosition)here;
				else if (area == mapBWEB.getNaturalArea() && mapBWEB.getNaturalChoke())
					center = mapBWEB.getNaturalChoke()->Center();
				else if (area == mapBWEB.getMainArea() && mapBWEB.getMainChoke())
					center = mapBWEB.getMainChoke()->Center();

				else if (area) {
					for (auto &c : area->ChokePoints()) {
						double dist = mapBWEB.getGroundDistance(Position(c->Center()), Terrain().getEnemyStartingPosition());
						if (dist < distBest) {
							distBest = dist;
							center = c->Center();
						}
					}
				}
			};

			const auto checkbest = [&](WalkPosition w) {
				TilePosition t(w);
				Position p = Position(w) + Position(4, 4);

				double dist = p.getDistance(Position(center)) * log(p.getDistance(mapBWEB.getMainPosition()));

				if (!w.isValid()
					|| !Util().isMobile(unit.getWalkPosition(), w, unit.getType())
					|| (here != Terrain().getDefendPosition() && area && mapBWEM.GetArea(t) != area)
					|| (unit.getGroundRange() > 32.0 && p.getDistance(Position(center)) < min)
					|| Buildings().overlapsQueuedBuilding(unit.getType(), t)
					|| dist > distBest
					|| Commands().overlapsCommands(unit.unit(), UnitTypes::None, p, 8)
					|| (unit.getType() == UnitTypes::Protoss_Reaver && Terrain().isDefendNatural() && mapBWEM.GetArea(w) != mapBWEB.getNaturalArea()))
					return false;

				bestPosition = p;
				distBest = dist;
				return true;
			};

			// Find the center
			getConcaveCenter();

			// If this is the defending position, grab from a vector we already made
			// TODO: generate a vector for every choke and store as a map<Choke, vector<Position>>?
			if (here == Terrain().getDefendPosition()) {
				for (auto &position : Terrain().getChokePositions()) {
					checkbest(WalkPosition(position));
				}
			}

			// Find a position around the center that is suitable
			else {
				for (int x = center.x - 40; x <= center.x + 40; x++) {
					for (int y = center.y - 40; y <= center.y + 40; y++) {
						WalkPosition w(x, y);
						checkbest(w);
					}
				}
			}
			return bestPosition;
		}

		template<class T>
		const Position getExplorePosition(T &unit)
		{

		}

		const BWEM::ChokePoint * getClosestChokepoint(Position);
	};
}

typedef Singleton<McRave::UtilManager> UtilSingleton;
