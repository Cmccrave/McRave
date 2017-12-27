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
			bestLocation = Terrain().getNatural();
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
		if (building == UnitTypes::Protoss_Pylon) return Terrain().getSmallWall();
		if (building == UnitTypes::Protoss_Cybernetics_Core) return Terrain().getMediumWall();
		if (building == UnitTypes::Protoss_Gateway) return Terrain().getLargeWall();
		//if (building == UnitTypes::Protoss_Photon_Cannon) here = getBuildLocationNear(building, Terrain().getSecondChoke());
		//if (!here.isValid()) here = getBuildLocationNear(building, Terrain().getFFEPosition(), true);
		return here;
	}

	// If we are forge expanding
	if (BuildOrder().isOpener() && BuildOrder().isForgeExpand() && (building == UnitTypes::Protoss_Photon_Cannon || (building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) <= 0) || (building == UnitTypes::Protoss_Gateway && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) <= 0) || (building == UnitTypes::Protoss_Forge && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Forge) <= 0)))
	{
		if (building == UnitTypes::Protoss_Pylon) return Terrain().getSmallWall();
		if (building == UnitTypes::Protoss_Forge) return Terrain().getMediumWall();
		if (building == UnitTypes::Protoss_Gateway) return Terrain().getLargeWall();
		if (building == UnitTypes::Protoss_Photon_Cannon)
		{
		//	if (Strategy().getEnemyBuild() == "Z12HatchMuta" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) >= 2 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) < 4) here = getBuildLocationNear(building, Terrain().getPlayerStartingTilePosition());
		//	else here = getBuildLocationNear(building, Terrain().getSecondChoke());
		}
		//if (!here.isValid()) here = getBuildLocationNear(building, Terrain().getFFEPosition(), true);
		return here;
	}	

	/*if (building == UnitTypes::Protoss_Photon_Cannon)
	{
		for (auto &base : Bases().getMyBases())
		{
			if (Grids().getDefenseGrid(base.second.getTilePosition()) <= 0 && Grids().getPylonGrid(base.second.getTilePosition()) > 0) return getBuildLocationNear(building, base.second.getTilePosition());
		}
		return TilePositions::Invalid;
	}*/

	here = theBuilder.getBuildPosition(building, &usedTiles);
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

	//if (BuildOrder().isOpener() && (BuildOrder().isForgeExpand() || BuildOrder().isNexusFirst()))
	//{
	//	if (building == UnitTypes::Protoss_Gateway && buildTilePosition == Terrain().getLargeWall()) return true;
	//	if ((building == UnitTypes::Protoss_Forge || building == UnitTypes::Protoss_Cybernetics_Core) && buildTilePosition == Terrain().getMediumWall()) return true;
	//}

	//if (Broodwar->self()->getRace() == Races::Protoss && building.requiresPsi() && !Pylons().hasPower(buildTilePosition, building)) return false; // If Protoss, check if it's not a pylon and in a preset buildable position based on power grid
	//if (building == UnitTypes::Protoss_Shield_Battery && Strategy().isHoldChoke() && (Terrain().getDefendPosition().getDistance(Position(buildTilePosition)) < 256 || !Terrain().isInAllyTerritory(buildTilePosition))) return false;

	//for (int x = buildTilePosition.x; x < buildTilePosition.x + building.tileWidth(); x++)
	//{
	//	for (int y = buildTilePosition.y; y < buildTilePosition.y + building.tileHeight(); y++)
	//	{
	//		if (!TilePosition(x, y).isValid()) return false;	
	//		if (!theMap.GetTile(TilePosition(x, y)).Buildable() || !Broodwar->isBuildable(x, y, true)) return false;
	//		if (Terrain().overlapsBases(TilePosition(x, y)) && !building.isResourceDepot()) return false;
	//		if (building == UnitTypes::Protoss_Pylon && Grids().getPylonGrid(x, y) > 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) < 4) return false;
	//		if (building == UnitTypes::Protoss_Photon_Cannon && x >= Terrain().getMediumWall().x && x < Terrain().getMediumWall().x + 3 && y >= Terrain().getMediumWall().y && y < Terrain().getMediumWall().y + 2) return false;
	//		if (building == UnitTypes::Protoss_Photon_Cannon && x >= Terrain().getLargeWall().x && x < Terrain().getLargeWall().x + 4 && y >= Terrain().getLargeWall().y && y < Terrain().getLargeWall().y + 3) return false;
	//	}
	//}	
	
	// If no issues, return true
	return true;
}

bool BuildingTrackerClass::isSuitable(UnitType building, TilePosition buildTilePosition)
{
	if (BuildOrder().isOpener() && BuildOrder().isForgeExpand() && building == UnitTypes::Protoss_Photon_Cannon)
	{
		Position center = Position(buildTilePosition + TilePosition(1, 1));
		if (center.getDistance(Position(Terrain().getSecondChoke())) < 128 || !Terrain().isInAllyTerritory(TilePosition(center))) return false;
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