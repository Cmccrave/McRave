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
	// Update general information
	shuttle.setType(shuttle.unit()->getType());
	shuttle.setPosition(shuttle.unit()->getPosition());
	shuttle.setWalkPosition(Util().getWalkPosition(shuttle.unit()));

	// Update cargo information
	if (shuttle.getCargoSize() < 4)
	{
		// See if any Reavers need a shuttle
		for (auto &r : Units().getAllyUnitsFilter(UnitTypes::Protoss_Reaver))
		{
			UnitInfo &reaver = r.second;
			if (reaver.unit() && reaver.unit()->exists() && (!reaver.getTransport() || !reaver.getTransport()->exists()) && shuttle.getCargoSize() + 2 < 4)
			{
				reaver.setTransport(shuttle.unit());
				shuttle.assignCargo(reaver.unit());
			}
		}
		// See if any High Templars need a shuttle
		for (auto &t : Units().getAllyUnitsFilter(UnitTypes::Protoss_High_Templar))
		{
			UnitInfo &templar = t.second;
			if (templar.unit() && templar.unit()->exists() && (!templar.getTransport() || !templar.getTransport()->exists()) && shuttle.getCargoSize() + 1 < 4)
			{
				templar.setTransport(shuttle.unit());
				shuttle.assignCargo(templar.unit());
			}
		}
		// TODO: Else grab something random (DT?)
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

	// Check if we should be loading/unloading any cargo
	for (auto &c : shuttle.getAssignedCargo())
	{		
		UnitInfo& cargo = Units().getAllyUnit(c);
		
		// If the cargo is not loaded
		if (cargo.unit() && !cargo.unit()->isLoaded())
		{
			// If it's requesting a pickup
			if ((cargo.getTargetPosition().getDistance(cargo.getPosition()) > 320) || (cargo.getType() == UnitTypes::Protoss_Reaver && cargo.unit()->getGroundWeaponCooldown() > 0) || (cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() < 75))
			{
				shuttle.unit()->load(cargo.unit());
				shuttle.setLoadState(1);
				continue;
			}
		}
		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded())
		{
			shuttle.setDrop(cargo.getTargetPosition());

			// If we are harassing, check if we are close to drop point
			if (shuttle.isHarassing() && shuttle.getPosition().getDistance(shuttle.getDrop()) < 160)
			{
				shuttle.unit()->unload(cargo.unit());
				shuttle.setLoadState(2);
			}
			// Else check if we are in a position to help the main army
			else if (!shuttle.isHarassing() && cargo.getStrategy() == 1 && cargo.getPosition().getDistance(cargo.getTargetPosition()) < 320 && (cargo.unit()->getGroundWeaponCooldown() <= Broodwar->getLatencyFrames() || cargo.unit()->getEnergy() >= 75))
			{
				shuttle.unit()->unload(cargo.unit());
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
		if (Grids().getEGroundThreat(tile) > 0.0 || Grids().getEAirThreat(tile) > 0.0)
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

			if (Grids().getEAirThreat(WalkPosition(x, y)) > 0.0)
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

			// If low mobility, no threat and closest to the drop point
			if ((32 * Grids().getMobilityGrid(x, y)) + Position(WalkPosition(x, y)).getDistance(shuttle.getDrop()) < closestD || closestD == 0.0)
			{
				bool bestTile = true;
				for (int i = x - shuttle.getType().width() / 16; i < x + shuttle.getType().width() / 16; i++)
				{
					for (int j = y - shuttle.getType().height() / 16; j < y + shuttle.getType().height() / 16; j++)
					{
						if (WalkPosition(i, j).isValid())
						{								
							// If position has a threat, don't move there
							if (Grids().getEGroundThreat(i, j) > 0.0 || Grids().getEAirThreat(i, j) > 0.0)
							{
								bestTile = false;
							}
						}
						else
						{
							bestTile = false;
						}
					}
				}
				if (bestTile)
				{
					closestD = (32 * Grids().getMobilityGrid(x, y)) + Position(WalkPosition(x, y)).getDistance(Position(shuttle.getDrop()));
					bestPosition = Position(WalkPosition(x, y));
				}				
			}
		}
	}
	shuttle.setDestination(bestPosition);
	shuttle.unit()->move(shuttle.getDestination());
	return;
}

void TransportTrackerClass::removeUnit(Unit unit)
{
	//// Delete if it's a shuttled unit
	//for (auto &shuttle : myShuttles)
	//{
	//	for (auto &cargo : shuttle.second.getAssignedCargo())
	//	{
	//		if (cargo == unit)
	//		{
	//			shuttle.second.removeCargo(cargo);
	//			return;
	//		}
	//	}
	//}

	//// Delete if it's the shuttle
	//if (myShuttles.find(unit) != myShuttles.end())
	//{		
	//	for (auto &c : myShuttles[unit].getAssignedCargo())
	//	{
	//		UnitInfo &cargo = Units().getAllyUnit(c);
	//		cargo.setTransport(nullptr);
	//	}
	//	myShuttles.erase(unit);
	//}
	//return;
}

void TransportTrackerClass::storeUnit(Unit unit)
{
	myShuttles[unit].setUnit(unit);
	return;
}