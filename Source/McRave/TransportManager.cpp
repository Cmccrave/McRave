#include "McRave.h"

void TransportManager::onFrame()
{
	Display().startClock();
	updateTransports();
	Display().performanceTest(__FUNCTION__);
}

void TransportManager::updateTransports()
{
	for (auto &u : Units().getMyUnits()) {
		auto &unit = u.second;

		if (unit.getRole() == Role::Transporting) {
			updateCargo(unit);
			updateDecision(unit);
			updateMovement(unit);
		}
	}
}

void TransportManager::updateCargo(UnitInfo& transport)
{
	auto cargoSize = 0;
	for (auto &u : transport.getAssignedCargo())
		cargoSize += u->getType().spaceRequired();

	// Check if we are ready to assign this worker to a transport
	const auto readyToAssignWorker = [&](UnitInfo& unit) {
		if (cargoSize + unit.getType().spaceRequired() > 8 || unit.hasTransport())
			return false;

		return false;

		auto buildDist = unit.getBuildingType().isValid() ? BWEB::Map::getGroundDistance((Position)unit.getBuildPosition(), (Position)unit.getTilePosition()) : 0.0;
		auto resourceDist = unit.hasResource() ? BWEB::Map::getGroundDistance(unit.getPosition(), unit.getResource().getPosition()) : 0.0;

		if ((unit.getBuildPosition().isValid() && buildDist == DBL_MAX) || (unit.getBuildingType().isResourceDepot() && Terrain().isIslandMap()))
			return true;
		if (unit.hasResource() && resourceDist == DBL_MAX)
			return true;
		return false;
	};

	// Check if we are ready to assign this unit to a transport
	const auto readyToAssignUnit = [&](UnitInfo& unit) {
		if (cargoSize + unit.getType().spaceRequired() > 8 || unit.hasTransport())
			return false;

		auto targetDist = BWEB::Map::getGroundDistance(unit.getPosition(), unit.getEngagePosition());

		if (unit.getType() == UnitTypes::Protoss_Reaver || unit.getType() == UnitTypes::Protoss_High_Templar)
			return true;
		if (Terrain().isIslandMap() && !unit.getType().isFlyer() && targetDist > 640.0)
			return true;
		return false;
	};

	// Update cargo information
	if (cargoSize < 8) {
		for (auto &u : Units().getMyUnits()) {
			auto &unit = u.second;

			if (unit.getRole() == Role::Fighting && readyToAssignUnit(unit)) {
				unit.setTransport(&transport);
				transport.getAssignedCargo().insert(&unit);
				cargoSize += unit.getType().spaceRequired();
			}

			if (unit.getRole() == Role::Working && readyToAssignWorker(unit)) {
				unit.setTransport(&transport);
				transport.getAssignedCargo().insert(&unit);
				cargoSize += unit.getType().spaceRequired();
			}
		}
	}
}

void TransportManager::updateDecision(UnitInfo& transport)
{
	// TODO: Broke transports for islands, fix later
	transport.setDestination(BWEB::Map::getMainPosition());
	transport.setTransportState(TransportState::None);

	UnitInfo * closestCargo = nullptr;
	auto distBest = DBL_MAX;
	for (auto &c : transport.getAssignedCargo()) {
		auto dist = c->getDistance(transport);
		if (dist < distBest) {
			distBest = dist;
			closestCargo = c;
		}
	}

	// Check if this unit is ready to fight
	const auto readyToFight = [&](UnitInfo& cargo) {
		auto attackCooldown = (Broodwar->getFrameCount() - cargo.getLastAttackFrame() <= 60 - Broodwar->getRemainingLatencyFrames());

		auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
		auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;

		// TESTING THIS:
		if (Util().getHighestThreat(transport.getWalkPosition(), transport) < 1.0)
			return true;

		if (cargo.getLocalState() == LocalState::Retreating || transport.unit()->isUnderAttack() || (cargo.getShields() == 0 && cargo.getSimValue() < 1.2))
			return false;
		if (cargo.getLocalState() == LocalState::Engaging && ((reaver && !attackCooldown) || (ht && cargo.getEnergy() >= 75)))
			return true;
		return false;
	};

	// Check if this unit is ready to unload
	const auto readyToUnload = [&](UnitInfo& cargo) {
		auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
		auto targetDist = cargo.getPosition().getDistance(cargo.getEngagePosition());
		if (targetDist <= 64.0)
			return true;
		return false;
	};

	// Check if this unit is ready to be picked up
	const auto readyToPickup = [&](UnitInfo& cargo) {
		auto attackCooldown = (Broodwar->getFrameCount() - cargo.getLastAttackFrame() <= 60 - Broodwar->getRemainingLatencyFrames());
		auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
		auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;
		auto threat = Grids().getEGroundThreat(cargo.getWalkPosition()) > 0.0;
		auto targetDist = reaver && cargo.hasTarget() ? BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getTarget().getPosition()) - 256.0 : cargo.getPosition().getDistance(cargo.getEngagePosition());

		if (transport.getPosition().getDistance(cargo.getPosition()) <= 160.0 || &cargo == closestCargo) {
			if (!cargo.hasTarget() || cargo.getLocalState() == LocalState::Retreating || (targetDist > 128.0 || (ht && cargo.unit()->getEnergy() < 75) || (reaver && attackCooldown && threat))) {
				return true;
			}
		}
		return false;
	};

	// Check if this worker is ready to mine
	const auto readyToMine = [&](UnitInfo& worker) {
		if (Terrain().isIslandMap() && worker.hasResource() && worker.getResource().getTilePosition().isValid() && mapBWEM.GetArea(transport.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition()))
			return true;
		return false;
	};

	// Check if this worker is ready to build
	const auto readyToBuild = [&](UnitInfo& worker) {
		if (Terrain().isIslandMap() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getBuildPosition()))
			return true;
		return false;
	};

	const auto pickupPosition = [&](UnitInfo& cargo) {
		double distance = transport.getPosition().getDistance(cargo.getPosition());
		Position direction = (transport.getPosition() - cargo.getPosition()) * 64 / (int)distance;
		return cargo.getPosition() - direction;
	};

	// Check if we should be loading/unloading any cargo	
	bool shouldMonitor = false;
	for (auto &c : transport.getAssignedCargo()) {
		auto &cargo = *c;

		// If the cargo is not loaded
		if (!cargo.unit()->isLoaded()) {

			// If it's requesting a pickup, set load state to 1	
			if (readyToPickup(cargo)) {
				transport.setTransportState(TransportState::Loading);
				transport.setDestination(pickupPosition(cargo));
				cargo.unit()->rightClick(transport.unit());
				return;
			}
			else {
				transport.setDestination(cargo.getPosition());
				shouldMonitor = true;
			}
		}

		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded() && cargo.hasTarget() && cargo.getEngagePosition().isValid()) {
			transport.setDestination(cargo.getEngagePosition());

			if (readyToFight(cargo)) {
				transport.setTransportState(TransportState::Engaging);

				if (readyToUnload(cargo))
					transport.unit()->unload(cargo.unit());
			}
			else
				transport.setTransportState(TransportState::Retreating);
		}

		// Dont attack until we're ready
		else if (cargo.getGlobalState() == GlobalState::Retreating)
			transport.setDestination(BWEB::Map::getNaturalPosition());
	}

	if (shouldMonitor)
		transport.setTransportState(TransportState::Monitoring);

	//for (auto &w : transport.getAssignedWorkers()) {
	//	bool miner = false;
	//	bool builder = false;

	//	if (!w || !w->unit())
	//		continue;

	//	auto &worker = *w;
	//	builder = worker.getBuildPosition().isValid();
	//	miner = !worker.getBuildPosition().isValid() && worker.hasResource();

	//	if (worker.unit()->exists() && !worker.unit()->isLoaded() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
	//		if ((miner && readyToMine(worker)) || (builder && readyToBuild(worker))) {
	//			transport.removeWorker(&worker);
	//			worker.setTransport(nullptr);
	//			return;
	//		}
	//		else if (transport.unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(worker.getPosition()) < 320.0) {
	//			transport.setLoading(true);
	//			transport.unit()->load(worker.unit());
	//			return;
	//		}
	//	}
	//	else {
	//		double airDist = DBL_MAX;
	//		double grdDist = DBL_MAX;

	//		if (worker.getBuildPosition().isValid())
	//			transport.setDestination((Position)worker.getBuildPosition());

	//		else if (worker.hasResource())
	//			transport.setDestination(worker.getResource().getPosition());

	//		if ((miner && readyToMine(worker)) || (builder && readyToBuild(worker)))
	//			transport.unit()->unload(worker.unit());
	//	}
	//}

	//if (transport.getDestination() == BWEB::Map::getMainPosition() && Terrain().isIslandMap()) {
	//	double distBest = DBL_MAX;
	//	Position posBest = Positions::None;
	//	for (auto &tile : mapBWEM.StartingLocations()) {
	//		Position center = Position(tile) + Position(64, 48);
	//		double dist = center.getDistance(BWEB::Map::getMainPosition());

	//		if (!Broodwar->isExplored(tile) && dist < distBest) {
	//			distBest = dist;
	//			posBest = center;
	//		}
	//	}
	//	if (posBest.isValid()) {
	//		transport.setDestination(posBest);
	//	}
	//}
}

void TransportManager::updateMovement(UnitInfo& transport)
{
	// Check if the destination can be used for ground distance
	auto dropTarget = transport.getDestination();
	if (!Util().isWalkable(dropTarget) || BWEB::Map::getGroundDistance(transport.getPosition(), dropTarget) == DBL_MAX) {
		auto distBest = DBL_MAX;
		for (auto &cargo : transport.getAssignedCargo()) {
			if (cargo->hasTarget()) {
				auto cargoTarget = cargo->getTarget().getPosition();
				auto dist = cargoTarget.getDistance(transport.getPosition());
				if (Util().isWalkable(cargoTarget) && dist < distBest) {
					dropTarget = cargoTarget;
					distBest = dist;
				}
			}
		}
	}

	auto bestPos = Positions::Invalid;
	auto start = transport.getWalkPosition();
	auto best = 0.0;

	for (int x = start.x - 24; x < start.x + 24 + transport.getType().tileWidth() * 4; x++) {
		for (int y = start.y - 24; y < start.y + 24 + transport.getType().tileWidth() * 4; y++) {
			WalkPosition w(x, y);
			Position p = Position(w) + Position(4, 4);
			TilePosition t(p);

			if (!w.isValid()
				|| (transport.getTransportState() == TransportState::Engaging && !Util().isWalkable(start, w, UnitTypes::Protoss_Reaver))
				|| (transport.getTransportState() == TransportState::Engaging && Broodwar->getGroundHeight(TilePosition(w)) < Broodwar->getGroundHeight(TilePosition(dropTarget))))
				continue;

			// If we just dropped units, we need to make sure not to leave them
			if (transport.getTransportState() == TransportState::Monitoring) {
				bool proximity = true;
				for (auto &u : transport.getAssignedCargo()) {
					if (!u->unit()->isLoaded() && u->getPosition().getDistance(p) > 32.0)
						proximity = false;
				}
				if (!proximity)
					continue;
			}

			double threat = Util().getHighestThreat(w, transport);
			double distance = (transport.getType().isFlyer() ? p.getDistance(transport.getDestination()) : BWEB::Map::getGroundDistance(p, transport.getDestination()));
			double visited = log(min(500.0, double(Broodwar->getFrameCount() - Grids().lastVisitedFrame(w))));
			double score = visited / (threat * distance);

			if (score > best) {
				best = score;
				bestPos = p;
			}
		}
	}

	if (bestPos.isValid())
		transport.command(UnitCommandTypes::Move, bestPos);
	else
		Commands().move(transport);
}

void TransportManager::removeUnit(Unit unit)
{
	for (auto &t : Units().getMyUnits()) {
		auto &transport = t.second;

		for (auto &cargo : transport.getAssignedCargo()) {

			// Remove cargo if cargo died
			if (cargo->unit() == unit) {
				transport.getAssignedCargo().erase(cargo);
				return;
			}

			// Set transport as nullptr if the transport died
			if (cargo->hasTransport() && cargo->getTransport().unit() == unit)
				cargo->setTransport(nullptr);
		}
	}
}