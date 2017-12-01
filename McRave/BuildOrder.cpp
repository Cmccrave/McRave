#include "McRave.h"
#include <fstream>

void BuildOrderTrackerClass::onEnd(bool isWinner)
{
	ofstream config("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");
	string t1;
	int t2, t3;
	int lineBreak = 0;

	while (ss >> t1 >> t2 >> t3)
	{
		t1 == currentBuild ? (isWinner ? t2++ : t3++) : 0;
		config << t1 << " " << t2 << " " << t3 << endl;
	}
	return;
}

void BuildOrderTrackerClass::onStart()
{
	string build, buffer;
	ifstream config("bwapi-data/read/" + Broodwar->enemy()->getName() + ".txt");
	int wins, losses, gamesPlayed, totalGamesPlayed = 0;
	double best = 0.0;

	// Write what builds you're using
	if (Broodwar->self()->getRace() == Races::Protoss) buildNames = { "PZZCore", "PZCore", "PNZCore", "PFFESafe", "PFFEStandard",/* "PFFEGreedy",*/ "P12Nexus", "P21Nexus", "PDTExpand", "P4Gate", "P2GateZealot", "P2GateDragoon"/*, "PDTRush", "PReaverRush"*/ };
	if (Broodwar->self()->getRace() == Races::Terran) buildNames = { "T2Fact", "TSparks" };
	if (Broodwar->self()->getRace() == Races::Zerg) buildNames = { "ZOverpool" };

	// If we don't have a file in the /read/ folder, then check the /write/ folder
	if (!config)
	{
		ifstream localConfig("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");

		// If still no file, then we need to create one and add all builds to it that we're using
		if (!localConfig.good())
		{
			for (auto &build : buildNames)
			{
				ss << build << " 0 0 ";
			}
		}

		// If there is a file, load it into the stringstream
		else
		{
			while (localConfig >> buffer)
			{
				ss << buffer << " ";
			}
		}
	}

	// If we do have a file, load it into the stringstream
	else
	{
		while (config >> buffer)
		{
			ss << buffer << " ";
		}
	}

	// Check SS if the build exists, if it doesn't, add it
	string s = ss.str();
	for (auto &build : buildNames)
	{
		if (s.find(build) == s.npos)
		{
			ss << build << " 0 0 ";
		}
	}

	// Calculate how many games we've played against this opponent
	stringstream ss2;
	ss2 << ss.str();
	while (!ss2.eof())
	{
		ss2 >> build >> wins >> losses;
		totalGamesPlayed += wins + losses;
	}

	if (totalGamesPlayed == 0)
	{
		getDefaultBuild();
		return;
	}

	// Calculate which build is best
	stringstream ss3;
	ss3 << ss.str();
	while (!ss3.eof())
	{
		ss3 >> build >> wins >> losses;
		gamesPlayed = wins + losses;

		// Against static easy bots to beat, just stick with whatever is working (prevents uncessary losses)
		if (gamesPlayed > 0 && losses == 0)
		{
			currentBuild = build;
			return;
		}

		if ((Players().getNumberProtoss() > 0 && isBuildAllowed(Races::Protoss, build)) || (Players().getNumberTerran() > 0 && isBuildAllowed(Races::Terran, build)) || (Players().getNumberZerg() > 0 && isBuildAllowed(Races::Zerg, build)) || (Players().getNumberRandom() > 0 && isBuildAllowed(Races::Random, build)))
		{
			if (gamesPlayed <= 0)
			{
				currentBuild = build;
				return;
			}
			else
			{
				double winRate = gamesPlayed > 0 ? wins / static_cast<double>(gamesPlayed) : 0;
				double ucbVal = 0.5 * sqrt(log((double)totalGamesPlayed) / gamesPlayed);
				double val = winRate + ucbVal;
				if (val >= best)
				{
					best = val;
					currentBuild = build;
				}
			}
		}
	}
	return;
}

bool BuildOrderTrackerClass::isBuildAllowed(Race enemy, string build)
{
	if (enemy == Races::Zerg && (build == "PFFESafe" || build == "PFFEStandard" /*|| build == "PFFEGreedy" */ || build == "P2GateZealot" || build == "P4Gate")) return true;
	if (enemy == Races::Terran && (build == "P12Nexus" || build == "P21Nexus" || build == "PDTExpand" || build == "P2GateDragoon")) return true;
	if (enemy == Races::Protoss && (build == "PZCore" || build == "PNZCore")) return true;
	if (enemy == Races::Random && (build == "PZZCore")) return true;
	return false;
}

void BuildOrderTrackerClass::getDefaultBuild()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Players().getNumberProtoss() > 0) currentBuild = "PZCore";
		else if (Players().getNumberZerg() > 0) currentBuild = "P4Gate";
		else if (Players().getNumberTerran() > 0) currentBuild = "P12Nexus";
		else if (Players().getNumberRandom() > 0) currentBuild = "PZZCore";
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		currentBuild = "T2Fact";
	}
	else if (Broodwar->self()->getRace() == Races::Zerg)
	{
		currentBuild = "ZOverpool";
	}
	return;
}

void BuildOrderTrackerClass::update()
{
	Display().startClock();
	updateBuild();
	Display().performanceTest(__FUNCTION__);
	return;
}

void BuildOrderTrackerClass::updateBuild()
{
	// Protoss
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		protossOpener();
		protossSituational();
		protossTech();
	}

	// Terran
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		terranOpener();
		terranSituational();
		terranTech();
	}

	// Zerg
	else if (Broodwar->self()->getRace() == Races::Zerg)
	{
		zergOpener();
		zergTech();
		zergSituational();
	}
	return;
}

void BuildOrderTrackerClass::protossOpener()
{
	if (getOpening)
	{
		if (currentBuild == "PZZCore") PZZCore();
		if (currentBuild == "PZCore") PZCore();
		if (currentBuild == "PNZCore") PNZCore();
		if (currentBuild == "PFFESafe") PFFESafe();
		if (currentBuild == "PFFEStandard") PFFEStandard();
		//if (currentBuild == "PFFEGreedy") PFFEGreedy();
		if (currentBuild == "P12Nexus") P12Nexus();
		if (currentBuild == "P21Nexus") P21Nexus();
		if (currentBuild == "PDTExpand") PDTExpand();
		if (currentBuild == "P4Gate") P4Gate();
		if (currentBuild == "P2GateZealot") P2GateZealot();
		if (currentBuild == "P2GateDragoon") P2GateDragoon();
		//if (currentBuild == "PDTRush") PDTRush();
		//if (currentBuild == "PReaverRush") PReaverRush();
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
			techUnit = UnitTypes::Protoss_Observer;
			unlockedType.insert(techUnit);
			techList.insert(techUnit);
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
		else if (currentBuild == "P21Nexus" && techList.size() == 0)
		{
			Strategy().getUnitScore()[UnitTypes::Protoss_Reaver] > Strategy().getUnitScore()[UnitTypes::Protoss_Observer] ? techUnit = UnitTypes::Protoss_Reaver : techUnit = UnitTypes::Protoss_Observer;
			unlockedType.insert(techUnit);
			techList.insert(techUnit);
			getTech = false;
		}

		// Otherwise, choose a tech based on highest unit score
		else if (techUnit == UnitTypes::None)
		{
			double highest = 0.0;
			for (auto &tech : Strategy().getUnitScore())
			{
				if (tech.first == UnitTypes::Protoss_Dragoon || tech.first == UnitTypes::Protoss_Zealot)
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
	}

	if (techUnit == UnitTypes::Protoss_Observer)
	{
		unlockedType.insert(UnitTypes::Protoss_Observer);
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = 1;
		buildingDesired[UnitTypes::Protoss_Observatory] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility) > 0;
	}
	else if (techUnit == UnitTypes::Protoss_Reaver)
	{
		unlockedType.insert(UnitTypes::Protoss_Reaver);
		unlockedType.insert(UnitTypes::Protoss_Observer);
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = 1;
		buildingDesired[UnitTypes::Protoss_Observatory] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility) > 0;
		buildingDesired[UnitTypes::Protoss_Robotics_Support_Bay] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer) > 0;

	}
	else if (techUnit == UnitTypes::Protoss_Corsair)
	{
		unlockedType.insert(UnitTypes::Protoss_Corsair);
		unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
		buildingDesired[UnitTypes::Protoss_Stargate] = 1 + (Units().getSupply() >= 200);
		buildingDesired[UnitTypes::Protoss_Stargate] = 1;
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate) > 0;
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0;
	}
	else if (techUnit == UnitTypes::Protoss_Scout)
	{
		unlockedType.insert(UnitTypes::Protoss_Scout);
		buildingDesired[UnitTypes::Protoss_Stargate] = 1 + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) > 0;
		buildingDesired[UnitTypes::Protoss_Fleet_Beacon] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate) > 0;
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
		buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = 1;
		buildingDesired[UnitTypes::Protoss_Templar_Archives] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0;
	}
	return;
}

void BuildOrderTrackerClass::protossSituational()
{
	int sat = Players().getNumberTerran() > 0 ? 2 : 3;
	bool techSat = techList.size() >= Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
	bool productionSat = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= sat * techList.size();

	if (Broodwar->self()->visibleUnitCount(techUnit) > 0) techUnit = UnitTypes::None; // If we have our tech unit, set to none	
	if (Strategy().needDetection() || (!getOpening && !getTech && productionSat && techUnit == UnitTypes::None && (!Production().hasIdleProduction() || Units().getSupply() > 380))) getTech = true; // If production is saturated and none are idle or we need detection, choose a tech

	// Check if we hit our Zealot cap based on our build
	if (!Strategy().isRush() && (((currentBuild == "PZZCore" || currentBuild == "PDTExpand") && getOpening &&  Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) >= 2) || (currentBuild == "PZCore" && getOpening &&  Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) >= 1) || (getOpening && currentBuild == "PNZCore") || (Players().getNumberTerran() > 0 && currentBuild != "PDTExpand" && !Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) && !Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) <= 0)))
		unlockedType.erase(UnitTypes::Protoss_Zealot);
	else unlockedType.insert(UnitTypes::Protoss_Zealot);
	unlockedType.insert(UnitTypes::Protoss_Dragoon);

	// Pylon logic
	if (Strategy().isAllyFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Pylon) <= 0)
		buildingDesired[UnitTypes::Protoss_Pylon] = Units().getSupply() >= 14;
	else buildingDesired[UnitTypes::Protoss_Pylon] = min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Protoss_Pylon))))));

	// Additional cannon for FFE logic (add on at most 2 at a time)
	if (forgeExpand && Units().getGlobalEnemyGroundStrength() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense())
	{
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = min(2 + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon), 1 + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));
	}

	// Shield battery logic
	if ((Players().getNumberTerran() == 0 && Strategy().isRush() && !Strategy().isAllyFastExpand()) || (oneGateCore && Players().getNumberZerg() > 0 && Units().getGlobalGroundStrategy() == 0))
	{
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core);
	}

	// If we're not in our opener
	if (!getOpening)
	{
		// Expansion logic
		if (Broodwar->self()->minerals() > 500 + 100 * Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) || (productionSat && Resources().isMinSaturated() && !Production().hasIdleProduction()))
		{
			buildingDesired[UnitTypes::Protoss_Nexus] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) + 1;
		}

		// Gateway logic
		if (!Production().hasIdleProduction() && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 150) || (!productionSat && Resources().isMinSaturated())))
		{
			buildingDesired[UnitTypes::Protoss_Gateway] = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);
		}

		// Assimilator logic
		if (!Strategy().isPlayPassive() && Resources().isGasSaturated() && Broodwar->self()->gas() < Broodwar->self()->minerals() * 3 && Broodwar->self()->minerals() > 100)
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
				if (Grids().getPylonGrid(base.second.getTilePosition()) == 0)
				{
					buildingDesired[UnitTypes::Protoss_Pylon] += 1;
				}
				else if (Grids().getDefenseGrid(base.second.getTilePosition()) <= 0 && Grids().getPylonGrid(base.second.getTilePosition()) > 0)
				{
					buildingDesired[UnitTypes::Protoss_Photon_Cannon] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1;
				}
			}
		}
	}
	return;
}

void BuildOrderTrackerClass::terranOpener()
{
	if (getOpening)
	{
		if (currentBuild == "T2Fact") T2Fact();
		if (currentBuild == "TSparks") TSparks();
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
	bool productionSat = (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= min(12, (3 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))));

	if (!BuildOrder().isBioBuild() && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) >= 4)
	{
		unlockedType.erase(UnitTypes::Terran_Marine);
	}

	if (!BuildOrder().isBioBuild())
	{
		unlockedType.erase(UnitTypes::Terran_Medic);
		unlockedType.erase(UnitTypes::Terran_Firebat);
	}
	else
	{
		unlockedType.insert(UnitTypes::Terran_Medic);
		unlockedType.insert(UnitTypes::Terran_Firebat);
	}

	unlockedType.insert(UnitTypes::Terran_Vulture);
	unlockedType.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);
	unlockedType.insert(UnitTypes::Terran_Goliath);

	// Supply Depot logic
	buildingDesired[UnitTypes::Terran_Supply_Depot] = min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Terran_Supply_Depot))))));

	// Expansion logic
	if (Units().getGlobalAllyGroundStrength() > Units().getGlobalEnemyGroundStrength() && Resources().isMinSaturated() && productionSat && Production().getIdleProduction().size() == 0)
	{
		buildingDesired[UnitTypes::Terran_Command_Center] = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) + 1;
	}

	// Bunker logic
	if (Strategy().isRush())
	{
		buildingDesired[UnitTypes::Terran_Bunker] = 1;
	}

	// Refinery logic
	if (!Strategy().isPlayPassive() && Resources().isGasSaturated() && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) == buildingDesired[UnitTypes::Terran_Command_Center] && Broodwar->self()->gas() < Broodwar->self()->minerals() * 5 && Broodwar->self()->minerals() > 100)
	{
		buildingDesired[UnitTypes::Terran_Refinery] = Resources().getTempGasCount();
	}

	// Armory logic - TODO find a better solution to this garbage
	if (Strategy().getUnitScore()[UnitTypes::Terran_Goliath] > 1.0)
	{
		buildingDesired[UnitTypes::Terran_Armory] = 1;
	}

	// Academy logic
	if (Strategy().needDetection())
	{
		buildingDesired[UnitTypes::Terran_Academy] = 1;
	}

	// Barracks logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= 3 && (Production().getIdleProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200) || (!productionSat && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Barracks] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) + 1);
	}

	// Factory logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2 && (Production().getIdleProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200 && Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 100) || (!productionSat && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Factory] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Factory) + 1);
	}

	// Machine Shop logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 1)
	{
		buildingDesired[UnitTypes::Terran_Machine_Shop] = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) - (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center));
	}
}

void BuildOrderTrackerClass::zergOpener()
{
	if (getOpening)
	{
		if (currentBuild == "ZOverpool") ZOverpool();
	}
}

void BuildOrderTrackerClass::zergTech()
{

}

void BuildOrderTrackerClass::zergSituational()
{

}