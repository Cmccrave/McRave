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
	if (enemyBasePositions.size() <= 0)
	{
		for (auto &unit : Units().getEnemyUnits())
		{
			if (unit.second.getType().isBuilding() && Terrain().getEnemyBasePositions().size() == 0 && unit.second.getPosition().getDistance(Terrain().getPlayerStartingPosition()) > 640)
			{
				double distance = 0.0;
				TilePosition closest;
				for (auto &base : theMap.StartingLocations())
				{
					if (unit.second.getPosition().getDistance(Position(base)) < distance || distance == 0.0)
					{
						distance = unit.second.getPosition().getDistance(Position(base));
						closest = base;
					}
				}
				if (closest.isValid())
				{
					enemyBasePositions.emplace(Position(closest));
					enemyStartingTilePosition = closest;
					enemyStartingPosition = Position(closest);
					path = theMap.GetPath(playerStartingPosition, enemyStartingPosition);
				}
			}
		}
	}

	for (auto &base : enemyBasePositions)
	{
		if (base.isValid() && Broodwar->isVisible(TilePosition(base)) && Broodwar->getUnitsInRadius(base, 128, Filter::IsEnemy).size() == 0)
		{
			enemyBasePositions.erase(base);
			break;
		}
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
				allBaseLocations.emplace(base.Location());
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
		}
	}
	for (auto &base : Bases().getMyBases())
	{
		mineralHold = Position(base.second.getResourcesPosition());
		backMineralHold = (Position(base.second.getResourcesPosition()) - Position(base.second.getPosition())) + Position(base.second.getResourcesPosition());
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

void TerrainTrackerClass::removeTerritory(Unit base)
{
	if (base && base->exists() && base->getType().isResourceDepot())
	{
		if (enemyBasePositions.find(base->getPosition()) != enemyBasePositions.end())
		{
			enemyBasePositions.erase(base->getPosition());
		}

		if (theMap.GetArea(base->getTilePosition()))
		{
			if (allyTerritory.find(theMap.GetArea(base->getTilePosition())->Id()) != allyTerritory.end())
			{
				allyTerritory.erase(theMap.GetArea(base->getTilePosition())->Id());
			}
		}
	}
	return;
}

bool TerrainTrackerClass::isInAllyTerritory(Unit unit)
{
	if (unit && unit->exists() && unit->getTilePosition().isValid() && theMap.GetArea(unit->getTilePosition()))
	{
		if (allyTerritory.find(theMap.GetArea(unit->getTilePosition())->Id()) != allyTerritory.end())
		{
			return true;
		}
	}
	return false;
}

Position TerrainTrackerClass::getClosestEnemyBase(Position here)
{
	double closestD = 0.0;
	Position closestP;
	for (auto &base : Terrain().getEnemyBasePositions())
	{
		if (here.getDistance(base) < closestD || closestD == 0.0)
		{
			closestP = base;
			closestD = here.getDistance(base);
		}
	}
	return closestP;
}

Position TerrainTrackerClass::getClosestBaseCenter(Unit unit)
{
	double closestD = 0.0;
	Position closestB;
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
