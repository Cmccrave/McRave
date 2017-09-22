#include "SpecialUnitInfo.h"

SupportUnitInfo::SupportUnitInfo()
{
	position = Positions::None;
	destination = Positions::None;
	walkPosition = WalkPositions::None;
	thisUnit = nullptr;
	target = nullptr;
}

TransportInfo::TransportInfo()
{
	position = Positions::None;
	destination = Positions::None;
	drop = Positions::None;
	walkPosition = WalkPositions::None;
	thisUnit = nullptr;
	loadState = 0;
	cargoSize = 0;
	harassing = false;
}