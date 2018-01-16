#include "McRave.h"

namespace McRave
{
	void BuildOrderTrackerClass::FFEScout()
	{
		firstUpgrade = UpgradeTypes::Protoss_Air_Weapons;
		firstTech = TechTypes::None;
		unlockedType.insert(UnitTypes::Protoss_Scout);
		techList.insert(UnitTypes::Protoss_Scout);
		techUnit = UnitTypes::Protoss_Scout;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 30);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Forge] = (Units().getSupply() >= 20);
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 50);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = (Units().getSupply() >= 40);
		buildingDesired[UnitTypes::Protoss_Stargate] = 4 * (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core));
		getOpening = Units().getSupply() < 80;
		fastExpand = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
	}	
}