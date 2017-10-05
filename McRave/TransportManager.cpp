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
			if (reaver.unit() && reaver.unit()->exists() && reaver.getDeadFrame() == 0 && (!reaver.getTransport() || !reaver.getTransport()->exists()) && transport.getCargoSize() + 2 < 4)
			{
				reaver.setTransport(transport.unit());
				transport.assignCargo(reaver.unit());
			}
		}
		// See if any High Templars need a shuttle
		for (auto &t : Units().getAllyUnitsFilter(UnitTypes::Protoss_High_Templar))
		{
			UnitInfo &templar = t.second;
			if (templar.unit() && templar.unit()->exists() && templar.getDeadFrame() == 0 && (!templar.getTransport() || !templar.getTransport()->exists()) && transport.getCargoSize() + 1 < 4)
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
	transport.setLoading(false);
	transport.setUnloading(false);
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
			transport.setDestination(cargo.getPosition());

			// If it's requesting a pickup, set load state to 1
			if (cargo.getTargetPosition().getDistance(cargo.getPosition()) > cargo.getGroundRange() || cargo.getStrategy() != 1 || (cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() < 75) || (cargo.getType() != UnitTypes::Protoss_High_Templar && Broodwar->getFrameCount() - cargo.getLastAttackFrame() > cargo.getType().groundWeapon().damageCooldown()))
			{
				transport.unit()->load(cargo.unit());
				transport.setLoading(true);
				continue;
			}
		}
		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded())
		{
			transport.setDestination(cargo.getTargetPosition());

			// If cargo wants to fight, find a spot to unload
			if (cargo.getStrategy() == 1)
			{
				transport.setUnloading(true);

				if (transport.getPosition().getDistance(transport.getDestination()) < cargo.getGroundRange() && Util().isSafe(transport.getWalkPosition(), WalkPosition(transport.getDestination()), transport.getType(), true, true, true))
				{
					transport.unit()->unload(cargo.unit());
					continue;
				}
			}
		}
	}
	return;
}

void TransportTrackerClass::updateMovement(TransportInfo& transport)
{
	// If loading, ignore movement commands
	if (transport.isLoading())
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

			// Must keep the shuttle moving to retain maximum speed
			if (Position(WalkPosition(x, y)).getDistance(Position(start)) <= 64)
			{
				continue;
			}

			double distance = max(1.0, transport.getDestination().getDistance(Position(WalkPosition(x, y))));
			double threat = max(0.1, Grids().getEGroundThreat(x, y) + Grids().getEAirThreat(x, y));
			double mobility = max(1.0, double(Grids().getMobilityGrid(x, y)));

			// Include mobility when looking to unload
			if (transport.isUnloading())
			{
				if (1.0 / distance > best && Util().isSafe(start, WalkPosition(x, y), transport.getType(), true, true, true))
				{
					best = 1.0 / distance;
				}
			}
			else
			{
				if (1.0 / distance > best && Util().isSafe(start, WalkPosition(x, y), transport.getType(), true, true, false))
				{
					best = 1.0 / distance;
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