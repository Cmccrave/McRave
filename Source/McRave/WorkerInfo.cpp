#include "WorkerInfo.h"
#include "McRave.h"

namespace McRave
{
	WorkerInfo::WorkerInfo()
	{
		thisUnit = nullptr;
		resource = nullptr;
		transport = nullptr;
		resourceHeldFrames = 0;

		type = UnitTypes::None;
		buildingType = UnitTypes::None;

		position = Positions::None;
		destination = Positions::None;
		walkPosition = WalkPositions::None;
		tilePosition = TilePositions::None;
		buildPosition = TilePositions::None;
	}

	void WorkerInfo::update() {
		position =  thisUnit->getPosition();
		walkPosition = Util().getWalkPosition(thisUnit);
		tilePosition = thisUnit->getTilePosition();
		type = thisUnit->getType();

		if (thisUnit->isCarryingGas() || thisUnit->isCarryingMinerals())
			resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
		else if (thisUnit->isGatheringGas() || thisUnit->isGatheringMinerals())
			resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
		else
			resourceHeldFrames = 0;
	}
}
