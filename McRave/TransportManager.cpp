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
			transport.getPosition().getDistance(transport.getDestination()) <= cargo.getGroundRange() + 32 ? transport.setMonitoring(true) : transport.setMonitoring(false);

			// If it's requesting a pickup, set load state to 1
			if (cargo.getTargetPosition().getDistance(cargo.getPosition()) > cargo.getGroundRange() + 64 || cargo.getStrategy() != 1 || (cargo.getType() == UnitTypes::Protoss_High_Templar && cargo.unit()->getEnergy() < 75) || (cargo.getType() == UnitTypes::Protoss_Reaver && cargo.unit()->getScarabCount() < 5))
			{
				transport.setLoading(true);
				transport.unit()->load(cargo.unit());
				return;
			}			
		}
		// Else if the cargo is loaded
		else if (cargo.unit()->isLoaded() && cargo.getTargetPosition().isValid())
		{
			transport.setDestination(cargo.getTargetPosition());
			// If cargo wants to fight, find a spot to unload
			if (cargo.getStrategy() == 1) transport.setUnloading(true);			
			if (transport.getPosition().getDistance(transport.getDestination()) <= cargo.getGroundRange() + 32)
			{
				transport.unit()->unload(cargo.unit());				
			}
		}
	}
	return;
}

void TransportTrackerClass::updateMovement(TransportInfo& transport)
{
	// If loading, ignore movement commands
	if (transport.isLoading()) return;
	if (Bases().getClosestEnemyBase(transport.getPosition()).isValid() && (transport.getPosition().getDistance(transport.getDestination()) > 640 || Units().getGlobalGroundStrategy() == 1))
		transport.setDestination(Bases().getClosestEnemyBase(transport.getPosition()));

	Position bestPosition = transport.getDestination();
	WalkPosition start = transport.getWalkPosition();
	double best = 0.0;

	// First look for mini tiles with no threat that are closest to the enemy and on low mobility
	for (int x = start.x - 8; x <= start.x + 8 + transport.getType().tileWidth() * 4; x++)
	{
		for (int y = start.y - 8; y <= start.y + 8 + transport.getType().tileWidth() * 4; y++)
		{
			if (!WalkPosition(x, y).isValid()) continue;
			if (Position(WalkPosition(x, y)).getDistance(Position(start)) <= 32) continue;			
			if (transport.isUnloading() && (!Util().isMobile(start, WalkPosition(x, y), transport.getType()) || Broodwar->getGroundHeight(transport.getTilePosition()) >= Broodwar->getGroundHeight(TilePosition(transport.getDestination())))) continue;

			// If we just dropped units, we need to make sure not to leave them
			if (transport.isMonitoring())
			{
				for (auto &u : transport.getAssignedCargo())
				{
					UnitInfo& unit = Units().getAllyUnit(u);
					if (unit.getPosition().getDistance(Position(WalkPosition(x, y))) > 128) continue;
				}
			}

			double distance = max(1.0, transport.getDestination().getDistance(Position(WalkPosition(x, y))) - 256);
			double threat = max(0.01, Grids().getEGroundThreat(x, y) + Grids().getEAirThreat(x, y));

			if (threat * distance < best || best == 0.0)
			{
				best = threat * distance;
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