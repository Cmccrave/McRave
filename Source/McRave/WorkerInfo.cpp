#include "WorkerInfo.h"
#include "McRave.h"

namespace McRave
{
	WorkerInfo::WorkerInfo()
	{
		unitInfo = nullptr;
		buildingType = UnitTypes::None;
		buildPosition = TilePositions::Invalid;
		assignedResource = nullptr;
		resourceHeldFrames = 0;		
	}

	void WorkerInfo::update() {
		if (unitInfo->unit()->isCarryingGas() || unitInfo->unit()->isCarryingMinerals())
			resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
		else if (unitInfo->unit()->isGatheringGas() || unitInfo->unit()->isGatheringMinerals())
			resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
		else
			resourceHeldFrames = 0;
	}
}
