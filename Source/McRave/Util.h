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

		// Returns the minimum number of frames for the given unit type to wait before having another command issued to it
		int getMinStopFrame(UnitType);

		// Returns the WalkPosition of the unit
		WalkPosition getWalkPosition(Unit);

		// Returns 1 if the tiles at the finish that would be under the unit meet the criteria of the options chosen
		// If groundcheck/aircheck, then this function checks if every WalkPosition around finish has no ground/air threat
		bool isSafe(WalkPosition finish, UnitType, bool groundCheck, bool airCheck);

		// Returns 1 if the tiles at the finish are all walkable tiles and checks for overlap with this unit
		bool isMobile(WalkPosition start, WalkPosition finish, UnitType);

		// Returns 1 if the unit is in range of a position
		bool unitInRange(UnitInfo& unit);

		// Returns 1 if the worker should fight
		bool reactivePullWorker(Unit unit);
		bool proactivePullWorker(Unit unit);
		bool pullRepairWorker(Unit unit);

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

			// Defend chokepoint with concave
			int min = int(unit.getGroundRange());
			int max = int(unit.getGroundRange()) + 256;

			if (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Zergling) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) == 0)
				min = 0, max = 256;

			if (unit.getType() == UnitTypes::Protoss_Zealot)
				min = 64;
			if (unit.getType() == UnitTypes::Zerg_Zergling)
				min = 128;

			double distBest = DBL_MAX;
			WalkPosition start = unit.getWalkPosition();
			WalkPosition choke;
			Position bestPosition;
			
			if (here.isValid())
				choke = (WalkPosition)here;
			else if (area == mapBWEB.getNaturalArea())
				choke = mapBWEB.getNaturalChoke()->Center();
			else if (area == mapBWEB.getMainArea())
				choke = mapBWEB.getMainChoke()->Center();

			else if (area){
				for (auto &c : area->ChokePoints()) {
					double dist = mapBWEB.getGroundDistance(Position(c->Center()),Terrain().getEnemyStartingPosition());
					if (dist < distBest) {
						distBest = dist;
						choke = c->Center();
					}
				}
			}

			// Find closest chokepoint to defend			
			double distHome = Grids().getDistanceHome(choke);
			distBest = DBL_MAX;

			for (int x = choke.x - 24; x <= choke.x + 24; x++) {
				for (int y = choke.y - 24; y <= choke.y + 24; y++) {
					WalkPosition w(x, y);
					TilePosition t(w);
					Position p = Position(w) + Position(4, 4);

					if (!w.isValid() || !Util().isMobile(start, w, unit.getType()))
						continue;
					if ((area && mapBWEM.GetArea(t) != area) || p.getDistance(Position(choke)) < min || p.getDistance(Position(choke)) > max || Buildings().overlapsQueuedBuilding(unit.getType(), t))
						continue;
					if (here == Terrain().getDefendPosition() && Grids().getDistanceHome(w) > distHome)
						continue;

					double dist = p.getDistance(Position(choke));
					if (dist < distBest) {
						bestPosition = p;
						distBest = dist;
					}
				}
			}
			return bestPosition;
		}

		template<class T>
		static UnitInfo* getClosestAllyUnit(T& unit, const UnitFilter& pred = nullptr)
		{
			double distBest = DBL_MAX;
			UnitInfo* best = nullptr;
			for (auto&a : Units().getMyUnits()) {
				UnitInfo& ally = a.second;
				if (!ally.unit() || (pred.isValid() && !pred(ally.unit())))
					continue;

				if (unit.unit() != ally.unit()) {
					double dist = unit.getPosition().getDistance(ally.getPosition());
					if (dist < distBest)
						best = &ally, distBest = dist;
				}
			}
			return best;
		}

		template<class T>
		static UnitInfo* getClosestEnemyUnit(T& unit, const UnitFilter& pred = nullptr)
		{
			double distBest = DBL_MAX;
			UnitInfo* best = nullptr;
			for (auto&e : Units().getEnemyUnits()) {
				UnitInfo& enemy = e.second;

				if (!enemy.unit() || (pred.isValid() && !pred(enemy.unit())))
					continue;

				if (unit.unit() != enemy.unit()) {
					double dist = unit.getPosition().getDistance(enemy.getPosition());
					if (dist < distBest)
						best = &enemy, distBest = dist;
				}
			}
			return best;
		}

		template<class T>
		static BuildingInfo* getClosestAllyBuilding(T& unit, const UnitFilter& pred = nullptr)
		{
			double distBest = DBL_MAX;
			BuildingInfo* best = nullptr;
			for (auto&a : Buildings().getMyBuildings()) {
				BuildingInfo& ally = a.second;
				if (!ally.unit() || (pred.isValid() && !pred(ally.unit())))
					continue;

				if (unit.unit() != ally.unit()) {
					double dist = unit.getPosition().getDistance(ally.getPosition());
					if (dist < distBest)
						best = &ally, distBest = dist;
				}
			}
			return best;
		}

		template<class T>
		static WorkerInfo* getClosestAllyWorker(T& unit, const UnitFilter& pred = nullptr)
		{
			double distBest = DBL_MAX;
			WorkerInfo* best = nullptr;
			for (auto&a : Workers().getMyWorkers()) {
				WorkerInfo& ally = a.second;
				if (!ally.unit() || (pred.isValid() && !pred(ally.unit())))
					continue;

				if (unit.unit() != ally.unit()) {
					double dist = unit.getPosition().getDistance(ally.getPosition());
					if (dist < distBest)
						best = &ally, distBest = dist;
				}
			}
			return best;
		}
	};
}

typedef Singleton<McRave::UtilManager> UtilSingleton;
