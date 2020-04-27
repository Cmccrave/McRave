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

        int gasMax() {
            return Resources::getGasCount() * 3;
        }

        int hatchCount() {
            return vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);
        }

        int capGas(int value) {

            auto onTheWay = 0;
            for (auto &w : Units::getUnits(PlayerState::Self)) {
                auto &worker = *w;
                if (worker.unit()->isCarryingGas())
                    onTheWay+=8;
            }

            return (value - Broodwar->self()->gas() + onTheWay) / 8;
        }
    }

    void ZvZPoolLair()
    {
        inOpeningBook =                                     s < 60;
        scout =                                             false;
        wallNat =                                           hatchCount() >= 3;
        gasLimit =                                          (lingSpeed() && com(Zerg_Lair) == 0) ? 2 - (vis(Zerg_Lair) > 0 && !atPercent(Zerg_Lair, 0.75)) : gasMax();

        // Openers
        if (currentOpener == "9Pool") {
            transitionReady =                               lingSpeed();
            droneLimit =                                    9 - (vis(Zerg_Extractor) > 0) + (vis(Zerg_Overlord) > 1);
            lingLimit =                                     10;

            firstUpgrade =                                  vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Extractor] =                    s >= 18 && vis(Zerg_Spawning_Pool) > 0;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }
        else if (currentOpener == "5Pool") {
            transitionReady =                               lingSpeed();
            droneLimit =                                    total(Zerg_Zergling) >= 30 ? 9 : 5 - (vis(Zerg_Spawning_Pool) > 0);
            lingLimit =                                     total(Zerg_Zergling) >= 30 ? 0 : INT_MAX;

            rush =                                          true;
            firstUpgrade =                                  vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;

            buildQueue[Zerg_Spawning_Pool] =                s >= 10;
            buildQueue[Zerg_Extractor] =                    s >= 18 && vis(Zerg_Spawning_Pool) > 0;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "1HatchMuta") {
                lockedTransition =                          vis(Zerg_Lair) > 0;
                droneLimit =                                !Strategy::enemyFastExpand() && com(Zerg_Lair) > 0 ? 12 : 9;

                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::enemyRush();

                // Build
                buildQueue[Zerg_Lair] =                     gas(100) && vis(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) >= 6 && vis(Zerg_Drone) >= 8;
                buildQueue[Zerg_Spire] =                    lingSpeed() && atPercent(Zerg_Lair, 0.8) && vis(Zerg_Drone) >= 9;
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 50 || (total(Zerg_Mutalisk) >= 3 && Broodwar->self()->minerals() >= 350));
                buildQueue[Zerg_Overlord] =                 1 + (vis(Zerg_Extractor) >= 1) + (s >= 30);

                // Army Composition
                if (com(Zerg_Spire)) {
                    armyComposition[Zerg_Drone] =               0.40;
                    armyComposition[Zerg_Zergling] =            0.10;
                    armyComposition[Zerg_Mutalisk] =            0.50;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.50;
                    armyComposition[Zerg_Zergling] =            0.50;
                }

                // Lings
                if (com(Zerg_Lair) > 0)
                    lingLimit = 0;
                else if (vis(Zerg_Lair) > 0)
                    lingLimit = 12;
                else
                    lingLimit = 6;
            }
        }
    }
}