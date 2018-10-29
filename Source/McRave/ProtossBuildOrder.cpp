#include "McRave.h"

namespace McRave
{
	void BuildOrderManager::protossOpener()
	{
		if (currentBuild == "P1GateCore") P1GateCore();
		if (currentBuild == "PFFE") PFFE();
		if (currentBuild == "P12Nexus") P12Nexus();
		if (currentBuild == "P21Nexus") P21Nexus();


		if (currentBuild == "PScoutMemes") PScoutMemes();
		if (currentBuild == "PDWEBMemes") PDWEBMemes();
		if (currentBuild == "PArbiterMemes") PArbiterMemes();
		if (currentBuild == "PShuttleMemes") PShuttleMemes();
	}

	void BuildOrderManager::protossTech()
	{
		if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) == 0)
			return;

		// Some hardcoded techs based on needing detection or specific build orders
		if (getTech) {

			// If we need observers
			if (Strategy().needDetection() || (!Terrain().isIslandMap() && Players().vP() && techList.find(UnitTypes::Protoss_Observer) == techList.end() && !techList.empty()))
				techUnit = UnitTypes::Protoss_Observer;

			// HACK: Make carriers on Blue Storm
			else if (Broodwar->mapFileName().find("BlueStorm") != string::npos && techList.find(UnitTypes::Protoss_Carrier) == techList.end() && Players().vT())
				techUnit = UnitTypes::Protoss_Carrier;

			// HACK: Just double adds tech units on island maps
			else if (techUnit == UnitTypes::None && currentBuild == "P12Nexus" && Terrain().isIslandMap() && techList.size() <= 2) {
				techList.insert(UnitTypes::Protoss_Shuttle);
				unlockedType.insert(UnitTypes::Protoss_Shuttle);

				if (Players().getNumberTerran() > 0 || Broodwar->enemy()->getRace() == Races::Terran)
					techUnit = UnitTypes::Protoss_Carrier;
				else if (Players().getNumberZerg() > 0 || Broodwar->enemy()->getRace() == Races::Zerg)
					techUnit = UnitTypes::Protoss_Corsair;
				else if (Players().getNumberProtoss() > 0 || Broodwar->enemy()->getRace() == Races::Protoss)
					techUnit = UnitTypes::Protoss_Carrier;
			}

			else if (Strategy().getEnemyBuild() == "P4Gate" && currentBuild != "P4Gate" && techList.find(UnitTypes::Protoss_Dark_Templar) == techList.end() && !Strategy().enemyGasSteal())
				techUnit = UnitTypes::Protoss_Dark_Templar;
			else if (Terrain().isIslandMap() && techUnit == UnitTypes::None && currentBuild == "P1GateCorsair" && techList.find(UnitTypes::Protoss_Shuttle) == techList.end())
				techUnit = UnitTypes::Protoss_Shuttle;
			else if (techUnit == UnitTypes::None && currentBuild == "PZealotDrop" && techList.find(UnitTypes::Protoss_Scout) == techList.end() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shuttle) > 0)
				techUnit = UnitTypes::Protoss_Scout;
			else if (techUnit == UnitTypes::None)
				getNewTech();
		}

		checkNewTech();
		checkAllTech();
		checkExoticTech();
	}

	void BuildOrderManager::protossSituational()
	{
		auto skipFirstTech = (currentBuild == "P4Gate" || (Strategy().enemyGasSteal() && !Terrain().isNarrowNatural()));

		// Metrics for when to Expand/Add Production/Add Tech
		satVal = (Players().getNumberTerran() > 0 || Broodwar->enemy()->getRace() == Races::Terran) ? 2 : 3;
		prodVal = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) + (satVal * skipFirstTech);
		baseVal = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus);
		techVal = techList.size() + skipFirstTech + (Broodwar->enemy()->getRace() == Races::Terran);

		// HACK: Don't count obs as a tech unit
		if (techList.find(UnitTypes::Protoss_Observer) != techList.end())
			techVal--;

		//// HACK: No gas workers until at 14 supply (sometimes we get gas early to prevent steals
		//if (Units().getSupply() < 28)
		//	gasLimit = 0;

		// HACK: Against FFE just add a Nexus
		if (Strategy().getEnemyBuild() == "PFFE" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1)
			itemQueue[UnitTypes::Protoss_Nexus] = Item(2);

		// Saturation
		productionSat = (prodVal >= satVal * baseVal);
		techSat = (techVal >= baseVal);

		// If we have our tech unit, set to none
		if (techComplete())
			techUnit = UnitTypes::None;

		// If production is saturated and none are idle or we need detection, choose a tech
		if (Terrain().isIslandMap() || Strategy().needDetection() || (!getOpening && !getTech && !techSat && !Production().hasIdleProduction()))
			getTech = true;

		// Pylon logic
		if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > int(fastExpand)) {
			int providers = buildCount(UnitTypes::Protoss_Pylon) > 0 ? 14 : 16;
			int count = min(22, Units().getSupply() / providers);

			if (buildCount(UnitTypes::Protoss_Pylon) < count)
				itemQueue[UnitTypes::Protoss_Pylon] = Item(count);

			if (!getOpening && !Buildings().hasPoweredPositions() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 10)
				itemQueue[UnitTypes::Protoss_Pylon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) + 1);
		}

		// If we're not in our opener
		if (!getOpening) {
			gasLimit = INT_MAX;

			// Adding bases
			if (shouldExpand())
				itemQueue[UnitTypes::Protoss_Nexus] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) + 1);

			// Adding production
			if (shouldAddProduction()) {
				if (Terrain().isIslandMap())
					protossIslandPlay();				
				else {
					int gateCount = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);
					itemQueue[UnitTypes::Protoss_Gateway] = Item(gateCount);
				}
			}

			// Adding gas
			if (shouldAddGas())
				itemQueue[UnitTypes::Protoss_Assimilator] = Item(Resources().getGasCount());

			// Adding upgrade buildings
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Assimilator) >= 4 - (int)Players().vT()) {
				itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(1 + (int)Terrain().isIslandMap());
				itemQueue[UnitTypes::Protoss_Forge] = Item(2 - (int)Terrain().isIslandMap());
			}

			// Ensure we build a core outside our opening book
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= 2)
				itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(1);

			// Defensive Cannons
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1 && ((Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) >= 3 + (Players().getNumberTerran() > 0 || Players().getNumberProtoss() > 0)) || (Terrain().isIslandMap() && Players().getNumberZerg() > 0))) {
				itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));

				for (auto &station : Stations().getMyStations()) {
					auto &s = *station.second;

					if (Stations().needDefenses(s))
						itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1);
				}
			}
		}
		return;
	}

	void BuildOrderManager::protossUnlocks()
	{
		// Leg upgrade check
		auto isUpgradingLegs = Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements)
			|| Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements)
			|| (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0 && firstReady());

		// Check if we should always make Zealots
		if ((Players().vZ() && (!Terrain().isIslandMap() || Broodwar->getFrameCount() > 10000))
			|| (Terrain().isIslandMap() && currentBuild != "P1GateCorsair")
			|| (zealotLimit > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot))
			|| Strategy().enemyProxy()
			|| Strategy().enemyRush()
			|| isUpgradingLegs
			|| (techUnit == UnitTypes::Protoss_Dark_Templar && Players().vP())
			|| (Broodwar->mapFileName().find("BlueStorm") != string::npos && techList.find(UnitTypes::Protoss_Zealot) == techList.end())) {
			unlockedType.insert(UnitTypes::Protoss_Zealot);
		}
		else
			unlockedType.erase(UnitTypes::Protoss_Zealot);

		// TEST
		if (!techComplete() && techUnit == UnitTypes::Protoss_Dark_Templar && techList.size() == 1 && Players().vP() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) == 1)
			dragoonLimit = 0;

		// Check if we should always make Dragoons
		if (!Terrain().isIslandMap() && Broodwar->mapFileName().find("BlueStorm") == string::npos &&
			((Players().vZ() && Broodwar->getFrameCount() > 20000)
				|| Units().getEnemyCount(UnitTypes::Zerg_Lurker) > 0
				|| dragoonLimit > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)))
			unlockedType.insert(UnitTypes::Protoss_Dragoon);
		else
			unlockedType.erase(UnitTypes::Protoss_Dragoon);
	}

	void BuildOrderManager::protossIslandPlay()
	{
		if (shouldAddProduction()) {

			// PvZ island
			if (Broodwar->enemy()->getRace() == Races::Zerg) {
				int nexusCount = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
				int roboCount = min(nexusCount - 2, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Robotics_Facility) + 1);
				int stargateCount = min(nexusCount, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) + 1);

				if (Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 150) {
					itemQueue[UnitTypes::Protoss_Stargate] = Item(stargateCount);
					itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(roboCount);
					itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
				}
				itemQueue[UnitTypes::Protoss_Gateway] = Item(nexusCount);
			}

			// PvP island
			else if (Broodwar->enemy()->getRace() == Races::Protoss) {
				int nexusCount = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
				int gateCount = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);

				if (Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 200) {
					itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
					if (techList.find(UnitTypes::Protoss_Scout) != techList.end() || techList.find(UnitTypes::Protoss_Carrier) != techList.end())
						itemQueue[UnitTypes::Protoss_Stargate] = Item(nexusCount);
				}

				itemQueue[UnitTypes::Protoss_Gateway] = Item(gateCount);
			}

			// PvT island
			else {
				int nexusCount = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
				int stargateCount = min(nexusCount + 1, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) + 1);
				if (Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 150) {
					itemQueue[UnitTypes::Protoss_Stargate] = Item(stargateCount);
					itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(min(1, stargateCount - 2));
					itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
				}
				itemQueue[UnitTypes::Protoss_Gateway] = Item(nexusCount);
			}
		}
	}
}