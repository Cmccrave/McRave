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
	}

	void BuildOrderManager::onStart()
	{
		if (!Broodwar->enemy()) {
			getDefaultBuild();
			return;
		}
		
		string build, buffer;
		ifstream config("bwapi-data/read/" + Broodwar->enemy()->getName() + ".txt");
		int wins, losses, gamesPlayed, totalGamesPlayed = 0;
		double best = 0.0;
		getOpening = true;

		currentBuild = "PFFE";
		isBuildPossible(currentBuild);
		return;
				
		if (Broodwar->self()->getRace() == Races::Protoss)
			buildNames ={ "PZZCore", "PZCore", "PNZCore", "P4Gate", "PDTExpand", "P2GateDragoon", "PProxy6", "PProxy99", "PFFE", "P12Nexus", "P21Nexus", "P2GateExpand", "P1GateRobo", "PZealotDrop", "P1GateCorsair" };
		if (Broodwar->self()->getRace() == Races::Terran)
			buildNames ={ "T2Fact", "TSparks", "T2PortWraith", "T1RaxFE", "T2RaxFE", "T1FactFE", "TBCMemes" };
		if (Broodwar->self()->getRace() == Races::Zerg)
			buildNames ={ "Z2HatchMuta", "Z3HatchLing", "Z4Pool", "Z9Pool", "Z2HatchHydra", "Z3HatchBeforePool", "ZLurkerTurtle" };

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
	}

	bool BuildOrderManager::isBuildPossible(string build)
	{
		using namespace UnitTypes;
		vector<UnitType> buildings, defenses;

		if (Broodwar->self()->getRace() == Races::Protoss) {

			if (Terrain().isIslandMap())
				return (build == "P12Nexus" || build == "PZealotDrop" || build == "P1GateCorsair");

			// Blue Storm
			if (Broodwar->mapFileName().find("BlueStorm") != string::npos)
				return (build == "P12Nexus" || build == "PProxy99");

			// Destination - No 4Gate
			if (Broodwar->mapFileName().find("Destination") != string::npos && build == "P4Gate")
				return false;

			if (build == "P2GateDragoon") {
				buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Cybernetics_Core, Protoss_Pylon };
			}
			else if (build == "P2GateExpand") {
				buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Pylon };
				defenses.insert(defenses.end(), 6, Protoss_Photon_Cannon);
			}
			else if (build == "P12Nexus" || build == "P21Nexus" || build == "P3Nexus") {
				buildings ={ Protoss_Gateway, Protoss_Cybernetics_Core, Protoss_Pylon };
			}
			else if (build == "PFFE" || build.find("Meme") != string::npos) {
				buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
				defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
			}
			else if (Broodwar->mapFileName().find("Alchemist") != string::npos && build != "P4Gate") {
				return false;
			}
			else
				return true;
		}

		if (Broodwar->self()->getRace() == Races::Terran)
			buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };					

		if (Broodwar->self()->getRace() == Races::Zerg) {
			buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
			defenses.insert(defenses.end(), 3, Zerg_Sunken_Colony);
		}

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
		if (Broodwar->self()->getRace() == Races::Protoss) {
			//if (Terrain().isIslandMap()) {
			//	if (enemy == Races::Zerg)
			//		return (build == "P1GateCorsair");
			//	else if (enemy == Races::Random)
			//		return (build == "PZealotDrop");
			//	else
			//		return (build == "P12Nexus" || build == "PZealotDrop");
			//}

			//// HACK: Blue Storm hardcoded builds due to lack of pathfinding around chokes
			//if (Broodwar->mapFileName().find("BlueStorm") != string::npos) {
			//	if (enemy == Races::Terran)
			//		return (build == "PProxy99" || build == "P12Nexus" || build == "P21Nexus" || build == "PDTExpand");
			//	else if (enemy == Races::Zerg)
			//		return (build == "PProxy99" || build == "PFFE");
			//	else if (enemy == Races::Protoss || enemy == Races::Random)
			//		return (build == "PProxy99" || build == "PZCore");
			//}

			//// HACK: Only 4gate on Alchemist
			//if (Broodwar->mapFileName().find("Alchemist") != string::npos && build != "P4Gate")
			//	return false;

			//if (enemy == Races::Zerg && (build == "PFFE" || build == "P4Gate" || build == "P2GateExpand" || build == "P1GateCorsair"))
			//	return true;
			//if (enemy == Races::Terran && (build == "P12Nexus" || build == "P21Nexus" || build == "PDTExpand" || build == "P2GateDragoon"))
			//	return true;
			//if (enemy == Races::Protoss && (build == "PZCore" || build == "P4Gate" || build == "P1GateRobo" || build == "P2GateExpand"))
			//	return true;
			//if (enemy == Races::Random && (build == "PZZCore" || build == "P4Gate" || build == "PFFE" || build == "P2GateExpand"))
			//	return true;

			//// HACK: Only 4gate on Alchemist
			//if (Broodwar->mapFileName().find("Alchemist") != string::npos && build == "P4Gate")
			//	return true;
			return build == "P4Gate";
		}

		if (Broodwar->self()->getRace() == Races::Terran) {
			/*if (build == "TSparks" || build == "T2Fact" || build == "T2PortWraith")
				return true;
			if (build == "T1RaxFE" || build == "T2RaxFE" || build == "T1FactFE")
				return true;*/

			if (build == "T2Fact")
				return true;
			else
				return false;
		}

		if (Broodwar->self()->getRace() == Races::Zerg) {
			if (find(buildNames.begin(), buildNames.end(), build) != buildNames.end())
				return true;
		}
		return false;
	}

	bool BuildOrderManager::techComplete()
	{
		if (techUnit == UnitTypes::Protoss_Scout || techUnit == UnitTypes::Protoss_Corsair || techUnit == UnitTypes::Protoss_Observer || techUnit == UnitTypes::Terran_Science_Vessel)
			return (Broodwar->self()->completedUnitCount(techUnit) > 0);
		if (techUnit == UnitTypes::Protoss_High_Templar)
			return (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm));
		if (techUnit == UnitTypes::Protoss_Arbiter)
			return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
		if (techUnit == UnitTypes::Protoss_Dark_Templar)
			return (Broodwar->self()->completedUnitCount(techUnit) >= 2);
		if (techUnit == UnitTypes::Protoss_Reaver)
			return (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shuttle) >= 1) || (Broodwar->self()->completedUnitCount(techUnit) > 0 && Terrain().isNarrowNatural());
		return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
	}

	void BuildOrderManager::getDefaultBuild()
	{
		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (Players().getNumberProtoss() > 0)
				currentBuild = "PZCore";
			else if (Players().getNumberZerg() > 0)
				currentBuild = "P4Gate";
			else if (Players().getNumberTerran() > 0)
				currentBuild = "P12Nexus";
			else if (Players().getNumberRandom() > 0)
				currentBuild = "P4Gate";
		}
		if (Broodwar->self()->getRace() == Races::Terran)
			currentBuild = "TSparks";
		if (Broodwar->self()->getRace() == Races::Zerg)
			currentBuild = "Z2HatchMuta";
		return;
	}

	void BuildOrderManager::onFrame()
	{
		Display().startClock();
		updateBuild();
		Display().performanceTest(__FUNCTION__);
	}

	void BuildOrderManager::updateBuild()
	{
		if (Broodwar->self()->getRace() == Races::Protoss) {
			protossOpener();
			protossTech();
			protossSituational();
			protossUnlocks();
		}
		if (Broodwar->self()->getRace() == Races::Terran) {
			terranOpener();
			terranTech();
			terranSituational();
		}
		if (Broodwar->self()->getRace() == Races::Zerg) {
			zergOpener();
			zergTech();
			zergSituational();
		}
	}

	bool BuildOrderManager::shouldExpand()
	{
		UnitType baseType = Broodwar->self()->getRace().getResourceDepot();

		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (Broodwar->self()->minerals() > 500 + (100 * Broodwar->self()->completedUnitCount(baseType)))
				return true;
			else if (techUnit == UnitTypes::None && !Production().hasIdleProduction() && Resources().isMinSaturated() && techSat && productionSat)
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Terran) {
			if (Broodwar->self()->minerals() > 400 + (100 * Broodwar->self()->completedUnitCount(baseType)))
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Zerg) {
			if (Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() >= 300)
				return true;
		}
		return false;
	}

	bool BuildOrderManager::shouldAddProduction()
	{
		if (Broodwar->self()->getRace() == Races::Zerg) {
			if (Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Larva) <= 1 && Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 100)
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
		auto workerCount = Broodwar->self()->completedUnitCount(Broodwar->self()->getRace().getWorker());
		if (Broodwar->self()->getRace() == Races::Zerg) {
			if (Resources().isGasSaturated() && Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 100)
				return true;
		}
		
		else if (Broodwar->self()->getRace() == Races::Protoss) {
			if (Players().vP())
				return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Assimilator) != 1 || workerCount >= 32 || Broodwar->self()->minerals() > 600;
			else if (Players().vT() || Players().vZ())
				return true;
		}

		else if (Broodwar->self()->getRace() == Races::Terran)
			return true;
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
		else if (firstTech == TechTypes::None && firstUpgrade == UpgradeTypes::None)
			return true;
		return false;
	}

	void BuildOrderManager::getNewTech()
	{
		if (!getTech)
			return;

		// Otherwise, choose a tech based on highest unit score
		double highest = 0.0;
		for (auto &tech : Strategy().getUnitScores()) {
			if (tech.first == UnitTypes::Protoss_Dragoon
				|| tech.first == UnitTypes::Protoss_Zealot
				|| tech.first == UnitTypes::Protoss_Shuttle)
				continue;

			if (tech.second > highest) {
				highest = tech.second;
				techUnit = tech.first;
			}
		}
	}

	void BuildOrderManager::checkNewTech()
	{
		// No longer need to choose a tech
		if (techUnit != UnitTypes::None) {
			getTech = false;
			techList.insert(techUnit);
			unlockedType.insert(techUnit);
		}

		// Multi-unlock
		if (techUnit == UnitTypes::Protoss_Arbiter || techUnit == UnitTypes::Protoss_High_Templar) {
			unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
			techList.insert(UnitTypes::Protoss_Dark_Templar);
		}
		else if (techUnit == UnitTypes::Protoss_Reaver) {
			unlockedType.insert(UnitTypes::Protoss_Shuttle);
			techList.insert(UnitTypes::Protoss_Shuttle);
		}
		else if (techUnit == UnitTypes::Zerg_Mutalisk) {
			techList.insert(UnitTypes::Zerg_Scourge);
			unlockedType.insert(UnitTypes::Zerg_Scourge);
		}
		else if (techUnit == UnitTypes::Zerg_Lurker) {
			techList.insert(UnitTypes::Zerg_Hydralisk);
			unlockedType.insert(UnitTypes::Zerg_Hydralisk);
		}

		// HACK: If we have a Reaver add Obs to the tech
		if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Reaver) >= 3) {
			techList.insert(UnitTypes::Protoss_Observer);
			unlockedType.insert(UnitTypes::Protoss_Observer);
		}
	}

	void BuildOrderManager::checkAllTech()
	{
		bool moreToAdd;
		set<UnitType> toCheck;

		// For every unit in our tech list, ensure we are building the required buildings
		for (auto &type : techList) {
			toCheck.insert(type);
			toCheck.insert(type.whatBuilds().first);
		}

		// Iterate all required branches of buildings that are required for this tech unit
		do {
			moreToAdd = false;
			for (auto &check : toCheck) {
				for (auto &pair : check.requiredUnits()) {
					UnitType type(pair.first);
					if (Broodwar->self()->completedUnitCount(type) == 0 && toCheck.find(type) == toCheck.end()) {
						toCheck.insert(type);
						moreToAdd = true;
					}
				}
			}
		} while (moreToAdd);

		// For each building we need to check, add to our queue whatever is possible to build based on its required branch
		int i = 0;
		for (auto &check : toCheck) {

			if (!check.isBuilding())
				continue;

			i += 10;

			bool canAdd = true;
			for (auto &pair : check.requiredUnits()) {
				UnitType type(pair.first);
				if (type.isBuilding() && Broodwar->self()->completedUnitCount(type) == 0) {
					canAdd = false;
				}
			}

			// HACK: Our check doesn't look for required buildings for tech needed for Lurkers
			if (check == UnitTypes::Zerg_Lurker)
				itemQueue[UnitTypes::Zerg_Lair] = Item(1);

			// Add extra production - TODO: move to shouldAddProduction
			int s = Units().getSupply();
			if (canAdd && buildCount(check) <= 1) {
				if (check == UnitTypes::Protoss_Stargate) {
					if ((s >= 150 && techList.find(UnitTypes::Protoss_Corsair) != techList.end())
						|| (s >= 300 && techList.find(UnitTypes::Protoss_Arbiter) != techList.end()))
						itemQueue[check] = Item(2);
					else
						itemQueue[check] = Item(1);
				}
				else if (check != UnitTypes::Protoss_Gateway)
					itemQueue[check] = Item(1);

			}
		}
	}

	void BuildOrderManager::checkExoticTech()
	{
		// Corsair/Scout upgrades
		if ((techList.find(UnitTypes::Protoss_Scout) != techList.end() && currentBuild != "PDTExpand") || (techList.find(UnitTypes::Protoss_Corsair) != techList.end() && Units().getSupply() >= 300))
			itemQueue[UnitTypes::Protoss_Fleet_Beacon] = Item(1);

		// HACK: Bluestorm carrier hack
		if (Broodwar->mapFileName().find("BlueStorm") != string::npos && techList.find(UnitTypes::Protoss_Carrier) != techList.end())
			itemQueue[UnitTypes::Protoss_Stargate] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus));

		// Hive upgrades
		if (Broodwar->self()->getRace() == Races::Zerg && Units().getSupply() >= 200) {
			itemQueue[UnitTypes::Zerg_Queens_Nest] = Item(1);
			itemQueue[UnitTypes::Zerg_Hive] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Queens_Nest) >= 1);
			itemQueue[UnitTypes::Zerg_Lair] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Queens_Nest) < 1);
		}
	}

	void BuildOrderManager::checkUnitLimits()
	{

	}
}