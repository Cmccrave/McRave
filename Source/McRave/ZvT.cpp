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

            return 1 + ((value - Broodwar->self()->gas() + onTheWay) / 8);
        }
    }

    void ZvTHatchPool()
    {
        fastExpand =                                        !Strategy::enemyPressure() && !Strategy::enemyRush();
        wallNat =                                           hatchCount() >= 3 || !fastExpand;
        gasLimit =                                          gasMax();

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyRush())
                currentTransition = "2HatchSpeedling";
            if (Strategy::getEnemyTransition() == "WorkerRush") {
                currentBuild = "PoolHatch";
                currentOpener = "12Pool";
                currentTransition = "2HatchMuta";
            }
        }

        // Openers
        if (currentOpener == "12Hatch") {
            transitionReady =                               total(Zerg_Zergling) >= 1;
            lingLimit =                                     (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax") && vis(Zerg_Spawning_Pool) ? 12 : 2 * (1 + Players::getCurrentCount(PlayerState::Enemy, Terran_Marine) + Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture));
            droneLimit =                                    (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax") && vis(Zerg_Spawning_Pool) ? 9 : 24;
            scout =                                         s >= 18;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
            buildQueue[Zerg_Extractor] =                    (vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Spawning_Pool] =                (hatchCount() >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);

            // Composition
            armyComposition[Zerg_Drone] =                   0.60;
            armyComposition[Zerg_Zergling] =                0.40;
        }

        // Builds
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                lockedTransition =                          vis(Zerg_Lair) > 0;
                lingLimit =                                 (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax") && vis(Zerg_Spawning_Pool) ? 24 : 2;
                droneLimit =                                24;

                playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::getEnemyBuild() == "RaxFact" && Util::getTime() < Time(8, 0);
                inOpeningBook =                             total(Zerg_Mutalisk) < 12;
                firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5;

                buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
                buildQueue[Zerg_Extractor] =                1 + (vis(Zerg_Spire) > 0);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.4));
                buildQueue[Zerg_Lair] =                     gas(100);
                buildQueue[Zerg_Spire] =                    com(Zerg_Lair) > 0;

                // Army Composition
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.30;
            }

            if (currentTransition == "2HatchSpeedling") {
                droneLimit =                                total(Zerg_Zergling) >= 12 ? 11 : 9;
                lingLimit =                                 INT_MAX;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 18;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;

                // Build
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 26);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

                // Composition
                armyComposition[Zerg_Drone] =               0.00;
                armyComposition[Zerg_Zergling] =            1.00;
            }
        }
    }

    void ZvTPoolHatch()
    {
        fastExpand =                                        !Strategy::enemyPressure() && !Strategy::enemyRush();
        wallNat =                                           hatchCount() >= 3 || !fastExpand;
        gasLimit =                                          gasMax();

        // Reactions

        // Openers
        if (currentOpener == "12Pool") {
            transitionReady =                           total(Zerg_Zergling) >= 6;
            lingLimit =                                 (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax") && vis(Zerg_Spawning_Pool) ? 12 : 2 * (1 + Players::getCurrentCount(PlayerState::Enemy, Terran_Marine));
            droneLimit =                                (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax") && vis(Zerg_Spawning_Pool) ? 9 : 14;
            scout =                                     s >= 18;

            buildQueue[Zerg_Hatchery] =                 1 + (s >= 24 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                 1 + (s >= 18);
            buildQueue[Zerg_Spawning_Pool] =            s >= 24;
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                lockedTransition =                          vis(Zerg_Lair) > 0;
                lingLimit =                                 (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax") && vis(Zerg_Spawning_Pool) ? 24 : 2;
                droneLimit =                                24;

                playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::enemyPressure() && Util::getTime() < Time(8, 0);
                inOpeningBook =                             total(Zerg_Mutalisk) < 12;
                firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5;

                buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
                buildQueue[Zerg_Extractor] =                1 + (vis(Zerg_Spire) > 0);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.4));
                buildQueue[Zerg_Lair] =                     gas(100);
                buildQueue[Zerg_Spire] =                    com(Zerg_Lair) > 0;

                // Army Composition
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.30;
            }
        }
    }
}







//if (currentTransition == "4BaseGuardian") {
//    droneLimit =                                Strategy::enemyFastExpand() ? INT_MAX : 25;
//    playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::enemyPressure() && Util::getTime() < Time(8, 0);
//    inOpeningBook =                             total(Zerg_Mutalisk) < 12;
//    firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
//    firstUnit =                                 Zerg_Mutalisk;
//    inBookSupply =                              vis(Zerg_Overlord) < 5;
//    lingLimit =                                 Strategy::enemyPressure() ? 6 : 0;

//    buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
//    buildQueue[Zerg_Extractor] =                (vis(Zerg_Spawning_Pool) > 0) + (vis(Zerg_Spire) > 0);
//    buildQueue[Zerg_Overlord] =                 atPercent(Zerg_Spire, 0.5) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.4));
//    buildQueue[Zerg_Lair] =                     gas(90);
//    buildQueue[Zerg_Spire] =                    atPercent(Zerg_Lair, 0.75);
//    buildQueue[Zerg_Queens_Nest] =              total(Zerg_Mutalisk) >= 12;
//    buildQueue[Zerg_Hive] =                     vis(Zerg_Queens_Nest) > 0;
//    buildQueue[Zerg_Greater_Spire] =            vis(Zerg_Hive) > 0;

//    if (vis(Zerg_Greater_Spire) > 0) {
//        unlockedType.insert(Zerg_Guardian);
//        unlockedType.insert(Zerg_Devourer);
//    }

//    // Army Composition
//    armyComposition[Zerg_Drone] = 0.65;
//    armyComposition[Zerg_Zergling] = 0.05;
//    armyComposition[Zerg_Mutalisk] = 0.30;
//}