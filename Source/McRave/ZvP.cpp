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

        int colonyCount() {
            return vis(Zerg_Creep_Colony) + vis(Zerg_Sunken_Colony);
        }

        int lingsNeeded() {
            if (Strategy::getEnemyBuild() == "2Gate") {
                if (com(Zerg_Sunken_Colony) == 0)
                    return max(10, 2 * Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot));
                return 10;
            }
            if (Strategy::getEnemyBuild() == "1GateCore")
                return 6 * vis(Zerg_Lair);            
            if (Strategy::getEnemyBuild() == "FFE")
                return 6 * (com(Zerg_Lair) > 0 && vis(Zerg_Spire) == 0);
            if (Terrain::foundEnemy() && !Strategy::enemyFastExpand())
                return 6 + (6 * com(Zerg_Lair));
            return 6;
        }

        void defaultZvP() {
            inOpeningBook =                                 true;
            inBookSupply =                                  true;
            wallNat =                                       hatchCount() >= 3;
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
            firstUpgrade =                                  vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
            firstTech =                                     TechTypes::None;
            firstUnit =                                     None;

            armyComposition[Zerg_Drone] =                   0.60;
            armyComposition[Zerg_Zergling] =                0.40;
        }
    }

    void ZvPPoolHatch()
    {
        defaultZvP();

        auto enemy1BaseOnly = (Strategy::getEnemyBuild() == "1GateCore" || Strategy::getEnemyBuild() == "2Gate");

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyProxy())
                currentTransition = "2HatchSpeedling";
            if ((Strategy::getEnemyBuild() == "2Gate" || Strategy::getEnemyBuild() == "1GateCore") && Strategy::enemyFastExpand())
                currentTransition = "3HatchSpeedling";
        }

        // Openers
        if (currentOpener == "Overpool") {                  // 9 Overlord, 9 Pool, 11 Hatch
            transitionReady =                               hatchCount() >= 2;
            droneLimit =                                    Strategy::enemyFastExpand() ? INT_MAX : 12;
            lingLimit =                                     lingsNeeded();
            gasLimit =                                      0;
            wantNatural =                                   !Strategy::enemyProxy();

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);
        }
        if (currentOpener == "4Pool") {                     // 4 Pool, 9 Overlord
            scout =                                         vis(Zerg_Spawning_Pool) > 0 && com(Zerg_Drone) >= 4 && !Terrain::getEnemyStartingPosition().isValid();
            droneLimit =                                    4;
            lingLimit =                                     INT_MAX;
            transitionReady =                               total(Zerg_Zergling) >= 24;
            rush =                                          true;
            wantNatural =                                   !Strategy::enemyProxy();

            buildQueue[Zerg_Hatchery] =                     1;
            buildQueue[Zerg_Spawning_Pool] =                s >= 8;
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
        }
        if (currentOpener == "9Pool") {                     // 9 Pool, 9 Overlord, 12 Hatch
            transitionReady =                               hatchCount() >= 2;
            droneLimit =                                    14;
            lingLimit =                                     lingsNeeded();
            gasLimit =                                      0;
            wantNatural =                                   !Strategy::enemyProxy();

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18 && vis(Zerg_Spawning_Pool) > 0) + (s >= 30);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                lockedTransition =                          atPercent(Zerg_Lair, 0.25) || total(Zerg_Mutalisk) > 0;
                inOpeningBook =                             total(Zerg_Mutalisk) < 9;
                inBookSupply =                              vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;
                firstUpgrade =                              (!Strategy::enemyRush() && (vis(Zerg_Lair) > 0) && (vis(Zerg_Overlord) >= 3)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                hideTech =                                  true;
                droneLimit =                                24;
                lingLimit =                                 lingsNeeded();
                playPassive =                               enemy1BaseOnly && com(Zerg_Mutalisk) == 0;

                auto thirdHatch =                          (total(Zerg_Zergling) == 0 && s >= 50) || total(Zerg_Mutalisk) >= 6 || (Strategy::getEnemyTransition() == "Corsair" && vis(Zerg_Spire) > 0);
                //wantThird =                                !enemy1BaseOnly;

                // Build
                buildQueue[Zerg_Hatchery] =                 2 + thirdHatch;
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 11) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 18);
                buildQueue[Zerg_Lair] =                     (vis(Zerg_Drone) >= 10 && gas(80));
                buildQueue[Zerg_Spire] =                    (s >= 32 && atPercent(Zerg_Lair, 0.95));
                buildQueue[Zerg_Overlord] =                 (Strategy::getEnemyBuild() == "FFE" && atPercent(Zerg_Spire, 0.5)) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50)) + (s >= 80);

                // Composition
                if (Strategy::getEnemyBuild() == "2Gate" && (com(Zerg_Sunken_Colony) == 0 || com(Zerg_Drone) < 9)) {
                    armyComposition[Zerg_Drone] =               0.20;
                    armyComposition[Zerg_Zergling] =            0.80;
                    gasLimit =                                  0;
                }
                else if (Strategy::getEnemyBuild() == "1GateCore" && (com(Zerg_Sunken_Colony) == 0 || com(Zerg_Drone) < 11) && lingSpeed()) {
                    gasLimit =                                  1;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            com(Zerg_Spire) > 0 ? 0.10 : 0.40;
                    armyComposition[Zerg_Mutalisk] =            com(Zerg_Spire) > 0 ? 0.30 : 0.00;
                    gasLimit =                                  gasMax();
                }
            }

            if (currentTransition == "2HatchSpeedling") {
                lockedTransition =                          true;
                inOpeningBook =                             total(Zerg_Zergling) < 90;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                droneLimit =                                total(Zerg_Zergling) >= 12 ? 11 : 9;
                lingLimit =                                 INT_MAX;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;
                wallNat =                                   true;
                pressure =                                  !Strategy::enemyProxy();

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 28 && vis(Zerg_Spawning_Pool) > 0);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32);

                // Composition
                armyComposition[Zerg_Drone] =               0.20;
                armyComposition[Zerg_Zergling] =            0.80;
            }

            if (currentTransition == "3HatchSpeedling") {
                lockedTransition =                          true;
                inOpeningBook =                             total(Zerg_Zergling) < 80 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Reaver) == 0;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                firstUpgrade =                              hatchCount() >= 3 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                droneLimit =                                13;
                lingLimit =                                 hatchCount() >= 3 ? INT_MAX : 0;
                gasLimit =                                  !lingSpeed() ? capGas(100) : 0;
                wallNat =                                   true;
                pressure =                                  true;

                // Build
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
                buildQueue[Zerg_Extractor] =                (s >= 24);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 26);

                // Composition
                armyComposition[Zerg_Drone] =               0.20;
                armyComposition[Zerg_Zergling] =            0.80;
            }
        }
    }
}