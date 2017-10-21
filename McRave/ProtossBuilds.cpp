#include "McRave.h"

void BuildOrderTrackerClass::ZZCore()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
	getOpening = Units().getSupply() < 44;
	oneGateCore = true;
	return;
}

void BuildOrderTrackerClass::ZCore()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 38);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 34;
	getOpening = Units().getSupply() < 44;
	oneGateCore = true;
	return;
}

void BuildOrderTrackerClass::NZCore()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 36);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
	getOpening = Units().getSupply() < 44;
	oneGateCore = true;
	return;
}

void BuildOrderTrackerClass::FFECannon()
{
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 18;
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 28);
	buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 22) + (Units().getSupply() >= 24) + (Units().getSupply() >= 30);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 32) + (Units().getSupply() >= 46);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
	getOpening = Units().getSupply() < 46;
	forgeExpand = true;
	return;
}

void BuildOrderTrackerClass::FFEGateway()
{
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 46);
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 36;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 60;
	getOpening = Units().getSupply() < 60;
	forgeExpand = true;
	return;
}

void BuildOrderTrackerClass::FFENexus()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 36;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 60;
	getOpening = Units().getSupply() < 60;
	forgeExpand = true;
	return;
}

void BuildOrderTrackerClass::TwelveNexus()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 32);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 28;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 30;
	getOpening = Units().getSupply() < 32;
	return;
}

void BuildOrderTrackerClass::DTExpand()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 48);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 54);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 28;
	buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = Units().getSupply() >= 36;
	buildingDesired[UnitTypes::Protoss_Templar_Archives] = Units().getSupply() >= 48;
	getOpening = Units().getSupply() < 54;
	oneGateCore = true;
	return;
}

void BuildOrderTrackerClass::RoboExpand()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() > 42);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
	buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Units().getSupply() >= 56;
	buildingDesired[UnitTypes::Protoss_Robotics_Support_Bay] = Units().getSupply() >= 64;
	getOpening = Units().getSupply() < 64;
	oneGateCore = true;
	return;
}

void BuildOrderTrackerClass::FourGate()
{
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + 3 * (Units().getSupply() >= 62);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 34;
	getOpening = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) < 3;
	oneGateCore = true;
	return;
}

void BuildOrderTrackerClass::ZealotRush()
{
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 18) + (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) > 0);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 44;
	getOpening = Units().getSupply() < 44;
	return;
}