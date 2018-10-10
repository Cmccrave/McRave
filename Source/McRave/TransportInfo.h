#pragma once
#include <BWAPI.h>
#include <set>
#include "UnitInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TransportUnit
	{
		int cargoSize;
		bool harassing, loading, unloading, monitoring;
		UnitInfo * unitInfo;
		set<CombatUnit*> assignedCargo;
		vector<Position> cargoTargets;
	public:
		TransportUnit();
		void updateTransportUnit();

		// Cargo
		int getCargoSize() { return cargoSize; }
		void setCargoSize(int newSize) { cargoSize = newSize; }
		set<CombatUnit*>& getAssignedCargo() { return assignedCargo; }
		void assignCargo(CombatUnit*);
		//void assignWorker(WorkerUnit*);
		void removeCargo(CombatUnit*);
		//void removeWorker(WorkerUnit*);

		// Cargo targets
		void addCargoTarget(Position here) { cargoTargets.push_back(here); }
		vector<Position>& getCargoTargets() { return cargoTargets; }

		// Decision
		bool isLoading() { return loading; }
		bool isUnloading() { return unloading; }
		bool isHarassing() { return harassing; }
		bool isMonitoring() { return monitoring; }

		void setLoading(bool newState) { loading = newState; }
		void setUnloading(bool newState) { unloading = newState; }
		void setHarassing(bool newState) { harassing = newState; }
		void setMonitoring(bool newState) { monitoring = newState; }

		// UnitInfo shortcuts
		UnitInfo * info() { return unitInfo; }
		UnitType getType() { return unitInfo->getType(); }
		Position getPosition() { return unitInfo->getPosition(); }
		Position getDestination() { return unitInfo->getDestination(); }
		WalkPosition getWalkPosition() { return unitInfo->getWalkPosition(); }
		TilePosition getTilePosition() { return unitInfo->getTilePosition(); }
		void setDestination(Position newPosition) { unitInfo->setDestination(newPosition); }
		void setEngage() { unitInfo->setEngage(); }
		void setRetreat() { unitInfo->setRetreat(); }
	};
}