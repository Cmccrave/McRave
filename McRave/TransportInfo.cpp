#include "TransportInfo.h"

TransportInfo::TransportInfo()
{
	position = Positions::None;
	destination = Positions::None;
	walkPosition = WalkPositions::None;
	thisUnit = nullptr;
	cargoSize = 0;
	harassing = false;
	loading = false;
	unloading = false;
}

void TransportInfo::assignCargo(Unit unit)
{
	assignedCargo.insert(unit);
	cargoSize = cargoSize + unit->getType().spaceRequired();
	return;
}

void TransportInfo::removeCargo(Unit unit)
{
	assignedCargo.erase(unit);
	cargoSize = cargoSize - unit->getType().spaceRequired();
	return;
}