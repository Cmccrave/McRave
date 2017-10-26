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
	if (transport.getCargoSize() < 8)
	{
		// See if any Reavers need a shuttle
		for (auto &r : Units().getAllyUnitsFilter(UnitTypes::Protoss_Reaver))
		{
			UnitInfo &reaver = Units().getAllyUnit(r);

			if (reaver.unit() && reaver.unit()->exists() && !reaver.getTransport() && transport.getCargoSize() + reaver.getType().spaceRequired() <= 8)
			{
				reaver.setTransport(transport.unit());
				transport.assignCargo(reaver.unit());
			}
		}
		// See if any High Templars need a shuttle
		for (auto &t : Units().getAllyUnitsFilter(UnitTypes::Protoss_High_Templar))
		{
			UnitInfo &templar = Units().getAllyUnit(t);
			if (templar.unit() && templar.unit()->exists() && !templar.getTransport() && transport.getCargoSize() + templar.getType().spaceRequired() <= 8)
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
	transport.setTilePosition(transport.unit()->getTilePosition());
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
		if (!cargo.unit())	continue;

		// If the cargo is not loaded
		if (!cargo.unit()->isLoaded())
		{
			transport.setDestination(cargo.getPosition());

			// If it's requesting a pickup, set load state to 1
			if (cargo.getTargetPosition().getDistance(cargo.getPosition()) > cargo.getGroundRange() + 64 || cargo.getStrategy() != 1 || (cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() < 75) || (cargo.getType() != UnitTypes::Protoss_High_Templar && Broodwar->getFrameCount() - cargo.getLastAttackFrame() < cargo.getType().groundWeapon().damageCooldown()))
			{
				transport.unit()->load(cargo.unit());
				transport.setLoading(true);
				return;
			}
		}
		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded() && cargo.getTargetPosition().isValid())
		{
			transport.setDestination(cargo.getTargetPosition());

			// If cargo wants to fight, find a spot to unload
			if (cargo.getStrategy() == 1 && transport.getPosition().getDistance(transport.getDestination()) <= cargo.getGroundRange() && Broodwar->getGroundHeight(transport.getTilePosition()) == Broodwar->getGroundHeight(cargo.getTargetTilePosition()))
			{
				transport.setUnloading(true);
				transport.unit()->unload(cargo.unit());
				transport.setLastDropFrame(Broodwar->getFrameCount());
				continue;
			}
		}
	}
	return;
}

void TransportTrackerClass::updateMovement(TransportInfo& transport)
{
	// If loading, ignore movement commands
	if (transport.isLoading()) return;

	Position bestPosition = transport.getDestination();
	WalkPosition start = transport.getWalkPosition();
	double best = 1000.0;
	double closest = 0.0;

	// First look for mini tiles with no threat that are closest to the enemy and on low mobility
	for (int x = start.x - 32; x <= start.x + 32 + transport.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 32; y <= start.y + 32 + transport.getType().tileWidth() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid()) continue;
			if (Position(WalkPosition(x, y)).getDistance(Position(start)) <= 64) continue;
			if (transport.isUnloading() && !Util().isMobile(start, WalkPosition(x, y), transport.getType()) && !Util().isSafe(WalkPosition(x, y), UnitTypes::Protoss_Reaver, true, true)) continue;

			double distance = transport.getDestination().getDistance(Position(WalkPosition(x, y)));
			double threat = Grids().getEGroundThreat(x, y) + Grids().getEAirThreat(x, y);

			if (((threat < best) || (threat == best && (distance < closest || closest == 0.0))))
			{
				best = threat;
				closest = distance;
				bestPosition = Position(WalkPosition(x, y));
			}
		}
	}
	if (bestPosition.isValid())
	{
		transport.unit()->move(bestPosition);
	}
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