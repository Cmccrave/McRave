#include "McRave.h"

void GridTrackerClass::update()
{
	Display().startClock();
	draw();
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
	allyArmyCenter = Positions::Invalid;
	enemyArmyCenter = Positions::Invalid;
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
			eCenter = eGroundClusterGrid[x][y] * int(eGroundThreat[x][y]) + eAirClusterGrid[x][y] * int(eAirThreat[x][y]);
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
		eSplashGrid[x][y] = 0;

		psiStormGrid[x][y] = 0;
		EMPGrid[x][y] = 0;
		antiMobilityGrid[x][y] = 0;
	}

	// Wipe all the information
	memset(resetGrid, 0, 1024 * 1024 * sizeof(bool));
	return;
}

void GridTrackerClass::draw()
{
	return; // Remove this to draw stuff

	// Temp debugging for tile positions
	for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
	{
		for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
		{
			/*if (buildingGrid[x][y] > 0)
			{
			Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Black);
			}*/
			//if (mobilityGrid[x][y] > 0 && mobilityGrid[x][y] < 4)
			//{
			//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Black);
			//	//Broodwar->drawCircleMap(Position(TilePosition(x, y)) + Position(16, 16), 4, Colors::Black);
			//}
			//if (mobilityGrid[x][y] >= 4 && mobilityGrid[x][y] < 6)
			//{
			//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Purple);
			//}
			//if (mobilityGrid[x][y] >= 6 && mobilityGrid[x][y] < 8)
			//{
			//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Blue);
			//}
			//if (mobilityGrid[x][y] >= 8 && mobilityGrid[x][y] < 10)
			//{
			//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Green);
			//}
			//if (mobilityGrid[x][y] >= 10)
			//{
			//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Red);
			//}

			if (distanceGridHome[x][y] < 100)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Black);
			}
			if (distanceGridHome[x][y] >= 100 && distanceGridHome[x][y] < 500)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Brown);
			}
			if (distanceGridHome[x][y] >= 500 && distanceGridHome[x][y] < 1000)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Purple);
			}
			if (distanceGridHome[x][y] >= 1000 && distanceGridHome[x][y] < 1500)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Blue);
			}
			if (distanceGridHome[x][y] >= 1500 && distanceGridHome[x][y] < 2000)
			{
				Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 4, Colors::Green);
			}
		}
	}
}

void GridTrackerClass::updateAllyGrids()
{
	// Ally Unit Grid Update
	for (auto &u : Units().getAllyUnits())
	{
		UnitInfo &unit = u.second;
		if (unit.getType() == UnitTypes::Protoss_Arbiter || unit.getType() == UnitTypes::Protoss_Observer || unit.getType() == UnitTypes::Protoss_Shuttle) continue;
		double width = double(unit.getType().width());
		int walkWidth = int(double(unit.getType().width()) / 8.0);
		int radius = max(12, int((unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8.0));
		WalkPosition start = unit.getWalkPosition();
		WalkPosition center = WalkPosition(start.x + walkWidth, start.y + walkWidth);

		for (int x = start.x - radius; x <= start.x + radius + walkWidth; x++)
		{
			for (int y = start.y - radius; y <= start.y + radius + walkWidth; y++)
			{
				if (!WalkPosition(x, y).isValid()) continue;

				double distance = max(1.0, Position(WalkPosition(x, y)).getDistance(unit.getPosition()) - (width / 2.0));

				// Cluster grids
				if (center.getDistance(WalkPosition(x, y)) <= 12)
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
		WalkPosition center = WalkPosition(start.x + walkWidth, start.y + walkWidth);

		if (((unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine && !unit.unit()->isBurrowed()) || unit.getType() == UnitTypes::Protoss_Scarab) && unit.hasTarget() && unit.getTarget().unit()->exists() && unit.getTarget().getPlayer() == Broodwar->self() && !unit.getTarget().getType().isWorker())
		{
			WalkPosition targetWalk = unit.getTarget().getWalkPosition();
			for (int x = targetWalk.x - 12; x <= targetWalk.x + 12 + walkWidth; x++)
			{
				for (int y = targetWalk.y - 12; y <= targetWalk.y + 12 + walkWidth; y++)
				{
					if (!WalkPosition(x, y).isValid()) continue;
					if (x > targetWalk.x && x <= targetWalk.x + walkWidth && y > targetWalk.y && y <= targetWalk.y + walkWidth) continue;
					if (unit.getTarget().getPosition().getDistance(Position(WalkPosition(x, y))) > 96) continue;
					eSplashGrid[x][y] += 1;
				}
			}
		}

		if (unit.getType().isWorker())
		{
			if (unit.unit()->exists() && (!Terrain().isInAllyTerritory(unit.getTilePosition()) || (Broodwar->getFrameCount() - unit.getLastAttackFrame() < 500)))
			{
				radius = max(12, int((unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8.0));
				gReach = unit.getGroundRange() + unit.getSpeed();
			}
			else continue;
		}
		else
		{
			radius = max(12, int((unit.getSpeed() + max(unit.getGroundRange(), unit.getAirRange())) / 8.0));
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
				if (!unit.getType().isBuilding() && center.getDistance(WalkPosition(x, y)) <= 12)
				{
					if (unit.getType().isFlyer())
					{
						resetGrid[x][y] = true;
						eAirClusterGrid[x][y] += unit.getPriority();
					}
					if (!unit.getType().isFlyer())
					{
						resetGrid[x][y] = true;
						eGroundClusterGrid[x][y] += unit.getPriority();
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
	//// Add/remove building to/from grid
	TilePosition tile = building.getTilePosition();
	if (building.unit() && tile.isValid())
	{
		// Pylon Grid
		if (building.getType() == UnitTypes::Protoss_Pylon)
		{
			for (int x = building.getTilePosition().x - 6; x < building.getTilePosition().x + building.getType().tileWidth() + 6; x++)
			{
				for (int y = building.getTilePosition().y - 6; y < building.getTilePosition().y + building.getType().tileHeight() + 6; y++)
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
				if (!WalkPosition(x, y).isValid()) continue;
				if (mapBWEM.GetMiniTile(WalkPosition(x, y)).Walkable())
				{
					for (int i = -12; i <= 12; i++)
					{
						for (int j = -12; j <= 12; j++)
						{
							// The more tiles around x,y that are walkable, the more mobility x,y has				
							if (WalkPosition(x + i, y + j).isValid() && mapBWEM.GetMiniTile(WalkPosition(x + i, y + j)).Walkable())
							{
								mobilityGrid[x][y] += 1;
							}
						}
					}
					mobilityGrid[x][y] = int(double(mobilityGrid[x][y]) / 56);

					for (auto &area : mapBWEM.Areas())
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
										if (WalkPosition(x + i, y + j).isValid() && !mapBWEM.GetMiniTile(WalkPosition(x + i, y + j)).Walkable())
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

				if (!mapBWEM.GetArea(WalkPosition(x, y)) || mapBWEM.GetArea(WalkPosition(x, y))->AccessibleNeighbours().size() == 0)
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
		distanceAnalysis = true;
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++)
		{
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++)
			{
				WalkPosition here = WalkPosition(x, y);
				if (distanceGridHome[x][y] < 0) continue;
				distanceGridHome[x][y] = mapBWEB.getGroundDistance(Position(here), Terrain().getPlayerStartingPosition());
			}
		}		
	}
}