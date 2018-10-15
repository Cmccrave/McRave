#include "McRave.h"

namespace McRave
{
	void GoalManager::onFrame()
	{
		updateGoals();
	}

	void GoalManager::updateGoals()
	{
		// Defend my expansions
		for (auto &s : Stations().getMyStations()) {
			Station station = s.second;

			if (station.BWEMBase()->Location() != mapBWEB.getNaturalTile() && station.BWEMBase()->Location() != mapBWEB.getMainTile() && Grids().getDefense(station.BWEMBase()->Location()) == 0)
				assignClosestToGoal(station.BWEMBase()->Center(), vector<UnitType> {UnitTypes::Protoss_Dragoon, 4});
		}

		// Attack enemy expansions with a small force
		// PvP / PvT
		if (Players().vP() || Players().vT()) {
			auto distBest = 0.0;
			auto posBest = Positions::Invalid;
			for (auto &s : Stations().getEnemyStations()) {
				Station station = s.second;
				auto pos = station.BWEMBase()->Center();
				auto dist = pos.getDistance(Terrain().getEnemyStartingPosition());
				if (dist > distBest) {
					distBest = dist;
					posBest = pos;
				}
			}
			assignClosestToGoal(posBest, vector<UnitType> {UnitTypes::Protoss_Dragoon, 4});
		}

		// Secure our own future expansion position
		// PvT
		Position nextExpand(Buildings().getCurrentExpansion());
		if (nextExpand.isValid() && Players().vT()) {
			UnitType building = Broodwar->self()->getRace().getResourceDepot();
			if (BuildOrder().buildCount(building) > Broodwar->self()->visibleUnitCount(building)) {
				assignClosestToGoal(nextExpand, vector<UnitType> { UnitTypes::Protoss_Dragoon, 2 });
			}
		}

		// Escort shuttles
		// PvZ
		if (Players().vZ() && Units().getGlobalEnemyAirStrength() > 0.0) {
			for (auto &u : Units().getMyUnits()) {
				auto &unit = u.second;
				if (unit.getRole() == Role::Transporting)
					assignClosestToGoal(unit.getPosition(), vector<UnitType>{ UnitTypes::Protoss_Corsair, 4});
			}
		}

		// Deny enemy expansions
		if (Players().vT() && Stations().getMyStations().size() >= 3 && Stations().getMyStations().size() > Stations().getEnemyStations().size() && Terrain().getEnemyExpand().isValid() && Units().getSupply() >= 200)
			assignClosestToGoal((Position)Terrain().getEnemyExpand(), vector<UnitType> { UnitTypes::Protoss_Dragoon, 4 });

		// Send lurkers to expansions for fun
		if (Broodwar->self()->getRace() == Races::Zerg && !Stations().getMyStations().empty()) {
			auto lurkerPerBase = Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Lurker) / Stations().getMyStations().size();

			for (auto &base : Stations().getMyStations()) {
				assignClosestToGoal(base.second.ResourceCentroid(), vector<UnitType> { UnitTypes::Zerg_Lurker, UnitTypes::Zerg_Lurker, UnitTypes::Zerg_Lurker, UnitTypes::Zerg_Lurker });
			}
		}
	}

	void GoalManager::assignClosestToGoal(Position here, vector<UnitType> types)
	{
		// TODO: Remake this so that it grabs the correct number of units
		// Right now we only grab 1 type of unit, but may want multiple types later
		// Form concave and set destination
		map<double, UnitInfo*> unitByDist;

		for (auto &type : types) {
			for (auto &u : Units().getMyUnits()) {
				UnitInfo &unit = u.second;

				if (unit.unit() && unit.getType() == type)
					unitByDist[unit.getPosition().getDistance(here)] = &u.second;
			}
		}

		int i = 0;
		for (auto &u : unitByDist) {
			UnitInfo* unit = u.second;
			if (find(types.begin(), types.end(), u.second->getType()) != types.end() && !u.second->getDestination().isValid()) {
				u.second->setDestination(here);
				//myGoals[here] += u.second->getVisibleGroundStrength();
				i++;
			}
			if (i == int(types.size()))
				break;
		}
	}
}