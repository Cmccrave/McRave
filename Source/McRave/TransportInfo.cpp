#include "TransportInfo.h"

namespace McRave {
	McRave::TransportInfo::TransportInfo()
	{
		cargoSize = 0;
		harassing = false;
		loading = false;
		unloading = false;
	}

	void TransportInfo::updateTransportInfo()
	{
		loading = false;
		unloading = false;
		monitoring = false;
		cargoTargets.clear();
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

}