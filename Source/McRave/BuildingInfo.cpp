#include "BuildingInfo.h"

namespace McRave
{
	BuildingInfo::BuildingInfo()
	{
		energy = 0;
		thisUnit = nullptr;
		unitType = UnitTypes::None;
		nextUnit = UnitTypes::None;
		position = Positions::None;
		walkPosition = WalkPositions::None;
		tilePosition = TilePositions::None;
	}
}