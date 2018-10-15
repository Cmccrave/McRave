#include "McRave.h"

void WorkerManager::onFrame()
{
	Display().startClock();
	updateWorkers();
	Display().performanceTest(__FUNCTION__);
}

void WorkerManager::updateWorkers()
{
	for (auto &w : Units().getMyUnits()) {
		auto &worker = w.second;
		if (worker.getRole() == Role::Working)
			updateDecision(worker);
	}
}

void WorkerManager::updateDecision(UnitInfo& worker)
{
	// Workers that have a transport coming to pick them up should not do anything other than returning cargo
	if (worker.hasTransport() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
		if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
			worker.unit()->move(worker.getTransport().getPosition());
		return;
	}

	// If worker is potentially stuck, try to find a manner pylon
	if (worker.framesHoldingResource() >= 100 || worker.framesHoldingResource() <= -200) {
		auto pylon = Util().getClosestUnit(worker.getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Pylon);
		if (pylon && pylon->unit() && pylon->unit()->exists()) {
			if (worker.unit()->getLastCommand().getTarget() != pylon->unit())
				worker.unit()->attack(pylon->unit());
			return;
		}
	}

	// Assign a resource
	if (shouldAssign(worker))
		assign(worker);

	// Choose a command
	if (shouldReturnCargo(worker))
		returnCargo(worker);
	else if (shouldClearPath(worker)) {
		Broodwar->drawCircleMap(worker.getPosition(), 8, Colors::Purple, true);
		clearPath(worker);
	}
	else if (shouldBuild(worker))
		build(worker);
	else if (shouldGather(worker))
		gather(worker);
}

bool WorkerManager::shouldAssign(UnitInfo& worker)
{
	if (!worker.hasResource()
		|| needGas()
		|| (worker.hasResource() && !worker.getResource().getType().isMineralField() && (Broodwar->self()->visibleUnitCount(worker.getType()) <= 10 || gasWorkers > BuildOrder().gasWorkerLimit())))
		return true;

	// HACK: Try to make Zerg gather less gas
	if (Broodwar->self()->gas() > Broodwar->self()->minerals() && Broodwar->self()->getRace() == Races::Zerg)
		return true;
	return false;
}

bool WorkerManager::shouldBuild(UnitInfo& worker)
{
	if (worker.getBuildingType().isValid() && worker.getBuildPosition().isValid())
		return true;
	return false;
}

bool WorkerManager::shouldClearPath(UnitInfo& worker)
{
	if (Broodwar->getFrameCount() < 10000)
		return false;

	for (auto &b : Resources().getMyBoulders()) {
		ResourceInfo &boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists())
			continue;
		if (worker.getPosition().getDistance(boulder.getPosition()) < 480.0)
			return true;
	}
	return false;
}

bool WorkerManager::shouldFight(UnitInfo& worker)
{
	if (worker.hasTransport())
		return false;

	// This won't work
	//if (Util().reactivePullWorker(worker.unit()) || (Util().proactivePullWorker(worker.unit()) && &worker == Util().getClosestUnit(Terrain().getDefendPosition(), Broodwar->self(), worker.getType())))
	//	return true;
	//if (Util().pullRepairWorker(worker.unit()) && &worker == Util().getClosestUnit(Terrain().getDefendPosition(), Broodwar->self(), worker.getType()))
	//	return true;
	return false;
}

bool WorkerManager::shouldGather(UnitInfo& worker)
{
	if (worker.hasResource() && (worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() != worker.getResource().unit() && worker.getResource().getState() == 2)
		return true;
	else if (worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather)
		return true;
	return false;
}

bool WorkerManager::shouldReturnCargo(UnitInfo& worker)
{
	// Don't return cargo if we're on an island and just mined a blocking mineral
	if (worker.getBuildingType().isResourceDepot() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getBuildPosition()) && !mapBWEM.GetArea(worker.getBuildPosition())->AccessibleFrom(mapBWEB.getMainArea()) && worker.getPosition().getDistance((Position)worker.getBuildPosition()) < 160.0)
		return false;
	if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
		return true;
	return false;
}

bool WorkerManager::shouldScout(UnitInfo& worker)
{
	//if (worker.getBuildPosition().isValid())
	//	return false;
	//if (Units().getEnemyCount(UnitTypes::Terran_Vulture) >= 2 && !BuildOrder().firstReady())
	//	return false;
	//else if (deadScoutFrame > 0 && Broodwar->getFrameCount() - deadScoutFrame < 2000)
	//	return false;
	//else if (find(scouts.begin(), scouts.end(), worker.unit()) != scouts.end() && worker.getBuildingType() == UnitTypes::None && (BuildOrder().shouldScout() || proxyCheck))
	//	return true;
	//return false;
}

void WorkerManager::assign(UnitInfo& worker)
{
	// Remove current assignment if it has one
	if (worker.hasResource()) {
		worker.getResource().setGathererCount(worker.getResource().getGathererCount() - 1);
		worker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	double distBest = DBL_MAX;
	ResourceInfo* bestResource = nullptr;

	// Check if we need gas workers
	if (needGas()) {
		for (auto &g : Resources().getMyGas()) {
			ResourceInfo &gas = g.second;
			double dist = gas.getPosition().getDistance(worker.getPosition());
			if (dist < distBest && gas.getType() != UnitTypes::Resource_Vespene_Geyser && gas.unit()->exists() && gas.unit()->isCompleted() && gas.getGathererCount() < 3 && gas.getState() > 0)
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
	else {

		// If worker is injured early on, send him to the furthest mineral field
		bool injured = (worker.unit()->getHitPoints() + worker.unit()->getShields() < worker.getType().maxHitPoints() + worker.getType().maxShields());
		if (injured)
			distBest = 0.0;

		for (int i = 1; i <= 2; i++) {
			for (auto &m : Resources().getMyMinerals()) {
				ResourceInfo &mineral = m.second;

				double dist = mineral.getPosition().getDistance(worker.getPosition());
				if (((dist < distBest && !injured) || (dist > distBest && injured)) && mineral.getGathererCount() < i && mineral.getState() > 0)
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

void WorkerManager::build(UnitInfo& worker)
{
	Position center = Position(worker.getBuildPosition()) + Position(worker.getBuildingType().tileWidth() * 16, worker.getBuildingType().tileHeight() * 16);

	const auto shouldMoveToBuild = [&]() {
		auto mineralIncome = (minWorkers - 1) * 0.045;
		auto gasIncome = (gasWorkers - 1) * 0.07;
		auto speed = worker.getType().topSpeed();
		auto dist = mapBWEB.getGroundDistance(worker.getPosition(), center);
		auto time = (dist / speed) + 50.0;
		auto enoughGas = worker.getBuildingType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= worker.getBuildingType().gasPrice() : true;
		auto enoughMins = worker.getBuildingType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= worker.getBuildingType().mineralPrice() : true;

		return enoughGas && enoughMins;
	};

	// Spider mine removal
	if (worker.getBuildingType().isResourceDepot()) {
		UnitInfo * mine = Util().getClosestUnit(center, Broodwar->enemy(), UnitTypes::Terran_Vulture_Spider_Mine);

		if (mine && mine->getPosition().getDistance(center) < 160.0) {
			if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit || !worker.unit()->getLastCommand().getTarget() || !worker.unit()->getLastCommand().getTarget()->exists())
				worker.unit()->attack(mine->unit());
			return;
		}

		// HACK: Attack blocking units that are enemies
		if (UnitInfo* unit = Util().getClosestUnit(worker.getPosition(), Broodwar->enemy())) {
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
	if ((BuildOrder().buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings().isBuildable(worker.getBuildingType(), worker.getBuildPosition()))) {
		worker.setBuildingType(UnitTypes::None);
		worker.setBuildPosition(TilePositions::Invalid);
		worker.unit()->stop();
	}

	else if (shouldMoveToBuild()) {
		worker.circleRed();
		worker.setDestination(center);
		if (worker.getPosition().getDistance(center) > 128.0)
			Commands().safeMove(worker);
		else if (worker.unit()->getOrder() != Orders::PlaceBuilding || worker.unit()->isIdle())
			worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());
	}
}

void WorkerManager::clearPath(UnitInfo& worker)
{
	for (auto &b : Resources().getMyBoulders()) {
		ResourceInfo &boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists())
			continue;
		if (worker.getPosition().getDistance(boulder.getPosition()) >= 480.0)
			continue;

		if (!worker.unit()->isGatheringMinerals()) {
			if (worker.unit()->getOrderTargetPosition() != b.second.getPosition())
				worker.unit()->gather(b.first);
			return;
		}
	}
}

void WorkerManager::gather(UnitInfo& worker)
{	
	if (worker.hasResource() && worker.getResource().getState() == 2) {
		worker.setDestination(worker.getResource().getPosition());
		if (worker.getResource().unit() && worker.getResource().unit()->exists() && (worker.getPosition().getDistance(worker.getResource().getPosition()) < 320.0 || worker.unit()->getShields() < 20))
			worker.unit()->gather(worker.getResource().unit());
		else if (worker.getResource().getPosition().isValid())
			Commands().safeMove(worker);
	}
	else {
		auto closest = worker.unit()->getClosestUnit(Filter::IsMineralField);
		if (closest && closest->exists())
			worker.unit()->gather(closest);
	}
}

void WorkerManager::returnCargo(UnitInfo& worker)
{
	if (worker.unit()->getOrder() != Orders::ReturnMinerals && worker.unit()->getOrder() != Orders::ReturnGas && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
		worker.unit()->returnCargo();
}

bool WorkerManager::needGas() {
	if (Broodwar->self()->gas() > Broodwar->self()->minerals() && Broodwar->self()->getRace() == Races::Zerg)
		return false;

	if (Broodwar->self()->visibleUnitCount(Broodwar->self()->getRace().getWorker()) > 10 && !Resources().isGasSaturated() && ((gasWorkers < BuildOrder().gasWorkerLimit() && BuildOrder().isOpener()) || !BuildOrder().isOpener() || Resources().isMinSaturated()))
		return true;
	return false;
}