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

		// Returns the minimum number of frames for the given unit type to wait before having another command issued to it
		int getMinStopFrame(UnitType);

		// Returns the WalkPosition of the unit
		WalkPosition getWalkPosition(Unit);

		// Returns 1 if the tiles at the finish that would be under the unit meet the criteria of the options chosen
		// If groundcheck/aircheck, then this function checks if every WalkPosition around finish has no ground/air threat
		bool isSafe(WalkPosition finish, UnitType, bool groundCheck, bool airCheck);

		// Returns 1 if the tiles at the finish are all walkable tiles and checks for overlap with this unit
		bool isMobile(WalkPosition start, WalkPosition finish, UnitType);

		// Returns 1 if the unit is in range of its target
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
		static UnitInfo* getClosestAllyUnit(T& unit, const UnitFilter& pred = nullptr)
		{
			double distBest = DBL_MAX;
			UnitInfo* best = nullptr;
			for (auto &a : Units().getMyUnits()) {
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
		static BuildingInfo* getClosestAllyBuilding(T& unit, const UnitFilter& pred = nullptr)
		{
			double distBest = DBL_MAX;
			BuildingInfo* best = nullptr;
			for (auto &a : Buildings().getMyBuildings()) {
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

		template<class T>
		const Position getKitePosition(T &unit)
		{
			double best = 0.0;
			int radius = 16;
			WalkPosition start = unit.getWalkPosition();
			Position posBest = Positions::Invalid;

			// If size of unit is even, we want the division between the WalkPosition
			int walkWidth = (int)ceil(unit.getType().width() / 8.0);
			bool evenW = walkWidth % 2 == 0;

			int walkHeight = (int)ceil(unit.getType().height() / 8.0);
			bool evenH = walkHeight % 2 == 0;

			// Search a grid around the unit
			for (int x = start.x - radius; x <= start.x + radius + walkWidth; x++) {
				for (int y = start.y - radius; y <= start.y + radius + walkHeight; y++) {
					WalkPosition w(x, y);
					Position p = Position(w) + Position(4, 4) + Position(4 * evenW, 4 * evenH);

					if (!w.isValid()
						|| p.getDistance(unit.getPosition()) < 32.0
						|| Commands().isInDanger(p)
						|| !Util().isMobile(start, w, unit.getType())
						|| p.getDistance(unit.getPosition()) > radius * 8
						|| Grids().getESplash(w) > 0
						|| Buildings().overlapsQueuedBuilding(unit.getType(), unit.getTilePosition()))
						continue;

					double distance;
					if (!Strategy().defendChoke() && unit.getType() == UnitTypes::Protoss_Zealot && unit.hasTarget() && Terrain().isInAllyTerritory(unit.getTarget().getTilePosition()))
						distance = p.getDistance(Terrain().getMineralHoldPosition());
					else if (unit.hasTarget() && (Terrain().isInAllyTerritory(unit.getTarget().getTilePosition()) || Terrain().isInAllyTerritory(unit.getTilePosition())))
						distance = 1.0 / (32.0 + p.getDistance(unit.getTarget().getPosition()));
					else
						distance = (unit.getType().isFlyer() || Terrain().isIslandMap()) ? p.getDistance(mapBWEB.getMainPosition()) : exp(Grids().getDistanceHome(w)); // Distance value	

					double mobility = 1.0; // temp ignore mobility

					double threat = unit.getType().isFlyer() ? max(0.1, Grids().getEAirThreat(w)) : max(0.1, Grids().getEGroundThreat(w));			// If unit is a flyer, use air threat
					double grouping = (unit.getType().isFlyer() && double(Grids().getAAirCluster(w)) > 1.0) ? 2.0 : 1.0;							// Flying units should try to cluster
					double score = grouping * mobility / (threat * distance);

					// If position is valid and better score than current, set as current best
					if (score > best) {
						posBest = p;
						best = score;
					}
				}
			}
			return posBest;
		}

		template<class T>
		const Position getExplorePosition(T &unit)
		{

		}

		const BWEM::ChokePoint * getClosestChokepoint(Position);
	};
}

typedef Singleton<McRave::UtilManager> UtilSingleton;
