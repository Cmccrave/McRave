#include "McRave.h"

#ifdef MCRAVE_PROTOSS

namespace McRave
{
	void BuildOrderManager::protossOpener()
	{
		if (currentBuild == "PZZCore") PZZCore();
		if (currentBuild == "PZCore") PZCore();
		if (currentBuild == "PNZCore") PNZCore();
		if (currentBuild == "P4Gate") P4Gate();
		if (currentBuild == "PFFE") PFFE();
		if (currentBuild == "P12Nexus") P12Nexus();
		if (currentBuild == "P21Nexus") P21Nexus();
		if (currentBuild == "PDTExpand") PDTExpand();
		if (currentBuild == "P2GateDragoon") P2GateDragoon();
		if (currentBuild == "PScoutMemes") PScoutMemes();
		if (currentBuild == "PProxy6") PProxy6();
		if (currentBuild == "PProxy99") PProxy99();
		if (currentBuild == "P2GateExpand") P2GateExpand();
		if (currentBuild == "PDWEBMemes") PDWEBMemes();
		if (currentBuild == "PArbiterMemes") PArbiterMemes();
		if (currentBuild == "P1GateRobo") P1GateRobo();
		if (currentBuild == "P3Nexus") P3Nexus();
		return;
	}

	void BuildOrderManager::protossTech()
	{
		bool moreToAdd;
		set<UnitType> toCheck;

		if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) == 0)
			return;

		// Some hardcoded techs based on needing detection or specific build orders
		if (getTech) {

			// If we need detection, immediately stop building other tech
			if (Strategy().needDetection()) {
				techList.insert(UnitTypes::Protoss_Observer);
				unlockedType.insert(UnitTypes::Protoss_Observer);
				itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(1);
				itemQueue[UnitTypes::Protoss_Observatory] =	 Item(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Facility) > 0);
				return; // This return prevents us building other tech
			}

			// Just double adds tech units on island maps
			else if (Terrain().isIslandMap() && techList.size() <= 1) {
				techList.insert(UnitTypes::Protoss_Shuttle);
				unlockedType.insert(UnitTypes::Protoss_Shuttle);

				if (Players().getNumberTerran() > 0) {
					techUnit = UnitTypes::Protoss_Carrier;
				}
				else if (Players().getNumberZerg() > 0) {
					techUnit = UnitTypes::Protoss_Corsair;					
				}

				// PvP double tech
				if (Players().getNumberProtoss() > 0) {
					techList.insert(UnitTypes::Protoss_Corsair);
					unlockedType.insert(UnitTypes::Protoss_Corsair);
					techList.insert(UnitTypes::Protoss_Reaver);
					unlockedType.insert(UnitTypes::Protoss_Reaver);
				}				
			}

			// Otherwise, choose a tech based on highest unit score
			else if (techUnit == UnitTypes::None) {
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

			// No longer need to choose a tech
			if (techUnit != UnitTypes::None) {
				getTech = false;
				techList.insert(techUnit);
				unlockedType.insert(techUnit);
			}
		}

		// Multi-unlock
		if (techUnit == UnitTypes::Protoss_Arbiter) {
			unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
			techList.insert(UnitTypes::Protoss_Dark_Templar);
		}

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

			bool canAdd = true;
			for (auto &pair : check.requiredUnits()) {
				UnitType type(pair.first);
				if (type.isBuilding() && Broodwar->self()->completedUnitCount(type) == 0) {
					canAdd = false;
				}
			}

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
				else if (check == UnitTypes::Protoss_Robotics_Facility) {
					if (s >= 200 && techList.find(UnitTypes::Protoss_Reaver) != techList.end()) {
						itemQueue[check] = Item(2);
					}
					else
						itemQueue[check] = Item(1);
				}
				else if (check != UnitTypes::Protoss_Gateway)
					itemQueue[check] = Item(1);

			}
		}

		// Exotic tech/upgrades
		if (techList.find(UnitTypes::Protoss_Scout) != techList.end() || (techList.find(UnitTypes::Protoss_Corsair) != techList.end() && Units().getSupply() >= 300))
			itemQueue[UnitTypes::Protoss_Fleet_Beacon] = Item(1);
	}

	void BuildOrderManager::protossSituational()
	{
		// Metrics for when to Expand/Add Production/Add Tech
		satVal = Players().getNumberTerran() > 0 ? 2 : 3;
		prodVal = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway);
		baseVal = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
		techVal = techList.size() + 1 + (currentBuild == "P4Gate") + (Players().getNumberTerran() > 0);

		// Saturation
		productionSat = (prodVal >= satVal * baseVal);
		techSat = (techVal > baseVal);
		
		// If we have our tech unit, set to none
		if (techComplete())
			techUnit = UnitTypes::None;

		// If production is saturated and none are idle or we need detection, choose a tech
		if (Terrain().isIslandMap() || Strategy().needDetection() || (!getOpening && !getTech && !techSat && !Production().hasIdleProduction()))
			getTech = true;

		// Check if we should always make Zealots
		if ((Players().getNumberTerran() == 0 && firstReady())
			|| Players().getNumberZerg() > 0
			|| Terrain().isIslandMap()
			|| BuildOrder().getCurrentBuild() == "P1GateRobo"
			|| BuildOrder().getCurrentBuild() == "PScoutMemes"
			|| BuildOrder().getCurrentBuild() == "PDWEBMemes"
			|| BuildOrder().getCurrentBuild() == "P4Gate"
			|| (BuildOrder().getCurrentBuild() == "PZCore" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) < 1)
			|| (BuildOrder().getCurrentBuild() == "PZZCore" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) < 2)
			|| (BuildOrder().getCurrentBuild() == "PDTExpand" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) < 2)
			|| Strategy().enemyProxy()
			|| Strategy().enemyRush()
			|| Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements)
			|| Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements)
			|| Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0) {
			unlockedType.insert(UnitTypes::Protoss_Zealot);
		}
		else
			unlockedType.erase(UnitTypes::Protoss_Zealot);

		// Check if we should always make Dragoons
		if (Terrain().isIslandMap()
			|| ((BuildOrder().getCurrentBuild() == "PScoutMemes" || BuildOrder().getCurrentBuild() == "PDWEBMemes") && !firstReady())
			|| (currentBuild == "PFFE" && Broodwar->getFrameCount() < 20000))
			unlockedType.erase(UnitTypes::Protoss_Dragoon);
		else 
			unlockedType.insert(UnitTypes::Protoss_Dragoon);		

		// Shield battery for meme builds
		if ((BuildOrder().getCurrentBuild() == "PScoutMemes" || BuildOrder().getCurrentBuild() == "PDWEBMemes") && firstReady() && Units().getSupply() > 220)
			itemQueue[UnitTypes::Protoss_Shield_Battery] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus));

		// Pylon logic
		if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > fastExpand) {
			int providers = max(14, 16 - buildCount(UnitTypes::Protoss_Pylon));
			int count = min(22, Units().getSupply() / providers);

			if (buildCount(UnitTypes::Protoss_Pylon) + buildCount(UnitTypes::Protoss_Nexus) - 1 < count)
				itemQueue[UnitTypes::Protoss_Pylon] = Item(count);

			if (!getOpening && !Buildings().hasPoweredPositions() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 10)
				itemQueue[UnitTypes::Protoss_Pylon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) + 1);
		}

		// Reaction to enemy FFE
		if (Strategy().getEnemyBuild() == "PFFE" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1)
			itemQueue[UnitTypes::Protoss_Nexus] = Item(2);
		
		// If we're not in our opener
		if (!getOpening) {
			gasLimit = INT_MAX;
			if (shouldExpand())
				itemQueue[UnitTypes::Protoss_Nexus] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) + 1);

			if (shouldAddProduction()) {
				if (Terrain().isIslandMap()) {

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
						int roboCount = min(nexusCount - 1, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Robotics_Facility) + 1);
						int stargateCount = min(nexusCount - 1, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) + 1);

						if (Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 150) {
							itemQueue[UnitTypes::Protoss_Stargate] = Item(stargateCount);
							itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(roboCount);
							itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
						}
						itemQueue[UnitTypes::Protoss_Gateway] = Item(nexusCount - 1);
					}

					// PvT island
					else {
						int nexusCount = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
						int stargateCount = min(nexusCount - 1, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) + 1);
						if (Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 150) {
							itemQueue[UnitTypes::Protoss_Stargate] = Item(stargateCount);
							itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(min(1, stargateCount - 2));
							itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
						}
						itemQueue[UnitTypes::Protoss_Gateway] = Item(nexusCount);
					}
				}
				else {

					// Gates - TODO add stargates/robos
					int gateCount = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);
					itemQueue[UnitTypes::Protoss_Gateway] = Item(gateCount);
				}

			}

			if (shouldAddGas())
				itemQueue[UnitTypes::Protoss_Assimilator] = Item(Resources().getGasCount());

			// Air/Ground upgrade buildings
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) >= 3 || Units().getSupply() > 260) {

				if (Terrain().isIslandMap()) {
					itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(2);
					itemQueue[UnitTypes::Protoss_Forge] = Item(1);
				}
				else {
					itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(1);
					itemQueue[UnitTypes::Protoss_Forge] = Item(2);
				}
			}

			// Make sure we have a core
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= 2)
				itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(1);

			// Cannon logic
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) >= 3 + (Players().getNumberTerran() > 0 || Players().getNumberProtoss() > 0)) {
				itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));

				for (auto &station : Stations().getMyStations()) {
					Station s = station.second;

					if (!Pylons().hasPower((TilePosition)s.ResourceCentroid(), UnitTypes::Protoss_Photon_Cannon))
						continue;

					if ((s.BWEMBase()->Location() == mapBWEB.getNaturalTile() || s.BWEMBase()->Location() == mapBWEB.getMainTile()) && Grids().getDefense(TilePosition(s.ResourceCentroid())) <= 0)
						itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1);
					else if (Grids().getDefense(TilePosition(s.ResourceCentroid())) <= 1)
						itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1);
					else if ((Players().getPlayers().size() > 1 || Broodwar->enemy()->getRace() == Races::Zerg) && s.BWEMBase()->Location() != mapBWEB.getMainTile() && s.BWEMBase()->Location() != mapBWEB.getNaturalTile() && Grids().getDefense(TilePosition(s.ResourceCentroid())) <= 3)
						itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1);
				}
			}
		}
		return;
	}
}

#endif // MCRAVE_PROTOSS