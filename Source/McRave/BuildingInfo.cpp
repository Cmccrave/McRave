#include "BuildingInfo.h"
#include "McRave.h"

namespace McRave
{
	BuildingInfo::BuildingInfo()
	{
		remainingTrainFrame = 0;
		energy = 0;
		thisUnit = nullptr;
		type = UnitTypes::None;
		nextUnit = UnitTypes::None;
		position = Positions::None;
		walkPosition = WalkPositions::None;
		tilePosition = TilePositions::None;
	}

	void BuildingInfo::update() {
		type = thisUnit->getType();
		energy = thisUnit->getEnergy();
		position = thisUnit->getPosition();
		walkPosition = Util().getWalkPosition(thisUnit);
		tilePosition = thisUnit->getTilePosition();
		remainingTrainFrame = max(0, remainingTrainFrame - 1);
	}
}