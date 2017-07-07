#include "McRave.h"

void GridTrackerClass::update()
{
	reset();
	updateMobilityGrids();
	updateAllyGrids();
	updateEnemyGrids();
	updateNeutralGrids();
	updateGroundDistanceGrid();
	Display().performanceTest(__func__);
	return;
}

void GridTrackerClass::reset()
{
	// Reset all tiles to 0
	int center = 0;
	for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
	{
		for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
		{
			if (WalkPosition(x, y).isValid())
			{
				if (aClusterGrid[x][y] > center)
				{
					center = aClusterGrid[x][y];
					armyCenter = Position(WalkPosition(x, y));
				}

				/*if ()
				{
				Broodwar->drawBoxMap(Position(x * 8, y * 8), Position(x * 8 + 32, y * 8 + 32), Colors::Black);
				}*/

				// Reset WalkPosition grids
				aClusterGrid[x][y] = 0;
				antiMobilityGrid[x][y] = 0;
				eGroundGrid[x][y] = 0.0;
				eAirGrid[x][y] = 0.0;
				eGroundDistanceGrid[x][y] = 0.0;
				eAirDistanceGrid[x][y] = 0.0;
				observerGrid[x][y] = 0;
				arbiterGrid[x][y] = 0;
				eDetectorGrid[x][y] = 0;

				// Reset TilePosition grids
				if (x % 4 == 0 && y % 4 == 0 && TilePosition(x / 4, y / 4).isValid())
				{
					// Debug test
					if (reservePathHome[x / 4][y / 4] > 0)
					{
						//Broodwar->drawBoxMap(Position(x * 8, y * 8), Position(x * 8 + 32, y * 8 + 32), Colors::Black);v
					}

					// Reset cluster grids
					eGroundClusterGrid[x / 4][y / 4] = 0;
					eAirClusterGrid[x / 4][y / 4] = 0;

					// Reset other grids
					reserveGrid[x / 4][y / 4] = 0;
					baseGrid[x / 4][y / 4] = 0;
					pylonGrid[x / 4][y / 4] = 0;
					batteryGrid[x / 4][y / 4] = 0;
					defenseGrid[x / 4][y / 4] = 0;
					bunkerGrid[x / 4][y / 4] = 0;
					resourceGrid[x / 4][y / 4] = 0;
				}
			}

			// Temp path home stuff
			if (reservePathHome[x / 4][y / 4] == 1)
			{
				reserveGrid[x / 4][y / 4] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateAllyGrids()
{
	// Clusters and anti mobility from units
	for (auto &u : Units().getMyUnits())
	{
		UnitInfo &unit = u.second;
		if (unit.getDeadFrame() == 0)
		{
			WalkPosition start = unit.getWalkPosition();
			int offsetX = unit.getPosition().x % 32;
			int offsetY = unit.getPosition().y % 32;

			// Make sure unit is alive and not an Arbiter or Observer (and not medic for now)
			if (unit.getType() != UnitTypes::Protoss_Arbiter && unit.getType() != UnitTypes::Protoss_Observer && unit.getType() != UnitTypes::Terran_Medic)
			{
				for (int x = start.x - 20; x <= start.x + 20 + unit.getType().tileWidth() * 4; x++)
				{
					for (int y = start.y - 20; y <= start.y + 20 + unit.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid() && (unit.getPosition()).getDistance(Position((x * 8), (y * 8))) <= 160)
						{
							aClusterGrid[x][y] += 1;
						}
					}
				}
			}

			// Anti mobility doesn't apply to flying units
			if (!unit.getType().isFlyer())
			{
				for (int x = start.x; x <= start.x + unit.getType().tileWidth() * 4; x++)
				{
					for (int y = start.y; y <= start.y + unit.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid())
						{
							antiMobilityGrid[x][y] = 1;
						}
					}
				}
			}
		}
	}

	// Building Grid update
	for (auto &b : Buildings().getMyBuildings())
	{
		BuildingInfo &building = b.second;
		if (building.unit() && building.unit()->exists())
		{
			// Anti Mobility Grid
			WalkPosition start = building.getWalkPosition();
			for (int x = start.x - 2; x < 2 + start.x + building.getType().tileWidth() * 4; x++)
			{
				for (int y = start.y - 2; y < 2 + start.y + building.getType().tileHeight() * 4; y++)
				{
					if (WalkPosition(x, y).isValid())
					{
						antiMobilityGrid[x][y] = 1;
					}
				}
			}

			// Reserve Grid for Buildings
			TilePosition tile = building.getTilePosition();
			int offset = 0;
			if (building.getType() == UnitTypes::Protoss_Gateway || building.getType() == UnitTypes::Protoss_Robotics_Facility || building.getType() == UnitTypes::Terran_Barracks || building.getType() == UnitTypes::Terran_Factory)
			{
				offset = 1;
			}
			for (int x = tile.x - offset; x < tile.x + building.getType().tileWidth() + offset; x++)
			{
				for (int y = tile.y - offset; y < tile.y + building.getType().tileHeight() + offset; y++)
				{
					if (TilePosition(x, y).isValid())
					{
						reserveGrid[x][y] = 1;
					}
				}
			}

			// Reserve Grid for Addons
			if (building.getType().canBuildAddon())
			{
				for (int x = tile.x + building.getType().tileWidth(); x < tile.x + building.getType().tileWidth() + 2; x++)
				{
					for (int y = tile.y + 1; y < tile.y + 3; y++)
					{
						if (TilePosition(x, y).isValid())
						{
							reserveGrid[x][y] = 1;
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
							pylonGrid[x][y] += 1;
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
							batteryGrid[x][y] += 1;
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
							bunkerGrid[x][y] += 1;
						}
					}
				}
			}

			// Defense Grid
			if (building.getType() == UnitTypes::Protoss_Photon_Cannon || building.getType() == UnitTypes::Terran_Bunker || building.getType() == UnitTypes::Zerg_Sunken_Colony)
			{
				for (int x = building.getTilePosition().x - 7; x < building.getTilePosition().x + building.getType().tileWidth() + 7; x++)
				{
					for (int y = building.getTilePosition().y - 7; y < building.getTilePosition().y + building.getType().tileHeight() + 7; y++)
					{
						if (TilePosition(x, y).isValid() && building.getPosition().getDistance(Position(TilePosition(x, y))) < 224)
						{
							defenseGrid[x][y] += 1;
						}
					}
				}
			}
		}
	}

	// Worker Grid update
	for (auto &w : Workers().getMyWorkers())
	{
		WorkerInfo &worker = w.second;
		// Anti Mobility Grid
		WalkPosition start = worker.getWalkPosition();
		for (int x = start.x; x <= start.x + worker.unit()->getType().tileWidth() * 4; x++)
		{
			for (int y = start.y; y <= start.y + worker.unit()->getType().tileHeight() * 4; y++)
			{
				if (WalkPosition(x, y).isValid())
				{
					antiMobilityGrid[x][y] = 1;
				}
			}
		}
	}

	// Base Grid
	for (auto &b : Bases().getMyBases())
	{
		BaseInfo &base = b.second;
		for (int x = base.getTilePosition().x - 8; x < base.getTilePosition().x + 12; x++)
		{
			for (int y = base.getTilePosition().y - 8; y < base.getTilePosition().y + 11; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					if (base.unit()->isCompleted())
					{
						baseGrid[x][y] = 2;
					}
					else
					{
						baseGrid[x][y] = 1;
					}
				}
			}
		}
	}
	return;
}

void GridTrackerClass::updateEnemyGrids()
{
	for (auto &e : Units().getEnUnits())
	{
		UnitInfo enemy = e.second;

		if (enemy.unit() && enemy.getDeadFrame() == 0)
		{
			WalkPosition start = enemy.getWalkPosition();

			// Enemy Cluster Grid
			if (enemy.unit()->exists() && !enemy.getType().isBuilding() && !enemy.unit()->isStasised() && !enemy.unit()->isMaelstrommed())
			{
				for (int x = enemy.getTilePosition().x - 1; x <= enemy.getTilePosition().x + 1; x++)
				{
					for (int y = enemy.getTilePosition().y - 1; y <= enemy.getTilePosition().y + 1; y++)
					{
						if (TilePosition(x, y).isValid())
						{
							if (!enemy.getType().isFlyer())
							{
								eGroundClusterGrid[x][y] += 1;
							}
							else
							{
								eAirClusterGrid[x][y] += 1;
							}
							if (enemy.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
							{
								stasisClusterGrid[x][y] += 1;
							}
						}
					}
				}
			}

			// Detector Grid
			if (enemy.getType() == UnitTypes::Protoss_Observer || enemy.getType() == UnitTypes::Protoss_Photon_Cannon || enemy.getType() == UnitTypes::Zerg_Overlord || enemy.getType() == UnitTypes::Zerg_Spore_Colony || enemy.getType() == UnitTypes::Terran_Science_Vessel || enemy.getType() == UnitTypes::Terran_Missile_Turret)
			{
				for (int x = enemy.getWalkPosition().x - 40; x <= enemy.getWalkPosition().x + 40 + enemy.getType().tileWidth() * 4; x++)
				{
					for (int y = enemy.getWalkPosition().y - 40; y <= enemy.getWalkPosition().y + 40 + enemy.getType().tileHeight() * 4; y++)
					{
						if (WalkPosition(x, y).isValid() && Position(WalkPosition(x, y)).getDistance(enemy.getPosition()) < enemy.getType().sightRange())
						{
							eDetectorGrid[x][y] = 1;
						}
					}
				}
			}

			// Ground Threat Grids
			for (int x = enemy.getWalkPosition().x - int(enemy.getGroundRange() / 8) - enemy.getType().tileWidth() * 4; x <= enemy.getWalkPosition().x + int(enemy.getGroundRange() / 8) + enemy.getType().tileWidth() * 4; x++)
			{
				for (int y = enemy.getWalkPosition().y - int(enemy.getGroundRange() / 8) - enemy.getType().tileHeight() * 4; y <= enemy.getWalkPosition().y + int(enemy.getGroundRange() / 8) + enemy.getType().tileHeight() * 4; y++)
				{
					if (WalkPosition(x, y).isValid())
					{
						double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(enemy.getPosition()) - double(enemy.getType().tileWidth() * 32.0));

						if (enemy.getGroundDamage() > 0 && distance < enemy.getGroundRange() + enemy.getSpeed() * 16.0)
						{
							eGroundDistanceGrid[x][y] += (1.0 + enemy.getMaxStrength()) / distance;
						}

						if (enemy.getGroundDamage() > 0.0 && Position(x * 8, y * 8).getDistance(enemy.getPosition()) - enemy.getType().tileWidth() * 16 < enemy.getGroundRange())
						{
							eGroundGrid[x][y] += enemy.getMaxStrength();
						}
					}
				}
			}

			// Air Threat Grids
			for (int x = enemy.getWalkPosition().x - int(enemy.getAirRange() / 8) - enemy.getType().tileWidth() * 4; x <= enemy.getWalkPosition().x + int(enemy.getAirRange() / 8) + enemy.getType().tileWidth() * 4; x++)
			{
				for (int y = enemy.getWalkPosition().y - int(enemy.getAirRange() / 8) - enemy.getType().tileHeight() * 4; y <= enemy.getWalkPosition().y + int(enemy.getAirRange() / 8) + enemy.getType().tileHeight() * 4; y++)
				{
					if (WalkPosition(x, y).isValid())
					{
						double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(enemy.getPosition()) - double(enemy.getType().tileWidth() * 32.0));

						if (enemy.getAirDamage() > 0.0 && Position(x * 8, y * 8).getDistance(enemy.getPosition()) - enemy.getType().tileWidth() * 16 < enemy.getAirRange())
						{
							eAirGrid[x][y] += enemy.getMaxStrength();
						}
						if (enemy.getAirDamage() > 0 && distance < enemy.getAirRange() + enemy.getSpeed() * 16.0)
						{
							eAirDistanceGrid[x][y] += (1.0 + enemy.getMaxStrength()) / distance;
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
							antiMobilityGrid[x][y] = 1;
						}
					}
				}
			}
		}
	}
	return;
}

void GridTrackerClass::updateNeutralGrids()
{
	// Resource Grid for Minerals
	for (auto &m : Resources().getMyMinerals())
	{
		for (int x = m.second.getTilePosition().x - 5; x < m.second.getTilePosition().x + m.second.getType().tileWidth() + 5; x++)
		{
			for (int y = m.second.getTilePosition().y - 5; y < m.second.getTilePosition().y + m.second.getType().tileHeight() + 5; y++)
			{
				if (!m.second.getClosestBase())
				{
					continue;
				}
				if (TilePosition(x, y).isValid() && Position(Bases().getMyBases()[m.second.getClosestBase()].getResourcesPosition()).getDistance(Position(TilePosition(x, y))) <= 192 && m.second.getPosition().getDistance(m.second.getClosestBase()->getPosition()) + 64 > Position(x * 32, y * 32).getDistance(m.second.getClosestBase()->getPosition()))
				{
					resourceGrid[x][y] = 1;
				}
			}
		}
	}

	// Resource Grid for Gas
	for (auto &g : Resources().getMyGas())
	{
		for (int x = g.second.getTilePosition().x - 5; x < g.second.getTilePosition().x + g.second.getType().tileWidth() + 5; x++)
		{
			for (int y = g.second.getTilePosition().y - 5; y < g.second.getTilePosition().y + g.second.getType().tileHeight() + 5; y++)
			{
				if (!g.second.getClosestBase())
				{
					continue;
				}
				if (TilePosition(x, y).isValid() && Position(Bases().getMyBases()[g.second.getClosestBase()].getResourcesPosition()).getDistance(Position(TilePosition(x, y))) <= 320 && g.second.getPosition().getDistance(g.second.getClosestBase()->getPosition()) + 64 > Position(x * 32, y * 32).getDistance(g.second.getClosestBase()->getPosition()))
				{
					resourceGrid[x][y] = 1;
				}
			}
		}
	}

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
				if (WalkPosition(x, y).isValid() && theMap.GetMiniTile(WalkPosition(x, y)).Walkable())
				{
					for (int i = -12; i <= 12; i++)
					{
						for (int j = -12; j <= 12; j++)
						{
							// The more tiles around x,y that are walkable, the more mobility x,y has				
							if (WalkPosition(x + i, y + j).isValid() && theMap.GetMiniTile(WalkPosition(x + i, y + j)).Walkable())
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
										if (WalkPosition(x + i, y + j).isValid() && !theMap.GetMiniTile(WalkPosition(x + i, y + j)).Walkable())
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
		while (reservePathHome[end.x][end.y] == 0)
		{
			double closestD = 0.0;
			TilePosition closestT;
			for (int x = start.x - 1; x <= start.x + 1; x++)
			{
				for (int y = start.y - 1; y <= start.y + 1; y++)
				{
					if (!TilePosition(x, y).isValid() && TilePosition(x, y).getDistance(start) > 1)
					{
						continue;
					}
					if (Grids().getMobilityGrid(WalkPosition(TilePosition(x, y))) > 0 && TilePosition(x, y).isValid() && (Grids().getDistanceHome(WalkPosition(TilePosition(x, y))) < closestD || closestD == 0.0))
					{
						closestD = Grids().getDistanceHome(WalkPosition(TilePosition(x, y)));
						closestT = TilePosition(x, y);
					}
				}
			}

			if (closestT.isValid())
			{
				start = closestT;
				reservePathHome[closestT.x][closestT.y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateObserverMovement(Unit observer)
{
	WalkPosition destination = WalkPosition(SpecialUnits().getMyObservers()[observer].getDestination());

	for (int x = destination.x - 40; x <= destination.x + 40; x++)
	{
		for (int y = destination.y - 40; y <= destination.y + 40; y++)
		{
			// Create a circle of detection rather than a square
			if (WalkPosition(x, y).isValid() && SpecialUnits().getMyObservers()[observer].getDestination().getDistance(Position(WalkPosition(x, y))) < 320)
			{
				observerGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateArbiterMovement(Unit arbiter)
{
	WalkPosition destination = WalkPosition(SpecialUnits().getMyArbiters()[arbiter].getDestination());

	for (int x = destination.x - 20; x <= destination.x + 20; x++)
	{
		for (int y = destination.y - 20; y <= destination.y + 20; y++)
		{
			// Create a circle of detection rather than a square
			if (WalkPosition(x, y).isValid() && SpecialUnits().getMyArbiters()[arbiter].getDestination().getDistance(Position(WalkPosition(x, y))) < 160)
			{
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
				antiMobilityGrid[x][y] = 1;
			}
		}
	}
	return;
}

void GridTrackerClass::updateReservedLocation(UnitType building, TilePosition here)
{
	// When placing a building, reserve the tiles so no further locations are placed there
	for (int x = here.x; x < here.x + building.tileWidth(); x++)
	{
		for (int y = here.y; y < here.y + building.tileHeight(); y++)
		{
			if (TilePosition(x, y).isValid())
			{
				reserveGrid[x][y] = 1;
			}
		}
	}

	if (building.canBuildAddon())
	{
		for (int x = here.x + building.tileWidth(); x < here.x + building.tileWidth() + 2; x++)
		{
			for (int y = here.y + 1; y < here.y + 3; y++)
			{
				if (TilePosition(x, y).isValid())
				{
					reserveGrid[x][y] = 1;
				}
			}
		}
	}
	return;
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
					if (distanceGridHome[x][y] == 0 && theMap.GetMiniTile(WalkPosition(x, y)).AreaId() > 0)
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