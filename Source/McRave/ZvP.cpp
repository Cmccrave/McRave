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

    void ZvPPoolHatch()
    {
        fastExpand =                                        true;
        gasLimit =                                          vis(Zerg_Drone) >= 10 ? gasMax() : 0;
        firstTech =                                         TechTypes::None;
        scout =                                             s >= 20;
        wallNat =                                           !Strategy::enemyFastExpand();
        lingLimit =                                         max(6, 4 * Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot));

        if (currentOpener == "Overpool") {
            droneLimit =                                    Strategy::enemyFastExpand() ? INT_MAX : (com(Zerg_Spawning_Pool) > 0 ? 9 : 11);
            gasTrick =                                      false;
            transitionReady =                               hatchCount() >= 3;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (hatchCount() >= 2);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

            // Army Composition
            armyComposition[Zerg_Drone] =                   0.75;
            armyComposition[Zerg_Zergling] =                0.25;
        }

        // Check if locked transition and ready to transition
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                droneLimit =                                22;
                gasLimit =                                  ((vis(Zerg_Drone) >= 10) && (vis(Zerg_Lair) > 0 || !gas(100))) ? gasMax() : 0;
                inOpeningBook =                             total(Zerg_Mutalisk) < 12;
                firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5;

                buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Lair) > 0 && s >= 50);
                buildQueue[Zerg_Overlord] =                 atPercent(Zerg_Spire, 0.5) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50));
                buildQueue[Zerg_Lair] =                     gas(90);
                buildQueue[Zerg_Spire] =                    com(Zerg_Lair) >= 1;

                // Army Composition
                armyComposition[Zerg_Drone] =               0.50;
                armyComposition[Zerg_Zergling] =            0.10;
                armyComposition[Zerg_Mutalisk] =            0.40;
            }
        }
    }
}