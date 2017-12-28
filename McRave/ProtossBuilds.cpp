#include "McRave.h"

void BuildOrderTrackerClass::P4Gate()
{
	if (Broodwar->enemy()->getRace() == Races::Zerg)
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1;
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 24) + (Units().getSupply() >= 62) + (Units().getSupply() >= 70);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 42;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 48;
		getOpening = Units().getSupply() < 120;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) > 0;
	}
	else
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1;
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + 3 * (Units().getSupply() >= 62);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 34;
		getOpening = Units().getSupply() < 120;
		oneGateCore = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) > 0;
	}
}

void BuildOrderTrackerClass::PFFEStandard()
{
	if ((Strategy().getEnemyBuild() == "Unknown" && !Terrain().getEnemyStartingPosition().isValid()) || Strategy().getEnemyBuild() == "Z9Pool") // Cannons first against early lings or no scouting information
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 20;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 32) + (Units().getSupply() >= 46);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 68);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	else if (Strategy().getEnemyBuild() == "Z5Pool" || Strategy().isRush())
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 18;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = 2*(Units().getSupply() >= 18) + (Broodwar->self()->minerals() >= 125);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 46);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 68);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	else if (Strategy().getEnemyBuild() == "Z3HatchLing")
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 20;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 28);
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) + (Units().getSupply() >= 24) + (Units().getSupply() >= 26);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 32) + (Units().getSupply() >= 46);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 68);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	else if (Strategy().getEnemyBuild() == "None" && Terrain().getEnemyStartingPosition().isValid() && Strategy().isEnemyFastExpand())
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Forge] = (Units().getSupply() >= 28);
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 30);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 68);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	else if (Strategy().getEnemyBuild() == "Z12HatchHydra")
	{
		firstUpgrade = UpgradeTypes::None;
		firstTech = TechTypes::Psionic_Storm;
		unlockedType.insert(UnitTypes::Protoss_High_Templar);
		techList.insert(UnitTypes::Protoss_High_Templar);
		techUnit = UnitTypes::Protoss_High_Templar;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Forge] = (Units().getSupply() >= 28);
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 30) + 3*(Units().getSupply() > 44);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 68);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = (Units().getSupply() >= 42);
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	else if (Strategy().getEnemyBuild() == "Z12HatchMuta")
	{
		firstUpgrade = UpgradeTypes::Protoss_Air_Weapons;
		firstTech = TechTypes::None;
		unlockedType.insert(UnitTypes::Protoss_Corsair);
		techList.insert(UnitTypes::Protoss_Corsair);
		techUnit = UnitTypes::Protoss_Corsair;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26);
		buildingDesired[UnitTypes::Protoss_Forge] = (Units().getSupply() >= 28);
		
		if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) > 0)
		{
			buildingDesired[UnitTypes::Protoss_Photon_Cannon] = 2 + (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) >= 2) + (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) >= 3) + (Units().getSupply() >= 74);
		}
		
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 60);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	else
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 50);
		buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 24;
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 30);
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 38) + (Units().getSupply() >= 68);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 44;
		getOpening = Units().getSupply() < 80;
		forgeExpand = true;
	}
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::P12Nexus()
{
	if (Strategy().isRush())
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1;
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 36);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		getOpening = Units().getSupply() < 60;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else if (Strategy().isEnemyFastExpand())
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
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
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core)) + (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) > 1) + (Units().getSupply() >= 70) + (Units().getSupply() >= 82);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 30) + (Units().getSupply() >= 80);
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = (Units().getSupply() >= 80);
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
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42) + (Units().getSupply() >= 70);
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 44) + (Units().getSupply() >= 72);
		buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 24) + (Units().getSupply() >= 76);
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Units().getSupply() >= 80;
		getOpening = Units().getSupply() < 100;
		nexusFirst = true;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
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
	firstUpgrade = UpgradeTypes::Khaydarin_Core;
	firstTech = TechTypes::None;
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
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
		buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 30);
		buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 22;
		buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
		getOpening = Units().getSupply() < 50;
		scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
	}
	else
	{
		firstUpgrade = UpgradeTypes::Singularity_Charge;
		firstTech = TechTypes::None;
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
	firstUpgrade = UpgradeTypes::Singularity_Charge;
	firstTech = TechTypes::None;
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
	firstUpgrade = UpgradeTypes::Singularity_Charge;
	firstTech = TechTypes::None;
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
	firstUpgrade = UpgradeTypes::Singularity_Charge;
	firstTech = TechTypes::None;
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 38);
	buildingDesired[UnitTypes::Protoss_Assimilator] = (Units().getSupply() >= 24);
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = (Units().getSupply() >= 34);
	getOpening = Units().getSupply() < 60;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

void BuildOrderTrackerClass::PZZCore()
{
	firstUpgrade = UpgradeTypes::Singularity_Charge;
	firstTech = TechTypes::None;
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
	getOpening = Units().getSupply() < 60;
	oneGateCore = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 0;
}

// Retired builds for now

//void BuildOrderTrackerClass::P2GateZealot()
//{
//	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 24);
//	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 36;
//	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 44;
//	getOpening = Units().getSupply() < 50;
//}