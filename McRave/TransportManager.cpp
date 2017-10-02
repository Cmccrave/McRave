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
	// Reset load state
	shuttle.setLoadState(0);
	shuttle.setDrop(shuttle.getPosition());

	// Check if we should be loading/unloading any cargo
	for (auto &c : shuttle.getAssignedCargo())
	{
		UnitInfo& cargo = Units().getAllyUnit(c);

		if (!cargo.unit())
		{
			continue;
		}

		// If the cargo is not loaded
		if (!cargo.unit()->isLoaded())
		{
			// If it's requesting a pickup
			if (cargo.getTargetPosition().getDistance(cargo.getPosition()) > cargo.getGroundRange() || cargo.getStrategy() != 1 || (cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() < 75))
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
			Broodwar->drawLineMap(shuttle.getPosition(), cargo.getTargetPosition(), Colors::Black);

			// If we are harassing, check if we are close to drop point
			if (shuttle.isHarassing() && shuttle.getPosition().getDistance(shuttle.getDrop()) < cargo.getGroundRange())
			{
				shuttle.unit()->unload(cargo.unit());
				shuttle.setLoadState(2);
			}
			// Else check if we are in a position to help the main army
			else if (!shuttle.isHarassing() && cargo.getStrategy() == 1 && cargo.getPosition().getDistance(cargo.getTargetPosition()) < cargo.getGroundRange() && (cargo.getType() != UnitTypes::Protoss_High_Templar || cargo.unit()->getEnergy() >= 75))
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
	double best = 0.0;

	// First look for mini tiles with no threat that are closest to the enemy and on low mobility
	for (int x = start.x - 15; x <= start.x + 15 + shuttle.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 15; y <= start.y + 15 + shuttle.getType().tileWidth() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid())
			{
				continue;
			}

			// If trying to unload, must find a walkable tile
			if (shuttle.getLoadState() == 2 && Grids().getMobilityGrid(x, y) == 0)
			{
				continue;
			}

			// Must keep the shuttle moving to retain maximum speed
			if (Position(WalkPosition(x, y)).getDistance(Position(start)) <= 64)
			{
				continue;
			}

			double distance = max(1.0, shuttle.getDrop().getDistance(Position(WalkPosition(x, y))));
			double threat =  max(0.1, Grids().getEGroundThreat(x, y) + Grids().getEAirThreat(x, y));
			double mobility = max(1.0, double(Grids().getMobilityGrid(x, y)));			

			// If shuttle is harassing, then include mobility
			if (shuttle.isHarassing())
			{
				if (1.0 / (distance * threat * mobility) > best)
				{
					best = 1.0 / (distance * threat * mobility);
					bestPosition = Position(WalkPosition(x, y));
				}
			}
			else
			{
				if (1.0 / (distance * threat) > best)
				{
					best = 1.0 / (distance * threat);
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
	// Delete if it's a shuttled unit
	for (auto &shuttle : myShuttles)
	{
		for (auto &cargo : shuttle.second.getAssignedCargo())
		{
			if (cargo == unit)
			{
				shuttle.second.removeCargo(cargo);
				return;
			}
		}
	}

	// Delete if it's the shuttle
	if (myShuttles.find(unit) != myShuttles.end())
	{
		for (auto &c : myShuttles[unit].getAssignedCargo())
		{
			UnitInfo &cargo = Units().getAllyUnit(c);
			cargo.setTransport(nullptr);
		}
		myShuttles.erase(unit);
	}
	return;
}

void TransportTrackerClass::storeUnit(Unit unit)
{
	myShuttles[unit].setUnit(unit);
	return;
}