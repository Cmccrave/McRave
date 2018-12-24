#include "McRave.h"

void ScoutManager::onFrame()
{
	Display().startClock();
	updateScoutTargets();
	updateScouts();
	Display().performanceTest(__FUNCTION__);
}

void ScoutManager::updateScoutTargets()
{
	scoutTargets.clear();

	// If enemy start is valid and explored, add a target to the most recent one to scout
	if (Terrain().foundEnemy()) {
		for (auto &s : MyStations().getEnemyStations()) {
			auto &station = *s.second;
			TilePosition tile(station.BWEMBase()->Center());
			if (tile.isValid())
				scoutTargets.insert(Position(tile));
		}
		if (Players().vZ() && MyStations().getEnemyStations().size() == 1 && Strategy().getEnemyBuild() != "Unknown")
			scoutTargets.insert((Position)Terrain().getEnemyExpand());
	}

	// If we know where it is but it isn't explored
	else if (Terrain().getEnemyStartingTilePosition().isValid())
		scoutTargets.insert(Terrain().getEnemyStartingPosition());

	// If we have no idea where the enemy is
	else if (!Terrain().getEnemyStartingTilePosition().isValid()) {
		double best = DBL_MAX;
		Position pos = Positions::Invalid;
		int basesExplored = 0;
		for (auto &tile : mapBWEM.StartingLocations()) {
			Position center = Position(tile) + Position(64, 48);
			double dist = center.getDistance(BWEB::Map::getMainPosition());
			if (Broodwar->isExplored(tile))
				basesExplored++;

			if (!Broodwar->isExplored(tile))
				scoutTargets.insert(center);
		}

		// If we have scouted 2 bases (including our own), scout the middle for a proxy if it's walkable
		if (basesExplored == 2 && !Broodwar->isExplored((TilePosition)mapBWEM.Center()) && BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), mapBWEM.Center()) != DBL_MAX)
			scoutTargets.insert(mapBWEM.Center());
	}

	// If it's a 2gate, scout for an expansion if we found the gates
	if (Strategy().getEnemyBuild() == "P2Gate") {
/*		if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2)
			scoutTargets.insert((Position)Terrain().getEnemyExpand());
		else*/ if (Units().getEnemyCount(UnitTypes::Protoss_Pylon) == 0 || Strategy().enemyProxy())
			scoutTargets.insert(mapBWEM.Center());
	}

	// If it's a cannon rush, scout the main
	if (Strategy().getEnemyBuild() == "PCannonRush")
		scoutTargets.insert(BWEB::Map::getMainPosition());
}

void ScoutManager::updateScouts()
{
	// If we have too many scouts
	// TODO: Add removal
	for (auto &u : Units().getMyUnits()) {
		auto &unit = u.second;
		if (unit.getRole() == Role::Scouting) {
			updateAssignment(unit);
			updateDecision(unit);
		}
	}
}

void ScoutManager::updateAssignment(UnitInfo& unit)
{
	auto start = unit.getWalkPosition();
	auto distBest = DBL_MAX;
	auto posBest = unit.getDestination();

	// TODO: Use scout counts to correctly assign more scouts
	scoutAssignments.clear();
	scoutCount = 1;

	// If we have seen an enemy Probe before we've scouted the enemy, follow it
	if (Units().getEnemyCount(UnitTypes::Protoss_Probe) == 1) {
		auto w = Util().getClosestUnit(BWEB::Map::getMainPosition(), Broodwar->enemy(), UnitTypes::Protoss_Probe);
		proxyCheck = (w && !Terrain().getEnemyStartingPosition().isValid() && w->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1);
	}

	// Temp we don't do 2 scouts for some reason atm
	proxyCheck = false;

	// If we know a proxy possibly exists, we need a second scout
	auto foundProxyGates = Strategy().enemyProxy() && Strategy().getEnemyBuild() == "P2Gate" && Units().getEnemyCount(UnitTypes::Protoss_Gateway) > 0;
	if (((Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate") || proxyCheck || foundProxyGates) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1)
		scoutCount++;

	if (BuildOrder::isRush() && Broodwar->self()->getRace() == Races::Zerg && Terrain().getEnemyStartingPosition().isValid())
		scoutCount = 0;
	if (Strategy().getEnemyBuild() == "Z5Pool" && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 5)
		scoutCount = 0;

	if (!BuildOrder::firstReady() || !Terrain().getEnemyStartingPosition().isValid()) {

		// If it's a proxy (maybe cannon rush), try to find the unit to kill
		if ((Strategy().enemyProxy() || proxyCheck) && scoutCount > 1 && scoutAssignments.find(BWEB::Map::getMainPosition()) == scoutAssignments.end()) {

			UnitInfo* enemyunit = Util().getClosestUnit(unit.getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Probe);
			scoutAssignments.insert(BWEB::Map::getMainPosition());

			if (enemyunit && enemyunit->getPosition().isValid() && enemyunit->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0) {
				if (enemyunit->unit() && enemyunit->unit()->exists()) {
					unit.unit()->attack(enemyunit->unit());
					return;
				}
				else
					unit.setDestination(enemyunit->getPosition());
			}
			else if (Strategy().getEnemyBuild() == "P2Gate") {
				UnitInfo* enemyPylon = Util().getClosestUnit(unit.getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Pylon);
				if (enemyPylon && !Terrain().isInEnemyTerritory(enemyPylon->getTilePosition())) {
					if (enemyPylon->unit() && enemyPylon->unit()->exists()) {
						unit.unit()->attack(enemyPylon->unit());
						return;
					}
					else
						unit.unit()->move(enemyPylon->getPosition());
				}
			}
			else
				unit.setDestination(BWEB::Map::getMainPosition());
		}

		// If we have scout targets, find the closest target
		else if (!scoutTargets.empty()) {
			for (auto &target : scoutTargets) {
				double dist = target.getDistance(unit.getPosition());
				double time = 1.0 + (double(Grids().lastVisibleFrame((TilePosition)target)));
				double timeDiff = Broodwar->getFrameCount() - time;

				if (time < distBest && timeDiff > 500 && scoutAssignments.find(target) == scoutAssignments.end()) {
					distBest = time;
					posBest = target;
				}
			}
			if (posBest.isValid()) {
				unit.setDestination(posBest);
				scoutAssignments.insert(posBest);
			}
		}

		// TEMP
		if (!unit.getDestination().isValid())
			unit.setDestination(Terrain().getEnemyStartingPosition());

		if (unit.getDestination().isValid()) {
			UnitInfo* enemy = Util().getClosestThreat(unit);
			if (enemy && enemy->getPosition().getDistance(unit.getPosition()) < 320.0) {
				if (Command::hunt(unit)) {}
				else if (Command::kite(unit)) {}
			}
			else
				unit.command(UnitCommandTypes::Move, unit.getDestination());
		}
	}
	else
	{
		int best = INT_MAX;
		for (auto &area : mapBWEM.Areas()) {
			for (auto &base : area.Bases()) {
				if (area.AccessibleNeighbours().size() == 0 || Terrain().isInEnemyTerritory(base.Location()) || Terrain().isInAllyTerritory(base.Location()))
					continue;

				int time = Grids().lastVisibleFrame(base.Location());
				if (time < best)
					best = time, posBest = Position(base.Location());
			}
		}
		if (posBest.isValid() && unit.unit()->getOrderTargetPosition() != posBest)
			unit.unit()->move(posBest);
	}
}

void ScoutManager::updateDecision(UnitInfo& unit)
{
	// Convert our commands to strings to display what the unit is doing for debugging
	map<int, string> commandNames{
		make_pair(0, "Search"),
		make_pair(1, "Scout"),
		make_pair(2, "Hide"),
		make_pair(3, "Harass"),
	};

	int i = 0;
	int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
	for (auto cmd : commands) {
		if ((this->*cmd)(unit)) {
			Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
			break;
		}
		i++;
	}
}

bool ScoutManager::search(UnitInfo& unit)
{
	// Aggresive - Need information
	return false;
}

bool ScoutManager::scout(UnitInfo& unit)
{
	// Default
	return false;
}

bool ScoutManager::hide(UnitInfo& unit)
{
	// Passive - Need to wait
	return false;
}

bool ScoutManager::harass(UnitInfo& unit)
{
	return false;
}