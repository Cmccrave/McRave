#include "McRave.h"

bool isSmall(UnitType building)
{
	if (building.tileWidth() == 2 && building.tileHeight() == 2) return true;
	return false;
}

bool isMedium(UnitType building)
{
	if (building.tileWidth() == 3 && building.tileHeight() == 2) return true;
	return false;
}

bool isLarge(UnitType building)
{
	if (building.tileWidth() == 4 && building.tileHeight() == 3) return true;
	return false;
}

bool isDefensive(UnitType building)
{
	if (building == UnitTypes::Protoss_Shield_Battery || building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Terran_Bunker) return true;
	return false;
}


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
		int cnt = 0;
		for (auto &queued : buildingsQueued)
		{
			if (queued.second == b.first) cnt++;
		}

		// If our visible count is lower than our desired count
		if (b.second > Broodwar->self()->visibleUnitCount(b.first) && b.second - Broodwar->self()->visibleUnitCount(b.first) > cnt)
		{
			TilePosition here = Buildings().getBuildLocation(b.first);
			Unit builder = Workers().getClosestWorker(Position(here), true);

			// If the Tile Position and Builder are valid
			if (here.isValid() && builder)
			{
				// Queue at this building type a pair of building placement and builder	
				Workers().getMyWorkers()[builder].setBuildingType(b.first);
				Workers().getMyWorkers()[builder].setBuildPosition(here);
			}
		}
	}
}

void BuildingTrackerClass::constructBuildings()
{
	queuedMineral = 0;
	queuedGas = 0;
	buildingsQueued.clear();
	for (auto &worker : Workers().getMyWorkers())
	{
		if (worker.second.getBuildingType().isValid() && worker.second.getBuildPosition().isValid())
		{
			buildingsQueued[worker.second.getBuildPosition()] = worker.second.getBuildingType();
			queuedMineral += worker.second.getBuildingType().mineralPrice();
			queuedGas += worker.second.getBuildingType().gasPrice();
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
	// Refineries are only built on my own gas resources
	if (building.isRefinery())
	{
		double best = 0.0;
		TilePosition here = TilePositions::Invalid;
		for (auto &g : Resources().getMyGas())
		{
			ResourceInfo &gas = g.second;
			if (Grids().getBaseGrid(gas.getTilePosition()) > 1 && gas.getType() == UnitTypes::Resource_Vespene_Geyser && (gas.getPosition().getDistance(Terrain().getPlayerStartingPosition()) < best || best == 0.0))
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
			bestLocation = BWEB.getNatural();
		}

		// Other expansions must be as close to home but as far away from the opponent
		else
		{
			for (auto &area : theMap.Areas())
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

	// If we are doing nexus first
	if (BuildOrder().isOpener() && BuildOrder().isNexusFirst() && ((building == UnitTypes::Protoss_Gateway && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) <= 0) || building == UnitTypes::Protoss_Cybernetics_Core || (building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) <= 0)))
	{
		if (building == UnitTypes::Protoss_Pylon) return BWEB.getWall(BWEB.getNaturalArea()).getSmallWall();
		if (building == UnitTypes::Protoss_Cybernetics_Core) return BWEB.getWall(BWEB.getNaturalArea()).getMediumWall();
		if (building == UnitTypes::Protoss_Gateway) return BWEB.getWall(BWEB.getNaturalArea()).getLargeWall();
		return here;
	}

	// If we are forge expanding
	if (BuildOrder().isOpener() && BuildOrder().isForgeExpand() && (building == UnitTypes::Protoss_Photon_Cannon || (building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) <= 0) || (building == UnitTypes::Protoss_Gateway && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) <= 0) || (building == UnitTypes::Protoss_Forge && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Forge) <= 0)))
	{
		if (building == UnitTypes::Protoss_Pylon) return BWEB.getWall(BWEB.getNaturalArea()).getSmallWall();
		if (building == UnitTypes::Protoss_Forge) return BWEB.getWall(BWEB.getNaturalArea()).getMediumWall();
		if (building == UnitTypes::Protoss_Gateway) return BWEB.getWall(BWEB.getNaturalArea()).getLargeWall();
		if (building == UnitTypes::Protoss_Photon_Cannon)
		{
			if (Strategy().getEnemyBuild() == "Z12HatchMuta" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) >= 2 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) < 4) here = BWEB.getDefBuildPosition(building, &usedTiles);
			else here = BWEB.getDefBuildPosition(building, &usedTiles, BWEB.getSecondChoke());
		}
		return here;
	}	

	set<TilePosition> placements;
	if (isSmall(building))
	{
		if (isDefensive(building))
			placements = BWEB.getSDefPosition();
		else
			placements = BWEB.getSmallPosition();
	}
	else if (isMedium(building))
	{
		if (isDefensive(building))
			placements = BWEB.getMDefPosition();
		else
			placements = BWEB.getMediumPosition();
	}
	else if (isLarge(building)) placements = BWEB.getLargePosition();

	double distBest = DBL_MAX;
	for (auto tile : placements)
	{
		double dist = Position(tile).getDistance(Terrain().getPlayerStartingPosition());
		if (dist < distBest && isBuildable(building, tile) && isQueueable(building, tile))
		{
			Broodwar->drawCircleMap(Position(tile), 12, Colors::Red);
			here = tile;
			distBest = dist;
		}
	}

	if (here.isValid()) return here;

	here = BWEB.getBuildPosition(building, &usedTiles);
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
			if (buildTilePosition == gas.getTilePosition() && Grids().getBaseGrid(gas.getTilePosition()) > 1 && gas.getType() == UnitTypes::Resource_Vespene_Geyser) return true;
		}
		return false;
	}

	if (building.requiresPsi() && !Pylons().hasPower(buildTilePosition, building)) return false;
	if (usedTiles.find(buildTilePosition) != usedTiles.end()) return false;
	return true;
}

bool BuildingTrackerClass::isSuitable(UnitType building, TilePosition buildTilePosition)
{
	if (BuildOrder().isOpener() && BuildOrder().isForgeExpand() && building == UnitTypes::Protoss_Photon_Cannon)
	{
		Position center = Position(buildTilePosition + TilePosition(1, 1));
		if (center.getDistance(Position(BWEB.getSecondChoke())) < 128 || !Terrain().isInAllyTerritory(TilePosition(center))) return false;
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
	assert();
	return BuildingInfo();
}