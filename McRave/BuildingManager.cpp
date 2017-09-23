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
		if (!building.unit() || !building.unit()->exists())
		{
			continue;
		}
		
		building.setIdleStatus(building.unit()->getRemainingTrainTime() == 0);
		building.setEnergy(building.unit()->getEnergy());

		if (b.second.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit())
		{
			Unit builder = Workers().getClosestWorker(building.getPosition());
			if (builder)
			{
				builder->rightClick(building.unit());
			}
			continue;
		}
	}
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
	Grids().updateBuildingGrid(b);	
	return;
}

void BuildingTrackerClass::removeBuilding(Unit building)
{
	Grids().updateBuildingGrid(myBuildings[building]);
	myBuildings.erase(building);
	return;
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
	while (length < 50)
	{
		// If we can build here, return this tile position		
		if (TilePosition(x, y).isValid() && canBuildHere(building, TilePosition(x, y), ignoreCond))
		{
			bool fine = true;
			for (auto &w : Workers().getMyWorkers())
			{
				WorkerInfo worker = w.second;
				if (worker.getBuildPosition().isValid() && worker.getBuildPosition().getDistance(buildTilePosition) < worker.getBuildingType().tileWidth())
				{
					fine = false;
				}
			}
			if (fine)
			{
				return TilePosition(x, y);
			}
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
	double closestD = 0.0;
	TilePosition closestP;
	if (building.isResourceDepot())
	{
		// Fast expands must be as close to home and have a gas geyser
		if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1)
		{
			for (auto &area : theMap.Areas())
			{
				for (auto &base : area.Bases())
				{
					if ((base.Geysers().size() == 0) || area.AccessibleNeighbours().size() == 0)
					{
						continue;
					}
					if (Grids().getBaseGrid(base.Location()) == 0 && (Grids().getDistanceHome(WalkPosition(base.Location())) < closestD || closestD == 0))
					{
						closestD = Grids().getDistanceHome(WalkPosition(base.Location()));
						closestP = base.Location();
					}
				}
			}
		}

		// Other expansions must be as close to home but as far away from the opponent
		else
		{
			for (auto &area : theMap.Areas())
			{
				for (auto &base : area.Bases())
				{
					if (area.AccessibleNeighbours().size() == 0 || base.Center() == Terrain().getEnemyStartingPosition())
					{
						continue;
					}
					if (Grids().getReserveGrid(base.Location()) == 0 && (Grids().getDistanceHome(WalkPosition(base.Location())) / base.Center().getDistance(Terrain().getEnemyStartingPosition()) < closestD || closestD == 0))
					{
						closestD = Grids().getDistanceHome(WalkPosition(base.Location())) / base.Center().getDistance(Terrain().getEnemyStartingPosition());
						closestP = base.Location();
					}
				}
			}
		}
		return closestP;
	}

	// If we are fast expanding
	if (Strategy().isAllyFastExpand())
	{
		if (building == UnitTypes::Protoss_Pylon && Grids().getPylonGrid(Terrain().getFFEPosition()) <= 0)
		{
			return getBuildLocationNear(building, Terrain().getFFEPosition());
		}
		if (building == UnitTypes::Protoss_Photon_Cannon)
		{
			return getBuildLocationNear(building, Terrain().getSecondChoke());
		}
		if ((building == UnitTypes::Protoss_Gateway || building == UnitTypes::Protoss_Forge) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Forge) <= 0)
		{
			return getBuildLocationNear(building, Terrain().getSecondChoke());
		}
	}

	// If we are being rushed and need a battery
	if (Strategy().isRush())
	{
		if (building == UnitTypes::Protoss_Shield_Battery)
		{
			return getBuildLocationNear(building, Terrain().getFirstChoke());
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
			if (base.second.unit()->isCompleted() && Grids().getDefenseGrid(base.second.getTilePosition()) < 2 && Broodwar->hasPower(TilePosition(base.second.getPosition())))
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
	// Production buildings that create ground units require spacing so they don't trap units -- TEMP: Supply depot to not block SCVs (need to find solution)
	if (building == UnitTypes::Terran_Supply_Depot || (!building.isResourceDepot() && building.buildsWhat().size() > 0))
	{
		buildingOffset = 1;
	}
	else
	{
		buildingOffset = 0;
	}

	// Refineries are only built on my own gas resources
	if (building.isRefinery())
	{
		for (auto &gas : Resources().getMyGas())
		{
			if (Grids().getBaseGrid(gas.second.getTilePosition()) > 0 && buildTilePosition == gas.second.getTilePosition() && gas.second.getType() == UnitTypes::Resource_Vespene_Geyser)
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

	if (building == UnitTypes::Protoss_Shield_Battery && Broodwar->getUnitsInRadius(Position(buildTilePosition), 128, Filter::IsResourceDepot).size() == 0)
	{
		return false;
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
				if (Grids().getReserveGrid(x, y) > 0 && !building.isResourceDepot())
				{
					return false;
				}

				// If it's a pylon and overlapping too many pylons
				if (!Strategy().isAllyFastExpand() && building == UnitTypes::Protoss_Pylon && Grids().getPylonGrid(x, y) >= 1)
				{
					return false;
				}

				// If it's not a cannon and on top of the resource grid
				if (!building.isResourceDepot() && building != UnitTypes::Protoss_Photon_Cannon && building != UnitTypes::Protoss_Shield_Battery && building != UnitTypes::Terran_Bunker && !ignoreCond && Grids().getResourceGrid(x, y) > 0)
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
				if (!TilePosition(x, y).isValid())
				{
					return false;
				}
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