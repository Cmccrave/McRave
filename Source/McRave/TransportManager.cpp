#include "McRave.h"

void TransportManager::onFrame()
{
	// HACK: Prevent overlords from spazzing out
	if (Broodwar->self()->getRace() == Races::Zerg)
		return;

	for (auto &t : myTransports) {
		auto &transport = t.second;
		transport.updateTransportInfo();

		if (transport.info()->getRole() == Role::Transport) {
			updateCargo(transport);
			updateDecision(transport);
			updateMovement(transport);
		}
	}
	return;
}

void TransportManager::updateCargo(TransportInfo& transport)
{
	// Check if we are ready to assign this worker to a transport
	const auto readyToAssignWorker = [&](WorkerInfo& worker) {
		if (transport.getCargoSize() + worker.getType().spaceRequired() > 8 || worker.getTransport())
			return false;

		// Temp
		return false;

		auto buildDist = worker.getBuildingType().isValid() ? mapBWEB.getGroundDistance((Position)worker.getBuildPosition(), (Position)worker.getTilePosition()) : 0.0;
		auto resourceDist = worker.hasResource() ? mapBWEB.getGroundDistance(worker.getPosition(), worker.getResource().getPosition()) : 0.0;

		if ((worker.getBuildPosition().isValid() && buildDist == DBL_MAX) || (worker.getBuildingType().isResourceDepot() && Terrain().isIslandMap()))
			return true;
		if (worker.hasResource() && resourceDist == DBL_MAX)
			return true;
		return false;
	};

	// Check if we are ready to assign this unit to a transport
	const auto readyToAssignUnit = [&](UnitInfo& unit) {
		if (transport.getCargoSize() + unit.getType().spaceRequired() > 8 || unit.getTransport())
			return false;

		auto targetDist = mapBWEB.getGroundDistance(unit.getPosition(), unit.getEngagePosition());

		// Only assign units that are close, or if the shuttle is empty
		if (transport.info()->unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(unit.getPosition()) < 320.0) {
			if (!Terrain().isIslandMap() && (unit.getType() == UnitTypes::Protoss_Reaver /*|| unit.getType() == UnitTypes::Protoss_High_Templar*/))
				return true;
			if (Terrain().isIslandMap() && !unit.getType().isFlyer() && targetDist > 640.0)
				return true;
		}
		return false;
	};

	// Update cargo information
	if (transport.getCargoSize() < 8 && !transport.isUnloading()) {

		// See if any units need a shuttle
		for (auto &u : Units().getMyUnits()) {
			UnitInfo &unit = u.second;

			if (unit.getRole() == Role::Troop && readyToAssignUnit(unit)) {
				unit.setTransport(&transport);
				transport.assignCargo(&unit);
			}
		}

		/*for (auto &u : Workers().getMyWorkers()) {
			WorkerInfo &unit = u.second;

			if (unit.info()->getRole() == Role::Worker && readyToAssignWorker(unit)) {
				unit.setTransport(&transport);
				transport.assignCargo(&unit);
			}
		}*/
	}
}

void TransportManager::updateDecision(TransportInfo& transport)
{
	// TODO: Broke transports for islands, fix later

	// Check if this unit is ready to fight
	const auto readyToFight = [&](UnitInfo& cargo) {
		auto attackCooldown = (Broodwar->getFrameCount() - cargo.getLastAttackFrame() <= 60 - Broodwar->getRemainingLatencyFrames());
		auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
		auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;

		if (cargo.shouldRetreat() || transport.info()->unit()->isUnderAttack() || (cargo.getShields() == 0 && cargo.getSimValue() < 1.2))
			return false;
		if (Terrain().isIslandMap() && cargo.hasTarget() && cargo.getTarget().getTilePosition().isValid() && Broodwar->getGroundHeight(transport.getTilePosition()) == Broodwar->getGroundHeight(cargo.getTarget().getTilePosition()))
			return true;
		if (!Terrain().isIslandMap() && cargo.shouldEngage() && ((ht && cargo.unit()->getEnergy() >= 75) || (reaver && !attackCooldown)))
			return true;
		return false;
	};

	// Check if this unit is ready to unload
	const auto readyToUnload = [&](UnitInfo& cargo) {
		auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
		auto targetDist = reaver && cargo.hasTarget() ? mapBWEB.getGroundDistance(cargo.getPosition(), cargo.getTarget().getPosition()) - 256.0 : cargo.getPosition().getDistance(cargo.getEngagePosition());
		if (cargo.shouldEngage() && targetDist <= 128.0)
			return true;
		return false;
	};

	// Check if this unit is ready to be picked up
	const auto readyToPickup = [&](UnitInfo& cargo) {
		auto attackCooldown = (Broodwar->getFrameCount() - cargo.getLastAttackFrame() <= 60 - Broodwar->getRemainingLatencyFrames());
		auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
		auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;
		auto targetDist = reaver && cargo.hasTarget() ? mapBWEB.getGroundDistance(cargo.getPosition(), cargo.getTarget().getPosition()) - 256.0 : cargo.getPosition().getDistance(cargo.getEngagePosition());
		
		if (transport.info()->unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(cargo.getPosition()) < 160.0) {
			if (!cargo.hasTarget() || cargo.shouldRetreat() || !cargo.shouldEngage() || (targetDist > 128.0 || (ht && cargo.unit()->getEnergy() < 75) || (reaver && attackCooldown))) {
				return true;
			}
		}
		return false;
	};

	// Check if this worker is ready to mine
	const auto readyToMine = [&](WorkerInfo& worker) {
		if (Terrain().isIslandMap() && worker.hasResource() && worker.getResource().getTilePosition().isValid() && mapBWEM.GetArea(transport.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition()))
			return true;
		return false;
	};

	// Check if this worker is ready to build
	const auto readyToBuild = [&](WorkerInfo& worker) {
		if (Terrain().isIslandMap() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getBuildPosition()))
			return true;
		return false;
	};

	const auto pickupPosition = [&](UnitInfo& cargo) {
		double distance = transport.getPosition().getDistance(cargo.getPosition());
		Position direction = (transport.getPosition() - cargo.getPosition()) * int(64.0 / distance);
		return cargo.getPosition() - direction;
	};

	// Check if we should be loading/unloading any cargo	
	for (auto &c : transport.getAssignedCargo()) {
		if (!c->unit() || !c->unit()->exists())
			continue;
		auto &cargo = *c;

		if (cargo.hasTarget())
			transport.addCargoTarget(cargo.getTarget().getPosition());

		// If the cargo is not loaded
		if (!cargo.unit()->isLoaded()) {

			// If it's requesting a pickup, set load state to 1	
			if (readyToPickup(cargo)) {
				transport.setLoading(true);
				transport.setDestination(pickupPosition(cargo));
				cargo.unit()->rightClick(transport.info()->unit());

				// Turn off everything else
				transport.setMonitoring(false);
				transport.setUnloading(false);
				return;
			}
			else if (readyToFight(cargo))
				transport.setMonitoring(true);


			// HACK: Island maps just shuttle units for now
			if (readyToFight(cargo) && Terrain().isIslandMap()) {
				transport.removeCargo(&cargo);
				cargo.setTransport(nullptr);
				return;
			}
		}

		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded() && cargo.hasTarget() && cargo.getEngagePosition().isValid()) {
			transport.setDestination(cargo.getEngagePosition());

			if (readyToFight(cargo)) {
				transport.setEngage();
				transport.setUnloading(true);

				if (readyToUnload(cargo))
					transport.info()->unit()->unload(cargo.unit());
			}
			else
				transport.setRetreat();

			// Dont attack until we're ready
			if (cargo.getGlobalStrategy() == 0 && !cargo.shouldEngage())
				transport.setDestination(mapBWEB.getNaturalPosition());
		}
	}

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

	//if (transport.getDestination() == mapBWEB.getMainPosition() && Terrain().isIslandMap()) {
	//	double distBest = DBL_MAX;
	//	Position posBest = Positions::None;
	//	for (auto &tile : mapBWEM.StartingLocations()) {
	//		Position center = Position(tile) + Position(64, 48);
	//		double dist = center.getDistance(mapBWEB.getMainPosition());

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

void TransportManager::updateMovement(TransportInfo& transport)
{
	// Determine highest threat possible here
	const auto highestThreat = [&](WalkPosition here, UnitType t) {
		double highest = 0.01;
		auto dx = int(t.width() / 16.0);		// Half walk resolution width
		auto dy = int(t.height() / 16.0);		// Half walk resolution height

		for (int x = here.x - dx; x < here.x + dx; x++) {
			for (int y = here.y - dy; y < here.y + dy; y++) {
				WalkPosition w(x, y);
				double current = Grids().getEAirThreat(w) + Grids().getEGroundThreat(w);
				highest = (current > highest) ? current : highest;
			}
		}
		return highest;
	};

	// Check if the destination can be used for ground distance
	auto dropTarget = transport.getDestination();
	if (!Util().isWalkable(dropTarget) || mapBWEB.getGroundDistance(transport.getPosition(), dropTarget) == DBL_MAX) {
		auto distBest = DBL_MAX;
		for (auto pos : transport.getCargoTargets()) {
			auto dist = pos.getDistance(transport.getPosition());
			if (Util().isWalkable(pos) && dist < distBest) {
				dropTarget = pos;
				distBest = dist;
			}
		}
	}

	Position bestPosition = transport.getDestination();
	WalkPosition start = transport.getWalkPosition();
	double best = 0.0; 

	UnitInfo* enemy = Util().getClosestUnit(transport.getPosition(), Broodwar->enemy());
	if (enemy && enemy->getPosition().getDistance(transport.getPosition()) <= 320.0) {

		// First look for mini tiles with no threat that are closest to the enemy and on low mobility
		for (int x = start.x - 24; x < start.x + 24 + transport.getType().tileWidth() * 4; x++) {
			for (int y = start.y - 24; y < start.y + 24 + transport.getType().tileWidth() * 4; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				if (!w.isValid()
					|| p.getDistance(transport.getPosition()) <= 32.0
					|| (transport.isUnloading() && !Util().isMobile(start, w, UnitTypes::Protoss_Reaver))
					|| (transport.isUnloading() && Broodwar->getGroundHeight(TilePosition(w)) < Broodwar->getGroundHeight(TilePosition(dropTarget)))
					|| (p.getDistance(dropTarget) < 128.0 && dropTarget != Terrain().getDefendPosition())
					|| (Terrain().getEnemyStartingTilePosition().isValid() && mapBWEM.GetArea(w) == mapBWEM.GetArea(Terrain().getEnemyStartingTilePosition())))
					continue;

				// If we just dropped units, we need to make sure not to leave them
				if (transport.isMonitoring()) {
					bool proximity = true;
					for (auto &u : transport.getAssignedCargo()) {
						if (!u->unit()->isLoaded() && u->getPosition().getDistance(p) > 64.0)
							proximity = false;
					}
					if (!proximity)
						continue;
				}

				auto closeHome = transport.getPosition().getDistance(Terrain().getDefendPosition()) < 640.0;
				auto distDrop = mapBWEB.getGroundDistance(dropTarget, p);
				auto distAway = p.getDistance(enemy->getPosition()) / p.getDistance(mapBWEB.getNaturalPosition());
				
				
				double distance = 1.0 + (transport.info()->shouldRetreat() ? 1.0 / distAway : distDrop);
				double threat = highestThreat(w, transport.getType());
				double cluster = 0.1 + Grids().getAGroundCluster(w);
				double score = 1.0 / (distance * threat * cluster);

				if (transport.isLoading() && p.getDistance(transport.getDestination()) < 64.0)
					threat = 0.01;

				if (score > best) {
					best = score;
					bestPosition = p;
				}
			}
		}
	}

	if (bestPosition.isValid())
		transport.info()->unit()->move(bestPosition);

	Broodwar->drawLineMap(transport.getPosition(), bestPosition, Broodwar->self()->getColor());
}

void TransportManager::removeUnit(Unit unit)
{
	// Delete if it's a shuttled unit
	for (auto &t : myTransports) {
		auto &transport = t.second;
		for (auto &cargo : transport.getAssignedCargo()) {
			if (cargo->unit() == unit) {
				transport.removeCargo(cargo);
				return;
			}
		}
		//for (auto &worker : transport.getAssignedWorkers()) {
		//	if (worker->unit() == unit) {
		//		transport.removeWorker(worker);
		//		return;
		//	}
		//}
	}

	// Delete if it's the shuttle
	if (myTransports.find(unit) != myTransports.end()) {
		for (auto &c : myTransports[unit].getAssignedCargo())
			c->setTransport(nullptr);
		//for (auto &w : myTransports[unit].getAssignedWorkers())
		//	w->setTransport(nullptr);
		myTransports.erase(unit);
	}
}

void TransportManager::storeUnit(Unit unit)
{
	myTransports[unit];
}