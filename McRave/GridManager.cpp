#include "McRave.h"

void GridTrackerClass::update()
{
	Display().startClock();
	reset();
	updateMobilityGrids();
	updateAllyGrids();
	updateEnemyGrids();
	updateNeutralGrids();
	updateDistanceGrid();
	Display().performanceTest(__FUNCTION__);
	return;
}

void GridTrackerClass::reset()
{
	//// Temp debugging for tile positions
	//for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
	//{
	//	for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
	//	{
	//		if (eGroundThreat[x][y] > 0 && eGroundThreat[x][y] < 4)
	//		{
	//			Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Black);
	//			//Broodwar->drawCircleMap(Position(TilePosition(x, y)) + Position(16, 16), 4, Colors::Black);
	//		}
	//		if (eGroundThreat[x][y] >= 4 && eGroundThreat[x][y] < 5)
	//		{
	//			Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Blue);
	//		}
	//		if (eGroundThreat[x][y] >= 5 && eGroundThreat[x][y] < 6)
	//		{
	//			Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Green);
	//		}
	//		if (eGroundThreat[x][y] >= 6)
	//		{
	//			Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Red);
	//		}
	//	}
	//}

	int aCenter = 0, eCenter = 0;
	for (int x = 0; x < 1024; x++) for (int y = 0; y < 1024; y++)
	{
		if (!resetGrid[x][y]) continue;

		// Find army centers
		if (aGroundClusterGrid[x][y] > aCenter)
		{
			aCenter = aGroundClusterGrid[x][y];
			allyArmyCenter = Position(WalkPosition(x, y));
		}
		if (eGroundClusterGrid[x][y] * eGroundThreat[x][y] + eAirClusterGrid[x][y] * eAirThreat[x][y] > eCenter)
		{
			eCenter = eGroundClusterGrid[x][y] * eGroundThreat[x][y] + eAirClusterGrid[x][y] * eAirThreat[x][y];
			enemyArmyCenter = Position(WalkPosition(x, y));
		}

		// Reset WalkPosition grids		
		aGroundClusterGrid[x][y] = 0;
		aAirClusterGrid[x][y] = 0;
		aDetectorGrid[x][y] = 0;
		aGroundThreat[x][y] = 0.0;
		aAirThreat[x][y] = 0.0;
		arbiterGrid[x][y] = 0;

		eGroundThreat[x][y] = 0.0;
		eAirThreat[x][y] = 0.0;
		eGroundClusterGrid[x][y] = 0;
		eAirClusterGrid[x][y] = 0;
		eDetectorGrid[x][y] = 0;

		psiStormGrid[x][y] = 0;
		EMPGrid[x][y] = 0;
		antiMobilityGrid[x][y] = 0;
	}

	// Wipe all the information in our hashed set before gathering information
	memset(resetGrid, 0, 1024 * 1024 * sizeof(bool));
	return;
}

void GridTrackerClass::updateAllyGrids()
{
	// Ally Unit Grid Update
	for (auto &u : Units().getAllyUnits())
	{
		UnitInfo &unit = u.second;
		if (unit.getType() == UnitTypes::Protoss_Arbiter || unit.getType() == UnitTypes::Protoss_Observer) continue;
		double width = double(unit.getType().width());
		int walkWidth = int(double(unit.getType().width()) / 8.0);
		int radius = int((unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8.0);
		WalkPosition start = unit.getWalkPosition();

		for (int x = start.x - radius; x <= start.x + radius + walkWidth; x++)
		{
			for (int y = start.y - radius; y <= start.y + radius + walkWidth; y++)
			{
				if (!WalkPosition(x, y).isValid()) continue;

				double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(unit.getPosition()) - (width / 2.0));

				// Cluster grids
				if (start.getDistance(WalkPosition(x, y)) <= 20)
				{
					if (unit.getType().isFlyer())
					{
						resetGrid[x][y] = true;
						aAirClusterGrid[x][y] += 1;
					}
					else
					{
						resetGrid[x][y] = true;
						aGroundClusterGrid[x][y] += 1;
					}
				}

				// Anti mobility grids
				if (x >= start.x && x < start.x + unit.getType().tileWidth() * 4 && y >= start.y && y < start.y + unit.getType().tileHeight() * 4)
				{
					resetGrid[x][y] = true;
					antiMobilityGrid[x][y] += 1;
				}

				// Threat grids
				if (distance <= (unit.getGroundRange() + unit.getSpeed()))
				{
					resetGrid[x][y] = true;
					aGroundThreat[x][y] += 32.0 * unit.getMaxGroundStrength() / distance;
				}
				if (distance <= (unit.getAirRange() + unit.getSpeed()))
				{
					resetGrid[x][y] = true;
					aAirThreat[x][y] += 32.0 * unit.getMaxAirStrength() / distance;
				}
			}
		}
	}

	// Building Grid update
	for (auto &b : Buildings().getMyBuildings())
	{
		BuildingInfo &building = b.second;
		WalkPosition start = building.getWalkPosition();
		for (int x = start.x; x < start.x + building.getType().tileWidth() * 4; x++)
		{
			for (int y = start.y; y < start.y + building.getType().tileHeight() * 4; y++)
			{
				// Anti Mobility Grid directly under building
				if (WalkPosition(x, y).isValid())
				{
					resetGrid[x][y] = true;
					antiMobilityGrid[x][y] = 1;
				}
			}
		}
	}

	// Worker Grid update
	for (auto &w : Workers().getMyWorkers())
	{
		WorkerInfo &worker = w.second;
		WalkPosition start = worker.getWalkPosition();
		for (int x = start.x; x < start.x + worker.unit()->getType().tileWidth() * 4; x++)
		{
			for (int y = start.y; y < start.y + worker.unit()->getType().tileHeight() * 4; y++)
			{
				// Anti Mobility Grid directly under worker
				if (WalkPosition(x, y).isValid())
				{
					resetGrid[x][y] = true;
					antiMobilityGrid[x][y] = 1;
				}
			}
		}
	}
	return;
}

void GridTrackerClass::updateEnemyGrids()
{
	// Enemy Unit Grid Update
	for (auto &u : Units().getEnemyUnits())
	{
		UnitInfo &unit = u.second;
		double width = double(unit.getType().width());
		int walkWidth = int(double(unit.getType().width()) / 8.0);
		double gReach = 0.0, aReach = 0.0;
		double gRange = unit.getGroundRange(), aRange = unit.getAirRange();
		int radius = 0;
		WalkPosition start = unit.getWalkPosition();

		if (unit.getType().isWorker())
		{
			if (unit.unit()->exists() && (!Terrain().isInAllyTerritory(unit.unit()) || (Broodwar->getFrameCount() - unit.getLastAttackFrame() < 500)))
			{
				radius = int((unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8.0);
				gReach = unit.getGroundRange() + unit.getSpeed();
			}
			else continue;
		}
		else
		{
			radius = int((unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8.0);
			gReach = unit.getGroundRange() + unit.getSpeed();
			aReach = unit.getAirRange() + unit.getSpeed();
		}

		// Min radius for detectors with no attack
		if (unit.getType().isDetector())
		{
			radius = 40;
		}

		for (int x = start.x - radius; x <= start.x + radius + walkWidth; x++)
		{
			for (int y = start.y - radius; y <= start.y + radius + walkWidth; y++)
			{
				if (!WalkPosition(x, y).isValid()) continue;

				double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(unit.getPosition()) - (width / 2.0));

				// Cluster grids
				if (x >= start.x - 4 && x < start.x + 4 + (width / 8.0) && y >= start.y - 4 && y < start.y + 4 + (width / 8.0))
				{
					if (unit.getType().isFlyer())
					{
						resetGrid[x][y] = true;
						eAirClusterGrid[x][y] += 1;
					}
					if (!unit.getType().isFlyer())
					{
						resetGrid[x][y] = true;
						eGroundClusterGrid[x][y] += 1;
					}
					if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
					{
						resetGrid[x][y] = true;
						stasisClusterGrid[x][y] += 1;
					}
				}

				// Anti mobility grids
				if (!unit.getType().isFlyer() && x >= start.x && x < start.x + width / 16.0 && y >= start.y && y < start.y + width / 16.0)
				{
					resetGrid[x][y] = true;
					antiMobilityGrid[x][y] += 1;
				}

				// Threat grids				
				if (distance <= gReach)
				{
					resetGrid[x][y] = true;
					eGroundThreat[x][y] += unit.getMaxGroundStrength() * min(1.0, (unit.getGroundRange() / distance));
				}
				if (distance <= aReach)
				{
					resetGrid[x][y] = true;
					eAirThreat[x][y] += unit.getMaxAirStrength() * min(1.0, (unit.getAirRange() / distance));
				}

				// Detection grid
				if (unit.getType().isDetector() && Position(WalkPosition(x, y)).getDistance(unit.getPosition()) < 320)
				{
					resetGrid[x][y] = true;
					eDetectorGrid[x][y] += 1;
				}
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
					building.unit()->exists() ? buildingGrid[x][y] += 1 : buildingGrid[x][y] -= 1;
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
						building.unit()->exists() ? buildingGrid[x][y] += 1 : buildingGrid[x][y] -= 1;
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
						building.unit()->exists() ? pylonGrid[x][y] += 1 : pylonGrid[x][y] -= 1;
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
						building.unit()->exists() ? batteryGrid[x][y] += 1 : batteryGrid[x][y] -= 1;
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
						building.unit()->exists() ? bunkerGrid[x][y] += 1 : bunkerGrid[x][y] -= 1;
					}
				}
			}
		}
	}
}

void GridTrackerClass::updateResourceGrid(ResourceInfo& resource)
{
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
		for (int x = tile.x - 2; x < tile.x + resource.getType().tileWidth() + 2; x++)
		{
			for (int y = tile.y - 2; y < tile.y + resource.getType().tileHeight() + 2; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					if (baseGrid[x][y] > 0 && TilePosition(x, y).getDistance(TilePosition(resource.getClosestBasePosition())) < resource.getTilePosition().getDistance(TilePosition(resource.getClosestBasePosition())))
					{
						resourceGrid[x][y] += 1;
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
				if (TilePosition(x, y).isValid() && unit.getPosition().getDistance(Position(TilePosition(x, y))) < 256)
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
	// Anti Mobility Grid -- TODO: Improve by storing the units
	for (auto &u : Broodwar->neutral()->getUnits())
	{
		WalkPosition start = Util().getWalkPosition(u);
		if (u->getType().isFlyer())
		{
			continue;
		}
		int startX = (u->getTilePosition().x * 4);
		int startY = (u->getTilePosition().y * 4);
		for (int x = startX; x < startX + u->getType().tileWidth() * 4; x++)
		{
			for (int y = startY; y < startY + u->getType().tileHeight() * 4; y++)
			{
				if (WalkPosition(x, y).isValid())
				{
					resetGrid[x][y] = true;
					antiMobilityGrid[x][y] = 1;
				}
			}
		}

		for (int x = start.x; x < start.x + u->getType().tileWidth() * 4; x++)
		{
			for (int y = start.y; y < start.y + u->getType().tileHeight() * 4; y++)
			{
				// Anti Mobility Grid directly under building
				if (WalkPosition(x, y).isValid())
				{
					resetGrid[x][y] = true;
					antiMobilityGrid[x][y] = 1;
				}
			}
		}
	}
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

				if (!theMap.GetArea(WalkPosition(x, y)) || theMap.GetArea(WalkPosition(x, y))->AccessibleNeighbours().size() == 0)
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
	for (int x = here.x - unit->getType().tileWidth() * 2; x < here.x + unit->getType().tileWidth() * 2; x++)
	{
		for (int y = here.y - unit->getType().tileHeight() * 2; y < here.y + unit->getType().tileHeight() * 2; y++)
		{
			if (WalkPosition(x, y).isValid())
			{
				resetGrid[x][y] = true;
				antiMobilityGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updatePsiStorm(WalkPosition here)
{
	for (int x = here.x - 12; x < here.x + 12; x++)
	{
		for (int y = here.y - 12; y < here.y + 12; y++)
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
