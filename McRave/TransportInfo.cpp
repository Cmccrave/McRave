#include "TransportInfo.h"

TransportInfo::TransportInfo()
{
	position = Positions::None;
	destination = Positions::None;
	walkPosition = WalkPositions::None;
	thisUnit = nullptr;
	loadState = 0;
	cargoSize = 0;
	harassing = false;
}

void TransportInfo::assignCargo(Unit unit)
{
	assignedCargo.insert(unit);
	cargoSize = cargoSize + unit->getType().spaceRequired();
}

void TransportInfo::removeCargo(Unit unit)
{
	assignedCargo.erase(unit);
	cargoSize = cargoSize - unit->getType().spaceRequired();
}