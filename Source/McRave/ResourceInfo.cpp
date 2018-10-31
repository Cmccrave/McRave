#include "ResourceInfo.h"

namespace McRave {

	ResourceInfo::ResourceInfo() {
		station = nullptr;
		thisUnit = nullptr;
		gathererCount = 0;
		remainingResources = 0;
		//rState = ResourceState::None;
		type = UnitTypes::None;
		position = Positions::None;
		tilePosition = TilePositions::None;
	}

	void ResourceInfo::updateResource() {
		type				= thisUnit->getType();
		remainingResources	= thisUnit->getResources();
		position			= thisUnit->getPosition();
		tilePosition		= thisUnit->getTilePosition();
	}
}

