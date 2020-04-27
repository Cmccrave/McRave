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

    void ZvPPoolHatch()
    {
        fastExpand =                                        Strategy::getEnemyBuild() != "2Gate";
        firstTech =                                         TechTypes::None;
        wallNat =                                           Strategy::getEnemyBuild() == "2Gate" || currentTransition == "2HatchSpeedling" || currentTransition == "3HatchSpeedling" || hatchCount() >= 3;
        playPassive =                                       Strategy::getEnemyBuild() == "2Gate" && Strategy::enemyArrivalTime() < Time(999, 0) && Util::getTime() < Strategy::enemyArrivalTime() - Time(0, 15);

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyProxy() || Strategy::enemyRush())
                currentTransition = "2HatchSpeedling";
            else if (Strategy::getEnemyBuild() == "1GateCore")
                currentTransition = "3HatchSpeedling";
        }

        // Openers
        if (currentOpener == "Overpool") {
            transitionReady =                               total(Zerg_Zergling) >= 6;
            droneLimit =                                    Strategy::enemyFastExpand() ? INT_MAX : 11;
            lingLimit =                                     6;
            gasLimit =                                      com(Zerg_Drone) >= 10 ? gasMax() : 0;
            scout =                                         vis(Zerg_Spawning_Pool) > 0;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 28 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                lockedTransition =                          vis(Zerg_Lair) > 0;
                droneLimit =                                Strategy::getEnemyBuild() == "2Gate" && com(Zerg_Sunken_Colony) < 1 ? 9 : 22;
                lingLimit =                                 Strategy::getEnemyBuild() == "2Gate" && com(Zerg_Sunken_Colony) < 1 ? 12 : 0;
                gasLimit =                                  gasMax();

                inOpeningBook =                             total(Zerg_Mutalisk) < 12;
                firstUpgrade =                              ((vis(Zerg_Lair) > 0) && (vis(Zerg_Overlord) >= 3)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5;

                // Build
                buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
                buildQueue[Zerg_Extractor] =                (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && s >= 50);
                buildQueue[Zerg_Overlord] =                 atPercent(Zerg_Spire, 0.5) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50));
                buildQueue[Zerg_Lair] =                     Strategy::getEnemyBuild() == "2Gate" ? com(Zerg_Sunken_Colony) >= 2 : gas(100);
                buildQueue[Zerg_Spire] =                    (s >= 32 && com(Zerg_Lair) >= 1);

                // Composition
                armyComposition[Zerg_Drone] =               0.25;
                armyComposition[Zerg_Zergling] =            0.50;
                armyComposition[Zerg_Mutalisk] =            0.25;
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
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 26);
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
                inOpeningBook =                             total(Zerg_Zergling) < 60 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Corsair) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Reaver) == 0;
                firstUpgrade =                              UpgradeTypes::Metabolic_Boost;
                firstUnit =                                 UnitTypes::None;
                inBookSupply =                              vis(Zerg_Overlord) < 3;

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