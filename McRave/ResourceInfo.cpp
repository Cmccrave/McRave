#include "ResourceInfo.h"

McRave::ResourceInfo::ResourceInfo()
{	
	gathererCount = 0;
	remainingResources = 0;
	miningState = 0;
	storedUnit = nullptr;
	unitType = UnitTypes::None;
	position = Positions::None;
	tilePosition = TilePositions::None;
	walkPosition = WalkPositions::None;
}

McRave::ResourceInfo::~ResourceInfo()
{

}
