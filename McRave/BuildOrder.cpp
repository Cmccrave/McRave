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
	if (Broodwar->self()->getRace() == Races::Protoss) buildNames = { "ZZCore", "ZCore", "NZCore", "FFECannon", "FFEGateway", "FFENexus", "TwelveNexus", "DTExpand", "RoboExpand", "FourGate", "ZealotRush" };
	if (Broodwar->self()->getRace() == Races::Terran) buildNames = { "TwoFactVult", "Sparks" };

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
				if (val > best)
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
	if (enemy == Races::Zerg || enemy == Races::Random)
	{
		if (build == "TwelveNexus" || build == "NZCore" || build == "DTExpand")
		{
			return false;
		}
	}
	if (enemy == Races::Terran || enemy == Races::Protoss || enemy == Races::Random)
	{
		if (build == "FFECannon" || build == "FFEGateway" || build == "FFENexus")
		{
			return false;
		}
	}
	return true;
}

void BuildOrderTrackerClass::getDefaultBuild()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Players().getNumberProtoss() > 0)
		{
			currentBuild = "ZZCore";
		}
		else if (Players().getNumberZerg() > 0)
		{
			currentBuild = "FFECannon";
		}
		else if (Players().getNumberTerran() > 0)
		{
			currentBuild = "NZCore";
		}
		else if (Players().getNumberRandom() > 0)
		{
			currentBuild = "ZZCore";
		}
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		currentBuild = "TwoFactVult";
	}
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
	// If we have our tech unit, set to none
	if (Broodwar->self()->completedUnitCount(techUnit) > 0)
	{
		techList.insert(techUnit);
		techUnit = UnitTypes::None;
	}

	// If production is saturated and none are idle or we need detection for some invis units, choose a tech
	if (Strategy().needDetection() || (!getOpening && !getTech && techUnit == UnitTypes::None && Production().getIdleLowProduction().size() == 0 && Production().isProductionSat()))
	{
		getTech = true;
	}
	return;
}

void BuildOrderTrackerClass::updateBuild()
{
	// Protoss
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		protossOpener();
		protossTech();
		protossSituational();
	}

	// Terran
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		terranOpener();
		terranTech();
		terranSituational();
	}

	// Zerg
	else if (Broodwar->self()->getRace() == Races::Zerg)
	{
		zergOpener();
		zergTech();
		zergSituational();
	}
}

void BuildOrderTrackerClass::protossOpener()
{
	if (getOpening)
	{
		if (currentBuild == "ZZCore") ZZCore();
		if (currentBuild == "ZCore") ZCore();
		if (currentBuild == "NZCore") NZCore();
		if (currentBuild == "FFECannon") FFECannon();
		if (currentBuild == "FFEGateway") FFEGateway();
		if (currentBuild == "FFENexus") FFENexus();
		if (currentBuild == "TwelveNexus") TwelveNexus();
		if (currentBuild == "DTExpand") DTExpand();
		if (currentBuild == "RoboExpand") RoboExpand();
		if (currentBuild == "FourGate") FourGate();
		if (currentBuild == "ZealotRush") ZealotRush();
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
			getTech = false;
			techList.insert(techUnit);
		}
		else if (currentBuild == "DTExpand")
		{
			techUnit = UnitTypes::Protoss_Dark_Templar;
			getTech = false;
			techList.insert(techUnit);
		}
		else if (currentBuild == "RoboExpand")
		{
			techUnit = UnitTypes::Protoss_Reaver;
			getTech = false;
			techList.insert(techUnit);
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
		}
	}

	if (techUnit == UnitTypes::Protoss_Observer)
	{
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = 1;
		buildingDesired[UnitTypes::Protoss_Observatory] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility));
	}
	else if (techUnit == UnitTypes::Protoss_Reaver)
	{
		buildingDesired[UnitTypes::Protoss_Robotics_Facility] = 1;
		buildingDesired[UnitTypes::Protoss_Robotics_Support_Bay] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility));
		buildingDesired[UnitTypes::Protoss_Observatory] = min(1, buildingDesired[UnitTypes::Protoss_Observatory] + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver));
	}
	else if (techUnit == UnitTypes::Protoss_Corsair)
	{
		if (techList.find(techUnit) != techList.end())
		{
			buildingDesired[UnitTypes::Protoss_Stargate] = 2;
		}
		else
		{
			buildingDesired[UnitTypes::Protoss_Stargate] = 1;
			buildingDesired[UnitTypes::Protoss_Citadel_of_Adun] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Stargate));
			buildingDesired[UnitTypes::Protoss_Templar_Archives] = min(1, Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun));
		}
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

	// Additional cannon for FFE logic (add on at most 2 at a time)
	if (Strategy().isAllyFastExpand() && Units().getGlobalEnemyStrength() > Units().getGlobalAllyStrength() + Units().getAllyDefense())
	{
		buildingDesired[UnitTypes::Protoss_Photon_Cannon] = min(2 + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon), 1 + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));
	}

	// Shield battery logic
	if (Players().getNumberTerran() == 0 && Strategy().isRush() && !Strategy().isAllyFastExpand())
	{
		buildingDesired[UnitTypes::Protoss_Shield_Battery] = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core);
	}

	// Expansion logic
	if (Resources().isMinSaturated() && Production().isProductionSat() && Production().getIdleLowProduction().size() == 0 && Units().getGlobalStrategy() == 1)
	{
		buildingDesired[UnitTypes::Protoss_Nexus] = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) + 1;
	}

	// If we're not in our opener
	if (!getOpening)
	{
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
				if (Grids().getPylonGrid(base.second.getTilePosition()) == 0)
				{
					buildingDesired[UnitTypes::Protoss_Pylon] += 1;
				}
				else if (Grids().getDefenseGrid(base.second.getTilePosition()) < 1 && Broodwar->hasPower(TilePosition(base.second.getPosition())))
				{
					buildingDesired[UnitTypes::Protoss_Photon_Cannon] += 1;
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
		if (currentBuild == "TwoFactVult");
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
	if (Units().getGlobalAllyStrength() > Units().getGlobalEnemyStrength() && Resources().isMinSaturated() && Production().isProductionSat() && Production().getIdleLowProduction().size() == 0)
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
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= 3 && (Production().getIdleLowProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200) || (!Production().isBarracksSat() && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Barracks] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) + 1);
	}

	// Factory logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2 && (Production().getIdleLowProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200 && Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 100) || (!Production().isProductionSat() && Resources().isMinSaturated()))))
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

}

void BuildOrderTrackerClass::zergTech()
{

}

void BuildOrderTrackerClass::zergSituational()
{

}

//void BuildOrderTrackerClass::ReaverRush()
//{
//
//}