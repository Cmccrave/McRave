#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void defaultZvT() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wantNatural =                               true;
        wantThird =                                 false;
        mineralThird =                              false;
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        gasTrick =                                  false;
        reserveLarva =                              true;

        gasLimit =                                  gasMax();
        unitLimits[Zerg_Zergling] =                 lingsNeeded_ZvT();
        unitLimits[Zerg_Drone] =                    INT_MAX;

        desiredDetection =                          Zerg_Overlord;
        focusUnit =                                 UnitTypes::None;

        armyComposition[Zerg_Drone] =               0.60;
        armyComposition[Zerg_Zergling] =            0.40;

        wallNat =                                   false;
        wallMain =                                  false;
    }

    int lingsNeeded_ZvT() {

        auto initialValue = 6;
        if (!atPercent(Zerg_Spawning_Pool, 0.80))
            return 0;

        // 2Rax
        if (Spy::getEnemyBuild() == "2Rax") {
            initialValue = 6;
            if (Spy::getEnemyOpener() == "11/13")
                initialValue = 6;
            else if (Spy::getEnemyOpener() == "BBS")
                initialValue = 10;
        }

        // RaxCC
        if (Spy::getEnemyBuild() == "RaxCC") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "8Rax")
                initialValue = 10;
            else
                initialValue = 2;
        }

        // RaxFact
        if (Spy::getEnemyBuild() == "RaxFact") {
            initialValue = 2;
            if (Util::getTime() > Time(3, 45))
                initialValue = 6;
        }

        // TODO: Fix T spy
        if (Spy::getEnemyOpener() == "8Rax")
            initialValue = 10;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        if (Spy::getEnemyTransition() == "WorkerRush")
            return 24;
        return 0;
    }

    void ZvT_2HatchMuta()
    {
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        inBookSupply =                                  total(Zerg_Mutalisk) < 6;

        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        (com(Zerg_Spawning_Pool) > 0 ? 28 : 15 - hatchCount());
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 48);
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.25));
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        atPercent(Zerg_Lair, 0.95);

        // Research
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Reactions
        if (Spy::getEnemyOpener() == "8Rax" && total(Zerg_Zergling) < 6) 
            buildQueue[Zerg_Lair] = 0;

        // Pumping
        pumpLings = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && gas(80);

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10)
            gasLimit = gasMax();
    }

    void ZvT_3HatchMuta()
    {
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 7 || total(Zerg_Mutalisk) < 9;

        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : 15 - hatchCount();
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
        wantThird =                                     !Spy::enemyPressure() && !Spy::enemyRush() && Spy::getEnemyOpener() != "8Rax" && Spy::getEnemyBuild() != "RaxFact";
        planEarly =                                     hatchCount() < 3 && s >= 26;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + (total(Zerg_Mutalisk) >= 9);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 11) + (s >= 44 && vis(Zerg_Drone) >= 20);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (atPercent(Zerg_Spire, 0.5) * 3);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 42 && atPercent(Zerg_Lair, 0.80));

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Zergling) >= 6 && gas(80);

        // Research
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Pumping
        pumpLings = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && gas(80);

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10)
            gasLimit = gasMax();
    }

    void ZvT_2HatchSpeedling()
    {
        inTransition =                                  total(Zerg_Zergling) >= 10;
        inOpening =                                     total(Zerg_Zergling) < 36;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        total(Zerg_Zergling) >= 12 ? 11 : 9;
        rush =                                          true;

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 gas(90);

        // Pumping
        pumpLings = hatchCount() >= 3;

        // Gas
        gasLimit = 0;
        if (!lingSpeed())
            gasLimit = capGas(100);
    }

    void ZvT_3HatchSpeedling()
    {
        inTransition =                                  total(Zerg_Zergling) >= 10;
        inOpening =                                     total(Zerg_Zergling) < 40;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        13;
        rush =                                          true;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (s >= 24);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 gas(90);

        // Pumping
        pumpLings = hatchCount() >= 3;

        // Gas
        gasLimit = 0;
        if (!lingSpeed())
            gasLimit = capGas(100);
    }

    void ZvT()
    {
        defaultZvT();

        // Builds
        if (currentBuild == "HatchPool")
            ZvT_HP();
        if (currentBuild == "PoolHatch")
            ZvT_PH();

        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyTransition() == "WorkerRush") {
                currentBuild = "PoolHatch";
                currentOpener = "12Pool";
                currentTransition = "2HatchMuta";
            }
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta")
                ZvT_2HatchMuta();
            if (currentTransition == "3HatchMuta")
                ZvT_3HatchMuta();
            if (currentTransition == "2HatchSpeedling")
                ZvT_2HatchSpeedling();
            if (currentTransition == "3HatchSpeedling")
                ZvT_3HatchSpeedling();
        }
    }
}