#include "McRave.h"

void TerrainTrackerClass::update()
{
	Display().startClock();
	updateAreas();
	updateChokes();
	updateWalls();
	Display().performanceTest(__FUNCTION__);
	return;
}

void TerrainTrackerClass::updateAreas()
{
	if (Broodwar->getFrameCount() >= 100 && Broodwar->getFrameCount() < 105)
	{
		findNatural();
		findFirstChoke();
		findSecondChoke();
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
	if (Bases().getEnemyBases().size() > 0 && Grids().getEnemyArmyCenter().isValid())
	{
		double closestD = 0.0;
		Position closestP;
		for (auto &b : Bases().getEnemyBases())
		{
			BaseInfo &base = b.second;
			if (Grids().getEnemyArmyCenter().getDistance(base.getPosition()) > closestD)
			{
				closestD = Grids().getEnemyArmyCenter().getDistance(base.getPosition());
				closestP = base.getPosition();
			}
		}
		attackPosition = closestP;
	}
	else if (enemyStartingPosition.isValid() && !Broodwar->isExplored(enemyStartingTilePosition)) attackPosition = enemyStartingPosition;
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
	if (naturalArea)
	{
		//Broodwar->drawCircleMap(Position(firstChoke), 16, Colors::Blue);
		//Broodwar->drawCircleMap(Position(secondChoke), 16, Colors::Green);

		for (auto &area : naturalArea->AccessibleNeighbours())
		{
			for (auto & choke : area->ChokePoints())
			{
				if (TilePosition(choke->Center()) == firstChoke && allyTerritory.find(area->Id()) == allyTerritory.end())
				{
					allyTerritory.insert(area->Id());
				}
			}
		}
		FFEPosition = TilePosition(int(secondChoke.x*0.25 + natural.x*0.75), int(secondChoke.y*0.25 + natural.y*0.75));
		if (BuildOrder().isForgeExpand() && FFEPosition.isValid() && theMap.GetArea(FFEPosition))
		{
			allyTerritory.insert(theMap.GetArea(FFEPosition)->Id());
		}
	}
}

void TerrainTrackerClass::updateWalls()
{
	TilePosition start = secondChoke;
	double distance = 0.0;
	bool valid = false;

	// Large Building placement
	for (int x = start.x - 20; x <= start.x + 20; x++)
	{
		for (int y = start.y - 20; y <= start.y + 20; y++)
		{
			if (!TilePosition(x, y).isValid()) continue;

			valid = true;
			for (int i = x; i < x + 4; i++)
			{
				for (int j = y; j < y + 3; j++)
				{
					if (!Broodwar->isBuildable(TilePosition(i, j))) valid = false;
				}
			}
			if (!valid) continue;

			valid = false;
			int dx = x + 4;
			for (int dy = y; dy <= y + 3; dy++)
			{
				if (!Broodwar->isBuildable(TilePosition(dx, dy)) /*&& !Util().isWalkable(TilePosition(dx,dy))*/) valid = true;
			}

			int dy = y + 3;
			for (int dx = x; dx <= x + 4; dx++)
			{
				if (!Broodwar->isBuildable(TilePosition(dx, dy))/* && !Util().isWalkable(TilePosition(dx, dy))*/) valid = true;
			}

			if (valid && TilePosition(x, y).getDistance(natural) <= 16 && (TilePosition(x, y).getDistance(secondChoke) < distance || distance == 0.0)) bLarge = TilePosition(x, y), distance = TilePosition(x, y).getDistance(secondChoke);
		}
	}

	// Medium Building placement
	valid = false;
	distance = 0.0;
	for (int x = start.x - 20; x <= start.x + 20; x++)
	{
		for (int y = start.y - 20; y <= start.y + 20; y++)
		{
			if (!TilePosition(x, y).isValid()) continue;

			valid = true;
			for (int i = x; i < x + 3; i++)
			{
				for (int j = y; j < y + 2; j++)
				{
					if (!Broodwar->isBuildable(TilePosition(i, j))) valid = false;
					if (i >= bLarge.x && i < bLarge.x + 4 && j >= bLarge.y && j < bLarge.y + 3) valid = false;
				}
			}
			if (!valid) continue;

			valid = false;
			int dx = x - 1;
			for (int dy = y; dy < y + 2; dy++)
			{
				if (!Broodwar->isBuildable(TilePosition(dx, dy)) /*&& !Util().isWalkable(TilePosition(dx, dy))*/) valid = true;
			}

			int dy = y - 1;
			for (int dx = x; dx < x + 3; dx++)
			{
				if ((!Broodwar->isBuildable(TilePosition(dx, dy)) /*&& !Util().isWalkable(TilePosition(dx, dy))*/) || dy == bLarge.y + 2) valid = true;
			}

			if (valid && TilePosition(x, y).getDistance(natural) <= 16 && TilePosition(x, y).getDistance(bLarge) <= 6 && (TilePosition(x, y).getDistance(secondChoke) < distance || distance == 0.0)) bMedium = TilePosition(x, y), distance = TilePosition(x, y).getDistance(secondChoke);
		}
	}

	// Pylon placement
	valid = false;
	distance = 0.0;
	for (int x = natural.x - 20; x <= natural.x + 20; x++)
	{
		for (int y = natural.y - 20; y <= natural.y + 20; y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			if (theMap.GetArea(TilePosition(x, y)) != theMap.GetArea(natural)) continue;

			valid = true;
			for (int i = x; i < x + 2; i++)
			{
				for (int j = y; j < y + 2; j++)
				{
					if (!Broodwar->isBuildable(TilePosition(i, j))) valid = false;
					if (i >= bMedium.x && i < bMedium.x + 3 && j >= bMedium.y && j < bMedium.y + 2) valid = false;
					if (i >= bLarge.x && i < bLarge.x + 4 && j >= bLarge.y && j < bLarge.y + 3) valid = false;
					if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) valid = false;
				}
			}
			if (!valid) continue;
			if (valid && TilePosition(x, y).getDistance(bLarge) <= 4 && TilePosition(x, y).getDistance(bMedium) <= 4 && (TilePosition(x, y).getDistance(natural) < distance || distance == 0.0)) bSmall = TilePosition(x, y), distance = TilePosition(x, y).getDistance(natural);
		}
	}

	//Broodwar->drawBoxMap(Position(bSmall), Position(bSmall) + Position(64, 64), Colors::Red);
	//Broodwar->drawBoxMap(Position(bMedium), Position(bMedium) + Position(94, 64), Colors::Red);
	//Broodwar->drawBoxMap(Position(bLarge), Position(bLarge) + Position(128, 96), Colors::Red);
	//Broodwar->drawCircleMap(Position(secondChoke), 16, Colors::Green);
}

void TerrainTrackerClass::onStart()
{
	theMap.Initialize();
	theMap.EnableAutomaticPathAnalysis();
	bool startingLocationsOK = theMap.FindBasesForStartingLocations();
	assert(startingLocationsOK);
	playerStartingTilePosition = Broodwar->self()->getStartLocation();
	playerStartingPosition = Position(32 * playerStartingTilePosition.x + 64, 32 * playerStartingTilePosition.y + 48);
	return;
}

void TerrainTrackerClass::findNatural()
{
	// Find natural area
	double distance = 0.0;
	for (auto &area : theMap.Areas())
	{
		for (auto &base : area.Bases())
		{
			if (base.Geysers().size() == 0 || area.AccessibleNeighbours().size() == 0) continue;			
			if (Grids().getDistanceHome(WalkPosition(base.Location())) > 50 && (Grids().getDistanceHome(WalkPosition(base.Location())) < distance || distance == 0.0))
			{
				distance = Grids().getDistanceHome(WalkPosition(base.Location()));
				naturalArea = base.GetArea();
				natural = base.Location();
			}
		}
	}
}

void TerrainTrackerClass::findFirstChoke()
{
	// Find the first choke
	double distance = 0.0;
	for (auto &choke : naturalArea->ChokePoints())
	{
		if (choke && (Grids().getDistanceHome(choke->Center()) < distance || distance == 0.0))
		{
			firstChoke = TilePosition(choke->Center());
			distance = Grids().getDistanceHome(choke->Center());
		}
	}
}

void TerrainTrackerClass::findSecondChoke()
{
	// Find area closest to the center of the map to wall off
	double distanceToCenter = 0.0;
	const Area* second = nullptr;
	for (auto &area : naturalArea->AccessibleNeighbours())
	{
		WalkPosition center = WalkPosition((area->TopLeft() + area->BottomRight()) / 2);
		if (center.isValid() && (Position(center).getDistance(theMap.Center()) < distanceToCenter || distanceToCenter == 0.0))
		{
			distanceToCenter = Position(center).getDistance(theMap.Center());
			second = area;
		}
	}

	// Find second choke
	double distance = 0.0;
	for (auto &choke : naturalArea->ChokePoints())
	{
		if (TilePosition(choke->Center()) != firstChoke && (choke->GetAreas().first == second || choke->GetAreas().second == second) && (Position(choke->Center()).getDistance(playerStartingPosition) < distance || distance == 0.0))
		{
			distance = Position(choke->Center()).getDistance(playerStartingPosition);
			secondChoke = TilePosition(choke->Center());
		}
	}
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