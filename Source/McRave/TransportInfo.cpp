#include "TransportInfo.h"

namespace McRave {
	McRave::TransportUnit::TransportUnit()
	{
		cargoSize = 0;
		harassing = false;
		loading = false;
		unloading = false;
	}

	void TransportUnit::updateTransportUnit()
	{
		loading = false;
		unloading = false;
		monitoring = false;
		cargoTargets.clear();
	}

	void TransportUnit::assignCargo(UnitInfo* unit)
	{
		assignedCargo.insert(unit);
		cargoSize = cargoSize + unit->getType().spaceRequired();
	}

	void TransportUnit::removeCargo(UnitInfo* unit)
	{
		assignedCargo.erase(unit);
		cargoSize = cargoSize - unit->getType().spaceRequired();
	}

}