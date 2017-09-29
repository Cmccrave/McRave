#include "McRave.h"

void WorkerTrackerClass::update()
{
	Display().startClock();
	updateScout();
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

void WorkerTrackerClass::updateScout()
{
	// Update scout probes decision if we are above 9 supply and just placed a pylon
	if (!scout || (scout && !scout->exists()))
	{
		scout = getClosestWorker(Position(Terrain().getSecondChoke()));
	}
	return;
}

void WorkerTrackerClass::updateInformation(WorkerInfo& worker)
{
	worker.setPosition(worker.unit()->getPosition());
	worker.setWalkPosition(Util().getWalkPosition(worker.unit()));
	worker.setTilePosition(worker.unit()->getTilePosition());
	return;
}

void WorkerTrackerClass::exploreArea(WorkerInfo& worker)
{
	WalkPosition start = worker.getWalkPosition();
	Position bestPosition = Terrain().getPlayerStartingPosition();
	double closestD = 0.0;

	Unit closest = worker.unit()->getClosestUnit(Filter::IsEnemy && Filter::CanAttack);
	if (!closest || (closest && closest->exists() && worker.unit()->getDistance(closest) > 640))
	{
		worker.unit()->move(Terrain().getEnemyStartingPosition());
		return;
	}

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

	// Check a 8x8 walkposition grid for a potential new place to scout
	double highestMobility = 0.0;
	for (int x = start.x - 8; x < start.x + 8 + worker.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 8; y < start.y + 8 + worker.getType().tileHeight() * 4; y++)
		{
			if (Grids().getDistanceHome(start) - Grids().getDistanceHome(WalkPosition(x, y)) > 16)
			{
				continue;
			}

			double mobility = double(Grids().getMobilityGrid(x, y));
			double threat = max(0.01, Grids().getEGroundThreat(x, y));
			double distance = max(0.01, Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition()));

			if (mobility < 7)
			{
				continue;
			}

			if (WalkPosition(x, y).isValid() && Broodwar->getFrameCount() - recentExplorations[WalkPosition(x, y)] > 1500)
			{
				if (mobility / (threat * distance) >= highestMobility && Util().isSafe(start, WalkPosition(x, y), worker.getType(), true, false, true))
				{
					highestMobility = mobility / (threat * distance);
					bestPosition = Position(WalkPosition(x, y));
				}
			}
		}
	}
	if (bestPosition.isValid() && bestPosition != Position(start))
	{
		worker.unit()->move(bestPosition);
	}
	return;
}

void WorkerTrackerClass::updateGathering(WorkerInfo& worker)
{
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
	if (scout && worker.unit() == scout && !worker.unit()->isCarryingMinerals() && worker.getBuildingType() == UnitTypes::None)
	{
		if (Terrain().getEnemyBasePositions().size() == 0 && Broodwar->getFrameCount() - deadScoutFrame > 1000 && (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0 || Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0))
		{
			double closestD = 0.0;
			Position closestP;
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
		if (Terrain().getEnemyBasePositions().size() > 0)
		{
			exploreArea(worker);
			return;
		}
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
		// If our building desired has changed recently, remove
		if (!worker.unit()->isConstructing() && BuildOrder().getBuildingDesired()[worker.getBuildingType()] <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()))
		{
			worker.setBuildingType(UnitTypes::None);
			worker.setBuildPosition(TilePositions::None);
			worker.unit()->stop();
			return;
		}

		// If our building position is no longer buildable, remove
		if (!Buildings().canBuildHere(worker.getBuildingType(), worker.getBuildPosition()))
		{
			worker.setBuildingType(UnitTypes::None);
			worker.setBuildPosition(TilePositions::None);
		}
		else
		{
			if (!Broodwar->isVisible(worker.getBuildPosition()) || (Broodwar->self()->minerals() >= worker.getBuildingType().mineralPrice() / worker.getPosition().getDistance(Position(worker.getBuildPosition())) && Broodwar->self()->minerals() <= worker.getBuildingType().mineralPrice() && Broodwar->self()->gas() >= worker.getBuildingType().gasPrice() / worker.getPosition().getDistance(Position(worker.getBuildPosition())) && Broodwar->self()->gas() <= worker.getBuildingType().gasPrice()))
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
		if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit && worker.unit()->getClosestUnit(Filter::IsAlly && Filter::IsCompleted  && Filter::GetType == UnitTypes::Protoss_Shield_Battery && Filter::Energy > 10))
		{
			worker.unit()->rightClick(worker.unit()->getClosestUnit(Filter::IsAlly && Filter::GetType == UnitTypes::Protoss_Shield_Battery));
			return;
		}
	}

	// Bunker repair logic
	if (Grids().getBunkerGrid(worker.getTilePosition()) > 0)
	{
		Unit bunker = worker.unit()->getClosestUnit(Filter::GetType == UnitTypes::Terran_Bunker && Filter::HP_Percent < 100);
		if (bunker && bunker->getHitPoints() < bunker->getType().maxHitPoints())
		{
			worker.unit()->repair(bunker);
			return;
		}
	}

	// If we are fast expanding and enemy is rushing, we need to defend with workers
	if ((Strategy().isAllyFastExpand() && BuildOrder().isOpener() && Units().getGlobalAllyStrength() + Units().getAllyDefense()*0.8 < Units().getGlobalEnemyStrength()) || (Grids().getEGroundThreat(worker.getWalkPosition()) > 0.0 && Grids().getResourceGrid(worker.getTilePosition()) > 0))
	{
		Units().storeAlly(worker.unit());
		Workers().removeWorker(worker.unit());
		return;
	}

	// Reassignment logic
	if (worker.getResource() && worker.getResource()->exists() && ((!Resources().isGasSaturated() && minWorkers > gasWorkers * 10) || (!Resources().isMinSaturated() && minWorkers < gasWorkers * 4)))
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
			worker.setTarget(nullptr);
			return;
		}
	}

	// If not targeting the mineral field the worker is mapped to
	if (!worker.unit()->isCarryingGas() && !worker.unit()->isCarryingMinerals())
	{
		if ((worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() == worker.getResource())
		{
			worker.setLastGatherFrame(Broodwar->getFrameCount());
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
				worker.setLastGatherFrame(Broodwar->getFrameCount());
				return;
			}
			// Else, move to it
			else
			{
				worker.unit()->move(worker.getResourcePosition());
				return;
			}
		}
		else if (worker.unit()->getClosestUnit(Filter::IsMineralField))
		{
			worker.unit()->gather(worker.unit()->getClosestUnit(Filter::IsMineralField));
			worker.setTarget(nullptr);
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
	}
	if (Resources().getMyMinerals().find(worker.getResource()) != Resources().getMyMinerals().end())
	{
		Resources().getMyMinerals()[worker.getResource()].setGathererCount(Resources().getMyMinerals()[worker.getResource()].getGathererCount() - 1);
	}
	assignWorker(worker);
	return;
}