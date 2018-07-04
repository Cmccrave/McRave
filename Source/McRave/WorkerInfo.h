#pragma once
#include <BWAPI.h>
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerInfo{
		Unit thisUnit;
		Unit transport;
		UnitType unitType, buildingType;
		WalkPosition walkPosition;
		Position position, destination;
		TilePosition tilePosition, buildPosition;
		ResourceInfo* resource;
	public:
		WorkerInfo();
		bool hasResource()								{ return resource != nullptr; }
		Unit unit()										{ return thisUnit; }
		Unit getTransport()								{ return transport; }
		UnitType getType()								{ return unitType; }
		UnitType getBuildingType()						{ return buildingType; }
		Position getPosition()							{ return position; }
		Position getDestination()						{ return destination; }
		WalkPosition getWalkPosition()					{ return walkPosition; }
		TilePosition getTilePosition()					{ return tilePosition; }
		TilePosition getBuildPosition()					{ return buildPosition; }		
		ResourceInfo &getResource()						{ return *resource; }

		void setUnit(Unit newUnit)						{ thisUnit = newUnit; }		
		void setTransport(Unit newTransport)			{ transport = newTransport;	}
		void setType(UnitType newType)					{ unitType = newType; }
		void setBuildingType(UnitType newType)			{ buildingType = newType; }
		void setPosition(Position newPosition)			{ position = newPosition; }
		void setDestination(Position newPosition)		{ destination = newPosition; }
		void setWalkPosition(WalkPosition newPosition)	{ walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition)	{ tilePosition = newPosition; }
		void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
		void setResource(ResourceInfo * newResource)	{ resource = newResource; }
	};
}