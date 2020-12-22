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
                if (worker.unit()->isCarryingGas() || worker.unit()->getOrder() == Orders::WaitForGas || worker.unit()->getOrder() == Orders::MoveToGas)
                    onTheWay+=8;
            }

            return int(round(double(value - Broodwar->self()->gas() + onTheWay) / 8.0));
        }

        int colonyCount() {
            return vis(Zerg_Creep_Colony) + vis(Zerg_Sunken_Colony);
        }

        int lingsNeeded() {
            if (Strategy::getEnemyBuild() == "2Rax") {
                if (vis(Zerg_Sunken_Colony) == 0)
                    return int(max(6.0, 1.5 * Players::getVisibleCount(PlayerState::Enemy, Terran_Marine)));
                return 6;
            }
            if (Strategy::enemyRush())
                return 18;
            if (Strategy::enemyProxy() || Strategy::getEnemyOpener() == "8Rax" || Strategy::getEnemyTransition() == "WorkerRush")
                return 10;
            if (Strategy::getEnemyBuild() == "RaxFact" || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0)
                return Util::getTime() > Time(3, 00) ? 4 : 0;
            if (Strategy::enemyPressure() || Strategy::getEnemyBuild() == "2Rax")
                return 6;
            return 4;
        }

        void defaultZvT() {
            inOpeningBook =                                 true;
            inBookSupply =                                  true;
            wallNat =                                       hatchCount() >= 4;
            wallMain =                                      false;
            scout =                                         vis(Zerg_Spawning_Pool) > 0;
            wantNatural =                                   true;
            wantThird =                                     true;
            proxy =                                         false;
            hideTech =                                      false;
            playPassive =                                   false;
            rush =                                          false;
            pressure =                                      false;
            cutWorkers =                                    false;
            transitionReady =                               false;

            gasLimit =                                      gasMax();
            lingLimit =                                     lingsNeeded();
            droneLimit =                                    INT_MAX;

            desiredDetection =                              Zerg_Overlord;
            firstUpgrade =                                  vis(Zerg_Zergling) >= 8 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
            firstTech =                                     TechTypes::None;
            firstUnit =                                     None;

            armyComposition[Zerg_Drone] =                   0.60;
            armyComposition[Zerg_Zergling] =                0.40;
        }
    }

    void ZvTHatchPool()
    {
        defaultZvT();

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyRush() || Strategy::enemyProxy())
                currentTransition = "2HatchSpeedling";
            if (Strategy::getEnemyTransition() == "WorkerRush") {
                currentBuild = "PoolHatch";
                currentOpener = "12Pool";
                currentTransition = "2HatchMuta";
            }
        }

        // Openers
        if (currentOpener == "12Hatch") {
            transitionReady =                               Strategy::getEnemyOpener() == "8Rax" ? total(Zerg_Zergling) >= 6 : vis(Zerg_Extractor) > 0;
            lingLimit =                                     lingsNeeded();
            droneLimit =                                    14;
            gasLimit =                                      ((!Strategy::enemyProxy() || com(Zerg_Zergling) >= 6) && !lingSpeed()) ? capGas(100) : 0;
            scout =                                         com(Zerg_Drone) >= 12 || vis(Zerg_Hatchery) >= 2;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
            buildQueue[Zerg_Extractor] =                    Strategy::getEnemyOpener() == "8Rax" ? 0 : (vis(Zerg_Spawning_Pool) > 0);
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
                droneLimit =                                28;
                lingLimit =                                 lingsNeeded();
                gasLimit =                                  gasMax();

                playPassive =                               com(Zerg_Mutalisk) == 0 && Strategy::enemyPressure() && Util::getTime() < Time(8, 0);
                inOpeningBook =                             total(Zerg_Mutalisk) <= 6;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              total(Zerg_Mutalisk) < 6;

                buildQueue[Zerg_Hatchery] =                 2 + (total(Zerg_Mutalisk) >= 9);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (atPercent(Zerg_Spire, 0.1 + (0.05 * colonyCount())));
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.25));
                buildQueue[Zerg_Lair] =                     (vis(Zerg_Drone) >= 10 && gas(80));
                buildQueue[Zerg_Spire] =                    atPercent(Zerg_Lair, 0.80);

                // Composition
                if (Strategy::getEnemyBuild() == "2Rax" && !Strategy::enemyFastExpand() && (com(Zerg_Sunken_Colony) == 0 || com(Zerg_Drone) < 9)) {
                    armyComposition[Zerg_Drone] =               0.20;
                    armyComposition[Zerg_Zergling] =            0.80;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            com(Zerg_Spire) > 0 ? 0.00 : 0.40;
                    armyComposition[Zerg_Mutalisk] =            com(Zerg_Spire) > 0 ? 0.30 : 0.00;
                }
            }

            if (currentTransition == "2HatchSpeedling") {
                droneLimit =                                total(Zerg_Zergling) >= 12 ? 11 : 9;
                lingLimit =                                 INT_MAX;
                gasLimit =                                  ((!Strategy::enemyProxy() || com(Zerg_Zergling) >= 6) && !lingSpeed()) ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 36;
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
        defaultZvT();
        wallNat =                                           hatchCount() >= 3 || !wantNatural;
        gasLimit =                                          gasMax();

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyProxy())
                currentTransition = "2HatchSpeedling";
        }

        // Openers        
        if (currentOpener == "Overpool") {
            transitionReady =                               total(Zerg_Zergling) >= 2 || (Strategy::enemyFastExpand() && com(Zerg_Spawning_Pool) > 0);
            droneLimit =                                    Strategy::enemyFastExpand() ? INT_MAX : 14;
            lingLimit =                                     lingsNeeded();
            gasLimit =                                      com(Zerg_Drone) >= 11 ? gasMax() : 0;
            scout =                                         vis(Zerg_Spawning_Pool) > 0;

            buildQueue[Zerg_Hatchery] =                     1 + (Strategy::enemyProxy() ? total(Zerg_Zergling) >= 6 : s >= 22 && vis(Zerg_Spawning_Pool) > 0);
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
        if (currentOpener == "9Pool") {                     // Actually 7Pool but again whatever

        }
        if (currentOpener == "12Pool") {                    // Actually 13Pool but whatever
            transitionReady =                               total(Zerg_Zergling) >= 4;
            lingLimit =                                     4;
            gasLimit =                                      com(Zerg_Drone) >= 11 ? gasMax() : 0;
            droneLimit =                                    13;
            scout =                                         false;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 26 && vis(Zerg_Extractor) > 0);
            buildQueue[Zerg_Extractor] =                    (s >= 24 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
            buildQueue[Zerg_Spawning_Pool] =                s >= 26;
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                lockedTransition =                          vis(Zerg_Lair) > 0;
                droneLimit =                                24;
                lingLimit =                                 lingsNeeded();
                gasLimit =                                  com(Zerg_Drone) >= 11 ? gasMax() : 0;

                playPassive =                               (com(Zerg_Mutalisk) == 0 && Strategy::enemyPressure() && Util::getTime() < Time(8, 0)) || (com(Zerg_Mutalisk) == 0 && Strategy::getEnemyBuild() == "2Rax" && !Strategy::enemyFastExpand());
                inOpeningBook =                             total(Zerg_Mutalisk) <= 6;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;

                buildQueue[Zerg_Hatchery] =                 2 + (total(Zerg_Mutalisk) >= 9);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (atPercent(Zerg_Spire, 0.30));
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.25)) + (s >= 80);
                buildQueue[Zerg_Lair] =                     (vis(Zerg_Drone) >= 10 && gas(80));
                buildQueue[Zerg_Spire] =                    (s >= 32 && atPercent(Zerg_Lair, 0.80));

                // Composition
                if (Strategy::getEnemyBuild() == "2Rax" && !Strategy::enemyFastExpand() && (com(Zerg_Sunken_Colony) == 0 || com(Zerg_Drone) < 9)) {
                    armyComposition[Zerg_Drone] =               0.20;
                    armyComposition[Zerg_Zergling] =            0.80;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            vis(Zerg_Spire) > 0 ? 0.10 : 0.40;
                    armyComposition[Zerg_Mutalisk] =            vis(Zerg_Spire) > 0 ? 0.30 : 0.00;
                }
            }
            if (currentTransition == "2HatchSpeedling") {
                droneLimit =                                total(Zerg_Zergling) >= 12 ? 11 : 9;
                lingLimit =                                 INT_MAX;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 36 && Players::getTotalCount(PlayerState::Enemy, Zerg_Sunken_Colony) < 2;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 28 && vis(Zerg_Spawning_Pool) > 0);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

                // Composition
                armyComposition[Zerg_Drone] =               0.20;
                armyComposition[Zerg_Zergling] =            0.80;
            }
            if (currentTransition == "3HatchSpeedling") {
                droneLimit =                                13;
                lingLimit =                                 hatchCount() >= 3 ? INT_MAX : 0;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;

                wallNat =                                   true;
                inOpeningBook =                             total(Zerg_Zergling) < 80;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                rush =                                      true;

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 26);
                buildQueue[Zerg_Extractor] =                (s >= 24);

                // Composition
                armyComposition[Zerg_Drone] =               0.20;
                armyComposition[Zerg_Zergling] =            0.80;
            }
        }
    }
}