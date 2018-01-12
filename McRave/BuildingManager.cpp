#include "McRave.h"

void BuildingTrackerClass::update()
{
	Display().startClock();
	updateBuildings();
	queueBuildings();
	constructBuildings();
	Display().performanceTest(__FUNCTION__);
	return;
}

void BuildingTrackerClass::updateBuildings()
{
	for (auto& b : myBuildings)
	{
		BuildingInfo building = b.second;
		building.setIdleStatus(building.unit()->getRemainingTrainTime() == 0);
		building.setEnergy(building.unit()->getEnergy());

		if (building.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit())
		{
			Unit builder = Workers().getClosestWorker(building.getPosition(), true);
			if (builder) builder->rightClick(building.unit());
		}
	}
	return;
}

void BuildingTrackerClass::queueBuildings()
{
	// For each building desired
	for (auto &b : BuildOrder().getBuildingDesired())
	{
		int buildingDesired = b.second;
		int buildingQueued = 0;
		for (auto &queued : buildingsQueued)
		{
			if (queued.second == b.first) buildingQueued++;
		}

		// If our visible count is lower than our desired count
		if (buildingDesired > buildingQueued + Broodwar->self()->visibleUnitCount(b.first))
		{
			TilePosition here = Buildings().getBuildLocation(b.first);
			Unit builder = Workers().getClosestWorker(Position(here), true);

			// If the Tile Position and Builder are valid
			if (here.isValid() && builder)
			{
				// Queue at this building type a pair of building placement and builder	
				Workers().getMyWorkers()[builder].setBuildingType(b.first);
				Workers().getMyWorkers()[builder].setBuildPosition(here);
				buildingsQueued[here] = b.first;
				buildingQueued++;
			}
		}
	}
}

void BuildingTrackerClass::constructBuildings()
{
	queuedMineral = 0;
	queuedGas = 0;
	buildingsQueued.clear();
	for (auto &w : Workers().getMyWorkers())
	{
		WorkerInfo &worker = w.second;
		if (worker.getBuildingType().isValid() && worker.getBuildPosition().isValid() && usedTiles.find(worker.getBuildPosition()) == usedTiles.end())
		{
			buildingsQueued[worker.getBuildPosition()] = worker.getBuildingType();
			queuedMineral += worker.getBuildingType().mineralPrice();
			queuedGas += worker.getBuildingType().gasPrice();
		}
	}
}

void BuildingTrackerClass::storeBuilding(Unit building)
{
	BuildingInfo &b = myBuildings[building];
	b.setUnit(building);
	b.setType(building->getType());
	b.setEnergy(building->getEnergy());
	b.setPosition(building->getPosition());
	b.setWalkPosition(Util().getWalkPosition(building));
	b.setTilePosition(building->getTilePosition());
	usedTiles.insert(building->getTilePosition());
	Grids().updateBuildingGrid(b);
	return;
}

void BuildingTrackerClass::removeBuilding(Unit building)
{
	Grids().updateBuildingGrid(myBuildings[building]);
	usedTiles.erase(building->getTilePosition());
	myBuildings.erase(building);
	return;
}

TilePosition BuildingTrackerClass::getBuildLocation(UnitType building)
{
	if (building.getRace() == Races::Zerg && !building.isResourceDepot())
	{
		return Broodwar->getBuildLocation(building, Terrain().getPlayerStartingTilePosition());
	}

	// Refineries are only built on my own gas resources
	if (building.isRefinery())
	{
		double best = 0.0;
		TilePosition here = TilePositions::Invalid;
		for (auto &g : Resources().getMyGas())
		{
			ResourceInfo &gas = g.second;
			if (gas.getState() >= 2 && gas.getType() == UnitTypes::Resource_Vespene_Geyser && (gas.getPosition().getDistance(Terrain().getPlayerStartingPosition()) < best || best == 0.0))
			{
				best = gas.getPosition().getDistance(Terrain().getPlayerStartingPosition());
				here = gas.getTilePosition();
			}
		}
		return here;
	}

	// If we are expanding, it must be on an expansion area
	double best = 0.0;
	TilePosition bestLocation, here = TilePositions::Invalid;
	if (building.isResourceDepot())
	{
		// Fast expands must be as close to home and have a gas geyser
		if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1 || Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center) == 1)
		{
			bestLocation = mapBWEB.getNatural();
		}

		// Other expansions must be as close to home but as far away from the opponent
		else
		{
			for (auto &area : mapBWEM.Areas())
			{
				for (auto &base : area.Bases())
				{
					if (area.AccessibleNeighbours().size() == 0 || base.Center() == Terrain().getEnemyStartingPosition()) continue;

					// Get value of the expansion
					double value = 0.0;
					for (auto &mineral : base.Minerals())
						value += double(mineral->Amount());
					for (auto &gas : base.Geysers())
						value += double(gas->Amount());
					if (base.Geysers().size() == 0) value = value / 10.0;

					// Get distance of the expansion
					double distance;
					if (Players().getPlayers().size() > 1)
						distance = Terrain().getGroundDistance(Terrain().getPlayerStartingPosition(), base.Center());
					else
						distance = Terrain().getGroundDistance(Terrain().getPlayerStartingPosition(), base.Center()) / pow(Terrain().getGroundDistance(Terrain().getEnemyStartingPosition(), base.Center()), 2.0);

					if (Broodwar->isBuildable(base.Location(), true) && value / distance > best)
					{
						best = value / distance;
						bestLocation = base.Location();
					}
				}
			}
		}
		currentExpansion = bestLocation;
		return bestLocation;
	}

	if (building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) >= 4)
	{
		here = findDefLocation(building, Terrain().getPlayerStartingPosition());
		if (here.isValid() && isBuildable(building, here)) return here;
	}

	// If this is a defensive building
	if (building == UnitTypes::Protoss_Photon_Cannon)
	{
		if (Strategy().getEnemyBuild() == "Z12HatchMuta")
			here = findDefLocation(building, Terrain().getPlayerStartingPosition());
		if (!here.isValid())
			here = findDefLocation(building, Position(mapBWEB.getSecondChoke()));
	}
	else
	{
		// If we are fast expanding
		if (BuildOrder().isOpener() && BuildOrder().isFastExpand())
		{
			BWEB::Wall naturalWall = mapBWEB.getWall(mapBWEB.getNaturalArea());
			if (building.tileWidth() == 2) here = naturalWall.getSmallWall();
			else if (building.tileWidth() == 3) here = naturalWall.getMediumWall();
			else if (building.tileWidth() == 4) here = naturalWall.getLargeWall();
			if (here.isValid() && isBuildable(building, here)) return here;
		}

		here = findProdLocation(building, Terrain().getPlayerStartingPosition());
	}
	if (here.isValid()) return here;
	return TilePositions::Invalid;
}

bool BuildingTrackerClass::isQueueable(UnitType building, TilePosition buildTilePosition)
{
	// Check if there's a building queued there already
	for (auto &queued : buildingsQueued)
	{
		if (buildTilePosition.x < queued.first.x + queued.second.tileWidth() && queued.first.x < buildTilePosition.x + building.tileWidth() && buildTilePosition.y < queued.first.y + queued.second.tileHeight() && queued.first.y < buildTilePosition.y + building.tileHeight()) return false;
	}
	return true;
}

bool BuildingTrackerClass::isBuildable(UnitType building, TilePosition buildTilePosition)
{
	// Refineries are only built on my own gas resources
	if (building.isRefinery())
	{
		double best = 0.0;
		TilePosition here = TilePositions::Invalid;
		for (auto &g : Resources().getMyGas())
		{
			ResourceInfo &gas = g.second;
			if (buildTilePosition == gas.getTilePosition() && gas.getState() >= 2 && gas.getType() == UnitTypes::Resource_Vespene_Geyser) return true;
		}
		return false;
	}

	if (building.requiresPsi() && !Pylons().hasPower(buildTilePosition, building)) return false;
	if (usedTiles.find(buildTilePosition) != usedTiles.end()) return false;
	return true;
}

bool BuildingTrackerClass::isSuitable(UnitType building, TilePosition buildTilePosition)
{
	if (BuildOrder().isOpener() && BuildOrder().isFastExpand() && building == UnitTypes::Protoss_Photon_Cannon)
	{
		Position center = Position(buildTilePosition + TilePosition(1, 1));
		if (center.getDistance(Position(mapBWEB.getSecondChoke())) < 128 || !Terrain().isInAllyTerritory(TilePosition(center))) return false;
	}
	return true;
}

set<Unit> BuildingTrackerClass::getAllyBuildingsFilter(UnitType type)
{
	returnValues.clear();
	for (auto &u : myBuildings)
	{
		BuildingInfo &unit = u.second;
		if (unit.getType() == type)
		{
			returnValues.insert(unit.unit());
		}
	}
	return returnValues;
}

BuildingInfo& BuildingTrackerClass::getAllyBuilding(Unit building)
{
	if (myBuildings.find(building) != myBuildings.end()) return myBuildings[building];
	return BuildingInfo();
}

TilePosition BuildingTrackerClass::findDefLocation(UnitType building, Position here)
{
	set<TilePosition> placements;
	TilePosition tileBest = TilePositions::Invalid;
	double distBest = DBL_MAX;
	BWEB::Station station = mapBWEB.getClosestStation(TilePosition(here));

	if (building == UnitTypes::Protoss_Photon_Cannon)
	{
		for (auto &w : mapBWEB.getWalls())
		{
			BWEB::Wall wall = w.second;

			for (auto &tile : wall.getDefenses())
			{
				Position defenseCenter = Position(tile) + Position(32, 32);
				double dist = defenseCenter.getDistance(here);
				if (dist < distBest && usedTiles.find(tile) == usedTiles.end() && isQueueable(building, tile) && isBuildable(building, tile))
					distBest = dist, tileBest = tile;
			}
		}
	}

	for (auto &defense : station.DefenseLocations())
	{
		if (building == UnitTypes::Protoss_Pylon)
		{
			double dist = Position(defense).getDistance(Position(station.ResourceCenter()));
			if (dist < distBest)
			{
				distBest = dist;
				if (usedTiles.find(defense) == usedTiles.end() && Grids().getPylonGrid(TilePosition(station.BWEMBase()->Center())) <= 0 && isQueueable(building, defense) && isBuildable(building, defense))
					tileBest = defense;
			}
		}
		else
		{
			for (auto tile : placements)
			{
				double dist = Position(tile).getDistance(here);
				if (dist < distBest && usedTiles.find(tile) == usedTiles.end() && isQueueable(building, tile) && isBuildable(building, tile))
					distBest = dist, tileBest = tile;
			}
		}
	}
	return tileBest;
}

TilePosition BuildingTrackerClass::findProdLocation(UnitType building, Position here)
{
	set<TilePosition> placements;
	TilePosition tileBest = TilePositions::Invalid;
	double distBest = DBL_MAX;
	for (auto &block : mapBWEB.Blocks())
	{
		Position blockCenter = Position(block.Location()) + Position(block.width() * 16, block.height() * 16);

		if (building.tileWidth() == 4) placements = block.LargeTiles();
		else if (building.tileWidth() == 3) placements = block.MediumTiles();
		else placements = block.SmallTiles();
		double dist = blockCenter.getDistance(here);

		if (dist < distBest)
		{
			for (auto tile : placements)
			{
				if (usedTiles.find(tile) == usedTiles.end() && isQueueable(building, tile) && isBuildable(building, tile))
					distBest = dist, tileBest = tile;
			}
		}
	}
	return tileBest;
}