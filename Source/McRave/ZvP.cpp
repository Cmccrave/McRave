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
            if (Strategy::getEnemyBuild() == "2Gate" && colonyCount() < 2)
                return INT_MAX;
            return 6;
        }
    }

    void ZvPPoolHatch()
    {
        fastExpand =                                        Strategy::getEnemyBuild() != "2Gate" || Strategy::enemyFastExpand();
        firstTech =                                         TechTypes::None;
        playPassive =                                       Strategy::getEnemyBuild() == "2Gate" && Strategy::enemyArrivalTime() < Time(999, 0) && Util::getTime() < Strategy::enemyArrivalTime() - Time(0, 15);
        wallNat =                                           hatchCount() >= 3;

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyProxy())
                currentTransition = "2HatchSpeedling";
            else if (Strategy::getEnemyBuild() == "1GateCore")
                currentTransition = "3HatchSpeedling";

            if (Terrain::isShitMap()) {
                currentOpener = "4Pool";
                currentTransition = "2HatchSpeedling";
            }
        }

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

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                lockedTransition =                          vis(Zerg_Lair) > 0;
                droneLimit =                                (Strategy::getEnemyBuild() == "2Gate" && com(Zerg_Sunken_Colony) < 1) ? 16 : 24;
                lingLimit =                                 lingsNeeded();
                gasLimit =                                  (Strategy::enemyRush() && com(Zerg_Drone) < 12) ? 0 : gasMax();

                inOpeningBook =                             total(Zerg_Mutalisk) < 9;
                firstUpgrade =                              (!Strategy::enemyRush() && (vis(Zerg_Lair) > 0) && (vis(Zerg_Overlord) >= 3)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;
                hideTech =                                  true;
                playPassive =                               Strategy::getEnemyBuild() == "2Gate" && com(Zerg_Sunken_Colony) == 0;

                // Build
                buildQueue[Zerg_Hatchery] =                 2 + (total(Zerg_Zergling) == 0 ? s >= 50 : total(Zerg_Mutalisk) >= 6);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (atPercent(Zerg_Spire, 0.33));
                buildQueue[Zerg_Overlord] =                 atPercent(Zerg_Spire, 0.5) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50)) + (s >= 80);
                buildQueue[Zerg_Lair] =                     (vis(Zerg_Drone) >= 12 && gas(80));
                buildQueue[Zerg_Spire] =                    (s >= 32 && atPercent(Zerg_Lair, 0.80));

                // Composition
                if (Strategy::getEnemyBuild() == "2Gate" && com(Zerg_Sunken_Colony) == 0) {
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
                inOpeningBook =                             total(Zerg_Zergling) < 80 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Reaver) == 0;
                firstUpgrade =                              hatchCount() >= 3 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;
                rush =                                      inOpeningBook;

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