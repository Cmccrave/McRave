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
	for (auto &transport : myTransports)
	{
		updateInformation(transport.second);
		updateCargo(transport.second);
		updateDecision(transport.second);
		updateMovement(transport.second);
	}
	return;
}

void TransportTrackerClass::updateCargo(TransportInfo& transport)
{
	// Update cargo information
	if (transport.getCargoSize() < 4)
	{
		// See if any Reavers need a shuttle
		for (auto &r : Units().getAllyUnitsFilter(UnitTypes::Protoss_Reaver))
		{
			UnitInfo &reaver = r.second;
			if (reaver.unit() && reaver.unit()->exists() && (!reaver.getTransport() || !reaver.getTransport()->exists()) && transport.getCargoSize() + 2 < 4)
			{
				reaver.setTransport(transport.unit());
				transport.assignCargo(reaver.unit());
			}
		}
		// See if any High Templars need a shuttle
		for (auto &t : Units().getAllyUnitsFilter(UnitTypes::Protoss_High_Templar))
		{
			UnitInfo &templar = t.second;
			if (templar.unit() && templar.unit()->exists() && (!templar.getTransport() || !templar.getTransport()->exists()) && transport.getCargoSize() + 1 < 4)
			{
				templar.setTransport(transport.unit());
				transport.assignCargo(templar.unit());
			}
		}
		// TODO: Else grab something random (DT?)
	}
}

void TransportTrackerClass::updateInformation(TransportInfo& transport)
{
	// Update general information
	transport.setType(transport.unit()->getType());
	transport.setPosition(transport.unit()->getPosition());
	transport.setWalkPosition(Util().getWalkPosition(transport.unit()));
	transport.setLoadState(0);
	transport.setDestination(Terrain().getPlayerStartingPosition());
	return;
}

void TransportTrackerClass::updateDecision(TransportInfo& transport)
{
	// Check if we should be loading/unloading any cargo
	for (auto &c : transport.getAssignedCargo())
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
				transport.unit()->load(cargo.unit());
				transport.setLoadState(1);
				continue;
			}
		}
		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded())
		{			
			transport.setDestination(cargo.getTargetPosition());
			Broodwar->drawLineMap(transport.getPosition(), cargo.getTargetPosition(), Colors::Black);

			// If we are harassing, check if we are close to drop point
			if (transport.isHarassing() && transport.getPosition().getDistance(transport.getDestination()) < cargo.getGroundRange())
			{
				transport.unit()->unload(cargo.unit());
				transport.setLoadState(2);
			}
			// Else check if we are in a position to help the main army
			else if (!transport.isHarassing() && cargo.getStrategy() == 1 && cargo.getPosition().getDistance(cargo.getTargetPosition()) < cargo.getGroundRange() && (cargo.getType() != UnitTypes::Protoss_High_Templar || cargo.unit()->getEnergy() >= 75))
			{
				transport.unit()->unload(cargo.unit());
				transport.setLoadState(2);
			}
		}
	}
	return;
}

void TransportTrackerClass::updateMovement(TransportInfo& transport)
{
	// If performing a loading action, don't update movement
	if (transport.getLoadState() == 1)
	{
		return;
	}

	Position bestPosition = transport.getDestination();
	WalkPosition start = transport.getWalkPosition();
	double best = 0.0;

	// First look for mini tiles with no threat that are closest to the enemy and on low mobility
	for (int x = start.x - 15; x <= start.x + 15 + transport.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 15; y <= start.y + 15 + transport.getType().tileWidth() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid())
			{
				continue;
			}

			// If trying to unload, must find a walkable tile
			if (transport.getLoadState() == 2 && Grids().getMobilityGrid(x, y) == 0)
			{
				continue;
			}

			// Must keep the shuttle moving to retain maximum speed
			if (Position(WalkPosition(x, y)).getDistance(Position(start)) <= 64)
			{
				continue;
			}

			double distance = max(1.0, transport.getDestination().getDistance(Position(WalkPosition(x, y))));
			double threat =  max(0.1, Grids().getEGroundThreat(x, y) + Grids().getEAirThreat(x, y));
			double mobility = max(1.0, double(Grids().getMobilityGrid(x, y)));			

			// If shuttle is harassing, then include mobility
			if (transport.isHarassing())
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
	transport.unit()->move(bestPosition);
	return;
}

void TransportTrackerClass::removeUnit(Unit unit)
{
	// Delete if it's a shuttled unit
	for (auto &transport : myTransports)
	{
		for (auto &cargo : transport.second.getAssignedCargo())
		{
			if (cargo == unit)
			{
				transport.second.removeCargo(cargo);
				return;
			}
		}
	}

	// Delete if it's the shuttle
	if (myTransports.find(unit) != myTransports.end())
	{
		for (auto &c : myTransports[unit].getAssignedCargo())
		{
			UnitInfo &cargo = Units().getAllyUnit(c);
			cargo.setTransport(nullptr);
		}
		myTransports.erase(unit);
	}
	return;
}

void TransportTrackerClass::storeUnit(Unit unit)
{
	myTransports[unit].setUnit(unit);
	return;
}