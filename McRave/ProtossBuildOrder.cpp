#include "McRave.h"

void BuildOrderTrackerClass::protossOpener()
{
	if (getOpening)
	{
		if (currentBuild == "PZZCore") PZZCore();
		if (currentBuild == "PZCore") PZCore();
		if (currentBuild == "PNZCore") PNZCore();
		if (currentBuild == "P4Gate") P4Gate();
		if (currentBuild == "PFFEStandard") PFFEStandard();
		if (currentBuild == "P12Nexus") P12Nexus();
		if (currentBuild == "P21Nexus") P21Nexus();
		if (currentBuild == "PDTExpand") PDTExpand();
		if (currentBuild == "P2GateDragoon") P2GateDragoon();
	}
	return;
}

void BuildOrderTrackerClass::protossTech()
{
	// Some hardcoded techs based on needing detection or specific build orders
	if (getTech)
	{
		if (Strategy().needDetection())
		{
			unlockedType.insert(UnitTypes::Protoss_Observer);
			techUnit = UnitTypes::Protoss_Observer;
			techList.insert(UnitTypes::Protoss_Observer);
			getTech = false;
		}
		else if (currentBuild == "PDTExpand" && techList.size() == 0)
		{
			techUnit = UnitTypes::Protoss_Arbiter;
			unlockedType.insert(UnitTypes::Protoss_Arbiter);
			unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
			techList.insert(UnitTypes::Protoss_Arbiter);
			techList.insert(UnitTypes::Protoss_Dark_Templar);
			getTech = false;
		}
		else if (Players().getPlayers().size() <= 1 && Players().getNumberZerg() > 0 && techList.size() == 0)
		{
			if (Strategy().getEnemyBuild() == "Z12HatchHydra" || Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk] >= Units().getEnemyComposition()[UnitTypes::Zerg_Mutalisk])
			{
				unlockedType.insert(UnitTypes::Protoss_High_Templar);
				techList.insert(UnitTypes::Protoss_High_Templar);
				techUnit = UnitTypes::Protoss_High_Templar;
				getTech = false;
			}
			else if (Strategy().getEnemyBuild() == "Z12HatchMuta" || Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk] < Units().getEnemyComposition()[UnitTypes::Zerg_Mutalisk])
			{
				unlockedType.insert(UnitTypes::Protoss_Corsair);
				unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
				techList.insert(UnitTypes::Protoss_Corsair);
				techUnit = UnitTypes::Protoss_Corsair;
				getTech = false;
			}
		}
		else if (Players().getPlayers().size() <= 1 && Players().getNumberProtoss() > 0 && techList.size() == 0)
		{
			if (Units().getEnemyComposition()[UnitTypes::Protoss_Dragoon] > Units().getEnemyComposition()[UnitTypes::Protoss_Zealot])
			{
				unlockedType.insert(UnitTypes::Protoss_Reaver);
				techList.insert(UnitTypes::Protoss_Reaver);
				techUnit = UnitTypes::Protoss_Reaver;
				getTech = false;
			}
			else
			{
				unlockedType.insert(UnitTypes::Protoss_Observer);				
				techList.insert(UnitTypes::Protoss_Observer);
				techUnit = UnitTypes::Protoss_Observer;
				getTech = false;
			}
		}

		// Otherwise, choose a tech based on highest unit score
		else if (techUnit == UnitTypes::None)
		{
			double highest = 0.0;
			for (auto &tech : Strategy().getUnitScore())
			{
				if (tech.first == UnitTypes::Protoss_Dragoon || tech.first == UnitTypes::Protoss_Zealot || tech.first == UnitTypes::Protoss_Shuttle) continue;
				if (tech.first == UnitTypes::Protoss_Corsair && techList.size() != 0 && Units().getGlobalEnemyAirStrength() == 0.0) continue;
				if (tech.second > highest)
				{
					highest = tech.second;
					techUnit = tech.first;
				}
			}

			// No longer need to choose a tech
			if (techUnit != UnitTypes::None)
			{
				getTech = false;
				techList.insert(techUnit);
			}
		}
	}

	if (techUnit == UnitTypes::Protoss_Observer)
	{
		unlockedType.insert(UnitTypes::Protoss_Observer);
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
		buildingDesired[UnitTypes::Protoss_Observatory] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility) > 0;
	}
	else if (techUnit == UnitTypes::Protoss_Reaver)
	{
		unlockedType.insert(UnitTypes::Protoss_Reaver);
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
		buildingDesired[UnitTypes::Protoss_Robotics_Support_Bay] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility) > 0;
	}
	else if (techUnit == UnitTypes::Protoss_Corsair)
	{
		unlockedType.insert(UnitTypes::Protoss_Corsair);
		unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
		buildingDesired[UnitTypes::Protoss_Stargate] = (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0) +(Units().getSupply() >= 200);
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair) > 0;
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0;
	}
	else if (techUnit == UnitTypes::Protoss_Scout)
	{
		unlockedType.insert(UnitTypes::Protoss_Scout);
		buildingDesired[UnitTypes::Protoss_Stargate] = 2;
		buildingDesired[UnitTypes::Protoss_Fleet_Beacon] = (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate) > 0);
	}
	else if (techUnit == UnitTypes::Protoss_Arbiter)
	{
		unlockedType.insert(UnitTypes::Protoss_Arbiter);
		unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = 1;
		buildingDesired[UnitTypes::Protoss_Stargate] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Templar_Archives) > 0;
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0;
		buildingDesired[UnitTypes::Protoss_Arbiter_Tribunal] = (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate) > 0);
	}
	else if (techUnit == UnitTypes::Protoss_High_Templar)
	{
		unlockedType.insert(UnitTypes::Protoss_High_Templar);
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0;
	}
	return;
}

void BuildOrderTrackerClass::protossSituational()
{
	satVal = Players().getNumberTerran() > 0 ? 2 : 3;
	gateVal = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway);
	baseVal = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);

	techVal = techList.size() + 1;
	productionSat = (gateVal >= satVal * baseVal);
	techSat = (techVal > baseVal);	

	//Broodwar->drawTextScreen(50, 200, "satVal: %d", satVal);
	//Broodwar->drawTextScreen(50, 216, "gateVal: %d", gateVal);
	//Broodwar->drawTextScreen(50, 232, "techVal: %d", techVal);
	//Broodwar->drawTextScreen(50, 248, "prodSat: %d", productionSat);
	//Broodwar->drawTextScreen(50, 264, "techSat: %d", techSat);
	//Broodwar->drawTextScreen(50, 280, "techUnit: %s", techUnit.c_str());

	if (techComplete()) techUnit = UnitTypes::None; // If we have our tech unit, set to none	
	if (Strategy().needDetection() || (!getOpening && !getTech && !techSat && productionSat && techUnit == UnitTypes::None && (!Production().hasIdleProduction() || Units().getSupply() > 380))) getTech = true; // If production is saturated and none are idle or we need detection, choose a tech

	// Check if we hit our Zealot cap based on our build
	if (!Strategy().isRush() && (((currentBuild == "PZZCore" || currentBuild == "PDTExpand" || Strategy().getEnemyBuild() == "Z12HatchMuta") && getOpening && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) >= 2) || (currentBuild == "PZCore" && getOpening && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) >= 1) || (getOpening && currentBuild == "PNZCore") || (Players().getNumberTerran() > 0 && currentBuild != "PDTExpand" && !Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) && !Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) <= 0)))
		unlockedType.erase(UnitTypes::Protoss_Zealot);
	else unlockedType.insert(UnitTypes::Protoss_Zealot);
	unlockedType.insert(UnitTypes::Protoss_Dragoon);
	unlockedType.insert(UnitTypes::Protoss_Shuttle);

	// Pylon logic
	if (Strategy().isAllyFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Pylon) <= 0)
		buildingDesired[UnitTypes::Protoss_Pylon] = Units().getSupply() >= 14;
	else buildingDesired[UnitTypes::Protoss_Pylon] = min(22, (int)floor((Units().getSupply() / max(15, (16 - Broodwar->self()->allUnitCount(UnitTypes::Protoss_Pylon))))));

	// Additional cannon for FFE logic (add on at most 2 at a time)
	if (forgeExpand && Units().getGlobalEnemyGroundStrength() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense())
	{
		reinforceWall = true;
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = min(2 + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon), 1 + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));
	}
	else
	{
		reinforceWall = false;
	}

	if (Strategy().getEnemyBuild() == "PFFE" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1)
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = 2;
	}

	// Shield battery logic
	if ((Players().getNumberTerran() == 0 && Strategy().isRush() && !Strategy().isAllyFastExpand()) || (oneGateCore && Players().getNumberZerg() > 0 && Units().getGlobalGroundStrategy() == 0))
	{
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core);
	}

	// If we're not in our opener
	if (!getOpening)
	{
		if (shouldExpand()) buildingDesired[UnitTypes::Protoss_Nexus] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) + 1;
		if (shouldAddProduction()) buildingDesired[UnitTypes::Protoss_Gateway] = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);
		if (shouldAddGas()) buildingDesired[UnitTypes::Protoss_Assimilator] = Resources().getTempGasCount();

		// Forge logic
		if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) >= 3 && Units().getSupply() > 200)
		{
			buildingDesired[UnitTypes::Protoss_Forge] = 2;
		}

		// Cannon logic
		if (!Strategy().isAllyFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge))
		{
			buildingDesired[UnitTypes::Protoss_Photon_Cannon] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon);
			for (auto &station : Stations().getMyStations())
			{
				if (Grids().getPylonGrid(station.BWEMBase()->Location()) == 0)
				{
					buildingDesired[UnitTypes::Protoss_Pylon] += 1;
				}
				else if (Grids().getDefenseGrid(station.BWEMBase()->Location()) <= 0 && Grids().getPylonGrid(station.BWEMBase()->Location()) > 0)
				{
					//buildingDesired[UnitTypes::Protoss_Photon_Cannon] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1;
				}
			}
		}
	}
	return;
}

void BuildOrderTrackerClass::PvPSituational()
{

}