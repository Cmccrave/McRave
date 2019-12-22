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
    }

    void HatchPool()
    {
        fastExpand =                                        true;
        gasLimit =                                          vis(Zerg_Lair) || Broodwar->self()->isUpgrading(UpgradeTypes::Metabolic_Boost) ? 3 : 2;
        firstUpgrade =                                      UpgradeTypes::Metabolic_Boost;
        firstTech =                                         TechTypes::None;
        scout =                                             s >= 22;
        wallNat =                                           (vis(Zerg_Hatchery) + vis(Zerg_Lair)) >= 2;

        droneLimit =                                        vis(Zerg_Lair) >= 1 ? 22 : 12;
        lingLimit =                                         INT_MAX;

        // Reactions
        if (s < 30) {
            if (vis(Zerg_Sunken_Colony) >= 2)
                gasLimit--;
            if (vis(Zerg_Sunken_Colony) >= 4)
                gasLimit--;
        }

        // Check if locked opener
        if (currentOpener == "10Hatch") {
            gasTrick =                                      vis(Zerg_Hatchery) == 1;
            transitionReady =                               vis(Zerg_Overlord) >= 2;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 20) + (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Hatchery) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Spawning_Pool) > 0 && s >= 18);
        }
        else if (currentOpener == "12Hatch") {
            gasTrick =                                      false;
            transitionReady =                               vis(Zerg_Spawning_Pool) >= 1;

            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Hatchery) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
        }

        // Check if locked transition and ready to transition
        if (transitionReady) {
            if (currentTransition == "2HatchMuta") {
                playPassive =                               com(Zerg_Mutalisk) == 0 && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) && Util::getTime() < Time(8,0); 
                inOpeningBook =                             s < 60;
                firstUpgrade =                              vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 5;
                lingLimit =                                 atPercent(Zerg_Lair, 0.5) ? 0 : lingLimit;

                buildQueue[Zerg_Hatchery] =                 2 + (s >= 50);
                buildQueue[Zerg_Extractor] =                (vis(Zerg_Hatchery) >= 2) + ((vis(Zerg_Hatchery) + vis(Zerg_Lair)) >= 3);
                buildQueue[Zerg_Overlord] =                 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50));
                buildQueue[Zerg_Lair] =                     gas(90);
                buildQueue[Zerg_Spire] =                    com(Zerg_Lair) >= 1;

                // Army Composition
                armyComposition[Zerg_Drone] =               0.25;
                armyComposition[Zerg_Zergling] =            0.10;
                armyComposition[Zerg_Mutalisk] =            0.55;
                armyComposition[Zerg_Scourge] =             0.10;
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
                armyComposition[Zerg_Zergling] =            0.00;
                armyComposition[Zerg_Hydralisk] =           0.50;
            }
        }
    }

    void PoolHatch()
    {
        inOpeningBook =                                     s < 60 && Util::getTime() < Time(8, 0);
        gasLimit =                                          lingSpeed() ? 0 : 3 - gas(80) - gas(88);
        firstUpgrade =                                      UpgradeTypes::Metabolic_Boost;
        firstTech =                                         TechTypes::None;
        scout =                                             s >= 22;
        droneLimit =                                        10;
        lingLimit =                                         12;
        wallNat =                                           vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) >= 2;
        playPassive =                                       false;

        if (currentOpener == "9Pool10Hatch") {
            gasTrick =                                      vis(Zerg_Spawning_Pool) == 1;
            transitionReady =                               com(Zerg_Hatchery) >= 2;

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 20);
            buildQueue[Zerg_Extractor] =                    (s >= 18 && vis(Zerg_Hatchery) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 20 && vis(Zerg_Spawning_Pool) >= 1) + (s >= 32);
        }

        if (transitionReady) {
            if (currentTransition == "2HatchLing") {                
                inOpeningBook =                             s < 44 && Util::getTime() < Time(6, 0);
                lingLimit =                                 50;

                armyComposition[Zerg_Drone] =               0.25;
                armyComposition[Zerg_Zergling] =            0.75;
            }
        }
    }

    void PoolLair()
    {
        inOpeningBook =                                     s < 60 && Util::getTime() < Time(8, 0);
        gasLimit =                                          INT_MAX;
        firstUpgrade =                                      UpgradeTypes::Metabolic_Boost;
        firstTech =                                         TechTypes::None;
        scout =                                             false;
        droneLimit =                                        10;
        lingLimit =                                         12;
        wallNat =                                           vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) >= 2;
        playPassive =                                       com(Zerg_Mutalisk) == 0 && com(Zerg_Lurker) == 0;

        // Openers
        if (currentOpener == "4Pool") {
            droneLimit =                                    4;
            rush =                                          true;
            transitionReady =                               com(Zerg_Extractor) > 0;

            buildQueue[Zerg_Spawning_Pool] =                s >= 8;
            buildQueue[Zerg_Extractor] =                    (s >= 26 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
        }
        if (currentOpener == "9Pool") {
            transitionReady =                               atPercent(Zerg_Lair, 0.50);

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Extractor] =                    (s >= 18 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Lair] =                         gas(100) && vis(Zerg_Spawning_Pool) > 0 && vis(Zerg_Zergling) >= 6;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "1HatchMuta") {
                firstUnit =                                 Zerg_Mutalisk;
                inBookSupply =                              vis(Zerg_Overlord) < 3;

                // Build
                buildQueue[Zerg_Lair] =                     1;
                buildQueue[Zerg_Spire] =                    lingSpeed() && gas(100) && atPercent(Zerg_Lair, 1.00);
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 56);

                // Army Composition
                armyComposition[Zerg_Drone] =               0.15;
                armyComposition[Zerg_Zergling] =            0.00;
                armyComposition[Zerg_Mutalisk] =            0.85;
            }
            if (currentTransition == "1HatchLurker") {
                lingLimit =                                 16;
                firstUnit =                                 Zerg_Lurker;
                firstTech =                                 TechTypes::Lurker_Aspect;
                inBookSupply =                              vis(Zerg_Overlord) < 3;

                // Build
                buildQueue[Zerg_Lair] =                     1;
                buildQueue[Zerg_Hydralisk_Den] =            atPercent(Zerg_Lair, 0.50);
                buildQueue[Zerg_Hatchery] =                 1 + (s >= 56);

                // Army Composition
                armyComposition[Zerg_Drone] =               0.25;
                armyComposition[Zerg_Lurker] =              0.50;
                armyComposition[Zerg_Hydralisk] =           0.25;
            }
        }


    }
}