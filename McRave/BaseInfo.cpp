#include "BaseInfo.h"

BaseInfo::BaseInfo()
{
	thisUnit = nullptr;
	unitType = UnitTypes::None;
	position = Positions::None;
	walkPosition = WalkPositions::None;
	resourcesPosition = TilePositions::None;
	tilePosition = TilePositions::None;
}