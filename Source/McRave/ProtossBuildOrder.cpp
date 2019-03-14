#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss
{
    void opener()
    {
        if (Players::vT()) {
            if (currentBuild == "1GateCore")
                PvT1GateCore();
            else if (currentBuild == "NexusGate")
                PvTNexusGate();
            else if (currentBuild == "GateNexus")
                PvTGateNexus();
            else if (currentBuild == "2Gate")
                PvT2Gate();
        }
        else if (Players::vP()) {
            if (currentBuild == "1GateCore")
                PvP1GateCore();
            else if (currentBuild == "2Gate")
                PvP2Gate();
        }
        else if (Players::vZ() || Players::getNumberRandom() > 0) {
            if (currentBuild == "1GateCore")
                PvZ1GateCore();
            else if (currentBuild == "FFE")
                PvZFFE();
            else if (currentBuild == "2Gate")
                PvZ2Gate();
        }
    }

    void tech()
    {
        if (com(Protoss_Cybernetics_Core) == 0)
            return;

        if (firstUnit != None && !isTechUnit(firstUnit))
            techUnit = firstUnit;

        // Some hardcoded techs based on needing detection or specific build orders
        else if (getTech) {

            // If we need observers
            if (Strategy::needDetection() || (Players::vP() && techList.find(Protoss_Observer) == techList.end() && !techList.empty()))
                techUnit = Protoss_Observer;
            else if (currentTransition == "DoubleExpand" && !isTechUnit(Protoss_High_Templar))
                techUnit = Protoss_High_Templar;
            else if (Strategy::getEnemyBuild() == "4Gate" && !isTechUnit(Protoss_Dark_Templar) && !Strategy::enemyGasSteal())
                techUnit = Protoss_Dark_Templar;
            else if (techUnit == None)
                getNewTech();
        }

        checkNewTech();
        checkAllTech();
        checkExoticTech();
    }

    void situational()
    {
        auto skipFirstTech = int(currentTransition == "4Gate" || (Strategy::enemyGasSteal() && !Terrain::isNarrowNatural()));

        // Metrics for when to Expand/Add Production/Add Tech
        satVal = Players::vT() ? 2 : 3;
        prodVal = com(Protoss_Gateway) + (satVal * skipFirstTech);
        baseVal = com(Protoss_Nexus);
        techVal = techList.size() + skipFirstTech + Players::vT();

        // Against FFE add a Nexus for every 2 cannons we see
        if (Strategy::getEnemyBuild() == "FFE" && getOpening) {
            auto cannonCount = Units::getEnemyCount(Protoss_Photon_Cannon);

            if (cannonCount <= 2)
                itemQueue[Protoss_Nexus] = Item(2);
            else
                itemQueue[Protoss_Nexus] = Item(3);
        }

        // Saturation
        productionSat = (prodVal >= satVal * baseVal);
        techSat = (techVal >= baseVal) && (!delayFirstTech || productionSat);

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = None;

        // If production is saturated and none are idle or we need detection, choose a tech
        if (Terrain::isIslandMap() || (!getOpening && !getTech && !techSat) || Strategy::needDetection()) {
            if (desiredDetection == Protoss_Observer || !Strategy::needDetection())
                getTech = true;
            else {
                wallNat = vis(Protoss_Forge) > 0;
                itemQueue[Protoss_Forge] = Item(1);
                itemQueue[Protoss_Photon_Cannon] = Item(com(Protoss_Forge) * 2);
            }
        }

        // If we don't want an assimilator, set gas count to 0
        if (buildCount(Protoss_Assimilator) == 0)
            gasLimit = 0;

        // Pylon logic
        if (vis(Protoss_Pylon) > int(fastExpand)) {
            int providers = buildCount(Protoss_Pylon) > 0 ? 14 : 16;
            int count = min(22, Units::getSupply() / providers);
            int offset = com(Protoss_Nexus) - 1;
            int total = count - offset;

            if (buildCount(Protoss_Pylon) < total)
                itemQueue[Protoss_Pylon] = Item(total);

            if (!getOpening && !Buildings::hasPoweredPositions() && vis(Protoss_Pylon) > 10)
                itemQueue[Protoss_Pylon] = Item(vis(Protoss_Pylon) + 1);
        }

        // If we're not in our opener
        if (!getOpening) {
            gasLimit = INT_MAX;

            if (Units::getEnemyCount(Protoss_Observer) > 0 && isTechUnit(Protoss_Dark_Templar)) {
                techList.erase(Protoss_Dark_Templar);
                unlockedType.erase(Protoss_Dark_Templar);
                techList.insert(Protoss_High_Templar);
                unlockedType.insert(Protoss_High_Templar);
            }

            // Adding bases
            if (shouldExpand())
                itemQueue[Protoss_Nexus] = Item(com(Protoss_Nexus) + 1);

            // Adding production
            if (shouldAddProduction()) {
                int gateCount = min(com(Protoss_Nexus) * 3, vis(Protoss_Gateway) + 1) - (int(isUnitUnlocked(Protoss_Carrier)) * 2);
                itemQueue[Protoss_Gateway] = Item(gateCount);
            }

            // Adding gas
            if (shouldAddGas())
                itemQueue[Protoss_Assimilator] = Item(Resources::getGasCount());

            // Adding upgrade buildings
            if (com(Protoss_Assimilator) >= 4) {
                itemQueue[Protoss_Cybernetics_Core] = Item(1 + (int)Terrain::isIslandMap());
                itemQueue[Protoss_Forge] = Item(2 - (int)Terrain::isIslandMap());
            }
            if (com(Protoss_Nexus) >= 2 && Players::vZ())
                itemQueue[Protoss_Forge] = Item(1);

            // Ensure we build a core outside our opening book
            if (com(Protoss_Gateway) >= 2)
                itemQueue[Protoss_Cybernetics_Core] = Item(1);

            // Defensive Cannons
            if (com(Protoss_Forge) >= 1 && ((vis(Protoss_Nexus) >= 3 + (Players::getNumberTerran() > 0 || Players::getNumberProtoss() > 0)) || (Terrain::isIslandMap() && Players::getNumberZerg() > 0))) {
                itemQueue[Protoss_Photon_Cannon] = Item(vis(Protoss_Photon_Cannon));

                for (auto &station : Stations::getMyStations()) {
                    auto &s = *station.second;

                    if (Stations::needDefenses(s))
                        itemQueue[Protoss_Photon_Cannon] = Item(vis(Protoss_Photon_Cannon) + 1);
                }
            }
        }
    }

    void unlocks()
    {
        // Leg upgrade check
        auto zealotLegs = Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) > 0
            || (com(Protoss_Citadel_of_Adun) > 0 && Units::getSupply() >= 200);

        // Check if we should always make Zealots
        if ((zealotLimit > vis(Protoss_Zealot))
            || zealotLegs
            || (techUnit == Protoss_Dark_Templar && Players::vP()))
            unlockedType.insert(Protoss_Zealot);
        else
            unlockedType.erase(Protoss_Zealot);

        // Check if we should always make Dragoons
        if ((Players::vZ() && Broodwar->getFrameCount() > 20000)
            || Units::getEnemyCount(Zerg_Lurker) > 0
            || dragoonLimit > vis(Protoss_Dragoon))
            unlockedType.insert(Protoss_Dragoon);
        else
            unlockedType.erase(Protoss_Dragoon);
    }

    void island()
    {
        // DISABLED: Islands not being played until other bugs are fixed
        if (shouldAddProduction()) {

            // PvZ island
            if (Players::vZ()) {
                int nexusCount = vis(Protoss_Nexus);
                int roboCount = min(nexusCount - 2, vis(Protoss_Robotics_Facility) + 1);
                int stargateCount = min(nexusCount, vis(Protoss_Stargate) + 1);

                if (Broodwar->self()->gas() - Production::getReservedGas() - Buildings::getQueuedGas() > 150) {
                    itemQueue[Protoss_Stargate] = Item(stargateCount);
                    itemQueue[Protoss_Robotics_Facility] = Item(roboCount);
                    itemQueue[Protoss_Robotics_Support_Bay] = Item(1);
                }
                itemQueue[Protoss_Gateway] = Item(nexusCount);
            }

            // PvP island
            else if (Players::vP()) {
                int nexusCount = vis(Protoss_Nexus);
                int gateCount = min(com(Protoss_Nexus) * 3, vis(Protoss_Gateway) + 1);

                if (Broodwar->self()->gas() - Production::getReservedGas() - Buildings::getQueuedGas() > 200) {
                    itemQueue[Protoss_Robotics_Support_Bay] = Item(1);
                    if (techList.find(Protoss_Scout) != techList.end() || techList.find(Protoss_Carrier) != techList.end())
                        itemQueue[Protoss_Stargate] = Item(nexusCount);
                }

                itemQueue[Protoss_Gateway] = Item(gateCount);
            }

            // PvT island
            else {
                int nexusCount = vis(Protoss_Nexus);
                int stargateCount = min(nexusCount + 1, vis(Protoss_Stargate) + 1);
                if (Broodwar->self()->gas() - Production::getReservedGas() - Buildings::getQueuedGas() > 150) {
                    itemQueue[Protoss_Stargate] = Item(stargateCount);
                    itemQueue[Protoss_Robotics_Facility] = Item(min(1, stargateCount - 2));
                    itemQueue[Protoss_Robotics_Support_Bay] = Item(1);
                }
                itemQueue[Protoss_Gateway] = Item(nexusCount);
            }
        }
    }
}