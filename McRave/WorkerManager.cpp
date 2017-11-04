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
		updateGathering(worker.second);
	}
	return;
}

void WorkerTrackerClass::updateScout(WorkerInfo& worker)
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
		if (Bases().getEnemyBases().size() == 0)
		{
			for (auto &start : theMap.StartingLocations())
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
		if (Bases().getEnemyBases().size() > 0)
		{
			exploreArea(worker);
			return;
		}
	}
	else
	{
		for (auto &area : theMap.Areas())
		{
			for (auto &base : area.Bases())
			{
				if (area.AccessibleNeighbours().size() == 0 || Grids().getBaseGrid(base.Location()) > 0 || Terrain().isInEnemyTerritory(base.Location()))
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

void WorkerTrackerClass::updateInformation(WorkerInfo& worker)
{
	worker.setPosition(worker.unit()->getPosition());
	worker.setWalkPosition(Util().getWalkPosition(worker.unit()));
	worker.setTilePosition(worker.unit()->getTilePosition());
	worker.setType(worker.unit()->getType());
	return;
}

void WorkerTrackerClass::exploreArea(WorkerInfo& worker)
{
	WalkPosition start = worker.getWalkPosition();
	Position bestPosition = Terrain().getPlayerStartingPosition();

	Unit closest = worker.unit()->getClosestUnit(Filter::IsEnemy && Filter::CanAttack);
	if (!closest || (closest && closest->exists() && worker.unit()->getDistance(closest) > 640))
	{
		worker.unit()->move(Terrain().getEnemyStartingPosition());
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
			double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition()));
			double time = max(1.0, double(Broodwar->getFrameCount() - recentExplorations[WalkPosition(x, y)]));

			if ((threat < safest || (threat == safest && (time * mobility) / distance >= best)) && Util().isMobile(start, WalkPosition(x, y), worker.getType()))
			{
				safest = threat;
				best = (time * mobility) / distance;
				bestPosition = Position(WalkPosition(x, y));
			}
		}
	}
	if (bestPosition.isValid() && bestPosition != Position(start) && worker.unit()->getLastCommand().getTargetPosition() != bestPosition)
	{
		worker.unit()->move(bestPosition);
	}
	return;
}

void WorkerTrackerClass::updateGathering(WorkerInfo& worker)
{
	// If ready to remove unit role
	if (Units().getAllyUnits().find(worker.unit()) != Units().getAllyUnits().end())
	{
		Units().getAllyUnits().erase(worker.unit());
	}

	// If idle and carrying gas or minerals, return cargo			
	if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
	{
		if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
		{
			worker.unit()->returnCargo();
		}
		return;
	}

	// Scout logic
	if (!scout || (scout && !scout->exists()))
	{
		scout = getClosestWorker(Position(Terrain().getSecondChoke()));
	}
	if (scout && worker.unit() == scout && !worker.unit()->isCarryingMinerals() && worker.getBuildingType() == UnitTypes::None && Broodwar->getFrameCount() - deadScoutFrame > 1000 && (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0 || Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0))
	{
		updateScout(worker);
		return;
	}

	// Boulder removal logic
	if (Resources().getMyBoulders().size() > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) >= 2)
	{
		for (auto &b : Resources().getMyBoulders())
		{
			if (b.first && b.first->exists() && !worker.unit()->isCarryingMinerals() && worker.unit()->getDistance(b.second.getPosition()) < 320)
			{
				if (worker.unit()->getOrderTargetPosition() != b.second.getPosition() && !worker.unit()->isGatheringMinerals())
				{
					worker.unit()->gather(b.first);
				}
				return;
			}
		}
	}

	// Building logic
	if (worker.getBuildingType().isValid() && worker.getBuildPosition().isValid())
	{
		// My crappy temporary attempt at removing spider mines
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
		if (!worker.unit()->isConstructing() && BuildOrder().getBuildingDesired()[worker.getBuildingType()] <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()))
		{
			worker.setBuildingType(UnitTypes::None);
			worker.setBuildPosition(TilePositions::None);
			worker.unit()->stop();
			return;
		}

		// If our building position is no longer buildable, remove
		if (!Buildings().isBuildable(worker.getBuildingType(), worker.getBuildPosition()))
		{
			worker.setBuildingType(UnitTypes::None);
			worker.setBuildPosition(TilePositions::None);
		}
		else
		{
			if (Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() - (((Resources().getMPM() / 30.0) * worker.getPosition().getDistance(Position(worker.getBuildPosition()))) / (worker.getType().topSpeed() * 24.0)) && Broodwar->self()->minerals() <= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice() - (((Resources().getGPM() / 30.0) * worker.getPosition().getDistance(Position(worker.getBuildPosition()))) / (worker.getType().topSpeed() * 24.0)) && Broodwar->self()->gas() <= worker.getBuildingType().gasPrice())
			{
				if (worker.unit()->getOrderTargetPosition() != Position(worker.getBuildPosition()) || worker.unit()->isStuck())
				{
					worker.unit()->move(Position(worker.getBuildPosition()));
				}
				return;
			}
			else if (Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice())
			{
				if (worker.unit()->getOrderTargetPosition() != Position(worker.getBuildPosition()) || worker.unit()->isStuck())
				{
					worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());
				}
				return;
			}
		}
	}

	// Shield Battery logic
	if (worker.unit()->getShields() <= 5 && Grids().getBatteryGrid(worker.unit()->getTilePosition()) > 0)
	{
		for (auto &b : Buildings().getAllyBuildingsFilter(UnitTypes::Protoss_Shield_Battery))
		{
			BuildingInfo &battery = Buildings().getAllyBuilding(b);
			if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit)
			{
				worker.unit()->rightClick(battery.unit());
				return;
			}
		}
	}

	// Bunker repair logic
	if (Grids().getBunkerGrid(worker.getTilePosition()) > 0)
	{
		for (auto &b : Buildings().getAllyBuildingsFilter(UnitTypes::Terran_Bunker))
		{
			BuildingInfo &bunker = Buildings().getAllyBuilding(b);
			if (bunker.unit()->getHitPoints() < bunker.unit()->getType().maxHitPoints())
			{
				if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Repair)
				{
					worker.unit()->repair(bunker.unit());
				}
				return;
			}
		}
	}

	// If we need to use workers for defense - TEMP Removed probe pull stuff
	if ((Grids().getEGroundThreat(worker.getWalkPosition()) > 0.0 && Grids().getResourceGrid(worker.getTilePosition()) > 0 && Units().getSupply() < 80) || (BuildOrder().getCurrentBuild() == "Sparks" && Units().getGlobalGroundStrategy() == 1))
	{
		Units().storeAlly(worker.unit());
		return;
	}

	// Reassignment logic
	if (worker.getResource() && worker.getResource()->exists() && ((!Resources().isGasSaturated() && minWorkers > gasWorkers * 8) || (!Resources().isMinSaturated() && minWorkers < gasWorkers * 4)))
	{
		reAssignWorker(worker);
	}

	// If worker doesn't have an assigned resource, assign one
	if (!worker.getResource())
	{
		assignWorker(worker);
		// Any idle workers can gather closest mineral field until they are assigned again
		if (worker.unit()->isIdle() && worker.unit()->getClosestUnit(Filter::IsMineralField))
		{
			worker.unit()->gather(worker.unit()->getClosestUnit(Filter::IsMineralField));
			return;
		}
	}

	// If not targeting the mineral field the worker is mapped to
	if (!worker.unit()->isCarryingGas() && !worker.unit()->isCarryingMinerals())
	{
		if ((worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() == worker.getResource())
		{
			return;
		}
		// If the workers current target is a boulder, continue mining it
		if (worker.unit()->getTarget() && worker.unit()->getTarget()->getType().isMineralField() && worker.unit()->getTarget()->getResources() == 0)
		{
			return;
		}
		// If the mineral field is a valid mining target
		if (worker.getResource() && Grids().getBaseGrid(TilePosition(worker.getResourcePosition())) == 2)
		{
			// If it exists, mine it
			if (worker.getResource()->exists())
			{
				worker.unit()->gather(worker.getResource());
				return;
			}
			// Else, move to it
			else
			{
				worker.unit()->move(worker.getResourcePosition());
				return;
			}
		}
		else if (worker.unit()->getClosestUnit(Filter::IsMineralField) && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather)
		{
			worker.unit()->gather(worker.unit()->getClosestUnit(Filter::IsMineralField));
			return;
		}
	}
}

Unit WorkerTrackerClass::getClosestWorker(Position here)
{
	// Currently gets the closest worker that doesn't mine gas
	Unit closestWorker = nullptr;
	double closestD = 0.0;
	for (auto &worker : myWorkers)
	{
		if (worker.first == scout)
		{
			continue;
		}
		if (worker.second.getResource() && worker.second.getResource()->exists() && !worker.second.getResource()->getType().isMineralField())
		{
			continue;
		}
		if (worker.second.getBuildingType() != UnitTypes::None)
		{
			continue;
		}
		if (worker.first->getLastCommand().getType() == UnitCommandTypes::Gather && worker.first->getLastCommand().getTarget()->exists() && worker.first->getLastCommand().getTarget()->getInitialResources() == 0)
		{
			continue;
		}
		if (worker.second.getType() != UnitTypes::Protoss_Probe && worker.second.unit()->isConstructing())
		{
			continue;
		}
		if (worker.first->getDistance(here) < closestD || closestD == 0.0)
		{
			closestWorker = worker.first;
			closestD = worker.first->getDistance(here);
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
	if (Resources().getMyGas().find(myWorkers[worker].getResource()) != Resources().getMyGas().end())
	{
		Resources().getMyGas()[myWorkers[worker].getResource()].setGathererCount(Resources().getMyGas()[myWorkers[worker].getResource()].getGathererCount() - 1);
		gasWorkers--;
	}
	if (Resources().getMyMinerals().find(myWorkers[worker].getResource()) != Resources().getMyMinerals().end())
	{
		Resources().getMyMinerals()[myWorkers[worker].getResource()].setGathererCount(Resources().getMyMinerals()[myWorkers[worker].getResource()].getGathererCount() - 1);
		minWorkers--;
	}
	if (worker == scout)
	{
		deadScoutFrame = Broodwar->getFrameCount();
	}
	myWorkers.erase(worker);
	return;
}

void WorkerTrackerClass::assignWorker(WorkerInfo& worker)
{
	// Check if we need gas workers
	if (!Resources().isGasSaturated() && ((!Strategy().isRush() && Resources().isMinSaturated()) || minWorkers > gasWorkers * 10))
	{
		for (auto &g : Resources().getMyGas())
		{
			ResourceInfo &gas = g.second;
			if (gas.getType() != UnitTypes::Resource_Vespene_Geyser && gas.unit()->isCompleted() && gas.getGathererCount() < 3 && Grids().getBaseGrid(gas.getTilePosition()) > 0)
			{
				gas.setGathererCount(gas.getGathererCount() + 1);
				worker.setResource(gas.unit());
				worker.setResourcePosition(gas.getPosition());
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
				if (mineral.getGathererCount() < i && Grids().getBaseGrid(mineral.getTilePosition()) > 0)
				{
					mineral.setGathererCount(mineral.getGathererCount() + 1);
					worker.setResource(mineral.unit());
					worker.setResourcePosition(mineral.getPosition());
					minWorkers++;
					return;
				}
			}
		}
	}
	return;
}

void WorkerTrackerClass::reAssignWorker(WorkerInfo& worker)
{
	if (Resources().getMyGas().find(worker.getResource()) != Resources().getMyGas().end())
	{
		Resources().getMyGas()[worker.getResource()].setGathererCount(Resources().getMyGas()[worker.getResource()].getGathererCount() - 1);
		gasWorkers--;
	}
	if (Resources().getMyMinerals().find(worker.getResource()) != Resources().getMyMinerals().end())
	{
		Resources().getMyMinerals()[worker.getResource()].setGathererCount(Resources().getMyMinerals()[worker.getResource()].getGathererCount() - 1);
		minWorkers--;
	}
	assignWorker(worker);
	return;
}