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

        bool enemyMoreLings() {
            return com(Zerg_Zergling) <= Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling);
        }
    }

    void ZvZPoolLair()
    {
        auto stopProducing =                                (atPercent(Zerg_Spire, 0.75) && com(Zerg_Spire) == 0);

        inOpeningBook =                                     Util::getTime() < Time(9, 0);
        gasLimit =                                          vis(Zerg_Lair) == 0 ? 2 : com(Zerg_Drone) - 6;
        scout =                                             false;
        wallNat =                                           hatchCount() >= 3;
        playPassive =                                       com(Zerg_Mutalisk) == 0 && (Strategy::enemyRush() || enemyMoreLings());

        // Openers
        if (currentOpener == "9Pool") {
            transitionReady =                               atPercent(Zerg_Lair, 0.50);
            droneLimit =                                    stopProducing ? 0 : max(10, Players::getCurrentCount(PlayerState::Enemy, Zerg_Drone) + 1) + (2 * (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) > 0));
            lingLimit =                                     stopProducing ? 0 : max(6, Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) - 2);

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Extractor] =                    (s >= 18 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Lair] =                         gas(100) && vis(Zerg_Spawning_Pool) > 0 && vis(Zerg_Zergling) >= 6;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "1HatchMuta") {
                firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 4;

                // Build
                buildQueue[Zerg_Lair] =                     1;
                buildQueue[Zerg_Spire] =                    lingSpeed();
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 56);
                buildQueue[Zerg_Overlord] =                 atPercent(Zerg_Spire, 0.5) ? 4 : 1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);

                // Army Composition
                armyComposition[Zerg_Drone] =               0.25;
                armyComposition[Zerg_Zergling] =            0.35;
                armyComposition[Zerg_Mutalisk] =            0.40;
            }
        }
    }
}