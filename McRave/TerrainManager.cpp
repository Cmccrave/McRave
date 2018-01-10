#include "McRave.h"

void TerrainTrackerClass::update()
{
	Display().startClock();
	updateAreas();
	updateChokes();
	Display().performanceTest(__FUNCTION__);
	return;
}

void TerrainTrackerClass::updateAreas()
{
	if (enemyStartingPosition.isValid() && !enemyNatural.isValid())
	{
		findEnemyNatural();
	}

	// If we see a building, check for closest starting location
	if (!enemyStartingPosition.isValid())
	{
		for (auto &u : Units().getEnemyUnits())
		{
			UnitInfo &unit = u.second;
			double distance = 10000.0;
			TilePosition closest;
			if (!unit.getType().isBuilding() || !unit.getTilePosition().isValid()) continue;
			for (auto start : Broodwar->getStartLocations())
			{
				if (start.getDistance(unit.getTilePosition()) < distance)
				{
					distance = start.getDistance(unit.getTilePosition());
					closest = start;
				}
			}
			if (closest.isValid())
			{
				enemyStartingPosition = Position(TilePosition(closest.x + 2, closest.y + 1));
				enemyStartingTilePosition = closest;
			}
		}
	}

	// If there is at least one base position, set up attack position
	if (Stations().getEnemyStations().size() > 0 && Grids().getEnemyArmyCenter().isValid())
	{
		double closestD = 0.0;
		Position closestP;
		for (auto &station : Stations().getEnemyStations())
		{
			if (enemyStartingPosition.getDistance(station.BWEMBase()->Center()) > closestD)
			{
				closestD = enemyStartingPosition.getDistance(station.BWEMBase()->Center());
				closestP = station.BWEMBase()->Center();
			}
		}
		attackPosition = closestP;
	}
	else if (enemyStartingPosition.isValid() && !Broodwar->isExplored(enemyStartingTilePosition)) attackPosition = enemyStartingPosition;
	else attackPosition = Positions::Invalid;

	if (!Strategy().isHoldChoke() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) <= 1) defendPosition = playerStartingPosition;
	else if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) <= 1) defendPosition = Position(firstChoke);
	else if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) > 1) defendPosition = Position(secondChoke);

	// Set mineral holding positions
	for (auto &station : Stations().getMyStations())
	{
		mineralHold = Position(station.ResourceCenter());
		backMineralHold = (Position(station.ResourceCenter()) - Position(station.BWEMBase()->Center())) + Position(station.ResourceCenter());
	}
}

void TerrainTrackerClass::updateChokes()
{
	// Add "empty" areas (ie. Andromeda areas around main)	
	if (mapBWEB.getNaturalArea())
	{
		for (auto &area : mapBWEB.getNaturalArea()->AccessibleNeighbours())
		{
			for (auto & choke : area->ChokePoints())
			{
				if (TilePosition(choke->Center()) == firstChoke && allyTerritory.find(area->Id()) == allyTerritory.end())
				{
					allyTerritory.insert(area->Id());
				}
			}
		}

		if (BuildOrder().isForgeExpand() || BuildOrder().isNexusFirst() || BuildOrder().getBuildingDesired()[UnitTypes::Protoss_Nexus] > 1) allyTerritory.insert(mapBWEB.getNaturalArea()->Id());
	}
}

bool TerrainTrackerClass::overlapsBases(TilePosition here)
{
	for (auto base : allBases)
	{
		TilePosition tile = base->Location();
		if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 3) return true;
	}
	return false;
}

void TerrainTrackerClass::onStart()
{
	mapBWEM.Initialize();
	mapBWEM.EnableAutomaticPathAnalysis();
	bool startingLocationsOK = mapBWEM.FindBasesForStartingLocations();
	assert(startingLocationsOK);
	playerStartingTilePosition = Broodwar->self()->getStartLocation();
	playerStartingPosition = Position(playerStartingTilePosition) + Position(64,48);

	// Store non island bases	
	for (auto &area : mapBWEM.Areas())
	{
		if (area.AccessibleNeighbours().size() == 0) continue;
		for (auto &base : area.Bases())
		{
			allBases.insert(&base);
		}
	}

	return;
}

void TerrainTrackerClass::findEnemyNatural()
{
	// Find enemy natural area
	double distBest = DBL_MAX;
	for (auto &area : mapBWEM.Areas())
	{
		for (auto &base : area.Bases())
		{
			if (base.Geysers().size() == 0 || area.AccessibleNeighbours().size() == 0) continue;
			if (base.Center().getDistance(enemyStartingPosition) < 128) continue;

			double dist = getGroundDistance(base.Center(), enemyStartingPosition);
			if (dist < distBest)
				enemyNatural = base.Location(), distBest = dist;
		}
	}
}

bool TerrainTrackerClass::isInAllyTerritory(TilePosition here)
{
	// Find the area of this tile position and see if it exists in ally territory
	if (here.isValid() && mapBWEM.GetArea(here) && allyTerritory.find(mapBWEM.GetArea(here)->Id()) != allyTerritory.end()) return true;
	return false;
}

bool TerrainTrackerClass::isInEnemyTerritory(TilePosition here)
{
	// Find the area of this tile position and see if it exists in enemy territory
	if (here.isValid() && mapBWEM.GetArea(here) && enemyTerritory.find(mapBWEM.GetArea(here)->Id()) != enemyTerritory.end()) return true;
	return false;
}

Position TerrainTrackerClass::getClosestBaseCenter(Position here)
{
	// Find the base within the area of this position if one exists
	double distBest = DBL_MAX;
	Position posBest;
	if (!here.isValid() || !mapBWEM.GetArea(TilePosition(here))) return Positions::Invalid;
	for (auto &base : mapBWEM.GetArea(TilePosition(here))->Bases())
	{
		double dist = here.getDistance(base.Center());
		if (dist < distBest)
			posBest = base.Center(), distBest = here.getDistance(base.Center());
	}
	return posBest;
}

double TerrainTrackerClass::getGroundDistance(Position start, Position end)
{
	double dist = 0.0;
	if (!start.isValid() || !end.isValid()) return INT_MAX;
	if (!mapBWEM.GetArea(WalkPosition(start)) || !mapBWEM.GetArea(WalkPosition(end))) return INT_MAX;

	for (auto cpp : BWEM::Map::Instance().GetPath(start, end))
	{
		auto center = Position{ cpp->Center() };
		dist += start.getDistance(center);
		start = center;
	}

	return dist += start.getDistance(end);
}

bool TerrainTrackerClass::overlapsNeutrals(TilePosition here)
{
	for (auto &m : Resources().getMyMinerals())
	{
		ResourceInfo &mineral = m.second;
		if (here.x >= mineral.getTilePosition().x && here.x < mineral.getTilePosition().x + 2 && here.y >= mineral.getTilePosition().y && here.y < mineral.getTilePosition().y + 1) return true;
	}

	for (auto &g : Resources().getMyGas())
	{
		ResourceInfo &gas = g.second;
		if (here.x >= gas.getTilePosition().x && here.x < gas.getTilePosition().x + 4 && here.y >= gas.getTilePosition().y && here.y < gas.getTilePosition().y + 2) return true;
	}
	return false;
}