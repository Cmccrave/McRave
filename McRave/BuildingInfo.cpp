#include "BuildingInfo.h"

McRave::BuildingInfo::BuildingInfo()
{
	energy = 0;
	idleStatus = true;
	thisUnit = nullptr;
	unitType = UnitTypes::None;
	nextUnit = UnitTypes::None;
	position = Positions::None;
	walkPosition = WalkPositions::None;
	tilePosition = TilePositions::None;
}