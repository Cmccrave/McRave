#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    namespace {

        bool lingSpeed() {
            return Broodwar->self()->isUpgrading(UpgradeTypes::Metabolic_Boost) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost);
        }

        bool gas(int amount) {
            return Broodwar->self()->gas() >= amount;
        }
    }

    void HatchPool()
    {
        fastExpand =    true;
        gasLimit =      vis(Zerg_Lair) || Broodwar->self()->isUpgrading(UpgradeTypes::Metabolic_Boost) ? 3 : 2;
        firstUpgrade =  UpgradeTypes::Metabolic_Boost;
        firstTech =     TechTypes::None;
        scout =         s >= 22;
        wallNat =       vis(Zerg_Hatchery) >= 2;

        droneLimit =    vis(Zerg_Lair) >= 1 ? 22 : 12;
        lingLimit =     INT_MAX;

        // Reactions
        if (s < 30) {
            if (vis(Zerg_Sunken_Colony) >= 2)
                gasLimit--;
            if (vis(Zerg_Sunken_Colony) >= 4)
                gasLimit--;
        }

        // Check if locked opener
        if (currentOpener == "10Hatch") {
            gasTrick =      vis(Zerg_Hatchery) == 1;

            itemQueue[Zerg_Hatchery] =              Item(1 + (s >= 20) + (vis(Zerg_Overlord) >= 2));
            itemQueue[Zerg_Spawning_Pool] =         Item((vis(Zerg_Hatchery) >= 2));
            itemQueue[Zerg_Overlord] =              Item(1 + (vis(Zerg_Spawning_Pool) > 0 && s >= 18));
            transitionReady =                       vis(Zerg_Overlord) >= 2;
        }
        else if (currentOpener == "12Hatch") {
            gasTrick = false;

            itemQueue[Zerg_Hatchery] =              Item(1 + (s >= 24));
            itemQueue[Zerg_Spawning_Pool] =         Item((vis(Zerg_Hatchery) >= 2));
            itemQueue[Zerg_Overlord] =              Item(1 + (s >= 18));
            transitionReady =                       vis(Zerg_Spawning_Pool) >= 1;
        }

        // Check if locked transition and ready to transition
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                firstUpgrade =  vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit = Zerg_Mutalisk;
                bookSupply = vis(Zerg_Overlord) < 5;

                itemQueue[Zerg_Hatchery] =              Item(2 + (s >= 50));
                itemQueue[Zerg_Extractor] =             Item((vis(Zerg_Hatchery) >= 2) + (vis(Zerg_Hatchery) >= 3));
                itemQueue[Zerg_Overlord] =              Item(1 + (s >= 18) + (s >= 32) + (2 * (s >= 50)));
                itemQueue[Zerg_Lair] =					Item(gas(90));
                itemQueue[Zerg_Spire] =                 Item(com(Zerg_Lair) >= 1);
            }

            else if (currentTransition == "3HatchLing") {
                getOpening =    s < 60 && Broodwar->getFrameCount() < 12000;
                droneLimit =    13;
                gasLimit =      lingSpeed() ? 0 : 3;
                lingLimit =     INT_MAX;
                bookSupply =    vis(Zerg_Overlord) < 3;
                rush =          true;

                itemQueue[Zerg_Hatchery] =              Item(2 + (s >= 26));
                itemQueue[Zerg_Extractor] =             Item(vis(Zerg_Hatchery) >= 3);
                itemQueue[Zerg_Overlord] =              Item(1 + (s >= 18) + (s >= 26));
            }

            else if (currentTransition == "2HatchHydra") {
                getOpening =    s < 60;
                lingLimit =		6;
                droneLimit =	14;
                firstUpgrade =	UpgradeTypes::Grooved_Spines;
                firstUnit =		Zerg_Hydralisk;

                itemQueue[Zerg_Extractor] =				Item(1);
                itemQueue[Zerg_Hydralisk_Den] =			Item(gas(50));
                itemQueue[Zerg_Overlord] =              Item(1 + (s >= 18) + (s >= 32));
            }
        }
    }

    void PoolHatch()
    {

    }

    void PoolLair()
    {
        getOpening =    s < 70;
        gasLimit =      INT_MAX;
        firstUpgrade =  UpgradeTypes::Metabolic_Boost;
        firstTech =     TechTypes::None;
        scout =         false;
        droneLimit =    10;
        lingLimit =     12;
        wallNat =       vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) >= 2;
        playPassive =   com(Zerg_Mutalisk) == 0;

        if (currentOpener == "9Pool") {
            itemQueue[Zerg_Spawning_Pool] =			Item(s >= 18);
            itemQueue[Zerg_Extractor] =				Item((s >= 18 && vis(Zerg_Spawning_Pool) > 0));
            itemQueue[Zerg_Lair] =					Item(gas(100) && vis(Zerg_Spawning_Pool) > 0);
            transitionReady =                       vis(Zerg_Lair) > 0;
        }

        if (currentTransition == "1HatchMuta") {
            firstUnit =                             Zerg_Mutalisk;
            bookSupply =                            vis(Zerg_Overlord) < 3;

            itemQueue[Zerg_Spire] =					Item(lingSpeed() && gas(100));
            itemQueue[Zerg_Hatchery] =	            Item(1 + (s >= 56));
            itemQueue[Zerg_Overlord] =              Item(1 + (vis(Zerg_Extractor) >= 1) + (s >= 32));
        }
    }
}