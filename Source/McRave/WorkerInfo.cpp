#include "WorkerInfo.h"

McRave::WorkerInfo::WorkerInfo()
{
	thisUnit = nullptr;
	resource = nullptr;
	transport = nullptr;

	unitType = UnitTypes::None;
	buildingType = UnitTypes::None;

	position = Positions::None;
	destination = Positions::None;
	walkPosition = WalkPositions::None;
	tilePosition = TilePositions::None;
	buildPosition = TilePositions::None;
}