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
    }

    void ZvTHatchPool()
    {
        fastExpand =                                        true;
        gasLimit =                                          gasMax();
        firstTech =                                         TechTypes::None;
        scout =                                             false;
        wallNat =                                           hatchCount() >= 3 || Strategy::enemyPressure();

        droneLimit =                                        vis(Zerg_Lair) >= 1 ? 22 : 12;
        lingLimit =                                         2;

        // Check if locked opener
        if (currentOpener == "10Hatch") {
            gasTrick =                                      vis(Zerg_Hatchery) == 1;
            transitionReady =                               vis(Zerg_Overlord) >= 2;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 20) + (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Spawning_Pool] =                hatchCount() >= 2;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Spawning_Pool) > 0 && s >= 18);
        }
        else if (currentOpener == "12Hatch") {
            droneLimit =                                    12 - (vis(Zerg_Hatchery) > 1) - (vis(Zerg_Spawning_Pool) > 0) - (vis(Zerg_Extractor) > 0);
            gasTrick =                                      false;
            transitionReady =                               total(Zerg_Zergling) >= 1;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
            buildQueue[Zerg_Extractor] =                    (vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Spawning_Pool] =                (hatchCount() >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
        }

        // Check if locked transition and ready to transition
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                droneLimit =                                24;
                playPassive =                               com(Zerg_Mutalisk) == 0 && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) && Util::getTime() < Time(8, 0);
                inOpeningBook =                             total(Zerg_Mutalisk) < 12;
                firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5;
                lingLimit =                                 2 + (2 * Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture));

                buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
                buildQueue[Zerg_Extractor] =                (vis(Zerg_Spawning_Pool) > 0) + (com(Zerg_Spire) > 0);
                buildQueue[Zerg_Overlord] =                 atPercent(Zerg_Spire, 0.5) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.4));
                buildQueue[Zerg_Lair] =                     gas(90);
                buildQueue[Zerg_Spire] =                    atPercent(Zerg_Lair, 0.75);

                // Army Composition
                armyComposition[Zerg_Zergling] =            0.10;
                armyComposition[Zerg_Drone] =               0.60;
                armyComposition[Zerg_Mutalisk] =            0.30;
            }

            else if (currentTransition == "3HatchLing") {
                inOpeningBook =                             s < 60 && Broodwar->getFrameCount() < 12000;
                droneLimit =                                13;
                gasLimit =                                  lingSpeed() ? 0 : 3;
                lingLimit =                                 INT_MAX;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                rush =                                      true;

                buildQueue[Zerg_Hatchery] =                 2 + (s >= 26);
                buildQueue[Zerg_Extractor] =                vis(Zerg_Hatchery) >= 3;
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 26);

                // Army Composition
                armyComposition[Zerg_Drone] =               0.25;
                armyComposition[Zerg_Zergling] =            0.75;
            }

            else if (currentTransition == "2HatchHydra") {
                inOpeningBook =                             s < 60;
                lingLimit =                                 6;
                droneLimit =                                14;
                firstUpgrade =                              UpgradeTypes::Grooved_Spines;
                firstUnit =                                 Zerg_Hydralisk;

                buildQueue[Zerg_Extractor] =                1;
                buildQueue[Zerg_Hydralisk_Den] =            gas(50);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32);

                // Army Composition
                armyComposition[Zerg_Drone] =               0.50;
                armyComposition[Zerg_Hydralisk] =           0.50;
            }
        }
    }
}