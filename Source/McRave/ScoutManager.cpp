#include "McRave.h"

void ScoutManager::onFrame()
{
	Display().startClock();
	updateScouts();
	Display().performanceTest(__FUNCTION__);
}

void ScoutManager::updateScouts()
{
	scoutAssignments.clear();
	scoutCount = 1;	

	// If we have seen an enemy Probe before we've scouted the enemy, follow it
	if (Units().getEnemyCount(UnitTypes::Protoss_Probe) == 1) {
		auto w = Util().getClosestUnit(mapBWEB.getMainPosition(), Broodwar->enemy(), UnitTypes::Protoss_Probe);
		proxyCheck = (w && !Terrain().getEnemyStartingPosition().isValid() && w->getPosition().getDistance(mapBWEB.getMainPosition()) < 640.0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1);
	}

	// If we know a proxy possibly exists, we need a second scout
	auto foundProxyGates = Strategy().enemyProxy() && Strategy().getEnemyBuild() == "P2Gate" && Units().getEnemyCount(UnitTypes::Protoss_Gateway) > 0;
	if (((Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate") || proxyCheck || foundProxyGates) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1)
		scoutCount++;

	if (BuildOrder().isRush() && Broodwar->self()->getRace() == Races::Zerg && Terrain().getEnemyStartingPosition().isValid())
		scoutCount = 0;
	if (Strategy().getEnemyBuild() == "Z5Pool" && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 5)
		scoutCount = 0;

	// If we have too few scouts
	if (mapBWEB.getNaturalChoke() && BuildOrder().shouldScout() && Units().roleCount(Role::Scouting) < scoutCount) {
		auto type = Broodwar->self()->getRace().getWorker();
		auto scout = Util().getClosestUnit(Position(mapBWEB.getNaturalChoke()->Center()), Broodwar->self(), type);
		if (scout)
			scout->setRole(Role::Scouting);
	}

	// If we have too many scouts
	// TODO: Add removal

	for (auto &u : Units().getMyUnits()) {
		auto &unit = u.second;
		if (unit.getRole() == Role::Scouting)
			scout(unit);
	}
}

void ScoutManager::scout(UnitInfo& unit)
{
	WalkPosition start = unit.getWalkPosition();
	double distBest = DBL_MAX;
	Position posBest = unit.getDestination();
	
	if (!BuildOrder().firstReady() || Strategy().getEnemyBuild() == "Unknown") {

		// If it's a proxy (maybe cannon rush), try to find the unit to kill
		if ((Strategy().enemyProxy() || proxyCheck) && scoutCount > 1 && scoutAssignments.find(mapBWEB.getMainPosition()) == scoutAssignments.end()) {
			UnitInfo* enemyunit = Util().getClosestUnit(unit.getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Probe);
			scoutAssignments.insert(mapBWEB.getMainPosition());

			if (enemyunit && enemyunit->getPosition().isValid() && enemyunit->getPosition().getDistance(mapBWEB.getMainPosition()) < 640.0) {
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
				unit.setDestination(mapBWEB.getMainPosition());
		}

		// If we have scout targets, find the closest target
		else if (!Strategy().getScoutTargets().empty()) {
			for (auto &target : Strategy().getScoutTargets()) {
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

		if (unit.getDestination().isValid())
			explore(unit);

		Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Green);
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

void ScoutManager::explore(UnitInfo& unit)
{
	WalkPosition start = unit.getWalkPosition();
	Position bestPosition = unit.getDestination();
	int longest = 0;
	UnitInfo* enemy = Util().getClosestUnit(unit.getPosition(), Broodwar->enemy());
	double test = 0.0;

	if (unit.getPosition().getDistance(unit.getDestination()) > 256.0 && (!enemy || (enemy && (enemy->getType().isWorker() || enemy->getPosition().getDistance(unit.getPosition()) > 320.0)))) {
		bestPosition = unit.getDestination();

		if (bestPosition.isValid() && bestPosition != Position(start) && unit.unit()->getLastCommand().getTargetPosition() != bestPosition) {
			unit.unit()->move(bestPosition);
			Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Blue);
		}
	}
	else
		Commands().hunt(unit);


}