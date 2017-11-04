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
	// If we see a building, check for closest starting location
	if (Bases().getEnemyBases().size() <= 0)
	{
		for (auto &u : Units().getEnemyUnits())
		{
			UnitInfo &unit = u.second;
			double distance = 0.0;
			TilePosition closest;
			if (!unit.getType().isBuilding()) continue;
			enemyStartingPosition = getClosestBaseCenter(unit.unit());
			enemyStartingTilePosition = TilePosition(getClosestBaseCenter(unit.unit()));
		}
	}	

	// If there is at least one base position, set up attack position
	if (Bases().getEnemyBases().size() > 0 && Grids().getEnemyArmyCenter().isValid())
	{
		double closestD = 0.0;
		Position closestP;
		for (auto &b : Bases().getEnemyBases())
		{
			BaseInfo &base = b.second;
			Broodwar->drawCircleMap(base.getPosition(), 16, Colors::Red);
			if (Grids().getEnemyArmyCenter().getDistance(base.getPosition()) > closestD)
			{
				closestD = Grids().getEnemyArmyCenter().getDistance(base.getPosition());
				closestP = base.getPosition();
			}
		}
		attackPosition = closestP;
	}
	else attackPosition = Positions::Invalid;

	// Set mineral holding positions
	for (auto &base : Bases().getMyBases())
	{
		mineralHold = Position(base.second.getResourcesPosition());
		backMineralHold = (Position(base.second.getResourcesPosition()) - Position(base.second.getPosition())) + Position(base.second.getResourcesPosition());
	}
}

void TerrainTrackerClass::updateChokes()
{
	// Store non island bases	
	for (auto &area : theMap.Areas())
	{
		if (area.AccessibleNeighbours().size() > 0)
		{
			for (auto &base : area.Bases())
			{
				allBaseLocations.insert(base.Location());
			}
		}
	}

	// Establish FFE position	
	if (Broodwar->getFrameCount() > 100)
	{
		int x = 0;
		int y = 0;
		const Area* closestA = nullptr;
		double closestBaseDistance = 0.0, furthestChokeDistance = 0.0, closestChokeDistance = 0.0;
		TilePosition natural;
		for (auto &area : theMap.Areas())
		{
			for (auto &base : area.Bases())
			{
				if (base.Geysers().size() == 0 || area.AccessibleNeighbours().size() == 0)
				{
					continue;
				}

				if (Grids().getDistanceHome(WalkPosition(base.Location())) > 50 && (Grids().getDistanceHome(WalkPosition(base.Location())) < closestBaseDistance || closestBaseDistance == 0))
				{
					closestBaseDistance = Grids().getDistanceHome(WalkPosition(base.Location()));
					closestA = base.GetArea();
					natural = base.Location();
				}
			}
		}
		if (closestA)
		{
			double largest = 0.0;
			for (auto &choke : closestA->ChokePoints())
			{
				if (choke && (Grids().getDistanceHome(choke->Center()) < closestChokeDistance || closestChokeDistance == 0.0))
				{
					firstChoke = TilePosition(choke->Center());
					closestChokeDistance = Grids().getDistanceHome(choke->Center());
				}
			}

			for (auto &choke : closestA->ChokePoints())
			{
				if (choke && TilePosition(choke->Center()) != firstChoke && (Position(choke->Center()).getDistance(playerStartingPosition) / choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) < furthestChokeDistance || furthestChokeDistance == 0))
				{
					secondChoke = TilePosition(choke->Center());
					furthestChokeDistance = Position(choke->Center()).getDistance(playerStartingPosition) / choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2));
				}
			}

			for (auto &area : closestA->AccessibleNeighbours())
			{
				for (auto & choke : area->ChokePoints())
				{
					if (TilePosition(choke->Center()) == firstChoke && allyTerritory.find(area->Id()) == allyTerritory.end())
					{
						allyTerritory.insert(area->Id());
					}
				}
			}
			FFEPosition = TilePosition(int(secondChoke.x*0.35 + natural.x*0.65), int(secondChoke.y*0.35 + natural.y*0.65));
			if (BuildOrder().isForgeExpand() && FFEPosition.isValid() && theMap.GetArea(FFEPosition))
			{
				allyTerritory.insert(theMap.GetArea(FFEPosition)->Id());
			}
		}
	}
}

void TerrainTrackerClass::onStart()
{
	theMap.Initialize();
	theMap.EnableAutomaticPathAnalysis();
	bool startingLocationsOK = theMap.FindBasesForStartingLocations();
	assert(startingLocationsOK);
	playerStartingTilePosition = Broodwar->self()->getStartLocation();
	playerStartingPosition = Position(playerStartingTilePosition);
	return;
}

bool TerrainTrackerClass::isInAllyTerritory(TilePosition here)
{
	if (here.isValid() && theMap.GetArea(here))
	{
		if (allyTerritory.find(theMap.GetArea(here)->Id()) != allyTerritory.end())
		{
			return true;
		}
	}
	return false;
}

bool TerrainTrackerClass::isInEnemyTerritory(TilePosition here)
{
	if (here.isValid() && theMap.GetArea(here))
	{
		if (enemyTerritory.find(theMap.GetArea(here)->Id()) != enemyTerritory.end())
		{
			return true;
		}
	}
	return false;
}

Position TerrainTrackerClass::getClosestBaseCenter(Unit unit)
{
	double closestD = 0.0;
	Position closestB;
	if (!unit->getTilePosition().isValid() || !theMap.GetArea(unit->getTilePosition())) return Positions::Invalid;
	for (auto &base : theMap.GetArea(unit->getTilePosition())->Bases())
	{
		if (unit->getDistance(base.Center()) < closestD || closestD == 0.0)
		{
			closestD = unit->getDistance(base.Center());
			closestB = base.Center();
		}
	}
	return closestB;
}