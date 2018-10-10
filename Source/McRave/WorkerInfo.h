#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerUnit{
		UnitInfo * unitInfo;
		UnitType buildingType;		
		TilePosition buildPosition;
		ResourceInfo* assignedResource;
		TransportUnit* assignedTransport;
		int resourceHeldFrames;		
	public:
		WorkerUnit(UnitInfo *);

		// WorkInfo members
		void updateWorkerUnit();
		UnitInfo* info() { return unitInfo; }

		bool hasResource()								{ return assignedResource != nullptr; }
		int framesHoldingResource()						{ return resourceHeldFrames; }
		UnitType getBuildingType()						{ return buildingType; }
		TilePosition getBuildPosition()					{ return buildPosition; }		
		ResourceInfo &getResource()						{ return *assignedResource; }		

		void setBuildingType(UnitType newType)			{ buildingType = newType; }			
		void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
		void setResource(ResourceInfo * newResource)	{ assignedResource = newResource; }
		
		// Transport
		void setTransport(TransportUnit * newTransport) { assignedTransport = newTransport; }		
		TransportUnit * getTransport() { return getTransport(); }
	};
}