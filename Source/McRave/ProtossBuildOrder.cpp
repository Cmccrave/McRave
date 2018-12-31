#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss
{
    void opener()
    {
        if (currentBuild == "1GateCore")
            P1GateCore();
        else if (currentBuild == "FFE")
            PFFE();
        else if (currentBuild == "NexusGate")
            PNexusGate();
        else if (currentBuild == "GateNexus")
            PGateNexus();
        else if (currentBuild == "2Gate")
            P2Gate();
    }

    void tech()
    {
        if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) == 0)
            return;

        // Some hardcoded techs based on needing detection or specific build orders
        if (getTech) {

            // If we need observers
            if (Strategy::needDetection() || (!Terrain().isIslandMap() && Players::vP() && techList.find(UnitTypes::Protoss_Observer) == techList.end() && !techList.empty()))
                techUnit = UnitTypes::Protoss_Observer;

            else if (currentTransition == "DoubleExpand" && techList.find(UnitTypes::Protoss_High_Templar) == techList.end())
                techUnit = UnitTypes::Protoss_High_Templar;
            else if (Strategy::getEnemyBuild() == "P4Gate" && techList.find(UnitTypes::Protoss_Dark_Templar) == techList.end() && !Strategy::enemyGasSteal())
                techUnit = UnitTypes::Protoss_Dark_Templar;
            else if (techUnit == UnitTypes::None)
                getNewTech();
        }

        checkNewTech();
        checkAllTech();
        checkExoticTech();
    }

    void situational()
    {
        auto skipFirstTech = (/*currentBuild == "P4Gate" ||*/ currentTransition == "DoubleExpand" || (Strategy::enemyGasSteal() && !Terrain().isNarrowNatural()));

        // Metrics for when to Expand/Add Production/Add Tech
        satVal = Players::vT() ? 2 : 3;
        prodVal = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) + (satVal * skipFirstTech);
        baseVal = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus);
        techVal = techList.size() + skipFirstTech + Players::vT();

        // HACK: Against FFE just add a Nexus
        if (Strategy::getEnemyBuild() == "FFE" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) == 1)
            itemQueue[UnitTypes::Protoss_Nexus] = Item(2);

        // Saturation
        productionSat = (prodVal >= satVal * baseVal);
        techSat = (techVal >= baseVal);

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = UnitTypes::None;

        // If production is saturated and none are idle or we need detection, choose a tech
        if (Terrain().isIslandMap() || (!getOpening && !getTech && !techSat && !Production::hasIdleProduction()))
            getTech = true;
        if (Strategy::needDetection()) {
            techList.insert(UnitTypes::Protoss_Observer);
            unlockedType.insert(UnitTypes::Protoss_Observer);
        }

        // Pylon logic
        if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > int(fastExpand)) {
            int providers = buildCount(UnitTypes::Protoss_Pylon) > 0 ? 14 : 16;
            int count = min(22, Units::getSupply() / providers);
            int offset = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) - 1;
            int total = count - offset;

            if (buildCount(UnitTypes::Protoss_Pylon) < total)
                itemQueue[UnitTypes::Protoss_Pylon] = Item(total);

            if (!getOpening && !Buildings::hasPoweredPositions() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 10)
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
                int gateCount = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);
                itemQueue[UnitTypes::Protoss_Gateway] = Item(gateCount);
            }

            // Adding gas
            if (shouldAddGas())
                itemQueue[UnitTypes::Protoss_Assimilator] = Item(Resources::getGasCount());

            // Adding upgrade buildings
            if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Assimilator) >= 4) {
                itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(1 + (int)Terrain().isIslandMap());
                itemQueue[UnitTypes::Protoss_Forge] = Item(2 - (int)Terrain().isIslandMap());
            }

            // Ensure we build a core outside our opening book
            if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= 2)
                itemQueue[UnitTypes::Protoss_Cybernetics_Core] = Item(1);

            // Defensive Cannons
            if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1 && ((Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) >= 3 + (Players::getNumberTerran() > 0 || Players::getNumberProtoss() > 0)) || (Terrain().isIslandMap() && Players::getNumberZerg() > 0))) {
                itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon));

                for (auto &station : MyStations().getMyStations()) {
                    auto &s = *station.second;

                    if (MyStations().needDefenses(s))
                        itemQueue[UnitTypes::Protoss_Photon_Cannon] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + 1);
                }
            }
        }
        return;
    }

    void unlocks()
    {
        // Leg upgrade check
        auto zealotLegs = Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) > 0
            || (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) > 0 && Units::getSupply() >= 200);

        // Check if we should always make Zealots
        if ((zealotLimit > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot))
            || Strategy::enemyProxy()
            || Strategy::enemyRush()
            || zealotLegs
            || (techUnit == UnitTypes::Protoss_Dark_Templar && Players::vP())) {
            unlockedType.insert(UnitTypes::Protoss_Zealot);
        }
        else
            unlockedType.erase(UnitTypes::Protoss_Zealot);

        // TEST
        if (!techComplete() && techUnit == UnitTypes::Protoss_Dark_Templar && techList.size() == 1 && Players::vP() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Citadel_of_Adun) == 1)
            dragoonLimit = 0;

        // Check if we should always make Dragoons
        if ((Players::vZ() && Broodwar->getFrameCount() > 20000)
            || Units::getEnemyCount(UnitTypes::Zerg_Lurker) > 0
            || dragoonLimit > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon))
            unlockedType.insert(UnitTypes::Protoss_Dragoon);
        else
            unlockedType.erase(UnitTypes::Protoss_Dragoon);
    }

    void island()
    {
        if (shouldAddProduction()) {

            // PvZ island
            if (Players::vZ()) {
                int nexusCount = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
                int roboCount = min(nexusCount - 2, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Robotics_Facility) + 1);
                int stargateCount = min(nexusCount, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Stargate) + 1);

                if (Broodwar->self()->gas() - Production::getReservedGas() - Buildings::getQueuedGas() > 150) {
                    itemQueue[UnitTypes::Protoss_Stargate] = Item(stargateCount);
                    itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(roboCount);
                    itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
                }
                itemQueue[UnitTypes::Protoss_Gateway] = Item(nexusCount);
            }

            // PvP island
            else if (Players::vP()) {
                int nexusCount = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus);
                int gateCount = min(Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) + 1);

                if (Broodwar->self()->gas() - Production::getReservedGas() - Buildings::getQueuedGas() > 200) {
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
                if (Broodwar->self()->gas() - Production::getReservedGas() - Buildings::getQueuedGas() > 150) {
                    itemQueue[UnitTypes::Protoss_Stargate] = Item(stargateCount);
                    itemQueue[UnitTypes::Protoss_Robotics_Facility] = Item(min(1, stargateCount - 2));
                    itemQueue[UnitTypes::Protoss_Robotics_Support_Bay] = Item(1);
                }
                itemQueue[UnitTypes::Protoss_Gateway] = Item(nexusCount);
            }
        }
    }
}