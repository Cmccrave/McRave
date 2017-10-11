#include "McRave.h"

// NOTES: Transitioning to One Shot Update for Grids for ALL grids
// Anti Mobility
// Cluster
// Threat
// Special Units - Detector/Arbiter
// Detector
// Ally movement


void GridTrackerClass::update()
{
	Display().startClock();
	reset();
	updateMobilityGrids();
	updateNeutralGrids();
	updateDistanceGrid();
	Display().performanceTest(__FUNCTION__);
	return;
}

void GridTrackerClass::reset()
{
	// Temp debugging for tile positions
	for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
	{
		for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
		{
			/*if (eGroundThreat[x][y] > 0.0 && eGroundThreat[x][y] < 1.0)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Black);
			}
			if (eGroundThreat[x][y] >= 1.0 && eGroundThreat[x][y] < 2.0)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Blue);
			}
			if (eGroundThreat[x][y] >= 2.0 && eGroundThreat[x][y] < 3.0)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Green);
			}*/
			if (eGroundThreat[x][y] >= 1.0)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Red);
			}
		}
	}
	return;
}

void GridTrackerClass::updateBuildingGrid(BuildingInfo& building)
{
	int buildingOffset;
	if (building.getType() == UnitTypes::Terran_Supply_Depot || (!building.getType().isResourceDepot() && building.getType().buildsWhat().size() > 0))
	{
		buildingOffset = 1;
	}
	else
	{
		buildingOffset = 0;
	}

	// Add/remove building to/from grid
	TilePosition tile = building.getTilePosition();
	if (building.unit() && tile.isValid())
	{
		// Building Grid
		for (int x = tile.x - buildingOffset; x < tile.x + building.getType().tileWidth() + buildingOffset; x++)
		{
			for (int y = tile.y - buildingOffset; y < tile.y + building.getType().tileHeight() + buildingOffset; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					if (building.unit()->exists())
					{
						reservedGrid[x][y] = 0;
						buildingGrid[x][y] += 1;
					}
					else
					{
						buildingGrid[x][y] -= 1;
					}
				}
			}
		}

		// If the building can build addons
		if (building.getType().canBuildAddon())
		{
			for (int x = building.getTilePosition().x + building.getType().tileWidth(); x <= building.getTilePosition().x + building.getType().tileWidth() + 2; x++)
			{
				for (int y = building.getTilePosition().y + 1; y <= building.getTilePosition().y + 3; y++)
				{
					if (TilePosition(x, y).isValid())
					{
						if (building.unit()->exists())
						{
							reservedGrid[x][y] = 0;
							buildingGrid[x][y] += 1;
						}
						else
						{
							buildingGrid[x][y] -= 1;
						}
					}
				}
			}
		}

		// Pylon Grid
		if (building.getType() == UnitTypes::Protoss_Pylon)
		{
			for (int x = building.getTilePosition().x - 4; x < building.getTilePosition().x + building.getType().tileWidth() + 4; x++)
			{
				for (int y = building.getTilePosition().y - 4; y < building.getTilePosition().y + building.getType().tileHeight() + 4; y++)
				{
					if (TilePosition(x, y).isValid())
					{
						if (building.unit()->exists())
						{
							pylonGrid[x][y] += 1;
						}
						else
						{
							pylonGrid[x][y] -= 1;
						}
					}
				}
			}
		}
		// Shield Battery Grid
		if (building.getType() == UnitTypes::Protoss_Shield_Battery)
		{
			for (int x = building.getTilePosition().x - 5; x < building.getTilePosition().x + building.getType().tileWidth() + 5; x++)
			{
				for (int y = building.getTilePosition().y - 5; y < building.getTilePosition().y + building.getType().tileHeight() + 5; y++)
				{
					if (TilePosition(x, y).isValid() && building.getPosition().getDistance(Position(TilePosition(x, y))) < 320)
					{
						if (building.unit()->exists())
						{
							batteryGrid[x][y] += 1;
						}
						else
						{
							batteryGrid[x][y] -= 1;
						}
					}
				}
			}
		}
		// Bunker Grid
		if (building.getType() == UnitTypes::Terran_Bunker)
		{
			for (int x = building.getTilePosition().x - 10; x < building.getTilePosition().x + building.getType().tileWidth() + 10; x++)
			{
				for (int y = building.getTilePosition().y - 10; y < building.getTilePosition().y + building.getType().tileHeight() + 10; y++)
				{
					if (TilePosition(x, y).isValid() && building.getPosition().getDistance(Position(TilePosition(x, y))) < 320)
					{
						if (building.unit()->exists())
						{
							bunkerGrid[x][y] += 1;
						}
						else
						{
							bunkerGrid[x][y] -= 1;
						}
					}
				}
			}
		}
	}
}

void GridTrackerClass::updateResourceGrid(ResourceInfo& resource)
{
	// When a base is created, update this
	// When a base finishes, update this
	// When a base is destroyed, update this
	// Resource Grid for Minerals

	TilePosition tile = resource.getTilePosition();
	if (resource.getType().isMineralField())
	{
		for (int x = tile.x - 5; x < tile.x + resource.getType().tileWidth() + 5; x++)
		{
			for (int y = tile.y - 5; y < tile.y + resource.getType().tileHeight() + 5; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					if (resource.getResourceClusterPosition().getDistance(Position(TilePosition(x, y))) <= 192 && resource.getClosestBasePosition().isValid() && resource.getPosition().getDistance(resource.getClosestBasePosition()) > Position(x * 32, y * 32).getDistance(resource.getClosestBasePosition()))
					{
						if (resource.unit()->exists())
						{
							resourceGrid[x][y] += 1;
						}
						else
						{
							resourceGrid[x][y] -= 1;
						}
					}
				}
			}
		}
	}
	else
	{
		for (int x = tile.x - 5; x < tile.x + resource.getType().tileWidth() + 5; x++)
		{
			for (int y = tile.y - 5; y < tile.y + resource.getType().tileHeight() + 5; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					if (resource.getResourceClusterPosition().getDistance(Position(TilePosition(x, y))) <= 256 && resource.getClosestBasePosition().isValid() && resource.getPosition().getDistance(resource.getClosestBasePosition()) > Position(x * 32, y * 32).getDistance(resource.getClosestBasePosition()))
					{
						if (resource.unit()->exists())
						{
							resourceGrid[x][y] += 1;
						}
						else
						{
							resourceGrid[x][y] -= 1;
						}
					}
				}
			}
		}
	}
}

void GridTrackerClass::updateBaseGrid(BaseInfo& base)
{
	// Base Grid
	TilePosition tile = base.getTilePosition();
	for (int x = tile.x - 8; x < tile.x + 12; x++)
	{
		for (int y = tile.y - 8; y < tile.y + 11; y++)
		{
			if (TilePosition(x, y).isValid())
			{
				if (base.unit()->isCompleted() && base.unit()->exists())
				{
					baseGrid[x][y] = 2;
				}
				else if (base.unit()->exists())
				{
					baseGrid[x][y] = 1;
				}
				else
				{
					baseGrid[x][y] = 0;
				}
			}
		}
	}
}

void GridTrackerClass::updateDefenseGrid(UnitInfo& unit)
{
	// Defense Grid
	if (unit.getType() == UnitTypes::Protoss_Photon_Cannon || unit.getType() == UnitTypes::Terran_Bunker || unit.getType() == UnitTypes::Zerg_Sunken_Colony)
	{
		for (int x = unit.getTilePosition().x - 7; x < unit.getTilePosition().x + unit.getType().tileWidth() + 7; x++)
		{
			for (int y = unit.getTilePosition().y - 7; y < unit.getTilePosition().y + unit.getType().tileHeight() + 7; y++)
			{
				if (TilePosition(x, y).isValid() && unit.getPosition().getDistance(Position(TilePosition(x, y))) < 224)
				{
					if (unit.unit()->exists())
					{
						defenseGrid[x][y] += 1;
					}
					else
					{
						defenseGrid[x][y] -= 1;
					}
					if (x >= unit.getTilePosition().x && x < unit.getTilePosition().x + unit.getType().tileWidth() && y >= unit.getTilePosition().y && y < unit.getTilePosition().y + unit.getType().tileHeight())
					{
						if (unit.unit()->exists())
						{
							buildingGrid[x][y] += 1;
						}
						else
						{
							buildingGrid[x][y] -= 1;
						}
					}
				}
			}
		}
	}
}

void GridTrackerClass::updateNeutralGrids()
{
	//// Anti Mobility Grid -- TODO: Improve by storing the units
	//for (auto &u : Broodwar->neutral()->getUnits())
	//{
	//	WalkPosition start = Util().getWalkPosition(u);
	//	if (u->getType().isFlyer())
	//	{
	//		continue;
	//	}
	//	int startX = (u->getTilePosition().x * 4);
	//	int startY = (u->getTilePosition().y * 4);
	//	for (int x = startX; x < startX + u->getType().tileWidth() * 4; x++)
	//	{
	//		for (int y = startY; y < startY + u->getType().tileHeight() * 4; y++)
	//		{
	//			if (WalkPosition(x, y).isValid())
	//			{
	//				resetGrid[x][y] = true;
	//				antiMobilityGrid[x][y] = 1;
	//			}
	//		}
	//	}

	//	for (int x = start.x; x < start.x + u->getType().tileWidth() * 4; x++)
	//	{
	//		for (int y = start.y; y < start.y + u->getType().tileHeight() * 4; y++)
	//		{
	//			// Anti Mobility Grid directly under building
	//			if (WalkPosition(x, y).isValid())
	//			{
	//				resetGrid[x][y] = true;
	//				antiMobilityGrid[x][y] = 1;
	//			}
	//		}
	//	}
	//}
	return;
}

void GridTrackerClass::updateMobilityGrids()
{
	if (!mobilityAnalysis)
	{
		mobilityAnalysis = true;
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
		{
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
			{
				if (WalkPosition(x, y).isValid() && theMap.getWalkPosition(WalkPosition(x, y)).Walkable())
				{
					for (int i = -12; i <= 12; i++)
					{
						for (int j = -12; j <= 12; j++)
						{
							// The more tiles around x,y that are walkable, the more mobility x,y has				
							if (WalkPosition(x + i, y + j).isValid() && theMap.getWalkPosition(WalkPosition(x + i, y + j)).Walkable())
							{
								mobilityGrid[x][y] += 1;
							}
						}
					}
					mobilityGrid[x][y] = int(double(mobilityGrid[x][y]) / 56);

					for (auto &area : theMap.Areas())
					{
						for (auto &choke : area.ChokePoints())
						{
							if (WalkPosition(x, y).getDistance(choke->Center()) < 40)
							{
								bool notCorner = true;
								int startRatio = int(pow(choke->Center().getDistance(WalkPosition(x, y)) / 8, 2.0));
								for (int i = 0 - startRatio; i <= startRatio; i++)
								{
									for (int j = 0 - startRatio; j <= 0 - startRatio; j++)
									{
										if (WalkPosition(x + i, y + j).isValid() && !theMap.getWalkPosition(WalkPosition(x + i, y + j)).Walkable())
										{
											notCorner = false;
										}
									}
								}

								if (notCorner)
								{
									mobilityGrid[x][y] = 10;
								}
							}
						}
					}

					// Max a mini grid to 10
					mobilityGrid[x][y] = min(mobilityGrid[x][y], 10);
				}

				if (theMap.GetArea(WalkPosition(x, y)) == nullptr || theMap.GetArea(WalkPosition(x, y))->AccessibleNeighbours().size() == 0)
				{
					// Island
					mobilityGrid[x][y] = -1;
				}

				// Setup what is possible to check ground distances on
				if (mobilityGrid[x][y] <= 0)
				{
					distanceGridHome[x][y] = -1;
				}
				else if (mobilityGrid[x][y] > 0)
				{
					distanceGridHome[x][y] = 0;
				}
			}
		}
	}

	bool reservePath = false;
	if (reservePath && Broodwar->getFrameCount() > 500)
	{
		reservePath = false;
		// Create reserve path home
		int basebuildingGrid[256][256] = {};
		for (auto &area : theMap.Areas())
		{
			for (auto &base : area.Bases())
			{
				for (int x = base.Location().x; x < base.Location().x + 4; x++)
				{
					for (int y = base.Location().y; y < base.Location().y + 4; y++)
					{
						basebuildingGrid[x][y] = 1;
					}
				}
			}
		}
		TilePosition end = Terrain().getPlayerStartingTilePosition();
		TilePosition start = Terrain().getSecondChoke();

		for (int i = 0; i <= 50; i++)
		{
			double closestD = 0.0;
			TilePosition closestT = TilePositions::None;
			for (int x = start.x - 1; x <= start.x + 1; x++)
			{
				for (int y = start.y - 1; y <= start.y + 1; y++)
				{
					if (!TilePosition(x, y).isValid())
					{
						continue;
					}
					if (buildingGrid[x][y] == 1 || basebuildingGrid[x][y] == 1)
					{
						continue;
					}
					if ((x == start.x - 1 && y == start.y - 1) || (x == start.x - 1 && y == start.y + 1) || (x == start.x + 1 && y == start.y - 1) || (x == start.x + 1 && y == start.y + 1))
					{
						continue;
					}
					if (Grids().getDistanceHome(WalkPosition(TilePosition(x, y))) < closestD || closestD == 0.0)
					{
						bool bestTile = true;
						for (int i = 0; i <= 1; i++)
						{
							for (int j = 0; j <= 1; j++)
							{
								if (Grids().getMobilityGrid(WalkPosition(TilePosition(x, y)) + WalkPosition(i, j)) <= 0)
								{
									bestTile = false;
								}
							}
						}
						if (bestTile)
						{
							closestD = Grids().getDistanceHome(WalkPosition(TilePosition(x, y)));
							closestT = TilePosition(x, y);
						}
					}
				}
			}

			if (closestT.isValid())
			{
				start = closestT;
				buildingGrid[closestT.x][closestT.y] = 1;
			}

			if (start.getDistance(end) < 3)
			{
				break;
			}
		}

		start = Terrain().getSecondChoke();
		for (int i = start.x - 3; i <= start.x + 3; i++)
		{
			for (int j = start.y - 3; j <= start.y + 3; j++)
			{
				if (WalkPosition(i, j).isValid())
				{
					if (Grids().getDistanceHome(WalkPosition(TilePosition(i, j))) >= Grids().getDistanceHome(WalkPosition(start)))
					{
						buildingGrid[i][j] = 1;
					}
				}
			}
		}

	}
	return;
}

void GridTrackerClass::updateDetectorMovement(UnitInfo& observer)
{
	WalkPosition destination = WalkPosition(observer.getEngagePosition());

	for (int x = destination.x - 40; x <= destination.x + 40; x++)
	{
		for (int y = destination.y - 40; y <= destination.y + 40; y++)
		{
			// Create a circle of detection rather than a square
			if (WalkPosition(x, y).isValid() && observer.getEngagePosition().getDistance(Position(WalkPosition(x, y))) < 320)
			{
				resetGrid[x][y] = true;
				aDetectorGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateArbiterMovement(UnitInfo& arbiter)
{
	WalkPosition destination = WalkPosition(arbiter.getEngagePosition());

	for (int x = destination.x - 20; x <= destination.x + 20; x++)
	{
		for (int y = destination.y - 20; y <= destination.y + 20; y++)
		{
			// Create a circle of detection rather than a square
			if (WalkPosition(x, y).isValid() && arbiter.getEngagePosition().getDistance(Position(WalkPosition(x, y))) < 160)
			{
				resetGrid[x][y] = true;
				arbiterGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateAllyMovement(Unit unit, WalkPosition here)
{
	//for (int x = here.x - unit->getType().tileWidth() * 2; x < here.x + unit->getType().tileWidth() * 2; x++)
	//{
	//	for (int y = here.y - unit->getType().tileHeight() * 2; y < here.y + unit->getType().tileHeight() * 2; y++)
	//	{
	//		if (WalkPosition(x, y).isValid())
	//		{
	//			antiMobilityGrid[x][y] = 1;
	//		}
	//	}
	//}
	return;
}

void GridTrackerClass::updatePsiStorm(WalkPosition here)
{
	for (int x = here.x - 4; x < here.x + 8; x++)
	{
		for (int y = here.y - 4; y < here.y + 8; y++)
		{
			if (WalkPosition(x, y).isValid())
			{
				resetGrid[x][y] = true;
				psiStormGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updatePsiStorm(Bullet storm)
{
	WalkPosition here = WalkPosition(storm->getPosition());
	updatePsiStorm(here);
	return;
}

void GridTrackerClass::updateEMP(Bullet EMP)
{
	WalkPosition here = WalkPosition(EMP->getTargetPosition());
	for (int x = here.x - 4; x < here.x + 8; x++)
	{
		for (int y = here.y - 4; y < here.y + 8; y++)
		{
			if (WalkPosition(x, y).isValid())
			{
				resetGrid[x][y] = true;
				EMPGrid[x][y] = 1;
			}
		}
	}
}

void GridTrackerClass::updateDistanceGrid()
{
	// TODO: Improve this somehow
	if (!distanceAnalysis)
	{
		WalkPosition start = WalkPosition(Terrain().getPlayerStartingPosition());
		distanceGridHome[start.x][start.y] = 1;
		distanceAnalysis = true;
		bool done = false;
		int cnt = 0;
		int segment = 0;
		clock_t myClock;
		double duration;
		myClock = clock();

		while (!done)
		{
			duration = (clock() - myClock) / (double)CLOCKS_PER_SEC;
			if (duration > 9.9)
			{
				break;
			}
			done = true;
			cnt++;
			segment += 8;
			for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
			{
				for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
				{
					// If any of the grid is 0, we're not done yet
					if (distanceGridHome[x][y] == 0 && theMap.getWalkPosition(WalkPosition(x, y)).AreaId() > 0)
					{
						done = false;
					}
					if (distanceGridHome[x][y] == cnt)
					{
						for (int i = x - 1; i <= x + 1; i++)
						{
							for (int j = y - 1; j <= y + 1; j++)
							{
								if (distanceGridHome[i][j] == 0 && Position(WalkPosition(i, j)).getDistance(Position(start)) <= segment)
								{
									distanceGridHome[i][j] = cnt + 1;
								}
							}
						}
					}
				}
			}
		}
		Broodwar << "Distance Grid Analysis time: " << duration << endl;
		if (duration > 9.9)
		{
			Broodwar << "Hit maximum, check for islands." << endl;
		}
	}
}

void GridTrackerClass::addToGrid(UnitInfo& unit)
{
	int radius = 4.0 + (unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8;
	WalkPosition start = unit.getWalkPosition();

	for (int x = start.x - radius; x <= start.x + radius + unit.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - radius; y <= start.y + radius + unit.getType().tileHeight() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid()) return;
			double distance = max(1.0, WalkPosition(x, y).getDistance(start) - double(unit.getType().tileWidth() * 4.0));

			// Cluster grids
			if (unit.getPlayer() == Broodwar->self() && start.getDistance(WalkPosition(x, y)) <= 20)
			{
				aClusterGrid[x][y] += 1;
			}
			else if (unit.getPlayer() != Broodwar->self())
			{
				if (x >= start.x - 4 && x < start.x + 4 + unit.getType().tileWidth() * 4 && y >= start.y - 4 && y < start.y + 4 + unit.getType().tileHeight() * 4)
				{
					if (unit.getType().isFlyer()) eAirClusterGrid[x][y] += 1;
					if (!unit.getType().isFlyer()) eGroundClusterGrid[x][y] += 1;
					if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) stasisClusterGrid[x][y] += 1;
				}
			}

			// Anti mobility grids
			if (x >= start.x && x < start.x + unit.getType().tileWidth() * 4 && y >= start.y && y < start.y + unit.getType().tileHeight() * 4)
			{
				antiMobilityGrid[x][y] += 1;
			}

			// Threat grids
			if (unit.getGroundDamage() > 0.0 && distance < (unit.getGroundRange() + (unit.getSpeed())) / 8.0)
			{
				if (unit.getPlayer() == Broodwar->self())
				{
					aGroundThreat[x][y] += max(0.1, unit.getMaxGroundStrength() / distance);
				}
				else
				{
					eGroundThreat[x][y] += max(0.1, unit.getMaxGroundStrength() / distance);
				}
			}
			if (unit.getAirDamage() > 0.0 && distance < (unit.getAirRange() + (unit.getSpeed())) / 8.0)
			{
				if (unit.getPlayer() == Broodwar->self())
				{
					aAirThreat[x][y] += max(0.1, unit.getMaxAirStrength() / distance);
				}
				else
				{
					eAirThreat[x][y] += max(0.1, unit.getMaxAirStrength() / distance);
				}
			}
		}
	}
	return;
}

void GridTrackerClass::removeFromGrid(UnitInfo& unit)
{
	int radius = max(4.0, (unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8);
	WalkPosition start = unit.getWalkPosition();

	for (int x = start.x - radius; x <= start.x + radius + unit.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - radius; y <= start.y + radius + unit.getType().tileHeight() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid()) return;
			double distance = max(1.0, WalkPosition(x, y).getDistance(start) - double(unit.getType().tileWidth() * 4.0));

			// Cluster grids
			if (unit.getPlayer() == Broodwar->self() && start.getDistance(WalkPosition(x, y)) <= 20)
			{
				aClusterGrid[x][y] -= 1;
			}
			else if (unit.getPlayer() != Broodwar->self())
			{
				if (x >= start.x - 4 && x < start.x + 4 + unit.getType().tileWidth() * 4 && y >= start.y - 4 && y < start.y + 4 + unit.getType().tileHeight() * 4)
				{
					if (unit.getType().isFlyer()) eAirClusterGrid[x][y] -= 1;
					if (!unit.getType().isFlyer()) eGroundClusterGrid[x][y] -= 1;
					if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) stasisClusterGrid[x][y] -= 1;
				}
			}

			// Anti mobility grids
			if (x >= start.x && x < start.x + unit.getType().tileWidth() * 4 && y >= start.y && y < start.y + unit.getType().tileHeight() * 4)
			{
				antiMobilityGrid[x][y] -= 1;
			}

			// Threat grids
			if (unit.getGroundDamage() > 0.0 && distance < (unit.getGroundRange() + (unit.getSpeed())) / 8.0)
			{
				if (unit.getPlayer() == Broodwar->self())
				{
					aGroundThreat[x][y] -= max(0.1, unit.getMaxGroundStrength() / distance);
				}
				else
				{
					eGroundThreat[x][y] -= max(0.1, unit.getMaxGroundStrength() / distance);
				}
			}
			if (unit.getAirDamage() > 0.0 && distance < (unit.getAirRange() + (unit.getSpeed())) / 8.0)
			{
				if (unit.getPlayer() == Broodwar->self())
				{
					aAirThreat[x][y] -= max(0.1, unit.getMaxAirStrength() / distance);
				}
				else
				{
					eAirThreat[x][y] -= max(0.1, unit.getMaxAirStrength() / distance);
				}
			}
		}
	}
	return;
}

void GridTrackerClass::addToGrid(WorkerInfo& worker)
{
	WalkPosition end = Util().getWalkPosition(worker.unit());
	Position newPos = worker.unit()->getPosition();
	int width = worker.getType().tileWidth() * 4;
	int height = worker.getType().tileHeight() * 4;

	for (int x = end.x; x <= end.x + width; x++)
	{
		for (int y = end.y; y <= end.y + height; y++)
		{
			if (!WalkPosition(x, y).isValid()) return;

			// Anti mobility grid
			if (x >= end.x && x < end.x + width && y >= end.y && y < end.y + height)
			{
				antiMobilityGrid[x][y] += 1;
			}
		}
	}
	return;
}

void GridTrackerClass::removeFromGrid(WorkerInfo& worker)
{
	WalkPosition start = worker.getWalkPosition();
	Position oldPos = worker.getPosition();
	int width = worker.getType().tileWidth() * 4;
	int height = worker.getType().tileHeight() * 4;

	for (int x = start.x; x <= start.x + width; x++)
	{
		for (int y = start.y; y <= start.y + height; y++)
		{
			if (!WalkPosition(x, y).isValid()) return;

			// Anti mobility grids
			if (x >= start.x && x < start.x + width && y >= start.y && y < start.y + height)
			{
				antiMobilityGrid[x][y] -= 1;
			}
		}
	}
	return;
}

void GridTrackerClass::addToGrid(BuildingInfo& building)
{
	WalkPosition end = building.getWalkPosition();
	int width = building.getType().tileWidth() * 4;
	int height = building.getType().tileHeight() * 4;

	// Buildings are non inclusive on last tile
	for (int x = end.x; x < end.x + width; x++)
	{
		for (int y = end.y; y < end.y + height; y++)
		{
			if (!WalkPosition(x, y).isValid()) return;

			// Anti mobility grid
			if (x >= end.x && x < end.x + width && y >= end.y && y < end.y + height)
			{
				antiMobilityGrid[x][y] += 1;
			}
		}
	}
	return;
}

void GridTrackerClass::removeFromGrid(BuildingInfo& building)
{
	WalkPosition end = building.getWalkPosition();
	int width = building.getType().tileWidth() * 4;
	int height = building.getType().tileHeight() * 4;

	// Buildings are non inclusive on last tile
	for (int x = end.x; x < end.x + width; x++)
	{
		for (int y = end.y; y < end.y + height; y++)
		{
			if (!WalkPosition(x, y).isValid()) return;

			// Anti mobility grid
			if (x >= end.x && x < end.x + width && y >= end.y && y < end.y + height)
			{
				antiMobilityGrid[x][y] -= 1;
			}
		}
	}
	return;
}

// Circle thing
//inline void UnitManager::addToDeathMatrix(BWAPI::Position pos, BWAPI::UnitType ut, BWAPI::Player p) {
//	const int mapH = BWAPI::Broodwar->mapHeight() * 4, mapW = BWAPI::Broodwar->mapWidth() * 4;
//	if (ut.groundWeapon()) {
//		const int range = p->weaponMaxRange(ut.groundWeapon()) / 8;
//		const int death = unitDeathGround(ut);
//		const int mx = pos.x + range > mapW ? mapW : pos.x + range;
//		for (int dx = pos.x - range < 0 ? -pos.x : -range; dx <= mx; ++dx) {
//			const int yw = (int)ceil(sqrt(range * range - dx * dx));
//			const int minY = MAX(pos.y - yw, 0), maxY = MIN(pos.y + yw, mapH);
//			for (int y = minY; y <= maxY; ++y)
//				deathMatrixGround[y*deathMatrixSideLen + pos.x + dx] += death;
//		}
//	}
//
//	if (ut.airWeapon()) {
//		const int range = p->weaponMaxRange(ut.groundWeapon()) / 8;
//		const int death = unitDeathAir(ut);
//		const int mx = pos.x + range > mapW ? mapW : pos.x + range;
//		for (int dx = pos.x - range < 0 ? -pos.x : -range; dx <= mx; ++dx) {
//			const int yw = (int)ceil(sqrt(range * range - dx * dx));
//			const int minY = MAX(pos.y - yw, 0), maxY = MIN(pos.y + yw, mapH);
//			for (int y = minY; y <= maxY; ++y)
//				deathMatrixAir[y*deathMatrixSideLen + pos.x + dx] += death;
//		}
//	}
//}