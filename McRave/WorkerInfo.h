#pragma once
#include <BWAPI.h>
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerInfo{
		Unit thisUnit;
		UnitType unitType, buildingType;
		WalkPosition walkPosition;
		Position position;
		TilePosition tilePosition, buildPosition;
		ResourceInfo* resource;
	public:
		WorkerInfo();
		Unit unit() { return thisUnit; }
		UnitType getType() { return unitType; }
		UnitType getBuildingType() { return buildingType; }
		Position getPosition() { return position; }
		WalkPosition getWalkPosition() { return walkPosition; }
		TilePosition getTilePosition() { return tilePosition; }
		TilePosition getBuildPosition() { return buildPosition; }

		void setUnit(Unit newUnit) { thisUnit = newUnit; }		
		void setType(UnitType newType) { unitType = newType; }
		void setBuildingType(UnitType newBuildingType) { buildingType = newBuildingType; }
		void setPosition(Position newPosition) { position = newPosition; }
		void setWalkPosition(WalkPosition newPosition) { walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition) { tilePosition = newPosition; }
		void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }

		void setResource(ResourceInfo * newResource) { resource = newResource; }
		bool hasResource() { return resource != nullptr; }
		ResourceInfo &getResource() { return *resource; }
	};
}