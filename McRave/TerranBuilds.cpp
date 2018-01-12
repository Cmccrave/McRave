#include "McRave.h"

void BuildOrderTrackerClass::T2Fact()
{
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Tank_Siege_Mode;
	buildingDesired[UnitTypes::Terran_Barracks] = (Units().getSupply() >= 22);
	buildingDesired[UnitTypes::Terran_Refinery] = (Units().getSupply() >= 24);
	buildingDesired[UnitTypes::Terran_Factory] = (Units().getSupply() >= 30) + (Units().getSupply() >= 36);
	getOpening = Units().getSupply() < 36;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;
}

void BuildOrderTrackerClass::TSparks()
{
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Stim_Packs;
	buildingDesired[UnitTypes::Terran_Barracks] = (Units().getSupply() >= 20) + (Units().getSupply() >= 22) + (Units().getSupply() >= 46);	
	buildingDesired[UnitTypes::Terran_Refinery] = (Units().getSupply() >= 40);
	buildingDesired[UnitTypes::Terran_Academy] = (Units().getSupply() >= 42);
	getOpening = Units().getSupply() < 60;
	bioBuild = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;
}