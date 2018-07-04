#include "McRave.h"

void WorkerManager::onFrame()
{
	Display().startClock();
	updateScouts();
	updateWorkers();
	Display().performanceTest(__FUNCTION__);
}

void WorkerManager::updateWorkers()
{
	for (auto &worker : myWorkers) {
		updateInformation(worker.second);
		updateDecision(worker.second);
	}
}

void WorkerManager::updateScouts()
{
	scoutAssignments.clear();
	scoutCount = 1;

	// If we know a proxy possibly exists, we need a second scout
	if (Strategy().enemyProxy() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1)
		scoutCount++;	
	
	// If we have too many scouts, remove closest to home
	double distBest = DBL_MAX;
	Unit removal;
	if (scouts.size() > scoutCount) {
		for (auto &scout : scouts) {

			if (!scout || !scout->exists())
				continue;

			double dist = scout->getPosition().getDistance(mapBWEB.getMainPosition());
			if (dist < distBest) {
				distBest = dist;
				removal = scout;
			}
		}
		scouts.erase(removal);
	}

	// If we have too few scouts
	if (mapBWEB.getNaturalChoke() && BuildOrder().shouldScout() && (int)scouts.size() < scoutCount)
		scouts.insert(getClosestWorker(Position(mapBWEB.getNaturalChoke()->Center()), false));
}

void WorkerManager::updateInformation(WorkerInfo& worker)
{
	worker.setPosition(worker.unit()->getPosition());
	worker.setWalkPosition(Util().getWalkPosition(worker.unit()));
	worker.setTilePosition(worker.unit()->getTilePosition());
	worker.setType(worker.unit()->getType());
}

void WorkerManager::updateDecision(WorkerInfo& worker)
{
	// Workers that have a transport coming to pick them up should not do anything other than returning cargo
	if (worker.getTransport() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
		if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
			worker.unit()->move(worker.getTransport()->getPosition());
		return;
	}

	// Remove combat unit role
	if (Units().getMyUnits().find(worker.unit()) != Units().getMyUnits().end())
		Units().getMyUnits().erase(worker.unit());

	// Draw building placement
	if (worker.getBuildPosition().isValid()) {
		//Broodwar->drawLineMap(worker.getPosition(), (Position)worker.getBuildPosition(), Colors::Red);
		//Broodwar->drawTextMap(worker.getPosition(), "%s", worker.getBuildingType().c_str());
	}
	//if (find(scouts.begin(), scouts.end(), worker.unit()) != scouts.end())
		//Broodwar->drawCircleMap(worker.getPosition(), 8, Colors::Yellow, true);

	// Assign a resource
	if (shouldAssign(worker))
		assign(worker);

	// Choose a command
	if (shouldReturnCargo(worker))
		returnCargo(worker);
	else if (shouldClearPath(worker))
		clearPath(worker);
	else if (shouldBuild(worker))
		build(worker);
	else if (shouldScout(worker))
		scout(worker);
	else if (shouldFight(worker))
		fight(worker);
	else if (shouldGather(worker))
		gather(worker);
}

bool WorkerManager::shouldAssign(WorkerInfo& worker)
{
	if (find(scouts.begin(), scouts.end(), worker.unit()) != scouts.end())
		return false;

	if (!worker.hasResource() && (!Resources().isMinSaturated() || !Resources().isGasSaturated()))
		return true;
	else if ((!Resources().isGasSaturated() && BuildOrder().isOpener() && gasWorkers < BuildOrder().gasWorkerLimit()) || (BuildOrder().isOpener() && gasWorkers > BuildOrder().gasWorkerLimit()) || (!BuildOrder().isOpener() && !Resources().isGasSaturated()))
		return true;
	else if (worker.hasResource() && !worker.getResource().getType().isMineralField() && Broodwar->self()->visibleUnitCount(worker.getType()) <= 10)
		return true;
	return false;
}

bool WorkerManager::shouldBuild(WorkerInfo& worker)
{
	if (worker.getBuildingType().isValid() && worker.getBuildPosition().isValid())
		return true;
	return false;
}

bool WorkerManager::shouldClearPath(WorkerInfo& worker)
{
	UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
	if (Broodwar->getFrameCount() < 10000)
		return false;

	for (auto &b : Resources().getMyBoulders()) {
		ResourceInfo boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists()) continue;
		if (worker.getPosition().getDistance(boulder.getPosition()) < 320) {
			return true;
		}
	}
	return false;
}

bool WorkerManager::shouldFight(WorkerInfo& worker)
{
	if (worker.getTransport())
		return false;
	if (Util().reactivePullWorker(worker.unit()) || (Util().proactivePullWorker(worker.unit()) && worker.unit() == getClosestWorker(Terrain().getDefendPosition(), true)))
		return true;
	if (Util().pullRepairWorker(worker.unit()) && worker.unit() == getClosestWorker(Terrain().getDefendPosition(), true))
		return true;
	return false;
}

bool WorkerManager::shouldGather(WorkerInfo& worker)
{
	if (worker.hasResource() && (worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() != worker.getResource().unit() && worker.getResource().getState() == 2)
		return true;
	else if (worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather)
		return true;
	return false;
}

bool WorkerManager::shouldReturnCargo(WorkerInfo& worker)
{
	// Don't return cargo if we're on an island and just mined a blocking mineral
	if (worker.getBuildingType().isResourceDepot() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getBuildPosition()) && !mapBWEM.GetArea(worker.getBuildPosition())->AccessibleFrom(mapBWEB.getMainArea()) && worker.getPosition().getDistance((Position)worker.getBuildPosition()) < 160.0)
		return false;
	if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
		return true;
	return false;
}

bool WorkerManager::shouldScout(WorkerInfo& worker)
{
	if (worker.getBuildPosition().isValid())
		return false;
	if (Units().getEnemyCount(UnitTypes::Terran_Vulture) >= 2 && !BuildOrder().firstReady())
		return false;
	else if (deadScoutFrame > 0 && Broodwar->getFrameCount() - deadScoutFrame < 2000)
		return false;
	else if (find(scouts.begin(), scouts.end(), worker.unit()) != scouts.end() && worker.getBuildingType() == UnitTypes::None && BuildOrder().shouldScout())
		return true;
	return false;
}

void WorkerManager::assign(WorkerInfo& worker)
{
	// Remove current assignment if it has one
	if (worker.hasResource()) {
		worker.getResource().setGathererCount(worker.getResource().getGathererCount() - 1);
		worker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	double distBest = DBL_MAX;
	ResourceInfo* bestResource = nullptr;

	// Check if we need gas workers
	if (Broodwar->self()->visibleUnitCount(worker.getType()) > 10 && ((!Resources().isGasSaturated() && BuildOrder().isOpener() && gasWorkers < BuildOrder().gasWorkerLimit()) || (!BuildOrder().isOpener() && !Resources().isGasSaturated()) || Resources().isMinSaturated())) {

		for (auto &g : Resources().getMyGas()) {
			ResourceInfo &gas = g.second;
			double dist = gas.getPosition().getDistance(worker.getPosition());
			if (dist < distBest && gas.getType() != UnitTypes::Resource_Vespene_Geyser && gas.unit()->isCompleted() && gas.getGathererCount() < 3 && gas.getState() > 0)
				bestResource = &gas, distBest = dist;
		}
		if (bestResource) {
			bestResource->setGathererCount(bestResource->getGathererCount() + 1);
			worker.setResource(bestResource);
			gasWorkers++;
			return;
		}
	}

	// Check if we need mineral workers
	else
	{
		for (int i = 1; i <= 2; i++) {
			for (auto &m : Resources().getMyMinerals()) {
				ResourceInfo &mineral = m.second;
				double dist = mineral.getPosition().getDistance(worker.getPosition());
				if (dist < distBest && mineral.getGathererCount() < i && mineral.getState() > 0)
					bestResource = &mineral, distBest = dist;
			}
			if (bestResource) {
				bestResource->setGathererCount(bestResource->getGathererCount() + 1);
				worker.setResource(bestResource);
				minWorkers++;
				return;
			}
		}
	}

	worker.setResource(nullptr);
}

void WorkerManager::build(WorkerInfo& worker)
{
	Position center = Position(worker.getBuildPosition()) + Position(worker.getBuildingType().tileWidth() * 16, worker.getBuildingType().tileHeight() * 16);

	// Spider mine removal
	if (worker.getBuildingType().isResourceDepot()) {
		if (Unit mine = Broodwar->getClosestUnit(Position(worker.getBuildPosition()), Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine, 128)) {
			if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit || !worker.unit()->getLastCommand().getTarget() || !worker.unit()->getLastCommand().getTarget()->exists()) {
				worker.unit()->attack(mine);
			}
			return;
		}

		if (UnitInfo* unit = Util().getClosestEnemyUnit(worker, !Filter::IsNeutral)) {
			if (unit->getPosition().getDistance(worker.getPosition()) < 160
				&& unit->getTilePosition().x >= worker.getBuildPosition().x
				&& unit->getTilePosition().x < worker.getBuildPosition().x + worker.getBuildingType().tileWidth()
				&& unit->getTilePosition().y >= worker.getBuildPosition().y
				&& unit->getTilePosition().y < worker.getBuildPosition().y + worker.getBuildingType().tileHeight())
			{
				worker.unit()->attack(unit->unit());
				return;
			}
		}
	}

	// If our building desired has changed recently, remove TODO
	if (!worker.unit()->isConstructing() && (BuildOrder().buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings().isBuildable(worker.getBuildingType(), worker.getBuildPosition()))) {
		worker.setBuildingType(UnitTypes::None);
		worker.setBuildPosition(TilePositions::Invalid);
		worker.unit()->stop();
	}

	else if (worker.getPosition().getDistance(Position(worker.getBuildPosition())) > 128 && Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() - (((Resources().getIncomeMineral() / 60.0) * worker.getPosition().getDistance(Position(worker.getBuildPosition()))) / (worker.getType().topSpeed() * 24.0)) && Broodwar->self()->minerals() <= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice() - (((Resources().getIncomeGas() / 60.0) * worker.getPosition().getDistance(Position(worker.getBuildPosition()))) / (worker.getType().topSpeed() * 24.0)) && Broodwar->self()->gas() <= worker.getBuildingType().gasPrice())
	{
		safeMove(worker);
	}
	else if (Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice())	{
		if (worker.getPosition().getDistance(Position(worker.getBuildPosition())) > 320)
			safeMove(worker);
		else if (worker.unit()->getOrder() != Orders::PlaceBuilding || worker.unit()->isIdle()) {
			worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());			
		}
		//Broodwar->drawTextMap(worker.getPosition(), "%s", worker.unit()->getOrder().c_str());
	}
}

void WorkerManager::clearPath(WorkerInfo& worker)
{
	//Broodwar->drawTextMap(worker.getPosition(), "Clear");

	for (auto &b : Resources().getMyBoulders()) {
		ResourceInfo &boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists())
			continue;
		if (worker.getPosition().getDistance(boulder.getPosition()) >= 320)
			continue;

		if (worker.unit()->getOrderTargetPosition() != b.second.getPosition() && !worker.unit()->isGatheringMinerals())
			worker.unit()->gather(b.first);
		return;
	}
}

void WorkerManager::fight(WorkerInfo& worker)
{
	Units().storeAlly(worker.unit());
}

void WorkerManager::gather(WorkerInfo& worker)
{
	//Broodwar->drawTextMap(worker.getPosition(), "Gather");
	if (worker.hasResource() && worker.getResource().getState() == 2) {
		if (worker.getResource().unit() && worker.getResource().unit()->exists() && worker.getPosition().getDistance(worker.getResource().getPosition()) < 320.0)
			worker.unit()->gather(worker.getResource().unit());
		else if (worker.getResource().getPosition().isValid())
			safeMove(worker);
	}
	else if (worker.unit()->isIdle() && worker.unit()->getClosestUnit(Filter::IsMineralField))
		worker.unit()->gather(worker.unit()->getClosestUnit(Filter::IsMineralField));
}

void WorkerManager::returnCargo(WorkerInfo& worker)
{
	//Broodwar->drawTextMap(worker.getPosition(), "Return");
	if (worker.unit()->getOrder() != Orders::ReturnMinerals && worker.unit()->getOrder() != Orders::ReturnGas && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
		worker.unit()->returnCargo();
}

void WorkerManager::scout(WorkerInfo& worker)
{
	WalkPosition start = worker.getWalkPosition();

	// All walkpositions in a 4x4 walkposition grid are set as scouted already to prevent overlapping
	for (int x = start.x - 8; x < start.x + 12; x++) {
		for (int y = start.y - 8; y < start.y + 12; y++) {
			WalkPosition w(x, y);
			Position p = Position(w) + Position(4, 4);

			if (w.isValid() && p.getDistance(worker.getPosition()) < 64.0)
				recentExplorations[w] = Broodwar->getFrameCount();
		}
	}


	double distBest = DBL_MAX;
	Position posBest;

	// If first not ready
	// Look at scout targets
	// 1) Get closest scout target that isn't assigned yet
	// 2) If no scout assignments, check if we know where the enemy is, if we do, scout their main
	// 3) If we don't know, scout any unexplored starting locations

	worker.setDestination(Positions::Invalid);
	//Broodwar->drawCircleMap(mapBWEB.getMainPosition(), 5, Colors::Green);

	if (!BuildOrder().firstReady()) {

		if (Strategy().enemyProxy() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) == 0) {
			UnitInfo* enemyWorker = Util().getClosestEnemyUnit(worker, Filter::IsWorker);

			if (enemyWorker && enemyWorker->getPosition().isValid() && Terrain().isInAllyTerritory(enemyWorker->getTilePosition()) && worker.getPosition().getDistance(enemyWorker->getPosition()) < 320.0 && scoutAssignments.find(enemyWorker->getPosition()) == scoutAssignments.end()) {

				worker.setDestination(enemyWorker->getPosition());
				scoutAssignments.insert(enemyWorker->getPosition());

				if (enemyWorker->unit() && enemyWorker->unit()->exists()) {
					worker.unit()->attack(enemyWorker->unit());
					return;
				}
			}
			else if (scoutAssignments.find(mapBWEB.getMainPosition()) == scoutAssignments.end()) {
				worker.setDestination(mapBWEB.getMainPosition());
				scoutAssignments.insert(mapBWEB.getMainPosition());
			}

			//Broodwar->drawCircleMap(worker.getPosition(), 10, Colors::Blue);
		}

		else if (Terrain().getEnemyStartingTilePosition().isValid()) {
			auto enemyMain = mapBWEB.getClosestStation(Terrain().getEnemyStartingTilePosition());

			if (enemyMain && mapBWEM.GetArea(enemyMain->BWEMBase()->Location()) != mapBWEM.GetArea(worker.getTilePosition()))
				worker.setDestination((Position)Terrain().getEnemyNatural());
		}

		// If we have scout targets, find the closest target
		else if (!Strategy().getScoutTargets().empty()) {
			for (auto &target : Strategy().getScoutTargets()) {
				double dist = target.getDistance(worker.getPosition());

				if (dist < distBest && scoutAssignments.find(target) == scoutAssignments.end()) {
					distBest = dist;
					posBest = target;
				}
			}
			if (posBest.isValid()) {
				worker.setDestination(posBest);
				scoutAssignments.insert(posBest);
			}

			//Broodwar->drawCircleMap(worker.getPosition(), 10, Colors::Green);
		}

		// If we have seen the enemy, scout their main
		if (!worker.getDestination().isValid() && Terrain().getEnemyStartingPosition().isValid() /*&& scoutAssignments.find(Terrain().getEnemyStartingPosition()) == scoutAssignments.end()*/) {
			worker.setDestination(Terrain().getEnemyStartingPosition());
			scoutAssignments.insert(Terrain().getEnemyStartingPosition());

			//Broodwar->drawCircleMap(worker.getPosition(), 10, Colors::Red);
		}

		// Find the enemy
		else if (!worker.getDestination().isValid()) {
			for (auto &tile : mapBWEM.StartingLocations()) {
				Position center = Position(tile) + Position(64, 48);
				double dist = center.getDistance(mapBWEB.getMainPosition());

				if (!Broodwar->isExplored(tile) && dist < distBest && scoutAssignments.find(center) == scoutAssignments.end()) {
					distBest = dist;
					posBest = center;
				}
			}
			if (posBest.isValid()) {
				worker.setDestination(posBest);
				scoutAssignments.insert(posBest);
			}

			//Broodwar->drawCircleMap(worker.getPosition(), 10, Colors::Yellow);
		}

		//Broodwar->drawLineMap(worker.getDestination(), worker.getPosition(), Colors::Blue);
		if (worker.getDestination().isValid())
			explore(worker);
	}
	else
	{
		int best = 0;
		for (auto &area : mapBWEM.Areas()) {
			for (auto &base : area.Bases()) {
				if (area.AccessibleNeighbours().size() == 0 || Terrain().isInEnemyTerritory(base.Location()) || Terrain().isInAllyTerritory(base.Location()))
					continue;

				int time = Broodwar->getFrameCount() - recentExplorations[WalkPosition(base.Location())];
				if (time > best)
					best = time, posBest = Position(base.Location());
			}
		}
		if (posBest.isValid() && worker.unit()->getOrderTargetPosition() != posBest)
			worker.unit()->move(posBest);
	}
}

void WorkerManager::explore(WorkerInfo& worker)
{
	WalkPosition start = worker.getWalkPosition();
	Position bestPosition = mapBWEB.getMainPosition();
	int longest = 0;
	UnitInfo* enemy = Util().getClosestEnemyUnit(worker);
	double test = 0.0;	

	if (worker.getPosition().getDistance(worker.getDestination()) > 960.0 && (!enemy || (enemy && enemy->getPosition().isValid() && enemy->getPosition().getDistance(worker.getPosition()) > 320.0))) {
		bestPosition = worker.getDestination();
	}
	else
	{
		// Check a 8x8 walkposition grid for a potential new place to scout
		double best = 0.0;

		for (int x = start.x - 8; x < start.x + 12; x++) {
			for (int y = start.y - 8; y < start.y + 12; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				TilePosition t(w);

				if (!w.isValid()
					|| !Util().isMobile(start, w, worker.getType())
					|| p.getDistance(worker.getPosition()) > 64.0)
					continue;

				double mobility = double(Grids().getMobility(w));
				double threat = max(1.0, Grids().getEGroundThreat(w));
				double distance = log(max(64.0, mapBWEB.getGroundDistance(p, worker.getDestination())));
				double time = log(5.0 + (double(Broodwar->getFrameCount() - recentExplorations[w]) / 5000.0));

				if (worker.getDestination() == mapBWEB.getMainPosition() && (mapBWEM.GetArea(t) == mapBWEB.getMainArea() || (Broodwar->getFrameCount() > 4000 && mapBWEM.GetTile(t).GroundHeight() > mapBWEM.GetTile(mapBWEB.getMainTile()).GroundHeight())))
					distance = 1.0;

				if (!Broodwar->isExplored(t))
					time = 5.0;

				double score = Util().isSafe(w, worker.getType(), true, false) ? mobility * time / distance : (mobility * time) / (threat * distance);

				if (score >= best) {
					best = score;
					bestPosition = p;
					test = time;
				}
			}
		}
	}

	if (bestPosition.isValid() && bestPosition != Position(start) && worker.unit()->getLastCommand().getTargetPosition() != bestPosition) {
		worker.unit()->move(bestPosition);
		//Broodwar->drawLineMap(worker.getPosition(), bestPosition, Colors::Green);
		//Broodwar->drawTextMap(bestPosition, "%.2f", test);
	}
}

void WorkerManager::safeMove(WorkerInfo& worker)
{
	UnitInfo* enemy = Util().getClosestEnemyUnit(worker, !Filter::IsWorker && Filter::CanAttack);
	Position bestPosition(worker.getBuildPosition());

	if (!worker.getBuildPosition().isValid())
		bestPosition = worker.getResource().getPosition();

	//Broodwar->drawTextMap(worker.getPosition(), "%s", worker.unit()->getOrder().c_str());

	if (enemy && enemy->getPosition().getDistance(worker.getPosition()) < 320.0) {
		WalkPosition start = worker.getWalkPosition();
		WalkPosition destination(worker.getBuildPosition());
		double best = 0.0;

		if (!worker.getBuildPosition().isValid())
			destination = (WalkPosition)worker.getResource().getPosition();

		for (int x = start.x - 8; x < start.x + 12; x++) {
			for (int y = start.y - 8; y < start.y + 12; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);

				if (!w.isValid()
					|| !Util().isMobile(start, w, worker.getType()))
					continue;

				double threat = max(0.01, Grids().getEGroundThreat(w));
				double distance = 1.0 + mapBWEB.getGroundDistance(p, worker.getDestination());

				double score = 1.0 / (threat * distance);
				if (score >= best) {
					best = score;
					bestPosition = Position(w);
				}
			}
		}
	}

	if (worker.unit()->getLastCommand().getTargetPosition() != bestPosition || worker.unit()->isIdle())
		worker.unit()->move(bestPosition);
}

Unit WorkerManager::getClosestWorker(Position here, bool isRemoving)
{
	// Currently gets the closest worker that doesn't mine gas
	Unit closestWorker = nullptr;
	double closestD = DBL_MAX;
	for (auto &w : myWorkers) {
		WorkerInfo &worker = w.second;

		if (worker.hasResource() && !worker.getResource().getType().isMineralField()) continue;
		if (isRemoving && worker.getBuildPosition().isValid()) continue;
		if (find(scouts.begin(), scouts.end(), worker.unit()) != scouts.end()) continue;
		if (worker.unit()->getLastCommand().getType() == UnitCommandTypes::Gather && worker.unit()->getLastCommand().getTarget()->exists() && worker.unit()->getLastCommand().getTarget()->getInitialResources() == 0)	continue;
		if (worker.getType() != UnitTypes::Protoss_Probe && worker.unit()->isConstructing()) continue;

		double dist = mapBWEB.getGroundDistance(worker.getPosition(), here);
		if (dist == DBL_MAX)
			dist = worker.getPosition().getDistance(here);

		if (dist < closestD) {
			closestWorker = worker.unit();
			closestD = dist;
		}
	}
	return closestWorker;
}

void WorkerManager::storeWorker(Unit unit)
{
	myWorkers[unit].setUnit(unit);
}

void WorkerManager::removeWorker(Unit worker)
{
	WorkerInfo& thisWorker = myWorkers[worker];
	if (thisWorker.hasResource()) {
		thisWorker.getResource().setGathererCount(thisWorker.getResource().getGathererCount() - 1);
		thisWorker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	if (find(scouts.begin(), scouts.end(), worker) != scouts.end()) {
		deadScoutFrame = Broodwar->getFrameCount();
		scouts.erase(find(scouts.begin(), scouts.end(), worker));
	}
	myWorkers.erase(worker);
}