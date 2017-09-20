#include "McRave.h"

void TransportTrackerClass::update()
{
	Display().startClock();
	updateTransports();
	Display().performanceTest(__FUNCTION__);
	return;
}

void TransportTrackerClass::updateTransports()
{
	for (auto &shuttle : myShuttles)
	{
		updateInformation(shuttle.second);
		updateDecision(shuttle.second);
		updateMovement(shuttle.second);
	}
	return;
}

void TransportTrackerClass::updateInformation(TransportInfo& shuttle)
{
	
	shuttle.setType(shuttle.unit()->getType());
	shuttle.setPosition(shuttle.unit()->getPosition());
	shuttle.setWalkPosition(Util().getWalkPosition(shuttle.unit()));

	// Check if the shuttle has space remaining
	if (shuttle.getCargoSize() < 4)
	{
		// See if any Reavers need a shuttle
		for (auto &reaver : SpecialUnits().getMyReavers())
		{
			if (reaver.first && reaver.first->exists() && (!Units().getAllyUnits()[reaver.first].getTransport() || !Units().getAllyUnits()[reaver.first].getTransport()->exists()) && shuttle.getCargoSize() + 2 < 4)
			{
				Units().getAllyUnits()[reaver.first].setTransport(shuttle.unit());
				shuttle.assignCargo(reaver.first);
			}
		}
		// See if any High Templars need a shuttle
		for (auto &templar : SpecialUnits().getMyTemplars())
		{
			if (templar.first && templar.first->exists() && (!Units().getAllyUnits()[templar.first].getTransport() || !Units().getAllyUnits()[templar.first].getTransport()->exists()) && shuttle.getCargoSize() + 1 < 4)
			{
				Units().getAllyUnits()[templar.first].setTransport(shuttle.unit());
				shuttle.assignCargo(templar.first);
			}
		}
	}
	return;
}

void TransportTrackerClass::updateDecision(TransportInfo& shuttle)
{
	// Update what tiles have been used recently
	for (auto &tile : Util().getWalkPositionsUnderUnit(shuttle.unit()))
	{
		recentExplorations[tile] = Broodwar->getFrameCount();
	}

	for (auto &tile : recentExplorations)
	{
		if (tile.second > 100)
		{

		}
	}


	// Reset load state
	shuttle.setLoadState(0);

	//// Check if we should harass or fight with main army
	//if (Strategy().globalAlly() > Strategy().globalEnemy())
	//{
	//	shuttle.setDrop(Terrain().getEnemyStartingPosition());
	//	shuttle.setHarassing(true);
	//}
	//else
	//{
	shuttle.setDrop(Grids().getArmyCenter());
	shuttle.setHarassing(false);
	//}

	// Check if we should be loading/unloading any cargo
	for (auto &cargo : shuttle.getAssignedCargo())
	{
		// If the cargo is not loaded
		if (!cargo->isLoaded())
		{
			// If it's requesting a pickup
			if ((Units().getAllyUnits()[cargo].getTargetPosition().getDistance(Units().getAllyUnits()[cargo].getPosition()) > 320) || (cargo->getType() == UnitTypes::Protoss_Reaver && cargo->getGroundWeaponCooldown() > Broodwar->getLatencyFrames()) || (cargo->getType() == UnitTypes::Protoss_High_Templar && cargo->getEnergy() < 75))
			{
				shuttle.unit()->load(cargo);
				shuttle.setLoadState(1);
				continue;
			}
		}
		// Else if the cargo is loaded
		else if (cargo->isLoaded())
		{
			shuttle.setDrop(Units().getAllyUnits()[cargo].getTargetPosition());

			// If we are harassing, check if we are close to drop point
			if (shuttle.isHarassing() && shuttle.getPosition().getDistance(shuttle.getDrop()) < 160)
			{
				shuttle.unit()->unload(cargo);
				shuttle.setLoadState(2);
			}
			// Else check if we are in a position to help the main army
			else if (!shuttle.isHarassing() && Units().getAllyUnits()[cargo].getStrategy() == 1 && Units().getAllyUnits()[cargo].getPosition().getDistance(Units().getAllyUnits()[cargo].getTargetPosition()) < 320 && (cargo->getGroundWeaponCooldown() <= Broodwar->getLatencyFrames() || cargo->getEnergy() >= 75))
			{
				shuttle.unit()->unload(cargo);
				shuttle.setLoadState(2);
			}
		}
	}
	return;
}

void TransportTrackerClass::updateMovement(TransportInfo& shuttle)
{
	// If performing a loading action, don't update movement
	if (shuttle.getLoadState() == 1)
	{
		return;
	}

	Position bestPosition = shuttle.getDrop();
	WalkPosition start = shuttle.getWalkPosition();
	double bestCluster = 0;
	int radius = 10;
	double closestD = ((32 * Grids().getMobilityGrid(start)) + Position(start).getDistance(shuttle.getDrop()));

	for (auto &tile : Util().getWalkPositionsUnderUnit(shuttle.unit()))
	{
		if (Grids().getEGroundDistanceGrid(tile) > 0 || Grids().getEAirDistanceGrid(tile) > 0)
		{
			radius = 25;
			closestD = 0.0;
			continue;
		}
	}


	// First look for mini tiles with no threat that are closest to the enemy and on low mobility
	for (int x = start.x - radius; x <= start.x + radius; x++)
	{
		for (int y = start.y - radius; y <= start.y + radius; y++)
		{
			if (!WalkPosition(x, y).isValid())
			{
				continue;
			}

			if (Grids().getEAirDistanceGrid(WalkPosition(x, y)) > 0)
			{
				continue;
			}

			// If trying to unload, must find a walkable tile
			if (shuttle.getLoadState() == 2 && Grids().getMobilityGrid(x, y) == 0)
			{
				continue;
			}

			// Else, move to high ally cluster areas
			if (Position(WalkPosition(x, y)).getDistance(Position(start)) > 64 && (Grids().getACluster(x, y) > bestCluster || (Grids().getACluster(x, y) == bestCluster && bestCluster != 0 && shuttle.getDrop().getDistance(Position(WalkPosition(x, y))) < closestD)))
			{
				closestD = shuttle.getDrop().getDistance(Position(WalkPosition(x, y)));
				bestCluster = Grids().getACluster(x, y);
				bestPosition = Position(WalkPosition(x, y));
			}

			//// If low mobility, no threat and closest to the drop point
			//if ((32 * Grids().getMobilityGrid(x, y)) + Position(WalkPosition(x, y)).getDistance(shuttle.getDrop()) < closestD || closestD == 0.0)
			//{
			//	bool bestTile = true;
			//	for (int i = x - shuttle.getType().width() / 16; i < x + shuttle.getType().width() / 16; i++)
			//	{
			//		for (int j = y - shuttle.getType().height() / 16; j < y + shuttle.getType().height() / 16; j++)
			//		{
			//			if (WalkPosition(i, j).isValid())
			//			{								
			//				// If position has a threat, don't move there
			//				if (Grids().getEGroundDistanceGrid(i, j) > 0.0 || Grids().getEAirDistanceGrid(i, j) > 0.0)
			//				{
			//					bestTile = false;
			//					//Broodwar->drawCircleMap(Position(WalkPosition(i, j)), 2, Colors::Blue, true);
			//				}
			//			}
			//			else
			//			{
			//				bestTile = false;
			//			}
			//		}
			//	}
			//	if (bestTile)
			//	{
			//		closestD = (32 * Grids().getMobilityGrid(x, y)) + Position(WalkPosition(x, y)).getDistance(Position(shuttle.getDrop()));
			//		bestPosition = Position(WalkPosition(x, y));
			//	}				
			//}
		}
	}
	shuttle.setDestination(bestPosition);
	Broodwar->drawLineMap(shuttle.getPosition(), shuttle.getDestination(), Broodwar->self()->getColor());
	shuttle.unit()->move(shuttle.getDestination());
	return;
}

void TransportTrackerClass::removeUnit(Unit unit)
{
	// Delete if it's a shuttled unit
	for (auto &shuttle : myShuttles)
	{
		if (shuttle.second.getAssignedCargo().find(unit) != shuttle.second.getAssignedCargo().end())
		{
			shuttle.second.removeCargo(unit);
			return;
		}
	}

	// Delete if it's the shuttle
	if (myShuttles.find(unit) != myShuttles.end())
	{
		myShuttles.erase(unit);
		for (auto &cargo : myShuttles[unit].getAssignedCargo())
		{
			if (Units().getAllyUnits().find(cargo) != Units().getAllyUnits().end())
			{
				Units().getAllyUnits()[cargo].setTransport(nullptr);
			}
		}
	}
	return;
}

void TransportTrackerClass::storeUnit(Unit unit)
{
	myShuttles[unit].setUnit(unit);
	return;
}