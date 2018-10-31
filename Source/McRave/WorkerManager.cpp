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
	else if (shouldClearPath(worker))
		clearPath(worker);
	else if (shouldBuild(worker))
		build(worker);
	else if (shouldGather(worker))
		gather(worker);
}


bool WorkerManager::shouldAssign(UnitInfo& worker)
{
	// Worker has no resource, we need gas, we need minerals, workers resource is threatened
	if (!worker.hasResource()
		|| needGas()
		|| (worker.hasResource() && !worker.getResource().getType().isMineralField() && gasWorkers > BuildOrder().gasWorkerLimit())
		|| (worker.hasResource() && !closeToResource(worker) && Util().accurateThreatOnPath(worker, worker.getTargetPath()) && Grids().getEGroundThreat(worker.getWalkPosition()) == 0.0)
		|| (worker.hasResource() && closeToResource(worker) && Grids().getEGroundThreat(worker.getWalkPosition()) > 0.0)
		|| ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) && worker.hasResource() && Util().quickThreatOnPath(worker, worker.getPosition(), worker.getResource().getPosition())))
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

bool WorkerManager::shouldGather(UnitInfo& worker)
{
	if (worker.hasResource()) {
		if (mapBWEM.GetArea(worker.getTilePosition()) != mapBWEM.GetArea(worker.getResource().getTilePosition()))
			return true;
		if ((worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() != worker.getResource().unit() && worker.getResource().getResourceState() == ResourceState::Mineable)
			return true;
	}
	if (worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather)
		return true;
	return false;
}

bool WorkerManager::shouldReturnCargo(UnitInfo& worker)
{
	// Check if we cant reach our main while building
	auto cantReturn = !worker.getBuildPosition().isValid() || (worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getBuildPosition()) && mapBWEM.GetArea(worker.getBuildPosition())->AccessibleFrom(mapBWEB.getMainArea()));
	if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
		return true;
	return false;
}

void WorkerManager::assign(UnitInfo& worker)
{
	ResourceInfo* bestResource = nullptr;
	auto injured = (worker.unit()->getHitPoints() + worker.unit()->getShields() < worker.getType().maxHitPoints() + worker.getType().maxShields());
	auto threatened = (worker.hasResource() && Util().accurateThreatOnPath(worker, worker.getTargetPath()));
	auto distBest = (injured || threatened) ? 0.0 : DBL_MAX;

	const auto resourceReady = [&](ResourceInfo& resource, int i) {
		if (!resource.unit()
			|| resource.getType() == UnitTypes::Resource_Vespene_Geyser
			|| (resource.unit()->exists() && !resource.unit()->isCompleted())
			|| (resource.getGathererCount() >= i + int(injured || threatened))
			|| resource.getResourceState() == ResourceState::None
			|| ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) && Grids().getEGroundThreat(WalkPosition(resource.getPosition())) > 0.0)
			|| Util().quickThreatOnPath(worker, worker.getPosition(), resource.getPosition()))
			return false;
		return true;
	};

	vector<const Station *> safeStations;
	const Station * closest = mapBWEB.getClosestStation(worker.getTilePosition());
	
	// 1) If threatened, find safe stations to move to on the Station network or generate a new path
	if (threatened) {
		for (auto &s : Stations().getMyStations()) {
			auto station = s.second;
			Path path;
			if (closest && worker.getPosition().getDistance(closest->ResourceCentroid()) < 128.0)
				path = Stations().pathStationToStation(closest, station);
			else
				path.createUnitPath(mapBWEB, mapBWEM, worker.getPosition(), station->ResourceCentroid());

			// Store station if it's safe
			if (!Util().accurateThreatOnPath(worker, path))
				safeStations.push_back(station);
		}
	}

	// 2) Check if we need gas workers
	if (needGas()) {
		for (auto &r : Resources().getMyGas()) {
			auto &resource = r.second;
			if (!resourceReady(resource, 3))
				continue;
			if (threatened && !safeStations.empty() && (!resource.getStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end()))
				continue;

			auto dist = resource.getPosition().getDistance(worker.getPosition());
			if ((dist < distBest && !injured) || (dist > distBest && injured)) {
				bestResource = &resource;
				distBest = dist;
			}
		}
	}

	// 3) Check if we need mineral workers
	else {
		for (int i = 1; i <= 2; i++) {
			for (auto &r : Resources().getMyMinerals()) {
				auto &resource = r.second;
				if (!resourceReady(resource, i))
					continue;
				if (threatened && !safeStations.empty() && (!resource.getStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end()))
					continue;

				double dist = resource.getPosition().getDistance(worker.getPosition());
				if ((dist < distBest && !injured && !threatened) || (dist > distBest && (injured || threatened))) {
					bestResource = &resource;
					distBest = dist;
				}
			}
			if (bestResource)
				break;
		}

	}

	// 3) Assign resource
	if (bestResource) {

		// Remove current assignment
		if (worker.hasResource()) {
			worker.getResource().setGathererCount(worker.getResource().getGathererCount() - 1);
			worker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
		}

		// Add next assignment
		bestResource->setGathererCount(bestResource->getGathererCount() + 1);
		bestResource->getType().isMineralField() ? minWorkers++ : gasWorkers++;

		Path emptyPath;
		worker.setResource(bestResource);
		worker.setTargetPath(emptyPath);
	}
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

	// 1) Attack any enemies inside the build area
	if (worker.hasTarget() &&
		worker.getTarget().getPosition().getDistance(worker.getPosition()) < 160
		&& worker.getTarget().getTilePosition().x >= worker.getBuildPosition().x
		&& worker.getTarget().getTilePosition().x < worker.getBuildPosition().x + worker.getBuildingType().tileWidth()
		&& worker.getTarget().getTilePosition().y >= worker.getBuildPosition().y
		&& worker.getTarget().getTilePosition().y < worker.getBuildPosition().y + worker.getBuildingType().tileHeight())
		Commands().attack(worker);

	// 2) Cancel any buildings we don't need
	else if ((BuildOrder().buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings().isBuildable(worker.getBuildingType(), worker.getBuildPosition()))) {
		worker.setBuildingType(UnitTypes::None);
		worker.setBuildPosition(TilePositions::Invalid);
		worker.unit()->stop();
	}

	// 3) Move to build if we have the resources
	else if (shouldMoveToBuild()) {
		worker.circleRed();
		worker.setDestination(center);
		Broodwar->drawTextMap(worker.getPosition(), "%s", worker.unit()->getOrder().c_str());

		if (worker.getPosition().getDistance(center) > 256.0) {
			if (worker.unit()->getOrderTargetPosition() != center)
				worker.unit()->move(center);
		}
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
	// Mine the closest mineral field
	const auto mineRandom =[&]() {
		auto closest = worker.unit()->getClosestUnit(Filter::IsMineralField);
		if (closest && closest->exists() && worker.unit()->getLastCommand().getTarget() != closest)
			worker.unit()->gather(closest);
	};

	// If the resource is close and mineable
	if (worker.hasResource() && worker.getResource().getResourceState() == ResourceState::Mineable) {

		// 1) If it's close or same area, mine it		
		if (closeToResource(worker) && worker.getResource().unit() && worker.getResource().unit()->exists()) {
			Path emptyPath;
			worker.unit()->gather(worker.getResource().unit());
			worker.setTargetPath(emptyPath);
			return;
		}

		// 2) If it's far, generate a path
		auto resourceCentroid = worker.getResource().getStation() ? worker.getResource().getStation()->ResourceCentroid() : Positions::Invalid;
		if (worker.getLastTile() != worker.getTilePosition() && resourceCentroid.isValid()) {
			Path newPath;
			newPath.createUnitPath(mapBWEB, mapBWEM, worker.getPosition(), resourceCentroid);
			worker.setTargetPath(newPath);
		}

		Display().displayPath(worker, worker.getTargetPath().getTiles());

		// 3) If no threat on path, mine it
		if (!Util().accurateThreatOnPath(worker, worker.getTargetPath())) {
			if (worker.getResource().unit() && worker.getResource().unit()->exists())
				worker.unit()->gather(worker.getResource().unit());
			else
				worker.unit()->move(worker.getResource().getPosition());
			return;
		}

		// 4) If we are under a threat, try to get away from it
		if (Grids().getEGroundThreat(worker.getWalkPosition()) > 0.0)
			Commands().kite(worker);
	}

	// Mine something else while waiting
	mineRandom();
}

void WorkerManager::returnCargo(UnitInfo& worker)
{
	auto checkPath = (worker.hasResource() && worker.getPosition().getDistance(worker.getResource().getPosition()) > 320.0) || (!worker.hasResource() && !Terrain().isInAllyTerritory(worker.getTilePosition()));
	if (checkPath) {
		// TODO: Create a path to the closest station and check if it's safe
	}
	if (worker.unit()->getOrder() != Orders::ReturnMinerals && worker.unit()->getOrder() != Orders::ReturnGas && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
		worker.unit()->returnCargo();
}


bool WorkerManager::needGas() {
	if (!Resources().isGasSaturated() && ((gasWorkers < BuildOrder().gasWorkerLimit() && BuildOrder().isOpener()) || !BuildOrder().isOpener() || Resources().isMinSaturated()))
		return true;
	return false;
}

bool WorkerManager::closeToResource(UnitInfo& worker)
{
	auto close = worker.getResource().getPosition().getDistance(worker.getPosition()) < 128.0;
	auto sameArea = mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition());
	return close || sameArea;
}

void WorkerManager::removeUnit(Unit unit)
{
}