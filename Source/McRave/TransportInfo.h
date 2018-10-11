#pragma once
#include <BWAPI.h>
#include <set>
#include "UnitInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerInfo;
	class UnitInfo;
	class TransportInfo
	{
		int cargoSize, lastDropFrame;
		bool harassing, loading, unloading, monitoring;
		Unit thisUnit;
		UnitType transportType;

		bool retreat, engage;

		set<WorkerInfo*> assignedWorkers;
		set<UnitInfo*> assignedCargo;
		vector<Position> cargoTargets;

		Position position, destination;
		WalkPosition walkPosition;
		TilePosition tilePosition;
	public:
		TransportInfo();

		int getCargoSize() { return cargoSize; }
		int getLastDropFrame() { return lastDropFrame; }
		bool isLoading() { return loading; }
		bool isUnloading() { return unloading; }
		bool isHarassing() { return harassing; }
		bool isMonitoring() { return monitoring; }
		bool isRetreating() { return retreat; }
		bool isEngaging() { return engage; }
		Unit unit() { return thisUnit; }
		UnitType getType() { return transportType; }

		set<WorkerInfo*>& getAssignedWorkers() { return assignedWorkers; }
		set<UnitInfo*>& getAssignedCargo() { return assignedCargo; }		
		Position getPosition() { return position; }
		Position getDestination() { return destination; }
		WalkPosition getWalkPosition() { return walkPosition; }
		TilePosition getTilePosition() { return tilePosition; }

		void setCargoSize(int newSize) { cargoSize = newSize; }
		void setLastDropFrame(int newDropFrame) { lastDropFrame = newDropFrame; }
		void setLoading(bool newState) { loading = newState; }
		void setUnloading(bool newState) { unloading = newState; }
		void setHarassing(bool newState) { harassing = newState; }
		void setMonitoring(bool newState) { monitoring = newState; }
		void setRetreating(bool newState) { retreat = newState; }
		void setEngaging(bool newState) { engage = newState; }
		void setUnit(Unit newTransport) { thisUnit = newTransport; }
		void setType(UnitType newType) { transportType = newType; }
		void setPosition(Position newPosition) { position = newPosition; }
		void setDestination(Position newPosition) { destination = newPosition; }
		void setWalkPosition(WalkPosition newWalkPosition) { walkPosition = newWalkPosition; }
		void setTilePosition(TilePosition newTilePosition) { tilePosition = newTilePosition; }

		// Add cargo to the assigned cargo set
		void assignCargo(UnitInfo*);
		void assignWorker(WorkerInfo*);

		// Cargo targets for easier use
		void addCargoTarget(Position here) { cargoTargets.push_back(here); }
		vector<Position>& getCargoTargets() { return cargoTargets; }

		// Remove cargo from the assigned cargo set
		void removeCargo(UnitInfo*);
		void removeWorker(WorkerInfo*);
	};
}