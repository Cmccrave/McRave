#include "McRave.h"

void BuildingTrackerClass::update()
{
	queueBuildings();
	constructBuildings();
	Display().performanceTest(__func__);
	return;
}

void BuildingTrackerClass::queueBuildings()
{
	// For each building desired
	for (auto &b : BuildOrder().getBuildingDesired())
	{
		// If our visible count is lower than our desired count
		if (b.second > Broodwar->self()->visibleUnitCount(b.first) && b.second - Broodwar->self()->visibleUnitCount(b.first) > buildingsQueued[b.first])
		{
			TilePosition here = Buildings().getBuildLocation(b.first);
			Unit builder = Workers().getClosestWorker(Position(here));

			// If the Tile Position and Builder are valid
			if (here.isValid() && builder)
			{
				// Queue at this building type a pair of building placement and builder		
				Workers().getMyWorkers()[builder].setBuildingType(b.first);
				Workers().getMyWorkers()[builder].setBuildPosition(here);
				Grids().updateReservedLocation(b.first, here);
			}
		}
	}
}

void BuildingTrackerClass::constructBuildings()
{
	queuedMineral = 0;
	queuedGas = 0;
	for (auto &building : buildingsQueued)
	{
		building.second = 0;
	}
	for (auto &worker : Workers().getMyWorkers())
	{
		if (worker.second.getBuildingType().isValid() && worker.second.getBuildPosition().isValid())
		{
			buildingsQueued[worker.second.getBuildingType()] += 1;
			queuedMineral += worker.second.getBuildingType().mineralPrice();
			queuedGas += worker.second.getBuildingType().gasPrice();
		}
	}
	for (auto &building : myBuildings)
	{
		if (building.second.getType().getRace() == Races::Terran && !building.first->isCompleted() && !building.first->getBuildUnit())
		{
			Unit builder = Workers().getClosestWorker(building.second.getPosition());
			if (builder)
			{
				builder->rightClick(building.first);
			}
			continue;
		}
	}
}

void BuildingTrackerClass::storeBuilding(Unit building)
{
	myBuildings[building].setUnit(building);
	myBuildings[building].setUnitType(building->getType());
	myBuildings[building].setPosition(building->getPosition());
	myBuildings[building].setWalkPosition(Util().getWalkPosition(building));
	myBuildings[building].setTilePosition(building->getTilePosition());
	myBuildings[building].setIdleStatus(building->getRemainingTrainTime() == 0);
	myBuildings[building].setEnergy(building->getEnergy());
}

void BuildingTrackerClass::storeBattery(Unit building)
{
	myBatteries[building].setUnit(building);
	myBatteries[building].setUnitType(building->getType());
	myBatteries[building].setPosition(building->getPosition());
	myBatteries[building].setWalkPosition(Util().getWalkPosition(building));
	myBatteries[building].setTilePosition(building->getTilePosition());
	myBatteries[building].setIdleStatus(building->getRemainingTrainTime() == 0);
	myBatteries[building].setEnergy(building->getEnergy());
}

void BuildingTrackerClass::removeBuilding(Unit building)
{
	myBuildings.erase(building);
}

TilePosition BuildingTrackerClass::getBuildLocationNear(UnitType building, TilePosition buildTilePosition, bool ignoreCond = false)
{
	int x = buildTilePosition.x;
	int y = buildTilePosition.y;
	int length = 1;
	int j = 0;
	bool first = true;
	int dx = 0;
	int dy = 1;

	// Searches in a spiral around the specified tile position
	while (length < 200)
	{
		// If we can build here, return this tile position		
		if (TilePosition(x, y).isValid() && canBuildHere(building, TilePosition(x, y), ignoreCond))
		{
			return TilePosition(x, y);
		}

		// Otherwise spiral out and find a new tile
		x = x + dx;
		y = y + dy;
		j++;
		if (j == length)
		{
			j = 0;
			if (!first)
				length++;
			first = !first;
			if (dx == 0)
			{
				dx = dy;
				dy = 0;
			}
			else
			{
				dy = -dx;
				dx = 0;
			}
		}
	}
	if (Broodwar->getFrameCount() - errorTime > 500)
	{
		Broodwar << "Issues placing: " << building.c_str() << endl;
		errorTime = Broodwar->getFrameCount();
	}
	return TilePositions::None;
}

TilePosition BuildingTrackerClass::getBuildLocation(UnitType building)
{
	// If we are expanding, it must be on an expansion area
	int closestD = 0;
	TilePosition closestP;
	if (building.isResourceDepot())
	{
		for (auto &area : theMap.Areas())
		{
			for (auto &base : area.Bases())
			{
				if ((Strategy().isFastExpand() && base.Geysers().size() == 0) || area.AccessibleNeighbours().size() == 0)
				{
					continue;
				}
				if (Grids().getReserveGrid(base.Location()) == 0 && (Grids().getDistanceHome(WalkPosition(base.Location())) < closestD || closestD == 0))
				{
					closestD = Grids().getDistanceHome(WalkPosition(base.Location()));
					closestP = base.Location();
				}
			}
		}
		return closestP;
	}

	/*if (Broodwar->self()->getRace() == Races::Terran && (building == UnitTypes::Terran_Supply_Depot || building == UnitTypes::Terran_Barracks))
	{
	for (auto wall : BWEM::utils::findWalls(theMap.Instance()))
	{
	if (wall.Center().getDistance(Terrain().getPlayerStartingPosition()) < 640 && wall.Possible())
	{

	}
	}
	}*/

	// If we are fast expanding
	if (Strategy().isFastExpand())
	{
		if (building == UnitTypes::Protoss_Pylon && Grids().getPylonGrid(Terrain().getFFEPosition()) <= 0 + Strategy().isBust())
		{
			return getBuildLocationNear(building, Terrain().getFFEPosition());
		}
		if (building == UnitTypes::Protoss_Photon_Cannon)
		{
			return getBuildLocationNear(building, Terrain().getSecondChoke());
		}
		if (building == UnitTypes::Protoss_Forge && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Forge) == 0)
		{
			return getBuildLocationNear(building, Terrain().getSecondChoke());
		}
	}

	// For each base, check if there's a Pylon or Cannon needed
	for (auto &base : Bases().getMyBases())
	{
		if (building == UnitTypes::Protoss_Pylon && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) >= 2)
		{
			if (Grids().getPylonGrid(base.second.getTilePosition()) == 0)
			{
				return getBuildLocationNear(building, base.second.getTilePosition());
			}
		}
		if (building == UnitTypes::Protoss_Photon_Cannon)
		{
			if (Grids().getDefenseGrid(base.second.getTilePosition()) < Grids().getDistanceHome(base.second.getWalkPosition()) / 100)
			{
				return getBuildLocationNear(building, base.second.getResourcesPosition());
			}
		}
	}

	// For each base, check if you can build near it, starting at the main
	for (auto &base : Bases().getMyOrderedBases())
	{
		TilePosition here = getBuildLocationNear(building, base.second);
		if (here.isValid())
		{
			return here;
		}
	}
	return TilePositions::None;
}

bool BuildingTrackerClass::canBuildHere(UnitType building, TilePosition buildTilePosition, bool ignoreCond)
{
	// Attempt to place Cannons in a concave around the second choke on a fast expansion
	if (Strategy().isFastExpand())
	{		
		if (building == UnitTypes::Protoss_Photon_Cannon && Position(Terrain().getSecondChoke()).getDistance(Position(buildTilePosition)) < 128)
		{
			return false;
		}
	}

	// Production buildings that create ground units require spacing so they don't trap units -- TEMP: Supply depot to not block SCVs (need to find solution)
	if (building == UnitTypes::Terran_Supply_Depot || building == UnitTypes::Protoss_Gateway || building == UnitTypes::Protoss_Robotics_Facility || building == UnitTypes::Terran_Barracks || building == UnitTypes::Terran_Factory)
	{
		buildingOffset = 1;
	}

	// Refineries are only built on my own gas resources
	if (building.isRefinery())
	{
		for (auto &gas : Resources().getMyGas())
		{
			if (buildTilePosition == gas.second.getTilePosition() && gas.second.getType() == UnitTypes::Resource_Vespene_Geyser)
			{
				return true;
			}
		}
		return false;
	}

	// If Protoss, check for power
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		// Check if it's not a pylon and in a preset buildable position based on power grid
		if (building.requiresPsi() && !Pylons().hasPower(buildTilePosition, building))
		{
			return false;
		}
	}

	// For every tile of a buildings size
	for (int x = buildTilePosition.x; x < buildTilePosition.x + building.tileWidth(); x++)
	{
		for (int y = buildTilePosition.y; y < buildTilePosition.y + building.tileHeight(); y++)
		{
			// Checking if the tile is valid, and the reasons that could make this position an unsuitable build location
			if (TilePosition(x, y).isValid())
			{
				// If it's reserved
				if (Grids().getReserveGrid(x, y) > 0)
				{
					return false;
				}

				// If it's a pylon and overlapping too many pylons
				if (!Strategy().isFastExpand() && building == UnitTypes::Protoss_Pylon && Grids().getPylonGrid(x, y) >= 1)
				{
					return false;
				}

				// If it's not a cannon and on top of the resource grid
				if (building != UnitTypes::Protoss_Shield_Battery && building != UnitTypes::Terran_Bunker && !ignoreCond && Grids().getResourceGrid(x, y) > 0)
				{
					return false;
				}

				// If it's on an unbuildable tile
				if (!Broodwar->isBuildable(TilePosition(x, y), true))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}

	// If the building requires an offset (production buildings and first pylon)
	if (buildingOffset > 0)
	{
		for (int x = buildTilePosition.x - buildingOffset; x < buildTilePosition.x + building.tileWidth() + buildingOffset; x++)
		{
			for (int y = buildTilePosition.y - buildingOffset; y < buildTilePosition.y + building.tileHeight() + buildingOffset; y++)
			{
				if (Grids().getReserveGrid(x, y) > 0 && !Broodwar->isBuildable(TilePosition(x, y), true))
				{
					return false;
				}
				if (building == UnitTypes::Protoss_Pylon && !Broodwar->isBuildable(TilePosition(x, y), true))
				{
					return false;
				}
			}
		}
	}

	// If the building is not a resource depot and being placed on an expansion
	if (!building.isResourceDepot())
	{
		for (auto &base : Terrain().getAllBaseLocations())
		{
			for (int i = 0; i < building.tileWidth(); i++)
			{
				for (int j = 0; j < building.tileHeight(); j++)
				{
					// If the x value of this tile of the building is within an expansion and the y value of this tile of the building is within an expansion, return false
					if (buildTilePosition.x + i >= base.x && buildTilePosition.x + i < base.x + 4 && buildTilePosition.y + j >= base.y && buildTilePosition.y + j < base.y + 3)
					{
						return false;
					}
				}
			}
		}
	}

	// If the building can build addons
	if (building.canBuildAddon())
	{
		for (int x = buildTilePosition.x + building.tileWidth(); x <= buildTilePosition.x + building.tileWidth() + 2; x++)
		{
			for (int y = buildTilePosition.y + 1; y <= buildTilePosition.y + 3; y++)
			{
				if (Grids().getReserveGrid(x, y) > 0 || !Broodwar->isBuildable(TilePosition(x, y), true))
				{
					return false;
				}
			}
		}
	}

	// If no issues, return true
	return true;
}