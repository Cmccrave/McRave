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

        int colonyCount() {
            return vis(Zerg_Creep_Colony) + vis(Zerg_Sunken_Colony);
        }

        int lingsNeeded() {
            if (vis(Zerg_Spawning_Pool) == 0 || saveLarva)
                return 0;
            if (Strategy::enemyRush() || Strategy::getEnemyBuild() == "2Rax")
                return 18;
            if (Strategy::enemyProxy() || Strategy::getEnemyOpener() == "8Rax")
                return 10;
            if (Strategy::enemyPressure() || Strategy::enemyRush())
                return 6;
            return 0;
        }
    }

    void ZvTHatchPool()
    {
        fastExpand =                                        !Strategy::enemyRush();
        wallNat =                                           hatchCount() >= 4 || !fastExpand;
        gasLimit =                                          (lingsNeeded() >= 6 && com(Zerg_Zergling) < 6) ? 0 : gasMax();

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyRush() && !Strategy::enemyProxy())
                currentTransition = "2HatchSpeedling";
            if (Strategy::getEnemyTransition() == "WorkerRush") {
                currentBuild = "PoolHatch";
                currentOpener = "12Pool";
                currentTransition = "2HatchMuta";
            }
        }

        // Openers
        if (currentOpener == "12Hatch") {
            transitionReady =                               lingsNeeded() > 0 ? total(Zerg_Zergling) >= 1 : vis(Zerg_Extractor) > 0;
            lingLimit =                                     lingsNeeded();
            droneLimit =                                    lingsNeeded() > 6 ? 9 : 12;
            scout =                                         com(Zerg_Drone) >= 12 || vis(Zerg_Hatchery) >= 2;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24 && com(Zerg_Drone) >= 12);
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
                droneLimit =                                24;
                lingLimit =                                 lingsNeeded();

                playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::enemyPressure() && Util::getTime() < Time(8, 0);
                inOpeningBook =                             total(Zerg_Mutalisk) <= 6;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              total(Zerg_Mutalisk) < 6;

                buildQueue[Zerg_Hatchery] =                 2 + (total(Zerg_Mutalisk) >= 6);
                buildQueue[Zerg_Extractor] =                1 + (atPercent(Zerg_Spire, 0.2 + (0.1 * colonyCount())));
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.25));
                buildQueue[Zerg_Lair] =                     gas(80);
                buildQueue[Zerg_Spire] =                    atPercent(Zerg_Lair, 0.80);

                // Army Composition
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.25;
                armyComposition[Zerg_Mutalisk] = 0.25;
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
                rush =                                      true;

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
        if (currentOpener == "4Pool") {
            scout =                                         vis(Zerg_Spawning_Pool) > 0 && com(Zerg_Drone) >= 4 && !Terrain::getEnemyStartingPosition().isValid();
            droneLimit =                                    4;
            lingLimit =                                     INT_MAX;
            transitionReady =                               total(Zerg_Zergling) >= 24;
            firstUpgrade =                                  UpgradeTypes::Metabolic_Boost;
            rush =                                          true;

            buildQueue[Zerg_Spawning_Pool] =                s >= 8;
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
        }
        if (currentOpener == "9Pool") {
            transitionReady =                               hatchCount() >= 2;
            droneLimit =                                    14;
            lingLimit =                                     6;
            gasLimit =                                      com(Zerg_Drone) >= 10 ? gasMax() : 0;

            firstUpgrade =                                  vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18 && vis(Zerg_Spawning_Pool) > 0) + (s >= 30);
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
        }
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
                droneLimit =                                24;
                lingLimit =                                 lingsNeeded();

                playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::enemyPressure() && Util::getTime() < Time(8, 0);
                inOpeningBook =                             total(Zerg_Mutalisk) <= 6;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;

                buildQueue[Zerg_Hatchery] =                 2 + (total(Zerg_Mutalisk) >= 6);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (atPercent(Zerg_Spire, 0.2 + (0.1 * colonyCount())));
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.25)) + (s >= 80);
                buildQueue[Zerg_Lair] =                     gas(80);
                buildQueue[Zerg_Spire] =                    (s >= 32 && atPercent(Zerg_Lair, 0.80));

                // Composition
                if (Strategy::getEnemyBuild() == "2Rax" && com(Zerg_Sunken_Colony) == 0) {
                    armyComposition[Zerg_Drone] =               0.00;
                    armyComposition[Zerg_Zergling] =            1.00;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            com(Zerg_Spire) > 0 ? 0.10 : 0.40;
                    armyComposition[Zerg_Mutalisk] =            com(Zerg_Spire) > 0 ? 0.30 : 0.00;
                }
            }
            if (currentTransition == "2HatchSpeedling") {
                droneLimit =                                total(Zerg_Zergling) >= 12 ? 11 : 9;
                lingLimit =                                 INT_MAX;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 36;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 28 && vis(Zerg_Spawning_Pool) > 0);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

                // Composition
                armyComposition[Zerg_Drone] =               0.00;
                armyComposition[Zerg_Zergling] =            1.00;
            }
            if (currentTransition == "3HatchSpeedling") {
                droneLimit =                                13;
                lingLimit =                                 hatchCount() >= 3 ? INT_MAX : 0;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 40 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Corsair) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Reaver) == 0;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                rush =                                      true;

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 26);
                buildQueue[Zerg_Extractor] =                (s >= 24);

                // Composition
                armyComposition[Zerg_Drone] =               0.00;
                armyComposition[Zerg_Zergling] =            1.00;
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