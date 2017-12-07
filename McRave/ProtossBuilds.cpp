#include "McRave.h"

void BuildOrderTrackerClass::P4Gate()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + 3 * (Units().getSupply() >= 62);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 34;
	getOpening = Units().getSupply() < 100;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) > 0;
}

void BuildOrderTrackerClass::PFFESafe()
{
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 18;
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 28);
	buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 22) + (Units().getSupply() >= 24) + (Units().getSupply() >= 30);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 32) + (Units().getSupply() >= 46);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
	getOpening = Units().getSupply() < 60;
	forgeExpand = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::PFFEStandard()
{
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 50);
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 30);
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 44;
	getOpening = Units().getSupply() < 66;
	forgeExpand = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::P12Nexus()
{
	if (Strategy().isRush())
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = 1;
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 36);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		getOpening = Units().getSupply() < 60;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else if (Strategy().isEnemyFastExpand())
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24) + (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core)) + (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) > 1);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 28);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 30;
		getOpening = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) < 3;
		nexusFirst = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core)) + (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) > 1) + (Units().getSupply() >= 76) + (Units().getSupply() >= 82);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 30) + (Units().getSupply() >= 70);
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = (Units().getSupply() >= 70);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = (Units().getSupply() >= 30);
		getOpening = Units().getSupply() < 120;
		nexusFirst = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
}

void BuildOrderTrackerClass::P21Nexus()
{
	if (Strategy().isEnemyFastExpand())
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 44);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Units().getSupply() >= 56;
		getOpening = Units().getSupply() < 70;
		nexusFirst = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 44) + (Units().getSupply() >= 76);
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = (Units().getSupply() >= 70);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Units().getSupply() >= 56;
		getOpening = Units().getSupply() < 100;
		nexusFirst = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
}

void BuildOrderTrackerClass::PDTExpand()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 48);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 54);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 28;
	buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = Units().getSupply() >= 36;
	buildingDesired[UnitTypes::Protoss_Templar_Archives] = Units().getSupply() >= 48;
	getOpening = Units().getSupply() < 54;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) > 0;
}

void BuildOrderTrackerClass::P2GateDragoon()
{
	if (Strategy().isEnemyFastExpand())
	{
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 30);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 22;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		getOpening = Units().getSupply() < 50;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 50) + (Units().getSupply() >= 70);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 30);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 22;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		getOpening = Units().getSupply() < 100;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
}

void BuildOrderTrackerClass::P3GateObs()
{
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + 2 * (Units().getSupply() >= 58);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 28;
	buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Units().getSupply() >= 52;
	getOpening = Units().getSupply() < 80;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::PNZCore()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 36);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
	getOpening = Units().getSupply() < 60;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::PZCore()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 38);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 34;
	getOpening = Units().getSupply() < 60;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::PZZCore()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
	getOpening = Units().getSupply() < 60;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

// Retired builds for now
//void BuildOrderTrackerClass::PFFEGreedy()
//{
//	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
//	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 42);
//	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 28;
//	buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 30);
//	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 38;
//	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
//	getOpening = Units().getSupply() < 66;
//	forgeExpand = true;
//}

//void BuildOrderTrackerClass::P2GateZealot()
//{
//	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 24);
//	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 36;
//	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 44;
//	getOpening = Units().getSupply() < 50;
//}