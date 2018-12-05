#include "McRave.h"

namespace McRave
{
	void GoalManager::onFrame()
	{
		updateProtossGoals();
		updateTerranGoals();
		updateZergGoals();
	}

	void GoalManager::updateProtossGoals()
	{
		map<UnitType, int> unitTypes;

		// Defend my expansions
		for (auto &s : MyStations().getMyStations()) {
			auto station = *s.second;

			if (station.BWEMBase()->Location() != BWEB::Map::getNaturalTile() && station.BWEMBase()->Location() != BWEB::Map::getMainTile() && station.getDefenseCount() == 0) {
				assignClosestToGoal(station.BWEMBase()->Center(), UnitTypes::Protoss_Dragoon, 4);
			}
		}

		// Attack enemy expansions with a small force
		// PvP / PvT
		if (Players().vP() || Players().vT()) {
			auto distBest = 0.0;
			auto posBest = Positions::Invalid;
			for (auto &s : MyStations().getEnemyStations()) {
				auto station = *s.second;
				auto pos = station.BWEMBase()->Center();
				auto dist = BWEB::Map::getGroundDistance(pos, Terrain().getEnemyStartingPosition());
				if (dist > distBest) {
					distBest = dist;
					posBest = pos;
				}
			}
			if (Players().vP())
				assignClosestToGoal(posBest, UnitTypes::Protoss_Dragoon, 4);
			else
				assignClosestToGoal(posBest, UnitTypes::Protoss_Zealot, 4);
		}

		// Secure our own future expansion position
		// PvT
		Position nextExpand(Buildings().getCurrentExpansion());
		if (nextExpand.isValid()) {
			UnitType building = Broodwar->self()->getRace().getResourceDepot();
			if (BuildOrder().buildCount(building) > Broodwar->self()->visibleUnitCount(building)) {
				if (Players().vZ())
					assignClosestToGoal(nextExpand, UnitTypes::Protoss_Zealot, 4);
				else
					assignClosestToGoal(nextExpand, UnitTypes::Protoss_Dragoon, 4);
			}
		}

		// Escort shuttles
		// PvZ
		if (Players().vZ() && Units().getGlobalEnemyAirStrength() > 0.0) {
			for (auto &u : Units().getMyUnits()) {
				auto &unit = u.second;
				if (unit.getRole() == Role::Transporting)
					assignClosestToGoal(unit.getPosition(), UnitTypes::Protoss_Corsair, 4);
			}
		}

		// Deny enemy expansions
		// PvT
		if (Players().vT() && MyStations().getMyStations().size() >= 3 && MyStations().getMyStations().size() > MyStations().getEnemyStations().size() && Terrain().getEnemyExpand().isValid() && Units().getSupply() >= 200)
			assignClosestToGoal((Position)Terrain().getEnemyExpand(), UnitTypes::Protoss_Dragoon, 4);
	}

	void GoalManager::updateTerranGoals()
	{

	}

	void GoalManager::updateZergGoals()
	{	

		// Send lurkers to expansions when turtling		
		if (Broodwar->self()->getRace() == Races::Zerg && !MyStations().getMyStations().empty()) {
			auto lurkerPerBase = Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Lurker) / MyStations().getMyStations().size();			

			for (auto &base : MyStations().getMyStations()) {
				auto station = *base.second;
				assignClosestToGoal(station.ResourceCentroid(), UnitTypes::Zerg_Lurker, 4);
			}
		}
	}

	void GoalManager::assignClosestToGoal(Position here, UnitType type, int count)
	{
		map<double, UnitInfo*> unitByDist;
		map<UnitType, int> unitByType;

		// Store units by distance if they have a matching type
		for (auto &u : Units().getMyUnits()) {
			UnitInfo &unit = u.second;

			if (unit.getType() == type) {
				double dist = unit.getType().isFlyer() ? unit.getPosition().getDistance(here) : BWEB::Map::getGroundDistance(unit.getPosition(), here);
				unitByDist[dist] = &u.second;
			}
		}
				
		// Iterate through closest units
		for (auto &u : unitByDist) {
			UnitInfo* unit = u.second;
			if (count > 0 && !unit->getDestination().isValid()) {
				Broodwar->drawLineMap(unit->getPosition(), here, Colors::Red);
				unit->setDestination(here);
				count --;
			}
		}
	}
}