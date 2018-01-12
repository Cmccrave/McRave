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
	if (Broodwar->self()->getRace() == Races::Protoss) buildNames = { "PZZCore", "PZCore", "PNZCore", "P4Gate", "PFFEStandard", "P12Nexus", "P21Nexus", "PDTExpand", "P2GateDragoon" };
	if (Broodwar->self()->getRace() == Races::Terran) buildNames = { "T2Fact", "TSparks" };
	if (Broodwar->self()->getRace() == Races::Zerg) buildNames = { "ZOverpool" };

	currentBuild = "FFEScout";
	return;

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
			ss << build << " 0 0 ";
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
		if (gamesPlayed > 0 && losses == 0 && ((Players().getNumberProtoss() > 0 && isBuildAllowed(Races::Protoss, build)) || (Players().getNumberTerran() > 0 && isBuildAllowed(Races::Terran, build)) || (Players().getNumberZerg() > 0 && isBuildAllowed(Races::Zerg, build)) || (Players().getNumberRandom() > 0 && isBuildAllowed(Races::Random, build))))
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
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (enemy == Races::Zerg && (build == "PFFEStandard" || build == "P4Gate")) return true;
		if (enemy == Races::Terran && (build == "P12Nexus" || build == "P21Nexus" /*|| build == "PDTExpand"*/ || build == "P2GateDragoon")) return true;
		if (enemy == Races::Protoss && (build == "PZCore" || build == "PNZCore" || build == "P4Gate")) return true;
		if (enemy == Races::Random && (build == "PZZCore" || build == "P4Gate" || build == "PFFEStandard")) return true;
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		if (build == "TSparks" || build == "T2Fact") return true;
	}
	else if (Broodwar->self()->getRace() == Races::Zerg && build == "ZOverpool") return true;
	return false;
}

bool BuildOrderTrackerClass::techComplete()
{
	if (techUnit == UnitTypes::Protoss_Scout || techUnit == UnitTypes::Protoss_Corsair || techUnit == UnitTypes::Protoss_Observer || techUnit == UnitTypes::Protoss_Reaver) return (Broodwar->self()->completedUnitCount(techUnit) > 0);
	if (techUnit == UnitTypes::Protoss_High_Templar) return (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm));
	if (techUnit == UnitTypes::Protoss_Arbiter) return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
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
	if (shouldExpand())
		buildingDesired[UnitTypes::Zerg_Hatchery] = Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Hatchery) + Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Hive) + 1;
}

bool BuildOrderTrackerClass::shouldExpand()
{
	UnitType baseType;
	if (Broodwar->self()->getRace() == Races::Protoss) baseType = UnitTypes::Protoss_Nexus;
	else if (Broodwar->self()->getRace() == Races::Terran) baseType = UnitTypes::Terran_Command_Center;
	else baseType = UnitTypes::Zerg_Hatchery;

	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Broodwar->self()->minerals() > 500 + (100 * Broodwar->self()->completedUnitCount(baseType))) return true;
		else if ((techUnit == UnitTypes::None && !Production().hasIdleProduction() && Resources().isMinSaturated() && techSat && productionSat) || (productionSat && Players().getPlayers().size() <= 1 && Players().getNumberTerran() > 0)) return true;
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		if (Broodwar->self()->minerals() > 500 + (100 * Broodwar->self()->completedUnitCount(baseType))) return true;
	}
	return false;
}

bool BuildOrderTrackerClass::shouldAddProduction()
{
	if (!Production().hasIdleProduction() && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 150) || (!productionSat && Resources().isMinSaturated()))) return true;
	return false;
}

bool BuildOrderTrackerClass::shouldAddGas()
{
	if (!Strategy().isPlayPassive() && Resources().isGasSaturated() && Broodwar->self()->gas() < Broodwar->self()->minerals() * 3 && Broodwar->self()->minerals() > 100) return true;
	return false;
}