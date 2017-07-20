#include "McRave.h"

// NOTES: Transitioning to One Shot Update for Grids if ? it is assumed to be working for now

// Disable: updateReservedLocation for building placement
// Completed and verified:
// Reserve Grid
// Pylon Grid
// Battery Grid (?)
// Defense Grid (?)
// Bunker Grid (?)

// Testing:
// Resource Grid
// Base Grid

// Non one shot grids:	 (it won't be any faster than it is right now, still iterating every unit/building/worker)
// Anti Mobility
// Cluster
// Threat
// Special Unit


void GridTrackerClass::update()
{
	Display().startClock();
	reset();
	updateMobilityGrids();
	updateAllyGrids();
	updateEnemyGrids();
	updateNeutralGrids();
	updateGroundDistanceGrid();
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
	//		if (reserveGrid[x / 4][y / 4] > 0)
	//		{
	//			Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(8, 8), 4, Colors::Red);
	//		}
	//	}
	//}

	int center = 0;
	for (int x=0; x<1024; x++) for (int y=0; y<1024; y++)
//		(auto &walk : resetWalks)
	{
//		int x = walk.x;
//		int y = walk.y;
		if (!resetGrid[x][y]) continue;

		// Find army center
		if (aClusterGrid[x][y] > center)
		{
			center = aClusterGrid[x][y];
			armyCenter = Position(WalkPosition(x, y));
		}

		// Reset WalkPosition grids		
		aClusterGrid[x][y] = 0;
		aDetectorGrid[x][y] = 0;
		arbiterGrid[x][y] = 0;
		
		eGroundGrid[x][y] = 0.0;
		eAirGrid[x][y] = 0.0;
		eGroundDistanceGrid[x][y] = 0.0;
		eAirDistanceGrid[x][y] = 0.0;		
		eGroundClusterGrid[x][y] = 0;
		eAirClusterGrid[x][y] = 0;
		eDetectorGrid[x][y] = 0;
		
		psiStormGrid[x][y] = 0;
		EMPGrid[x][y] = 0;
		antiMobilityGrid[x][y] = 0;
	}

	// Wipe all the information in our hashed set before gathering information
	memset(resetGrid, 0, 1024*1024*sizeof(bool));
//	resetWalks.clear();
	return;
}

void GridTrackerClass::updateAllyGrids()
{
	// Ally Unit Grid Update
	for (auto &u : Units().getAllyUnits())
	{
		UnitInfo &unit = u.second;
		WalkPosition start = unit.getWalkPosition();
		if (unit.getDeadFrame() == 0 && !unit.getType().isFlyer())
		{
			for (int x = start.x - 20; x <= start.x + 20 + unit.getType().tileWidth() * 4; x++)
			{
				for (int y = start.y - 20; y <= start.y + 20 + unit.getType().tileHeight() * 4; y++)
				{
					// Ally Cluster Grid in a 5 tile radius around each unit
					if (WalkPosition(x, y).isValid() && unit.getPosition().getDistance(Position((x * 8), (y * 8))) <= 160)
					{
//						resetWalks.insert(WalkPosition(x, y));
						resetGrid[x][y] = true;
						aClusterGrid[x][y] += 1;
					}

					// Anti Mobility Grid directly under unit
					if (WalkPosition(x, y).isValid() && x >= start.x && x <= start.x + unit.getType().tileWidth() * 4 && y >= start.y && y <= start.y + unit.getType().tileHeight() * 4)
					{
//						resetWalks.insert(WalkPosition(x, y));
						resetGrid[x][y] = true;
						antiMobilityGrid[x][y] = 1;
					}
				}
			}
		}
	}

	// Building Grid update
	for (auto &b : Buildings().getMyBuildings())
	{
		BuildingInfo &building = b.second;
		WalkPosition start = building.getWalkPosition();
		for (int x = start.x - 2; x < 2 + start.x + building.getType().tileWidth() * 4; x++)
		{
			for (int y = start.y - 2; y < 2 + start.y + building.getType().tileHeight() * 4; y++)
			{
				// Anti Mobility Grid directly under building
				if (WalkPosition(x, y).isValid())
				{
//					resetWalks.insert(WalkPosition(x, y));
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
		for (int x = start.x; x <= start.x + worker.unit()->getType().tileWidth() * 4; x++)
		{
			for (int y = start.y; y <= start.y + worker.unit()->getType().tileHeight() * 4; y++)
			{
				// Anti Mobility Grid directly under worker
				if (WalkPosition(x, y).isValid())
				{
//					resetWalks.insert(WalkPosition(x, y));
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
	for (auto &e : Units().getEnemyUnits())
	{
		UnitInfo enemy = e.second;
		WalkPosition start = enemy.getWalkPosition();

		if (enemy.getType() == UnitTypes::Protoss_Interceptor)
		{
			continue;
		}

		if (enemy.unit() && enemy.getDeadFrame() == 0)
		{
			if (enemy.unit()->exists() && !enemy.getType().isBuilding() && !enemy.unit()->isStasised() && !enemy.unit()->isMaelstrommed())
			{
				for (int x = start.x - 4; x <= 4 + start.x + enemy.getType().tileWidth() * 4; x++)
				{
					for (int y = start.y - 4; y <= 4 + start.y + enemy.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid())
						{
							// Enemy Ground Cluster Grid
							if (!enemy.getType().isFlyer())
							{
//								resetWalks.insert(WalkPosition(x, y));
								resetGrid[x][y] = true;
								eGroundClusterGrid[x][y] += 1;
							}

							// Enemy Air Cluster Grid
							else
							{
//								resetWalks.insert(WalkPosition(x, y));
								resetGrid[x][y] = true;
								eAirClusterGrid[x][y] += 1;
							}

							// Enemy Stasis Grid
							if (enemy.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
							{
//								resetWalks.insert(WalkPosition(x, y));
								resetGrid[x][y] = true;
								stasisClusterGrid[x][y] += 1;
							}
						}
					}
				}
			}

			// Enemy Detector Grid
			if (enemy.getType() == UnitTypes::Protoss_Observer || enemy.getType() == UnitTypes::Protoss_Photon_Cannon || enemy.getType() == UnitTypes::Zerg_Overlord || enemy.getType() == UnitTypes::Zerg_Spore_Colony || enemy.getType() == UnitTypes::Terran_Science_Vessel || enemy.getType() == UnitTypes::Terran_Missile_Turret)
			{
				for (int x = enemy.getWalkPosition().x - 40; x <= enemy.getWalkPosition().x + 40 + enemy.getType().tileWidth() * 4; x++)
				{
					for (int y = enemy.getWalkPosition().y - 40; y <= enemy.getWalkPosition().y + 40 + enemy.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid() && Position(WalkPosition(x, y)).getDistance(enemy.getPosition()) < enemy.getType().sightRange())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							eDetectorGrid[x][y] = 1;
						}
					}
				}
			}

			// Enemy Ground Threat Grid
			for (int x = enemy.getWalkPosition().x - int(enemy.getGroundRange() / 8) - enemy.getType().tileWidth() * 4; x <= enemy.getWalkPosition().x + int(enemy.getGroundRange() / 8) + enemy.getType().tileWidth() * 4; x++)
			{
				for (int y = enemy.getWalkPosition().y - int(enemy.getGroundRange() / 8) - enemy.getType().tileHeight() * 4; y <= enemy.getWalkPosition().y + int(enemy.getGroundRange() / 8) + enemy.getType().tileHeight() * 4; y++)
				{
					if (WalkPosition(x, y).isValid())
					{
						double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(enemy.getPosition()) - double(enemy.getType().width()));

						if (enemy.getGroundDamage() > 0.0 && distance < enemy.getGroundRange())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							eGroundGrid[x][y] += enemy.getMaxGroundStrength();
						}
						if (enemy.getGroundDamage() > 0.0 && distance < enemy.getGroundRange() + enemy.getSpeed())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							eGroundDistanceGrid[x][y] += max(0.1, enemy.getMaxGroundStrength() / distance);
						}
					}
				}
			}

			// Enemy Air Threat Grid
			for (int x = enemy.getWalkPosition().x - int(enemy.getAirRange() / 8) - enemy.getType().tileWidth() * 4; x <= enemy.getWalkPosition().x + int(enemy.getAirRange() / 8) + enemy.getType().tileWidth() * 4; x++)
			{
				for (int y = enemy.getWalkPosition().y - int(enemy.getAirRange() / 8) - enemy.getType().tileHeight() * 4; y <= enemy.getWalkPosition().y + int(enemy.getAirRange() / 8) + enemy.getType().tileHeight() * 4; y++)
				{
					if (WalkPosition(x, y).isValid())
					{
						double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(enemy.getPosition()) - double(enemy.getType().width()));

						if (enemy.getAirDamage() > 0.0 && distance < enemy.getAirRange())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							eAirGrid[x][y] += enemy.getMaxAirStrength();
						}
						if (enemy.getAirDamage() > 0.0 && distance < enemy.getAirRange() + enemy.getSpeed())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							eAirDistanceGrid[x][y] += max(0.1, enemy.getMaxAirStrength() / distance);
						}
					}
				}
			}

			// Anti Mobility Grid
			if (enemy.getType().isBuilding())
			{
				for (int x = start.x; x < start.x + enemy.getType().tileWidth() * 4; x++)
				{
					for (int y = start.y; y < start.y + enemy.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							antiMobilityGrid[x][y] = 1;
						}
					}
				}
			}
			else
			{
				for (int x = start.x; x <= start.x + enemy.getType().tileWidth() * 4; x++)
				{
					for (int y = start.y; y <= start.y + enemy.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid())
						{
//							resetWalks.insert(WalkPosition(x, y));
							resetGrid[x][y] = true;
							antiMobilityGrid[x][y] = 1;
						}
					}
				}
			}
		}
	}
	return;
}

void GridTrackerClass::updateBuildingGrid(BuildingInfo& building)
{
	int buildingOffset;
	if (building.getType() == UnitTypes::Terran_Supply_Depot || building.getType() == UnitTypes::Protoss_Gateway || building.getType() == UnitTypes::Protoss_Robotics_Facility || building.getType() == UnitTypes::Terran_Barracks || building.getType() == UnitTypes::Terran_Factory)
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
		// Reserve Grid
		for (int x = tile.x - buildingOffset; x < tile.x + building.getType().tileWidth() + buildingOffset; x++)
		{
			for (int y = tile.y - buildingOffset; y < tile.y + building.getType().tileHeight() + buildingOffset; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					if (building.unit()->exists())
					{
						reserveGrid[x][y] += 1;
					}
					else
					{
						reserveGrid[x][y] -= 1;
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
			for (int x = building.getTilePosition().x - 10; x < building.getTilePosition().x + building.getType().tileWidth() + 10; x++)
			{
				for (int y = building.getTilePosition().y - 10; y < building.getTilePosition().y + building.getType().tileHeight() + 10; y++)
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
					if (resource.getResourceClusterPosition().getDistance(Position(TilePosition(x, y))) <= 320 && resource.getClosestBasePosition().isValid() && resource.getPosition().getDistance(resource.getClosestBasePosition()) > Position(x * 32, y * 32).getDistance(resource.getClosestBasePosition()))
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
				if (base.unit()->isCompleted())
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
							reserveGrid[x][y] += 1;
						}
						else
						{
							reserveGrid[x][y] -= 1;
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
		if (u->getType().isFlyer())
		{
			continue;
		}
		int startX = (u->getTilePosition().x * 4);
		int startY = (u->getTilePosition().y * 4);
		for (int x = startX - 2; x < 2 + startX + u->getType().tileWidth() * 4; x++)
		{
			for (int y = startY - 2; y < 2 + startY + u->getType().tileHeight() * 4; y++)
			{
				if (WalkPosition(x, y).isValid())
				{
//					resetWalks.insert(WalkPosition(x, y));
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

				if (theMap.GetArea(WalkPosition(x, y))->AccessibleNeighbours().size() == 0)
				{
					// Island
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

	if (Broodwar->getFrameCount() > 500)
	{
		// Create reserve path home
		TilePosition end = Terrain().getPlayerStartingTilePosition();
		TilePosition start = Terrain().getSecondChoke();
		for (int i = 0; i <= 100; i++)
		{
			double closestD = 0.0;
			TilePosition closestT;
			for (int x = start.x - 1; x <= start.x + 1; x++)
			{
				for (int y = start.y - 1; y <= start.y + 1; y++)
				{
					if (!TilePosition(x, y).isValid())
					{
						continue;
					}
					if (reserveGrid[x][y] == 1)
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
				reserveGrid[closestT.x][closestT.y] = 1;
				if (closestT.getDistance(Terrain().getPlayerStartingTilePosition()) < 2)
				{
					return;
				}
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
						reserveGrid[i][j] = 1;
					}
				}
			}
		}
	}
	return;
}

void GridTrackerClass::updateDetectorMovement(SupportUnitInfo& observer)
{
	WalkPosition destination = WalkPosition(observer.getDestination());

	for (int x = destination.x - 40; x <= destination.x + 40; x++)
	{
		for (int y = destination.y - 40; y <= destination.y + 40; y++)
		{
			// Create a circle of detection rather than a square
			if (WalkPosition(x, y).isValid() && observer.getDestination().getDistance(Position(WalkPosition(x, y))) < 160)
			{
//				resetWalks.insert(WalkPosition(x, y));
				resetGrid[x][y] = true;
				aDetectorGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateArbiterMovement(SupportUnitInfo& arbiter)
{
	WalkPosition destination = WalkPosition(arbiter.getDestination());

	for (int x = destination.x - 20; x <= destination.x + 20; x++)
	{
		for (int y = destination.y - 20; y <= destination.y + 20; y++)
		{
			// Create a circle of detection rather than a square
			if (WalkPosition(x, y).isValid() && arbiter.getDestination().getDistance(Position(WalkPosition(x, y))) < 160)
			{
//				resetWalks.insert(WalkPosition(x, y));
				resetGrid[x][y] = true;
				arbiterGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateAllyMovement(Unit unit, WalkPosition here)
{
	for (int x = here.x - unit->getType().width() / 16; x <= here.x + unit->getType().width() / 16; x++)
	{
		for (int y = here.y - unit->getType().height() / 16; y <= here.y + unit->getType().height() / 16; y++)
		{
			if (WalkPosition(x, y).isValid())
			{
//				resetWalks.insert(WalkPosition(x, y));
				resetGrid[x][y] = true;
				antiMobilityGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateReservedLocation(UnitType building, TilePosition here)
{
	//// When placing a building, reserve the tiles so no further locations are placed there
	//for (int x = here.x; x < here.x + building.tileWidth(); x++)
	//{
	//	for (int y = here.y; y < here.y + building.tileHeight(); y++)
	//	{
	//		if (TilePosition(x, y).isValid())
	//		{
	//			resetTiles.insert(TilePosition(x, y));
	//			reserveGrid[x][y] = 1;
	//		}
	//	}
	//}

	//if (building.canBuildAddon())
	//{
	//	for (int x = here.x + building.tileWidth(); x < here.x + building.tileWidth() + 2; x++)
	//	{
	//		for (int y = here.y + 1; y < here.y + 3; y++)
	//		{
	//			if (TilePosition(x, y).isValid())
	//			{
	//				resetTiles.insert(TilePosition(x, y));
	//				reserveGrid[x][y] = 1;
	//			}
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
//				resetWalks.insert(WalkPosition(x, y));
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
//				resetWalks.insert(WalkPosition(x, y));
				resetGrid[x][y] = true;
				EMPGrid[x][y] = 1;
			}
		}
	}
}

void GridTrackerClass::updateGroundDistanceGrid()
{
	// TODO: Goal with this grid is to create a ground distance grid from home for unit micro
	// Need to check for islands
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
			if (duration > 1)
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
		if (duration > 2)
		{
			Broodwar << "Hit maximum, check for islands." << endl;
		}
	}
}