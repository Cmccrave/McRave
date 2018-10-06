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
		UnitType type, buildingType;
		WalkPosition walkPosition;
		Position position, destination;
		TilePosition tilePosition, buildPosition;
		ResourceInfo* resource;
		int resourceHeldFrames;
	public:
		WorkerInfo();
		void update();

		bool hasResource()								{ return resource != nullptr; }
		int framesHoldingResource()						{ return resourceHeldFrames; }
		Unit unit()										{ return thisUnit; }
		Unit getTransport()								{ return transport; }
		UnitType getType()								{ return type; }
		UnitType getBuildingType()						{ return buildingType; }
		Position getPosition()							{ return position; }
		Position getDestination()						{ return destination; }
		WalkPosition getWalkPosition()					{ return walkPosition; }
		TilePosition getTilePosition()					{ return tilePosition; }
		TilePosition getBuildPosition()					{ return buildPosition; }		
		ResourceInfo &getResource()						{ return *resource; }

		void setUnit(Unit newUnit)						{ thisUnit = newUnit; }		
		void setTransport(Unit newTransport)			{ transport = newTransport;	}		
		void setBuildingType(UnitType newType)			{ buildingType = newType; }		
		void setDestination(Position newPosition)		{ destination = newPosition; }		
		void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
		void setResource(ResourceInfo * newResource)	{ resource = newResource; }
	};
}