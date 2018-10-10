#pragma once
#include <BWAPI.h>
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerInfo{
		UnitInfo * unitInfo;
		UnitType buildingType;		
		TilePosition buildPosition;
		ResourceInfo* assignedResource;
		int resourceHeldFrames;		
	public:
		WorkerInfo();

		// WorkInfo members
		void updateWorkerInfo();

		bool hasResource()								{ return assignedResource != nullptr; }
		int framesHoldingResource()						{ return resourceHeldFrames; }
		UnitType getBuildingType()						{ return buildingType; }
		TilePosition getBuildPosition()					{ return buildPosition; }		
		ResourceInfo &getResource()						{ return *assignedResource; }		

		void setBuildingType(UnitType newType)			{ buildingType = newType; }			
		void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
		void setResource(ResourceInfo * newResource)	{ assignedResource = newResource; }

		// UnitInfo members
		Position getPosition() { return unitInfo->getPosition(); }
		Position getDestination() { return unitInfo->getDestination(); }
		WalkPosition getWalkPosition() { return unitInfo->getWalkPosition(); }
		TilePosition getTilePosition() { return unitInfo->getTilePosition(); }
		Unit unit() { return unitInfo->unit(); }
		UnitType getType() { return unitInfo->getType(); }
		UnitInfo* info() { return unitInfo; }

		void setDestination(Position newPosition) { unitInfo->setDestination(newPosition); }
		void setTransport(TransportInfo * newTransport) { unitInfo->setTransport(newTransport); }		
		TransportInfo * getTransport() { return unitInfo->getTransport(); }
	};
}