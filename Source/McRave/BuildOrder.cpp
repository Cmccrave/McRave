#include "McRave.h"
#include <fstream>

namespace McRave
{
	void BuildOrderManager::onEnd(bool isWinner)
	{
		ofstream config("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");
		string t1;
		int t2, t3;
		int lineBreak = 0;

		while (ss >> t1 >> t2 >> t3) {
			t1 == currentBuild ? (isWinner ? t2++ : t3++) : 0;
			config << t1 << " " << t2 << " " << t3 << endl;
		}
		return;
	}

	void BuildOrderManager::onStart()
	{
		if (!Broodwar->enemy())
			getDefaultBuild();

		string build, buffer;
		ifstream config("bwapi-data/read/" + Broodwar->enemy()->getName() + ".txt");
		int wins, losses, gamesPlayed, totalGamesPlayed = 0;
		double best = 0.0;
		getOpening = true;

		// Write what builds you're using
#ifdef MCRAVE_PROTOSS
		if (Broodwar->self()->getRace() == Races::Protoss)
			buildNames ={ "PZZCore", "PZCore", "PNZCore", "P4Gate", "PDTExpand", "P2GateDragoon", "PProxy6", "PProxy99", "PFFE", "P12Nexus", "P21Nexus", "P2GateExpand", "P1GateRobo" };

#endif // MCRAVE_PROTOSS

#ifdef MCRAVE_TERRAN
		if (Broodwar->self()->getRace() == Races::Terran)
			buildNames ={ "T2Fact", "TSparks", "T2PortWraith", "T1RaxFE", "T2RaxFE", "T1FactFE", "TBCMemes" };
#endif // MCRAVE_TERRAN

#ifdef MCRAVE_ZERG
		if (Broodwar->self()->getRace() == Races::Zerg)
			buildNames ={ "Z2HatchMuta", "Z3HatchLing", "Z4Pool", "Z9Pool", "Z2HatchHydra" };
#endif // MCRAVE_ZERG

		// If we don't have a file in the /read/ folder, then check the /write/ folder
		if (!config) {
			ifstream localConfig("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");

			// If still no file, then we need to create one and add all builds to it that we're using
			if (!localConfig.good()) {
				for (auto &build : buildNames)
					ss << build << " 0 0 ";
			}

			// If there is a file, load it into the stringstream
			else {
				while (localConfig >> buffer)
					ss << buffer << " ";
			}
		}

		// If we do have a file, load it into the stringstream
		else {
			while (config >> buffer)
				ss << buffer << " ";
		}

		// Check SS if the build exists, if it doesn't, add it
		string s = ss.str();
		for (auto &build : buildNames) {
			if (s.find(build) == s.npos)
				ss << build << " 0 0 ";
		}

		// Calculate how many games we've played against this opponent
		stringstream ss2;
		ss2 << ss.str();
		while (!ss2.eof()) {
			ss2 >> build >> wins >> losses;
			totalGamesPlayed += wins + losses;
		}

		if (totalGamesPlayed == 0) {
			getDefaultBuild();
			if (isBuildPossible(currentBuild))
				return;
		}

		// Calculate which build is best
		stringstream ss3;
		ss3 << ss.str();
		while (!ss3.eof()) {
			ss3 >> build >> wins >> losses;
			gamesPlayed = wins + losses;

			// Against static easy bots to beat, just stick with whatever is working (prevents uncessary losses)
			if (gamesPlayed > 0 && losses == 0 && ((Players().getNumberProtoss() > 0 && isBuildAllowed(Races::Protoss, build)) || (Players().getNumberTerran() > 0 && isBuildAllowed(Races::Terran, build)) || (Players().getNumberZerg() > 0 && isBuildAllowed(Races::Zerg, build)) || (Players().getNumberRandom() > 0 && isBuildAllowed(Races::Random, build)))) {
				currentBuild = build;
				break;
			}

			if ((Players().getNumberProtoss() > 0 && isBuildAllowed(Races::Protoss, build)) || (Players().getNumberTerran() > 0 && isBuildAllowed(Races::Terran, build)) || (Players().getNumberZerg() > 0 && isBuildAllowed(Races::Zerg, build)) || (Players().getNumberRandom() > 0 && isBuildAllowed(Races::Random, build))) {
				if (gamesPlayed <= 0) {
					currentBuild = build;
					break;
				}
				else {
					double winRate = gamesPlayed > 0 ? wins / static_cast<double>(gamesPlayed) : 0;
					double ucbVal = sqrt(2.0 * log((double)totalGamesPlayed) / gamesPlayed);
					double val = winRate + ucbVal;

					if (val >= best) {
						best = val;
						currentBuild = build;
					}
				}
			}
		}
		if (!isBuildPossible(currentBuild))
			getDefaultBuild();
		return;
	}

	bool BuildOrderManager::isBuildPossible(string build)
	{
		using namespace UnitTypes;
		vector<UnitType> buildings, defenses;

		if (Terrain().isIslandMap())
			return (build == "P12Nexus");

#ifdef MCRAVE_PROTOSS
		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (build == "P2GateDragoon") {
				buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Cybernetics_Core, Protoss_Pylon };
			}
			else if (build == "P2GateExpand") {
				buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Pylon };
				defenses.insert(defenses.end(), 6, Protoss_Photon_Cannon);
			}
			else if (build == "P12Nexus" || build == "P21Nexus" || build == "P1GateRobo" || build == "P3Nexus") {
				buildings ={ Protoss_Gateway, Protoss_Cybernetics_Core, Protoss_Pylon };
			}
			else if (build == "PFFE" || build == "PScoutMemes" || build == "PDWEBMemes") {
				buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
				defenses.insert(defenses.end(), 6, Protoss_Photon_Cannon);
			}
			else if (Broodwar->mapFileName().find("Alchemist") != string::npos && build != "P4Gate") {
				return false;
			}
			else
				return true;
		}
#endif // MCRAVE_PROTOSS

#ifdef MCRAVE_TERRAN
		if (Broodwar->self()->getRace() == Races::Terran) {
			if (build == "T1RaxFE" || build == "T2RaxFE" || build == "T2Fact" || build == "T1FactFE" || build == "TBCMemes")
				buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
			else
				return true;
		}
#endif // MCRAVE_TERRAN

#ifdef MCRAVE_ZERG
		if (Broodwar->self()->getRace() == Races::Zerg) {
			buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
			defenses.insert(defenses.end(), 6, Zerg_Sunken_Colony);
		}
#endif // MCRAVE_ZERG

		if (build == "T2Fact" || build == "TSparks") {
			if (Terrain().findMainWall(buildings, defenses))
				return true;
		}
		else {
			if (Terrain().findNaturalWall(buildings, defenses))
				return true;
		}
		return false;
	}

	bool BuildOrderManager::isBuildAllowed(Race enemy, string build)
	{
		if (Terrain().isIslandMap())
			return (build == "P12Nexus");

#ifdef MCRAVE_PROTOSS
		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (enemy == Races::Zerg && (build == "PFFE" || build == "P4Gate" || build == "P2GateExpand"))
				return true;
			if (enemy == Races::Terran && (build == "P12Nexus" || build == "P21Nexus" || build == "PDTExpand" || build == "P2GateDragoon"))
				return true;
			if (enemy == Races::Protoss && (build == "PZCore" || build == "PNZCore" || build == "P4Gate" || build == "P1GateRobo"))
				return true;
			if (enemy == Races::Random && (build == "PZZCore" || build == "P4Gate" || build == "PFFE"))
				return true;
			if (Broodwar->mapFileName().find("Alchemist") != string::npos && build == "P4Gate")
				return true;
		}
#endif

#ifdef MCRAVE_TERRAN
		if (Broodwar->self()->getRace() == Races::Terran) {
			if (build == "TSparks" || build == "T2Fact" || build == "T2PortWraith")
				return true;
			if (build == "T1RaxFE" || build == "T2RaxFE" || build == "T1FactFE")
				return true;
		}
#endif

#ifdef MCRAVE_ZERG
		if (Broodwar->self()->getRace() == Races::Zerg) {
			if (build == "Z2HatchMuta")
				return true;
		}
		return false;

#endif
	}

	bool BuildOrderManager::techComplete()
	{
		if (techUnit == UnitTypes::Protoss_Scout || techUnit == UnitTypes::Protoss_Corsair || techUnit == UnitTypes::Protoss_Observer || techUnit == UnitTypes::Protoss_Reaver || techUnit == UnitTypes::Terran_Science_Vessel) return (Broodwar->self()->completedUnitCount(techUnit) > 0);
		if (techUnit == UnitTypes::Protoss_High_Templar) return (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm));
		if (techUnit == UnitTypes::Protoss_Arbiter) return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
		if (techUnit == UnitTypes::Protoss_Dark_Templar) return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
		else return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
		return false;
	}

	void BuildOrderManager::getDefaultBuild()
	{
#ifdef MCRAVE_PROTOSS
		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (Players().getNumberProtoss() > 0)
				currentBuild = "PZCore";
			else if (Players().getNumberZerg() > 0)
				currentBuild = "P4Gate";
			else if (Players().getNumberTerran() > 0)
				currentBuild = "P12Nexus";
			else if (Players().getNumberRandom() > 0)
				currentBuild = "PZZCore";
		}
#endif

#ifdef MCRAVE_TERRAN
		if (Broodwar->self()->getRace() == Races::Terran) {
			currentBuild = "TSparks";
		}
#endif

#ifdef MCRAVE_ZERG
		if (Broodwar->self()->getRace() == Races::Zerg) {
			currentBuild = "Z2HatchMuta";
		}
#endif
		return;
	}

	void BuildOrderManager::onFrame()
	{
		Display().startClock();
		updateBuild();
		Display().performanceTest(__FUNCTION__);
		return;
	}

	void BuildOrderManager::updateBuild()
	{
#ifdef MCRAVE_PROTOSS
		if (Broodwar->self()->getRace() == Races::Protoss) {
			protossOpener();
			protossTech();
			protossSituational();
		}
#endif // MCRAVE_PROTOSS

#ifdef MCRAVE_TERRAN
		if (Broodwar->self()->getRace() == Races::Terran) {
			terranOpener();
			terranTech();
			terranSituational();
		}
#endif // MCRAVE_TERRAN

#ifdef MCRAVE_ZERG
		if (Broodwar->self()->getRace() == Races::Zerg) {
			zergOpener();
			zergTech();
			zergSituational();
		}
#endif // MCRAVE_ZERG
	}

	bool BuildOrderManager::shouldExpand()
	{
		UnitType baseType = Broodwar->self()->getRace().getResourceDepot();

		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			if (Broodwar->self()->minerals() > 500 + (100 * Broodwar->self()->completedUnitCount(baseType))) return true;
			else if (techUnit == UnitTypes::None && !Production().hasIdleProduction() && Resources().isMinSaturated() && techSat && productionSat) return true;
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			if (Broodwar->self()->minerals() > 400 + (100 * Broodwar->self()->completedUnitCount(baseType)))
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Zerg)
		{
			if (Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 450)
				return true;
		}
		return false;
	}

	bool BuildOrderManager::shouldAddProduction()
	{
		if (Broodwar->self()->getRace() == Races::Zerg)
		{
			if (Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Larva) <= 2 && Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 300)
				return true;
		}
		else {
			if ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 150) || (!productionSat && Resources().isMinSaturated()))
				return true;
		}
		return false;
	}

	bool BuildOrderManager::shouldAddGas()
	{
		if (Broodwar->self()->getRace() == Races::Zerg) {
			if (Resources().isGasSaturated() && Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 400)
				return true;
		}
		else {
			//if (Resources().isGasSaturated() /*&& Broodwar->self()->gas() < Broodwar->self()->minerals() * 3*/ && Broodwar->self()->minerals() > 100) 
			return true;
		}
		return false;
	}

	int BuildOrderManager::buildCount(UnitType unit)
	{
		if (itemQueue.find(unit) != itemQueue.end())
			return itemQueue[unit].getActualCount();
		return 0;
	}

	bool BuildOrderManager::firstReady()
	{
		if (firstTech != TechTypes::None && Broodwar->self()->hasResearched(firstTech))
			return true;
		else if (firstUpgrade != UpgradeTypes::None && Broodwar->self()->getUpgradeLevel(firstUpgrade) > 0)
			return true;
		return false;
	}
}