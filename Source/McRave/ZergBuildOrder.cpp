#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void opener()
    {
        if (currentBuild == "HatchPool")
            HatchPool();
        else if (currentBuild == "PoolHatch")
            PoolHatch();
        else if (currentBuild == "PoolLair")
            PoolLair();
    }

    void tech()
    {
        if (getTech)
            getNewTech();
        else if (firstUnit != None && !isTechUnit(firstUnit))
            techUnit = firstUnit;

        checkNewTech();
        checkAllTech();
        checkExoticTech();
    }

    void situational()
    {
        auto baseVal = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);
        auto techVal = int(techList.size()) + 1;

        // Gas Trick
        if (gasTrick) {
            if (Players::getSupply(PlayerState::Self) == 18 && vis(Zerg_Drone) == 9) {
                if (Broodwar->self()->minerals() >= 80)
                    itemQueue[Zerg_Extractor] = Item(1);
                if (vis(Zerg_Extractor) > 0)
                    itemQueue[Zerg_Extractor] = Item(0);
            }
        }

        // When to tech
        if (!getOpening && baseVal > techVal)
            getTech = true;
        if (techComplete())
            techUnit = UnitTypes::None;

        // Adding Sunkens/Spores
        if (vis(Zerg_Drone) >= 8) {
            auto sunkenCount = 0;
            if (Players::vP())
                sunkenCount = int(1 + Units::getEnemyCount(Protoss_Zealot) + Units::getEnemyCount(Protoss_Dragoon)) / 2;
            else if (Players::vT())
                sunkenCount = int(Units::getEnemyCount(Terran_Vulture) > 0) ? 2 : 0;
            itemQueue[Zerg_Creep_Colony] = Item(sunkenCount);
        }

        // Overlord
        if (!bookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(22, Players::getSupply(PlayerState::Self) / providers);
            if (vis(Zerg_Overlord) >= 3)
                itemQueue[Zerg_Overlord] = Item(count);
        }

        unlockedType.insert(Zerg_Drone);
        unlockedType.insert(Zerg_Overlord);

        if (!getOpening) {
            gasLimit = INT_MAX;
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            // Adding hatcheries when needed
            if (shouldExpand() || shouldAddProduction())
                itemQueue[Zerg_Hatchery] = Item(vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + 1);

            if (shouldAddGas())
                itemQueue[Zerg_Extractor] = Item(gasCount);

            if (Players::getSupply(PlayerState::Self) >= 100)
                itemQueue[Zerg_Evolution_Chamber] = Item(2);
        }
    }

    void unlocks()
    {
        if (getOpening) {
            if (droneLimit > vis(Zerg_Drone))
                unlockedType.insert(Zerg_Drone);
            else
                unlockedType.erase(Zerg_Drone);

            if (lingLimit > vis(Zerg_Zergling))
                unlockedType.insert(Zerg_Zergling);
            else
                unlockedType.erase(Zerg_Zergling);
        }
        else {
            unlockedType.insert(Zerg_Drone);
            unlockedType.insert(Zerg_Zergling);
        }
    }
}