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
        getNewTech();
        checkNewTech();
        checkAllTech();
        checkExoticTech();
    }

    void situational()
    {
        // Adding hatcheries when needed
        if (shouldExpand() || shouldAddProduction())
            itemQueue[Zerg_Hatchery] = Item(vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + 1);

        // Gas Trick
        if (gasTrick) {
            if (Units::getSupply() == 18 && vis(Zerg_Drone) == 9) {
                if (Broodwar->self()->minerals() >= 80)
                    itemQueue[Zerg_Extractor] = Item(1);
                if (vis(Zerg_Extractor) > 0)
                    itemQueue[Zerg_Extractor] = Item(0);
            }
        }

        // When to tech
        if (Broodwar->getFrameCount() > 5000 && vis(Zerg_Hatchery) > (1 + (int)techList.size()) * 2)
            getTech = true;
        if (techComplete())
            techUnit = UnitTypes::None;

        // Adding Sunkens/Spores
        auto myStrength = Players::getStrength(PlayerState::Self);
        auto enemyStrength = Players::getStrength(PlayerState::Enemy);
        auto sunkenCount = int(enemyStrength.groundToGround / (myStrength.groundDefense + myStrength.groundToGround));
        auto sporeCount = int((enemyStrength.airToAir + enemyStrength.airToGround)) / (myStrength.airDefense + myStrength.groundToAir + myStrength.airToAir);
        itemQueue[Zerg_Creep_Colony] = Item(sunkenCount);

        // Overlord
        if (!bookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(22, Units::getSupply() / providers);
            if (vis(Zerg_Overlord) >= 3)
                itemQueue[Zerg_Overlord] = Item(count);
        }

        unlockedType.insert(Zerg_Drone);
        unlockedType.insert(Zerg_Overlord);

        if (!getOpening) {
            gasLimit = INT_MAX;
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            if (shouldAddGas())
                itemQueue[Zerg_Extractor] = Item(gasCount);

            if (Units::getSupply() >= 100)
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