#include "TransportInfo.h"

namespace McRave {
	McRave::TransportInfo::TransportInfo()
	{
		position = Positions::None;
		destination = Positions::None;
		walkPosition = WalkPositions::None;
		thisUnit = nullptr;
		cargoSize = 0;
		lastDropFrame = 0;
		harassing = false;
		loading = false;
		unloading = false;
	}

	void TransportInfo::assignCargo(UnitInfo* unit)
	{
		assignedCargo.insert(unit);
		cargoSize = cargoSize + unit->getType().spaceRequired();
	}

	void TransportInfo::removeCargo(UnitInfo* unit)
	{
		assignedCargo.erase(unit);
		cargoSize = cargoSize - unit->getType().spaceRequired();
	}

	void TransportInfo::assignWorker(WorkerInfo* worker)
	{
		assignedWorkers.insert(worker);
		cargoSize = cargoSize + 1;
	}

	void TransportInfo::removeWorker(WorkerInfo* worker)
	{
		assignedWorkers.erase(worker);
		cargoSize = cargoSize - 1;
	}

}