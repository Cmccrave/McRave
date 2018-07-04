#include "McRave.h"

void TransportManager::onFrame()
{
	Display().startClock();
	updateTransports();
	Display().performanceTest(__FUNCTION__);
	return;
}

void TransportManager::updateTransports()
{
	if (Broodwar->self()->getRace() == Races::Zerg)
		return;

	for (auto &transport : myTransports) {
		updateInformation(transport.second);
		updateCargo(transport.second);
		updateDecision(transport.second);
		updateMovement(transport.second);
	}
	return;
}

void TransportManager::updateCargo(TransportInfo& transport)
{
	// Update cargo information
	if (transport.getCargoSize() < 8 && !transport.isUnloading()) {

		// See if any workers need a shuttle
		for (auto &w : Workers().getMyWorkers()) {
			WorkerInfo& worker = w.second;

			if (transport.unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(worker.getPosition()) < 160.0) {

				if (transport.getCargoSize() + worker.getType().spaceRequired() > 8 || worker.getTransport())
					continue;

				if (worker.getBuildPosition().isValid() && mapBWEB.getGroundDistance((Position)worker.getBuildPosition(), (Position)worker.getTilePosition()) == DBL_MAX) {
					worker.setTransport(transport.unit());
					transport.assignWorker(&worker);
				}

				if (worker.hasResource() && worker.getPosition().getDistance(worker.getResource().getPosition()) > 160.0 && mapBWEB.getGroundDistance(worker.getPosition(), worker.getResource().getPosition()) == DBL_MAX && mapBWEM.GetArea(worker.getTilePosition()) != mapBWEM.GetArea(worker.getResource().getTilePosition())) {
					worker.setTransport(transport.unit());
					transport.assignWorker(&worker);
				}
			}
		}

		// See if any units need a shuttle
		for (auto &u : Units().getMyUnits()) {
			UnitInfo &unit = u.second;
			if (transport.getCargoSize() + unit.getType().spaceRequired() > 8 || unit.getTransport())
				continue;

			if (transport.unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(unit.getPosition()) < 320.0) {

				/*if (unit.getType() == UnitTypes::Protoss_Reaver || unit.getType() == UnitTypes::Protoss_High_Templar) {
					unit.setTransport(transport.unit());
					transport.assignCargo(&unit);
				}
				else */if (Terrain().isIslandMap() && !unit.getType().isFlyer() && mapBWEB.getGroundDistance(unit.getPosition(), unit.getEngagePosition()) > 640.0) {
					unit.setTransport(transport.unit());
					transport.assignCargo(&unit);
				}
			}
		}
	}
}

void TransportManager::updateInformation(TransportInfo& transport)
{
	// Update general information
	transport.setType(transport.unit()->getType());
	transport.setPosition(transport.unit()->getPosition());
	transport.setWalkPosition(Util().getWalkPosition(transport.unit()));
	transport.setTilePosition(transport.unit()->getTilePosition());
	transport.setLoading(false);
	transport.setUnloading(false);
	transport.setDestination(mapBWEB.getMainPosition());
	return;
}

void TransportManager::updateDecision(TransportInfo& transport)
{
	// Check if we should be loading/unloading any cargo	
	for (auto &c : transport.getAssignedCargo()) {
		if (!c->unit() || !c->unit()->exists())
			continue;
		auto &cargo = *c;


		// If the cargo is not loaded
		if (!cargo.unit()->isLoaded()) {
			//transport.setDestination(cargo.getPosition());
			//transport.getPosition().getDistance(transport.getDestination()) <= cargo.getGroundRange() + 32 ? transport.setMonitoring(true) : transport.setMonitoring(false);
			
			// If it's requesting a pickup, set load state to 1	
			if (transport.unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(cargo.getPosition()) < 160.0) {
				if (!cargo.hasTarget() || (cargo.getPosition().getDistance(cargo.getEngagePosition()) > 640.0 /*|| (cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() < 75) || (cargo.getType() == UnitTypes::Protoss_Reaver && cargo.unit()->getScarabCount() < 5)*/)) {
					transport.setLoading(true);
					transport.unit()->load(cargo.unit());
					return;
				}
			}

			if (/*transport.getPosition().getDistance(transport.getDestination()) < 320 &&*/ mapBWEB.getGroundDistance(cargo.getPosition(), cargo.getEngagePosition()) < 640.0) {
				transport.removeCargo(&cargo);
				cargo.setTransport(nullptr);
				return;
			}
		}
		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded() && cargo.hasTarget() && cargo.getTarget().getPosition().isValid()) {
			transport.setDestination(cargo.getEngagePosition());

			Broodwar->drawTextMap(transport.getPosition(), "%.2f", mapBWEB.getGroundDistance(cargo.getPosition(), cargo.getEngagePosition()));

			// If cargo wants to fight, find a spot to unload
			if (cargo.getLocalStrategy() == 1)
				transport.setUnloading(true);

			//if (transport.getPosition().getDistance(transport.getDestination()) <= 32 && cargo.getLocalStrategy() == 1 && ((cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() >= 75) || (cargo.getType() == UnitTypes::Protoss_Reaver && cargo.unit()->getScarabCount() >= 5)))
			//	transport.unit()->unload(cargo.unit());

			if (mapBWEB.getGroundDistance(cargo.getPosition(), cargo.getEngagePosition()) < 640.0)
				transport.unit()->unload(cargo.unit());

		}
		else if (transport.unit()->getLastCommand().getTargetPosition() == mapBWEB.getMainPosition()) {
			int random = rand() % (Terrain().getAllBases().size());
			int i = 0;
			for (auto &base : Terrain().getAllBases()) {
				if (i == random) {
					transport.setDestination(base->Center());
				}
				else i++;
			}
		}
	}

	for (auto &w : transport.getAssignedWorkers()) {
		auto &worker = *w;
		bool miner = false;
		bool builder = false;

		if (!w || !worker.unit())
			continue;

		Broodwar->drawLineMap(transport.getPosition(), worker.getPosition(), Colors::Purple);

		if (worker.unit()->exists() && !worker.unit()->isLoaded()
			&& !worker.unit()->isCarryingMinerals()
			&& !worker.unit()->isCarryingGas()) {
			double airDist = DBL_MAX;
			double grdDist = DBL_MAX;
			if (worker.getBuildPosition().isValid()) {
				builder = true;
				grdDist = mapBWEB.getGroundDistance(worker.getPosition(), (Position)worker.getBuildPosition());
				airDist = worker.getPosition().getDistance(Position(worker.getBuildPosition()));
			}

			else if (worker.hasResource()) {
				miner = true;
				grdDist = mapBWEB.getGroundDistance(worker.getPosition(), worker.getResource().getPosition());
				airDist = worker.getPosition().getDistance(worker.getResource().getPosition());
			}

			if (grdDist != DBL_MAX || airDist < 64.0 || (miner && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition())) || (builder && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getBuildPosition()))) {
				transport.removeWorker(&worker);
				worker.setTransport(nullptr);
				return;
			}
			else if (transport.unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(worker.getPosition()) < 320.0) {
				transport.setLoading(true);
				transport.unit()->load(worker.unit());
				return;
			}
		}
		else {
			double airDist = DBL_MAX;
			double grdDist = DBL_MAX;

			if (worker.getBuildPosition().isValid()) {
				transport.setDestination((Position)worker.getBuildPosition());
								
				grdDist = mapBWEB.getGroundDistance((Position)worker.getBuildPosition(), (Position)worker.getTilePosition());
				airDist = airDist = worker.getPosition().getDistance(Position(worker.getBuildPosition()));

				if (grdDist != DBL_MAX || airDist < 64.0)
					transport.unit()->unload(worker.unit());				
			}
			else if (worker.hasResource()) {
				transport.setDestination(worker.getResource().getPosition());

				grdDist = mapBWEB.getGroundDistance(worker.getPosition(), worker.getResource().getPosition());
				airDist = worker.getPosition().getDistance(worker.getResource().getPosition());
								
				if (grdDist != DBL_MAX || airDist < 64.0 || mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition()))
					transport.unit()->unload(worker.unit());				
			}
		}
	}

	Broodwar->drawLineMap(transport.getPosition(), transport.getDestination(), Colors::Orange);
}

void TransportManager::updateMovement(TransportInfo& transport)
{
	// If loading, ignore movement commands
	if (transport.isLoading())
		return;

	Position bestPosition = transport.getDestination();
	WalkPosition start = transport.getWalkPosition();
	double best = 0.0;

	UnitInfo* enemy = Util().getClosestEnemyUnit(transport, Filter::CanAttack);
	if (enemy && enemy->getPosition().getDistance(transport.getPosition()) <= 320) {

		// First look for mini tiles with no threat that are closest to the enemy and on low mobility
		for (int x = start.x - 16; x < start.x + 16 + transport.getType().tileWidth() * 4; x++) {
			for (int y = start.y - 16; y < start.y + 16 + transport.getType().tileWidth() * 4; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				if (!w.isValid()
					|| p.getDistance(transport.getPosition()) <= 64
					|| (transport.isUnloading() && !Util().isMobile(start, w, UnitTypes::Protoss_Reaver)))
					continue;

				//// If we just dropped units, we need to make sure not to leave them
				//if (transport.isMonitoring()) {
				//	for (auto &u : transport.getAssignedCargo()) {
				//		if (u->getPosition().getDistance(p) > 128)
				//			continue;
				//	}
				//}

				double airDist = transport.getDestination().getDistance(p);
				double distance = 64.0 + airDist;
				double threat = max(0.01, Grids().getEAirThreat(w));


				double score = 1.0 / (distance * threat);

				if (Util().isSafe(w, UnitTypes::Protoss_Reaver, true, true))
					score = transport.isUnloading() ? 1.0 / distance : distance;


				if (score > best) {
					best = score;
					bestPosition = p;
				}
			}
		}
	}

	if (bestPosition.isValid()) {
		transport.unit()->move(bestPosition);
	}
}

void TransportManager::removeUnit(Unit unit)
{
	// Delete if it's a shuttled unit
	for (auto &transport : myTransports) {
		for (auto &cargo : transport.second.getAssignedCargo()) {
			if (cargo->unit() == unit) {
				transport.second.removeCargo(cargo);
				return;
			}
		}
		for (auto &worker : transport.second.getAssignedWorkers()) {
			if (worker->unit() == unit) {
				transport.second.removeWorker(worker);
				return;
			}
		}
	}

	// Delete if it's the shuttle
	if (myTransports.find(unit) != myTransports.end()) {
		for (auto &c : myTransports[unit].getAssignedCargo()) {
			c->setTransport(nullptr);
		}
		for (auto &w : myTransports[unit].getAssignedWorkers()) {
			w->setTransport(nullptr);
		}
		myTransports.erase(unit);
	}
}

void TransportManager::storeUnit(Unit unit)
{
	myTransports[unit].setUnit(unit);
}