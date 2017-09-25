#include "McRave.h"
#include <fstream>

void BuildOrderTrackerClass::recordWinningBuild(bool isWinner)
{
	ofstream config("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt", ios::trunc);
	int x = (opening*2) + !isWinner;
	if (config)
	{
		for (int i = 0; i < 6; i++)
		{
			if (i == x)
			{
				config << configStuff[i] + 1 << " ";
			}
			else
			{
				config << configStuff[i] << " ";
			}			
		}
	}
}

void BuildOrderTrackerClass::loadConfig()
{
	string line;
	int x;
	ifstream config("bwapi-data/read/" + Broodwar->enemy()->getName() + ".txt");

	if (config)
	{
		while (config >> x)
		{
			configStuff.push_back(x);
		}
	}
	else
	{
		config.open("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");
		if (config)
		{
			while (config >> x)
			{
				configStuff.push_back(x);
			}
		}
	}

	if (configStuff.size() == 0)
	{
		for (int i = 0; i < 6; i++)
		{
			configStuff.push_back(0);
		}
	}

	// Learning choice
	if (!learnedOpener)
	{
		learnedOpener = true;
		double w1 = 0.0, w2 = 0.0;
		int i = 0;
		int totalGamesPlayed = 0;
		double best = 0.0;

		for (int i = 0; i < 6; i++)
		{
			totalGamesPlayed += configStuff.at(i);
		}

		for (int i = 0; i < 3; i++)
		{
			// If we never played this strategy, try it first
			if (configStuff.at(0 + (i * 2)) + configStuff.at(1 + (i * 2)) == 0)
			{
				opening = i;
				return;
			}

			int gamesPlayed = configStuff.at(0 + (i * 2)) + configStuff.at(1 + (i * 2));
			double winRate = gamesPlayed > 0 ? configStuff.at(0 + (i * 2)) / static_cast<double>(gamesPlayed) : 0;
			double ucbVal = 0.5 * sqrt(log((double)totalGamesPlayed / gamesPlayed));
			double val = winRate + ucbVal;

			if (val > best)
			{
				best = val;
				opening = i;
			}
		}
	}

	config.close();
	return;
}

void BuildOrderTrackerClass::update()
{
	Display().startClock();
	updateDecision();
	updateBuild();
	Display().performanceTest(__FUNCTION__);
	return;
}

void BuildOrderTrackerClass::updateDecision()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		// If we have a Core and 2 Gates, opener is done
		if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) >= 1 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= 2 && getOpening)
		{
			getOpening = false;
		}

		// If we have our tech unit, set to none
		if (Broodwar->self()->completedUnitCount(techUnit) > 0)
		{
			techUnit = UnitTypes::None;
		}

		// If production is saturated and none are idle or we need detection for some invis units, choose a tech
		if (Strategy().needDetection() || (!Strategy().isPlayPassive() && Units().getGlobalAllyStrength() > Units().getGlobalEnemyStrength() && !getOpening && !getTech && techUnit == UnitTypes::None && Production().getIdleLowProduction().size() == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= 2))
		{
			getTech = true;
		}		
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		// If we have an Academy, opener is done
		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Academy) >= 1)
		{
			getOpening = false;
		}

		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) >= 2)
		{
			getTech = true;
		}

		// Only one opener for now
		opening = 2;
	}
	return;
}

void BuildOrderTrackerClass::updateBuild()
{
	// Protoss
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		protossSituational();
		protossTech();
		protossOpener();
	}

	// Terran
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		terranSituational();
		terranTech();
		terranOpener();
	}

	// Zerg
	else if (Broodwar->self()->getRace() == Races::Zerg)
	{
		zergSituational();
		zergTech();
		zergOpener();
	}
}

void BuildOrderTrackerClass::protossOpener()
{
	if (getOpening && Players().getNumberZerg() > 0)
	{
		// Safe - Cannons First
		if (opening == 0)
		{
			FFECannon();
		}

		// Normal - Gate First
		if (opening == 1)
		{
			FFEGateway();
		}

		// Greedy - Nexus First
		if (opening == 2)
		{
			FFENexus();
		}
	}
	else if (getOpening && Players().getNumberTerran() > 0)
	{
		// Safe - DT FE
		if (opening == 0)
		{
			DTExpand();			
		}

		// Normal - NZCore
		if (opening == 1)
		{
			NZCore();
		}

		// Greedy - 12 Nexus
		if (opening == 2)
		{
			TwelveNexus();
		}
	}

	else if (getOpening && Players().getNumberProtoss() > 0)
	{
		// Safe
		if (opening == 0)
		{
			ZZCore();
		}

		// Normal
		if (opening == 1)
		{
			NZCore();
		}

		// Greedy
		if (opening == 2)
		{
			TwelveNexus();
		}
	}

	else if (getOpening && Players().getNumberRandom() > 0)
	{
		// Safe
		if (opening == 0)
		{
			ZZCore();
		}

		// Normal
		if (opening == 1)
		{
			ZCore();
		}

		// Greedy
		if (opening == 2)
		{
			NZCore();
		}
	}
	return;
}

void BuildOrderTrackerClass::protossTech()
{
	if (getTech && techUnit == UnitTypes::None)
	{
		double highest = 0.0;
		for (auto &tech : Strategy().getUnitScore())
		{
			if (tech.first == UnitTypes::Protoss_Dragoon || tech.first == UnitTypes::Protoss_Zealot || techList.find(tech.first) != techList.end())
			{
				continue;
			}
			if (tech.second > highest)
			{
				highest = tech.second;
				techUnit = tech.first;
			}
		}

		// No longer need to choose a tech
		getTech = false;
		techList.insert(techUnit);
	}
	if (techUnit == UnitTypes::Protoss_Reaver)
	{
		if (Strategy().needDetection())
		{
			buildingDesired[UnitTypes::Protoss_Robotics_Facility] = 1;
			buildingDesired[UnitTypes::Protoss_Observatory] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility));
			buildingDesired[UnitTypes::Protoss_Robotics_Support_Bay] = min(1, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer));
		}
		else
		{
			buildingDesired[UnitTypes::Protoss_Robotics_Facility] = 1;
			buildingDesired[UnitTypes::Protoss_Robotics_Support_Bay] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility));
			buildingDesired[UnitTypes::Protoss_Observatory] = min(1, buildingDesired[UnitTypes::Protoss_Observatory] + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver));
		}
	}
	else if (techUnit == UnitTypes::Protoss_Corsair)
	{
		buildingDesired[UnitTypes::Protoss_Stargate] = 1;
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate));
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun));
	}
	else if (techUnit == UnitTypes::Protoss_Scout)
	{
		buildingDesired[UnitTypes::Protoss_Stargate] = min(2, 1 + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate));
		buildingDesired[UnitTypes::Protoss_Fleet_Beacon] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate));
	}
	else if (techUnit == UnitTypes::Protoss_Arbiter)
	{
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = 1;
		buildingDesired[UnitTypes::Protoss_Stargate] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun);
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun);
		buildingDesired[UnitTypes::Protoss_Arbiter_Tribunal] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives);
	}
	else if (techUnit == UnitTypes::Protoss_High_Templar)
	{
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = 1;
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun));
	}
	return;
}

void BuildOrderTrackerClass::protossSituational()
{
	// Pylon logic
	if (Strategy().isAllyFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Pylon) <= 0)
	{
		buildingDesired[UnitTypes::Protoss_Pylon] = Units().getSupply() >= 14;
	}
	else
	{
		buildingDesired[UnitTypes::Protoss_Pylon] = min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Protoss_Pylon))))));
	}

	// Expansion logic
	if (Units().getGlobalAllyStrength() > Units().getGlobalEnemyStrength() && (Resources().isMinSaturated() && Production().isGateSat() && Production().getIdleLowProduction().size() == 0) || (Strategy().isAllyFastExpand() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1))
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) + 1;
	}

	// Shield battery logic
	if (Players().getNumberTerran() == 0)
	{
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = Strategy().isRush() * Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core);
	}

	// Gateway logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= 2 && (Production().getIdleLowProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 150) || (!Production().isGateSat() && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Protoss_Gateway] = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);
	}

	// Assimilator logic
	if (!Strategy().isPlayPassive() && Resources().isGasSaturated() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) == buildingDesired[UnitTypes::Protoss_Nexus] && Broodwar->self()->gas() < Broodwar->self()->minerals() * 5 && Broodwar->self()->minerals() > 100)
	{
		buildingDesired[UnitTypes::Protoss_Assimilator] = Resources().getTempGasCount();
	}

	// Forge logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) >= 3 && Units().getSupply() > 200)
	{
		buildingDesired[UnitTypes::Protoss_Forge] = 1;
	}

	// Cannon logic
	if (!Strategy().isAllyFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge))
	{
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon);
		for (auto &base : Bases().getMyBases())
		{
			if (base.second.unit()->isCompleted() && Grids().getDefenseGrid(base.second.getTilePosition()) < 1 && Broodwar->hasPower(TilePosition(base.second.getPosition())))
			{
				buildingDesired[UnitTypes::Protoss_Photon_Cannon] += 1 - Grids().getDefenseGrid(base.second.getTilePosition());
			}
		}
	}

	// Additional cannon for FFE logic (add on at most 2 at a time)
	if (Strategy().isAllyFastExpand() && Units().getGlobalEnemyStrength() > Units().getGlobalAllyStrength() + Units().getAllyDefense())
	{
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = min(2 + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon), 1 + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));
	}
}

void BuildOrderTrackerClass::terranOpener()
{
	if (getOpening)
	{
		// Joyo
		if (opening == 1)
		{
			buildingDesired[UnitTypes::Terran_Barracks] = (Units().getSupply() >= 22) + (Units().getSupply() >= 26) + (Units().getSupply() >= 42);
			buildingDesired[UnitTypes::Terran_Engineering_Bay] = (Units().getSupply() >= 36);
			buildingDesired[UnitTypes::Terran_Refinery] = (Units().getSupply() >= 40);
			buildingDesired[UnitTypes::Terran_Academy] = (Units().getSupply() >= 48);
		}
		// 2 Fact Vulture
		if (opening == 2)
		{
			buildingDesired[UnitTypes::Terran_Barracks] = (Units().getSupply() >= 22);
			buildingDesired[UnitTypes::Terran_Refinery] = (Units().getSupply() >= 22);
			buildingDesired[UnitTypes::Terran_Factory] = (Units().getSupply() >= 30) + (Units().getSupply() >= 36);
		}
		// 2 Port Wraith
		if (opening == 3)
		{

		}
	}
	return;
}

void BuildOrderTrackerClass::terranTech()
{
	// If we have a Core and 2 Gates, opener is done
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2 && getOpening)
	{
		// Put opener function here instead
		getOpening = false;
	}
}

void BuildOrderTrackerClass::terranSituational()
{
	// Supply Depot logic
	buildingDesired[UnitTypes::Terran_Supply_Depot] = min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Terran_Supply_Depot))))));

	// Expansion logic
	if (Units().getGlobalAllyStrength() > Units().getGlobalEnemyStrength() && (Resources().isMinSaturated() && Production().getIdleLowProduction().size() == 0) || (Strategy().isAllyFastExpand() && Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center) == 1))
	{
		buildingDesired[UnitTypes::Terran_Command_Center] = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) + 1;
	}

	// Bunker logic
	if (Strategy().isRush())
	{
		buildingDesired[UnitTypes::Terran_Bunker] = 1;
	}

	// Refinery logic
	if (Resources().isMinSaturated())
	{
		buildingDesired[UnitTypes::Terran_Refinery] = Resources().getTempGasCount();
	}

	// Barracks logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= 3 && (Production().getIdleLowProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200) || (!Production().isBarracksSat() && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Barracks] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) + 1);
	}

	// Factory logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2 && (Production().getIdleLowProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200 && Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 100) || (!Production().isProductionSat() && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Factory] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Factory) + 1);
	}

	// CC logic
	if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs))
	{
		buildingDesired[UnitTypes::Terran_Command_Center] = 2;
	}
}

void BuildOrderTrackerClass::zergOpener()
{

}

void BuildOrderTrackerClass::zergTech()
{

}

void BuildOrderTrackerClass::zergSituational()
{

}

void BuildOrderTrackerClass::ZZCore()
{
	currentBuild = "ZZCore";
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 44);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 32;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 40;
	return;
}

void BuildOrderTrackerClass::ZCore()
{
	currentBuild = "ZCore";
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 44);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 34;
	return;
}

void BuildOrderTrackerClass::NZCore()
{
	currentBuild = "NZCore";
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 44);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 26;
	return;
}

void BuildOrderTrackerClass::FFECannon()
{
	currentBuild = "FFEC";
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 18;
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 28);
	buildingDesired[UnitTypes::Protoss_Photon_Cannon] = (Units().getSupply() >= 22) + (Units().getSupply() >= 24) + (Units().getSupply() >= 30);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 32) + (Units().getSupply() >= 46);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 40;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 42;
	return;
}

void BuildOrderTrackerClass::FFEGateway()
{
	currentBuild = "FFEG";
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 46);
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 50;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 56;
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 60;
	return;
}

void BuildOrderTrackerClass::FFENexus()
{
	currentBuild = "FFEN";
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 28) + (Units().getSupply() >= 42);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 50;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 56;
	buildingDesired[UnitTypes::Protoss_Forge] = Units().getSupply() >= 60;
	return;
}

void BuildOrderTrackerClass::TwelveNexus()
{
	currentBuild = "12Nexus";
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 24);
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 26) + (Units().getSupply() >= 32);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 28;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 30;
	return;
}

void BuildOrderTrackerClass::DTExpand()
{
	currentBuild = "DTExpand";
	buildingDesired[UnitTypes::Protoss_Nexus] = 1;
	buildingDesired[UnitTypes::Protoss_Gateway] = (Units().getSupply() >= 20) + (Units().getSupply() >= 54);
	buildingDesired[UnitTypes::Protoss_Assimilator] = Units().getSupply() >= 24;
	buildingDesired[UnitTypes::Protoss_Cybernetics_Core] = Units().getSupply() >= 28;
	buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = Units().getSupply() >= 36;
	buildingDesired[UnitTypes::Protoss_Templar_Archives] = Units().getSupply() >= 48;
	buildingDesired[UnitTypes::Protoss_Nexus] = 1 + (Units().getSupply() >= 48);
	return;
}