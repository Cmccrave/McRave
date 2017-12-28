#include "McRave.h"

void BuildOrderTrackerClass::ZOverpool()
{
	buildingDesired[UnitTypes::Zerg_Spawning_Pool] = Units().getSupply() >= 18;
	buildingDesired[UnitTypes::Zerg_Extractor] = Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Spawning_Pool) > 0;
	buildingDesired[UnitTypes::Zerg_Hatchery] = 2 * (Units().getSupply() >= 28);
	getOpening = Units().getSupply() < 30;
	return;
}