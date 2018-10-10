#include "WorkerInfo.h"
#include "McRave.h"

namespace McRave
{
	WorkerUnit::WorkerUnit(UnitInfo * newUnit)
	{
		// Store UnitInfo pointer
		unitInfo = newUnit;

		// Initialize the rest
		buildingType = UnitTypes::None;
		buildPosition = TilePositions::Invalid;
		assignedResource = nullptr;
		resourceHeldFrames = 0;		
	}

	void WorkerUnit::updateWorkerUnit() {
		if (unitInfo->unit()->isCarryingGas() || unitInfo->unit()->isCarryingMinerals())
			resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
		else if (unitInfo->unit()->isGatheringGas() || unitInfo->unit()->isGatheringMinerals())
			resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
		else
			resourceHeldFrames = 0;
	}
}
