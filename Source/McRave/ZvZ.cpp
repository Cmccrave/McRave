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

            rush =                                          s <= 60;
            firstUpgrade =                                  vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;

            buildQueue[Zerg_Spawning_Pool] =                s >= 10;
            buildQueue[Zerg_Extractor] =                    s >= 18 && vis(Zerg_Spawning_Pool) > 0;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "1HatchMuta") {
                inOpeningBook =                             total(Zerg_Mutalisk) < 6;
                lockedTransition =                          vis(Zerg_Lair) > 0;
                droneLimit =                                !Strategy::enemyFastExpand() && atPercent(Zerg_Lair, 0.50) ? 12 : 9;

                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                playPassive =                               (com(Zerg_Mutalisk) == 0 && Strategy::enemyRush()) || (Strategy::getEnemyOpener() == "9Pool" && !Strategy::enemyRush() && !Strategy::enemyPressure() && total(Zerg_Mutalisk) == 0);

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
                if (Players::getTotalCount(PlayerState::Enemy, Zerg_Sunken_Colony) >= 2) {
                    lingLimit = 0;
                    droneLimit = INT_MAX;
                    buildQueue[Zerg_Hatchery] =                 1 + (vis(Zerg_Spire) > 0);
                }
                else if (vis(Zerg_Spire) > 0)
                    lingLimit = 16;
                else if (com(Zerg_Lair) > 0)
                    lingLimit = 0;
                else if (vis(Zerg_Lair) > 0)
                    lingLimit = 12;
                else
                    lingLimit = 6;
            }
        }
    }

    void ZvZPoolHatch()
    {
        scout =                                             false || (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0 && vis(Zerg_Spawning_Pool) > 0);
        wallNat =                                           hatchCount() >= 3;
        gasLimit =                                          (lingSpeed() && com(Zerg_Lair) == 0) ? 2 - (vis(Zerg_Lair) > 0 && !atPercent(Zerg_Lair, 0.75)) : gasMax();

        // Openers
        if (currentOpener == "Overpool") {
            transitionReady =                               total(Zerg_Zergling) >= 6 || (Strategy::enemyFastExpand() && com(Zerg_Spawning_Pool) > 0);
            droneLimit =                                    Strategy::enemyFastExpand() ? INT_MAX : 11;
            lingLimit =                                     6;
            gasLimit =                                      com(Zerg_Drone) >= 10 ? gasMax() : 0;
            scout =                                         vis(Zerg_Spawning_Pool) > 0;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);
        }
        if (currentOpener == "9Pool") {
            transitionReady =                               lingSpeed();
            droneLimit =                                    9 - (vis(Zerg_Extractor) > 0) + (vis(Zerg_Overlord) > 1);
            lingLimit =                                     10;

            firstUpgrade =                                  vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Extractor] =                    s >= 18 && vis(Zerg_Spawning_Pool) > 0;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchSpeedling") {
                droneLimit =                                total(Zerg_Zergling) >= 12 ? 11 : 9;
                lingLimit =                                 INT_MAX;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 36;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                rush =                                      true;

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 28 && vis(Zerg_Spawning_Pool) > 0);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

                // Composition
                armyComposition[Zerg_Drone] =               0.00;
                armyComposition[Zerg_Zergling] =            1.00;
            }
        }
    }
}