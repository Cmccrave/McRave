#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

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
        focusUpgrade =                              (vis(Zerg_Zergling) >= 6 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        focusTech =                                 TechTypes::None;
        focusUnit =                                 None;

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
        unitLimits[Zerg_Drone] =                        (com(Zerg_Spawning_Pool) > 0 ? 28 : 15 - hatchCount());
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
        gasLimit =                                      vis(Zerg_Drone) >= 10 ? gasMax() : 0;

        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        focusUnit =                                     Zerg_Mutalisk;
        inBookSupply =                                  total(Zerg_Mutalisk) < 6;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 48);
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (2 * atPercent(Zerg_Spire, 0.25));
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        atPercent(Zerg_Lair, 0.95);

        // Upgrades
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Reactions
        if (Spy::getEnemyOpener() == "8Rax" && total(Zerg_Zergling) < 6) 
            buildQueue[Zerg_Lair] = 0;

        // Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvT() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvT_3HatchMuta()
    {
        inTransition =                                  vis(Zerg_Lair) > 0;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : 15 - hatchCount();
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
        gasLimit =                                      gasMax();

        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        focusUpgrade =                                  Spy::enemyRush() ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        focusUnit =                                     Zerg_Mutalisk;
        inBookSupply =                                  vis(Zerg_Overlord) < 7 || total(Zerg_Mutalisk) < 9;
        wantThird =                                     !Spy::enemyPressure() && !Spy::enemyRush() && Spy::getEnemyOpener() != "8Rax" && Spy::getEnemyBuild() != "RaxFact";
        planEarly =                                     hatchCount() < 3 && s >= 26;

        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + (total(Zerg_Mutalisk) >= 9);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 11) + (s >= 44 && vis(Zerg_Drone) >= 20);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (atPercent(Zerg_Spire, 0.5) * 3);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 42 && atPercent(Zerg_Lair, 0.80));

        // Upgrades
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvT() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvT_2HatchSpeedling()
    {
        unitLimits[Zerg_Drone] =                        total(Zerg_Zergling) >= 12 ? 11 : 9;
        unitLimits[Zerg_Zergling] =                     INT_MAX;
        gasLimit =                                      ((!Spy::enemyProxy() || com(Zerg_Zergling) >= 6) && !lingSpeed()) ? capGas(100) : 0;

        wallNat =                                       false;
        inOpening =                                     total(Zerg_Zergling) < 36;
        focusUpgrade =                                  gas(100) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        focusUnit =                                     UnitTypes::None;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;
        rush =                                          true;

        // Build
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

        // Composition
        armyComposition[Zerg_Drone] =                   0.00;
        armyComposition[Zerg_Zergling] =                1.00;
    }

    void ZvT_3HatchSpeedling()
    {
        unitLimits[Zerg_Drone] =                        13;
        unitLimits[Zerg_Zergling] =                     hatchCount() >= 3 ? INT_MAX : 0;
        gasLimit =                                      !lingSpeed() ? capGas(100) : 0;

        wallNat =                                       true;
        inOpening =                                     total(Zerg_Zergling) < 80;
        focusUpgrade =                                  gas(100) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        focusUnit =                                     UnitTypes::None;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;
        rush =                                          true;

        // Build
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (s >= 24);

        // Composition
        armyComposition[Zerg_Drone] =                   0.20;
        armyComposition[Zerg_Zergling] =                0.80;
    }

    void ZvT_6HatchCrackling()
    {
        // Uhhh, yeah who knows what this build is
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     true;
        inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;
        focusUpgrade =                                  vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 24 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     total(Zerg_Mutalisk) < 0 ? lingsNeeded_ZvP() : INT_MAX;

        wantThird =                                     !Spy::enemyPressure() && !Spy::enemyRush() && Spy::getEnemyOpener() != "8Rax" && Spy::getEnemyBuild() != "RaxFact";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);

        auto spireOverlords = (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "NeoBisu") ? (4 * (s >= 66)) : (3 * (s >= 66)) + (s >= 82);

        if (vis(Zerg_Hive) > 0) {
            unitLimits[Zerg_Drone] = 30;
            focusUpgrade = UpgradeTypes::Adrenal_Glands;
        }

        if (com(Zerg_Hive) > 0)
            gasLimit = 3;

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0) + (com(Zerg_Hive) > 0) + 2 * (s >= 100);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 11) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3 && vis(Zerg_Overlord) >= 3);
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Queens_Nest] =                  total(Zerg_Mutalisk) >= 9;
        buildQueue[Zerg_Hive] =                         atPercent(Zerg_Queens_Nest, 0.95);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + spireOverlords;

        // Composition
        if (com(Zerg_Hive) > 0 || vis(Zerg_Drone) >= 30) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            0.70;
            armyComposition[Zerg_Mutalisk] =            0.30;
        }
        else if (com(Zerg_Spire) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else if (vis(Zerg_Drone) < 30) {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
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
            if (currentTransition == "6HatchCrackling")
                ZvT_6HatchCrackling();
        }
    }
}