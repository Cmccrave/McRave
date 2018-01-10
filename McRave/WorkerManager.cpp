#include "McRave.h"

void WorkerTrackerClass::update()
{
	Display().startClock();
	updateWorkers();
	Display().performanceTest(__FUNCTION__);
	return;
}

void WorkerTrackerClass::updateWorkers()
{
	for (auto &worker : myWorkers)
	{
		updateInformation(worker.second);
		updateDecision(worker.second);
	}
	return;
}

void WorkerTrackerClass::updateInformation(WorkerInfo& worker)
{
	worker.setPosition(worker.unit()->getPosition());
	worker.setWalkPosition(Util().getWalkPosition(worker.unit()));
	worker.setTilePosition(worker.unit()->getTilePosition());
	worker.setType(worker.unit()->getType());
	return;
}

void WorkerTrackerClass::explore(WorkerInfo& worker)
{
	WalkPosition start = worker.getWalkPosition();
	Position bestPosition = Terrain().getPlayerStartingPosition();
	Position destination = Terrain().getEnemyStartingPosition();
	int longest = 0;

	if (Broodwar->isExplored(Terrain().getEnemyStartingTilePosition()))
	{
		for (auto &station : Stations().getMyStations())		
			destination = station.BWEMBase()->Center();		
	}

	if (Players().getNumberZerg() > 0 && Strategy().getPoolFrame() > 0 && !Strategy().isEnemyFastExpand())
	{
		destination = Position(Terrain().getEnemyNatural());
	}

	Unit closest = worker.unit()->getClosestUnit(Filter::IsEnemy && Filter::CanAttack);
	if (destination.isValid() && (!closest || (closest && closest->exists() && worker.unit()->getDistance(closest) > 640)))
	{
		worker.unit()->move(destination);
		return;
	}

	// Check a 8x8 walkposition grid for a potential new place to scout
	double best = 0.0, safest = 1000.0;
	for (int x = start.x - 8; x < start.x + 8 + worker.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 8; y < start.y + 8 + worker.getType().tileHeight() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid() || Grids().getDistanceHome(start) - Grids().getDistanceHome(WalkPosition(x, y)) > 16)	continue;

			double mobility = double(Grids().getMobilityGrid(x, y));
			double threat = Grids().getEGroundThreat(x, y);
			double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(destination));
			double time = min(100.0, double(Broodwar->getFrameCount() - recentExplorations[WalkPosition(x, y)]));

			if (Terrain().isInEnemyTerritory(TilePosition(x, y)) && !Broodwar->isExplored(TilePosition(WalkPosition(x, y))) && 1000 / distance > best && Util().isMobile(start, WalkPosition(x, y), worker.getType()))
			{
				safest = -1;
				best = 1000 / distance;
				bestPosition = Position(WalkPosition(x, y));
			}
			else
			{
				if ((threat < safest || (threat == safest && (time * mobility) / distance >= best)) && Util().isMobile(start, WalkPosition(x, y), worker.getType()))
				{
					safest = threat;
					best = (time * mobility) / distance;
					bestPosition = Position(WalkPosition(x, y));
				}
			}
		}
	}
	if (bestPosition.isValid() && bestPosition != Position(start) && worker.unit()->getLastCommand().getTargetPosition() != bestPosition)
	{
		worker.unit()->move(bestPosition);
	}
	return;
}

void WorkerTrackerClass::updateDecision(WorkerInfo& worker)
{
	// If ready to remove unit role
	if (Units().getAllyUnits().find(worker.unit()) != Units().getAllyUnits().end()) Units().getAllyUnits().erase(worker.unit());
	if (BuildOrder().shouldScout() && Broodwar->getFrameCount() - deadScoutFrame > 1000 && (!Terrain().getEnemyStartingPosition().isValid() || !Strategy().isPlayPassive()) && (!scouter || !scouter->exists())) scouter = getClosestWorker(Position(mapBWEB.getSecondChoke()), false);

	// Assign, return, scout, building, repair, fight, gather
	if (shouldAssign(worker)) assign(worker);
	if (shouldReturnCargo(worker)) returnCargo(worker);
	else if (shouldClearPath(worker)) clearPath(worker);
	else if (shouldBuild(worker)) build(worker);
	else if (shouldScout(worker)) scout(worker);
	else if (shouldRepair(worker)) repair(worker);
	else if (shouldFight(worker)) fight(worker);
	else if (shouldGather(worker)) gather(worker);
}

bool WorkerTrackerClass::shouldAssign(WorkerInfo& worker)
{
	if (!worker.hasResource() && (!Resources().isMinSaturated() || !Resources().isGasSaturated())) return true;
	else if ((!Resources().isGasSaturated() && minWorkers > gasWorkers * 6) || (!Resources().isMinSaturated() && minWorkers < gasWorkers * 2)) return true;
	return false;
}

bool WorkerTrackerClass::shouldBuild(WorkerInfo& worker)
{
	if (worker.getBuildingType().isValid() && worker.getBuildPosition().isValid()) return true;
	return false;
}

bool WorkerTrackerClass::shouldClearPath(WorkerInfo& worker)
{
	if (Resources().getMyBoulders().size() == 0 || Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) < 2) return false;
	for (auto &b : Resources().getMyBoulders())
	{
		ResourceInfo boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists()) continue;
		if (worker.getPosition().getDistance(boulder.getPosition()) < 320)	return true;
	}
	return false;
}

bool WorkerTrackerClass::shouldFight(WorkerInfo& worker)
{
	if (Util().shouldPullWorker(worker.unit()) && worker.unit() == getClosestWorker(Terrain().getDefendPosition(), false)) return true;
	return false;
}

bool WorkerTrackerClass::shouldGather(WorkerInfo& worker)
{
	if (worker.hasResource() && (worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() != worker.getResource().unit() && worker.getResource().getState() == 2) return true;
	else if (worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather) return true;
	return false;
}

bool WorkerTrackerClass::shouldRepair(WorkerInfo& worker)
{
	if (Grids().getBunkerGrid(worker.getTilePosition()) > 0)
	{
		for (auto &b : Buildings().getAllyBuildingsFilter(UnitTypes::Terran_Bunker))
		{
			BuildingInfo &bunker = Buildings().getAllyBuilding(b);
			if (bunker.unit()->getHitPoints() < bunker.unit()->getType().maxHitPoints() && Broodwar->self()->minerals() > 10) return true;
		}
	}
	return false;
}

bool WorkerTrackerClass::shouldReturnCargo(WorkerInfo& worker)
{
	if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals()) return true;
	return false;
}

bool WorkerTrackerClass::shouldScout(WorkerInfo& worker)
{
	if (worker.unit() == scouter && worker.getBuildingType() == UnitTypes::None && BuildOrder().shouldScout()) return true;
	return false;
}

void WorkerTrackerClass::assign(WorkerInfo& worker)
{
	// Remove current assignment if it has one
	if (worker.hasResource())
	{
		worker.getResource().setGathererCount(worker.getResource().getGathererCount() - 1);
		worker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	// Check if we need gas workers
	if ((!Resources().isGasSaturated() && minWorkers > gasWorkers * 6) || (Resources().isMinSaturated()))
	{
		for (auto &g : Resources().getMyGas())
		{
			ResourceInfo &gas = g.second;
			if (gas.getType() != UnitTypes::Resource_Vespene_Geyser && gas.unit()->isCompleted() && gas.getGathererCount() < 3 && gas.getState() > 0)
			{
				gas.setGathererCount(gas.getGathererCount() + 1);
				worker.setResource(&gas);
				gasWorkers++;
				return;
			}
		}
	}

	// Check if we need mineral workers
	else
	{
		for (int i = 1; i <= 2; i++)
		{
			for (auto &m : Resources().getMyMinerals())
			{
				ResourceInfo &mineral = m.second;
				if (mineral.getGathererCount() < i && mineral.getState() > 0)
				{
					mineral.setGathererCount(mineral.getGathererCount() + 1);
					worker.setResource(&mineral);
					minWorkers++;
					return;
				}
			}
		}
	}

	worker.setResource(nullptr);
	return;
}

void WorkerTrackerClass::build(WorkerInfo& worker)
{
	// Spider mine removal
	if (worker.getBuildingType().isResourceDepot())
	{
		if (Unit mine = Broodwar->getClosestUnit(Position(worker.getBuildPosition()), Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine, 128))
		{
			if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit || !worker.unit()->getLastCommand().getTarget() || !worker.unit()->getLastCommand().getTarget()->exists())
			{
				worker.unit()->attack(mine);
			}
			return;
		}
	}

	// If our building desired has changed recently, remove
	if (!worker.unit()->isConstructing() && (BuildOrder().getBuildingDesired()[worker.getBuildingType()] <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings().isBuildable(worker.getBuildingType(), worker.getBuildPosition())))
	{
		worker.setBuildingType(UnitTypes::None);
		worker.setBuildPosition(TilePositions::Invalid);
		worker.unit()->stop();
		return;
	}

	else if (Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() - (((Resources().getMPM() / 60.0) * worker.getPosition().getDistance(Position(worker.getBuildPosition()))) / (worker.getType().topSpeed() * 24.0)) && Broodwar->self()->minerals() <= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice() - (((Resources().getGPM() / 60.0) * worker.getPosition().getDistance(Position(worker.getBuildPosition()))) / (worker.getType().topSpeed() * 24.0)) && Broodwar->self()->gas() <= worker.getBuildingType().gasPrice())
	{
		if (worker.unit()->getOrderTargetPosition() != Position(worker.getBuildPosition()) || worker.unit()->isIdle())
		{
			worker.unit()->move(Position(worker.getBuildPosition()));
		}
		return;
	}
	else if (Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice())
	{
		if (worker.getPosition().getDistance(Position(worker.getBuildPosition())) > 320) worker.unit()->move(Position(worker.getBuildPosition()));
		else worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());
		return;
	}
}

void WorkerTrackerClass::clearPath(WorkerInfo& worker)
{
	for (auto &b : Resources().getMyBoulders())
	{
		ResourceInfo& boulder = b.second;
		for (auto &b : Resources().getMyBoulders())
		{
			ResourceInfo boulder = b.second;
			if (!boulder.unit() || !boulder.unit()->exists()) continue;
			if (worker.getPosition().getDistance(boulder.getPosition()) >= 320) continue;
			if (worker.unit()->getOrderTargetPosition() != b.second.getPosition() && !worker.unit()->isGatheringMinerals())
				worker.unit()->gather(b.first);
			return;
		}
	}
}

void WorkerTrackerClass::fight(WorkerInfo& worker)
{
	Units().storeAlly(worker.unit());
	return;
}

void WorkerTrackerClass::gather(WorkerInfo& worker)
{
	if (worker.hasResource() && worker.getResource().unit()->exists() && worker.getResource().getState() == 2) worker.unit()->gather(worker.getResource().unit()); // If it exists, mine it
	else if (worker.hasResource() && worker.getResource().getPosition().isValid() && worker.getResource().getState() == 2) worker.unit()->move(worker.getResource().getPosition());	// Else, move to it
	else if (worker.unit()->isIdle() && worker.unit()->getClosestUnit(Filter::IsMineralField)) worker.unit()->gather(worker.unit()->getClosestUnit(Filter::IsMineralField));
}

void WorkerTrackerClass::repair(WorkerInfo& worker)
{
	for (auto &b : Buildings().getAllyBuildingsFilter(UnitTypes::Terran_Bunker))
	{
		BuildingInfo &bunker = Buildings().getAllyBuilding(b);
		if (bunker.unit()->getHitPoints() < bunker.unit()->getType().maxHitPoints() && Broodwar->self()->minerals() > 10 && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Repair) worker.unit()->repair(bunker.unit());
	}
}

void WorkerTrackerClass::returnCargo(WorkerInfo& worker)
{
	if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo) worker.unit()->returnCargo();
}

void WorkerTrackerClass::scout(WorkerInfo& worker)
{
	double closestD = 0.0;
	int best = 0;
	Position closestP;
	WalkPosition start = worker.getWalkPosition();

	// All walkpositions in a 4x4 walkposition grid are set as scouted already to prevent overlapping
	for (int x = start.x - 4; x < start.x + 4 + worker.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 4; y < start.y + 4 + worker.getType().tileHeight() * 4; y++)
		{
			if (WalkPosition(x, y).isValid() && Grids().getEGroundThreat(x, y) == 0.0)
			{
				recentExplorations[WalkPosition(x, y)] = Broodwar->getFrameCount();
			}
		}
	}

	// Update scout probes decision if we are above 9 supply and just placed a pylon
	if (!Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
	{
		if (!Terrain().getEnemyStartingPosition().isValid() || !Broodwar->isExplored(Terrain().getEnemyStartingTilePosition()))
		{
			for (auto &start : mapBWEM.StartingLocations())
			{
				if (!Broodwar->isExplored(start) && (Position(start).getDistance(Terrain().getPlayerStartingPosition()) < closestD || closestD == 0.0))
				{
					closestD = Position(start).getDistance(Terrain().getPlayerStartingPosition());
					closestP = Position(start);
				}
			}
			if (closestP.isValid() && worker.unit()->getOrderTargetPosition() != closestP)
			{
				worker.unit()->move(closestP);
			}
			return;
		}
		else if (Stations().getEnemyStations().size() > 0)
		{
			explore(worker);
			return;
		}
	}
	else
	{
		for (auto &area : mapBWEM.Areas())
		{
			for (auto &base : area.Bases())
			{
				// TODO - check if it's owned
				if (area.AccessibleNeighbours().size() == 0 || Terrain().isInEnemyTerritory(base.Location()))
				{
					continue;
				}
				if (Broodwar->getFrameCount() - recentExplorations[WalkPosition(base.Location())] > best)
				{
					best = Broodwar->getFrameCount() - recentExplorations[WalkPosition(base.Location())];
					closestP = Position(base.Location());
				}
			}
		}
		worker.unit()->move(closestP);
	}
	return;
}

Unit WorkerTrackerClass::getClosestWorker(Position here, bool isBuilding)
{
	// Currently gets the closest worker that doesn't mine gas
	Unit closestWorker = nullptr;
	double closestD = 0.0;
	for (auto &w : myWorkers)
	{
		WorkerInfo &worker = w.second;
		if (isBuilding && worker.hasResource() && worker.getResource().unit()->exists() && !worker.getResource().getType().isMineralField()) continue;
		if (isBuilding && worker.getBuildPosition().isValid())	continue;
		if (worker.unit() == scouter) continue;
		if (worker.unit()->getLastCommand().getType() == UnitCommandTypes::Gather && worker.unit()->getLastCommand().getTarget()->exists() && worker.unit()->getLastCommand().getTarget()->getInitialResources() == 0)	continue;
		if (worker.getType() != UnitTypes::Protoss_Probe && worker.unit()->isConstructing()) continue;
		if (Terrain().getGroundDistance(worker.getPosition(), here) < closestD || closestD == 0.0)
		{
			closestWorker = worker.unit();
			closestD = Terrain().getGroundDistance(worker.getPosition(), here);
		}
	}
	return closestWorker;
}

void WorkerTrackerClass::storeWorker(Unit unit)
{
	myWorkers[unit].setUnit(unit);
	return;
}

void WorkerTrackerClass::removeWorker(Unit worker)
{
	WorkerInfo thisWorker = myWorkers[worker];
	if (thisWorker.hasResource())
	{
		thisWorker.getResource().setGathererCount(thisWorker.getResource().getGathererCount() - 1);
		thisWorker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	if (worker == scouter)
	{
		deadScoutFrame = Broodwar->getFrameCount();
	}
	myWorkers.erase(worker);
	return;
}