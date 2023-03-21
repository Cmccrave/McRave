﻿#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void defaultZvP() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wallNat =                                   hatchCount() >= 4;
        wallMain =                                  false;
        wantNatural =                               true;
        wantThird =                                 Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        gasTrick =                                  false;
        mineralThird =                              false;
        reserveLarva =                              true;

        gasLimit =                                  gasMax();
        unitLimits[Zerg_Zergling] =                 lingsNeeded_ZvP();
        unitLimits[Zerg_Drone] =                    INT_MAX;

        desiredDetection =                          Zerg_Overlord;
        firstUpgrade =                              (vis(Zerg_Zergling) >= 6 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        firstTech =                                 TechTypes::None;
        firstUnit =                                 None;

        armyComposition[Zerg_Drone] =               0.60;
        armyComposition[Zerg_Zergling] =            0.40;
    }

    int lingsNeeded_ZvP() {
        auto lings = 0;
        auto initialValue = 6;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        // 2Gate
        if (Spy::getEnemyBuild() == "2Gate") {
            if (Spy::getEnemyOpener() == "10/15")
                initialValue = 6;
            else if (currentOpener == "12Hatch")
                initialValue = 10;
            else if (Spy::getEnemyOpener() == "Proxy" || Spy::getEnemyOpener() == "9/9")
                initialValue = 16;
            else
                initialValue = 10;
        }

        // FFE
        if (Spy::getEnemyBuild() == "FFE") {
            if (Spy::getEnemyOpener() == "Forge" || Spy::getEnemyOpener() == "Nexus")
                initialValue = com(Zerg_Lair) * 6;
        }

        // 1GC
        if (Spy::getEnemyBuild() == "1GateCore")
            initialValue = 6;
        
        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // Specifically for proxy defense, we can allow pretty much any amount of lings after we've droned
        if (vis(Zerg_Drone) >= 18 && Spy::getEnemyOpener() == "Proxy" && Spy::getEnemyBuild() == "2Gate")
            return 24;

        // If this is a 2Hatch build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("2Hatch") != string::npos)
            return 0;

        // For each Zealot or Dragoon, assume it arrives with 45 seconds for us to create Zerglings
        set<UnitType> trackables ={ Protoss_Zealot, Protoss_Dragoon, Protoss_Dark_Templar };
        auto arrivalValue = int(2 * count_if(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (trackables.find(u->getType()) != trackables.end()) {
                const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 45);
            }
            return false;
        }));

        arrivalValue += int(2 * count_if(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (trackables.find(u->getType()) != trackables.end()) {
                const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 28);
            }
            return false;
        }));

        // For each Gateway, assume it pumps a unit out between the average of Zealot and Dragoon train times (675 frames)
        auto producingValue = 0;
        for_each(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (u->getType() == Protoss_Gateway && u->isCompleted())
                producingValue += 1 * ((Broodwar->getFrameCount() - u->frameCompletesWhen()) / 675);
        });

        auto maxValue = max({ initialValue, arrivalValue, producingValue });
        if (total(Zerg_Zergling) < maxValue && vis(Zerg_Zergling) < 36)
            return maxValue;
        return 0;
    }

    void ZvP2HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/2_Hatch_Muta_(vs._Protoss)'
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     Spy::getEnemyOpener() == "Proxy" ? total(Zerg_Mutalisk) < 3 : total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;
        firstUnit =                                     Zerg_Mutalisk;
        firstUpgrade =                                  (Spy::getEnemyOpener() == "Proxy" && hatchCount() >= 4 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 26 : unitLimits[Zerg_Drone] - vis(Zerg_Extractor);
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();

        wantThird =                                     Spy::enemyFastExpand() || hatchCount() >= 3 || Spy::getEnemyTransition() == "Corsair";
        gasLimit =                                      (vis(Zerg_Drone) >= 10) ? gasMax() : 0;

        auto thirdHatch = total(Zerg_Mutalisk) >= 6;
        if (Spy::getEnemyBuild() == "FFE") {
            thirdHatch = (s >= 26 && vis(Zerg_Drone) >= 11 && vis(Zerg_Lair) > 0);
            planEarly = false;
        }

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 20 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95));
        buildQueue[Zerg_Overlord] =                     (com(Zerg_Spire) == 0 && atPercent(Zerg_Spire, 0.35)) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50)) + (s >= 80);

        // Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvP3HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/3_Hatch_Spire_(vs._Protoss)'
        inTransition =                                  hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpening =                                     total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;
        firstUpgrade =                                  UpgradeTypes::None;
        firstUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();

        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);

        auto spireOverlords = (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "NeoBisu") ? (4 * (s >= 66)) : (3 * (s >= 66)) + (s >= 82);

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 11) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + spireOverlords;

        // Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvP3HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 32;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;
        firstUnit =                                     Zerg_Hydralisk;
        firstUpgrade =                                  UpgradeTypes::Muscular_Augments;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 19 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? 3 : 0;

        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 26;
        mineralThird =                                  true;

        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26 && vis(Zerg_Drone) >= 11);
        buildQueue[Zerg_Extractor] =                    (s >= 30 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair));
        buildQueue[Zerg_Hydralisk_Den] =                (vis(Zerg_Drone) >= 16 && s >= 38);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);


        // Composition
        if (com(Zerg_Hydralisk_Den) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
            armyComposition[Zerg_Hydralisk] =           0.00;
        }
        else {
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Drone] =               0.50;
            armyComposition[Zerg_Hydralisk] =           0.50;
        }
    }

    void ZvP4HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 64;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;
        firstUnit =                                     Zerg_Hydralisk;
        firstUpgrade =                                  (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore") ? UpgradeTypes::Grooved_Spines : UpgradeTypes::Muscular_Augments;
        unitLimits[Zerg_Drone] =                        (hatchCount() >= 5 || Spy::enemyFastExpand()) ? 30 : 24;
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? min(4, gasMax()) : 0;

        planEarly =                                     false;
        wantThird =                                     Spy::enemyFastExpand();
        reserveLarva =                                  false;

        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26 && vis(Zerg_Drone) >= 11) + (s >= 54 && vis(Zerg_Drone) >= 18) + (s >= 84);
        buildQueue[Zerg_Extractor] =                    (s >= 34 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (total(Zerg_Hydralisk) >= 16);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 38 && vis(Zerg_Drone) >= 14 && gas(30));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 52) + (s >= 60) + (s >= 74);

        // Composition
        if (com(Zerg_Hydralisk_Den) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
            armyComposition[Zerg_Hydralisk] =           0.00;
        }
        else {
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Drone] =               0.70;
            armyComposition[Zerg_Hydralisk] =           0.30;
        }
    }

    void ZvP6HatchHydra()
    {
        //'https://liquipedia.net/starcraft/6_hatch_(vs._Protoss)'
        inTransition =                                  Util::getTime() > Time(3, 15);
        inOpening =                                     total(Zerg_Hydralisk) < 24;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;

        firstUnit =                                     Zerg_Hydralisk;
        firstUpgrade =                                  UpgradeTypes::Muscular_Augments;

        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 36 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        unitLimits[Zerg_Scourge] =                      max(2, Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) * 2);

        wantThird =                                     Spy::getEnemyBuild() != "2Gate" && Spy::getEnemyBuild() != "1GateCore" && hatchCount() < 3;
        mineralThird =                                  false;
        reserveLarva =                                  false;
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 28;

        gasLimit =                                      (vis(Zerg_Drone) >= 16) ? gasMax() : 0;

        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 62);

        // Soulkey style vs 1base (early 4th hatch, early den, skip spire)
        if (Spy::getEnemyBuild() == "1GateCore" || Spy::getEnemyBuild() == "2Gate") {
            buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + (s >= 54) + (com(Zerg_Drone) >= 26 && s >= 64) + (com(Zerg_Drone) >= 26 && s >= 80);
            buildQueue[Zerg_Extractor] =                    (s >= 30) + (vis(Zerg_Drone) >= 28 && s >= 70);
            buildQueue[Zerg_Lair] =                         (vis(Zerg_Hydralisk_Den) > 0 && !Spy::enemyRush());
            buildQueue[Zerg_Spire] =                        (s >= 80);
            buildQueue[Zerg_Hydralisk_Den] =                (s >= 60);
            buildQueue[Zerg_Evolution_Chamber] =            (s >= 70);
        }

        // Larva style vs 2base (early 6th hatch, late den, early spire)
        else {
            buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Extractor) > 0) + (s >= 64) + (s >= 70) + (s >= 76);
            buildQueue[Zerg_Extractor] =                    (s >= 30) + (s >= 70) + (vis(Zerg_Evolution_Chamber) > 0);
            buildQueue[Zerg_Lair] =                         (s >= 36);
            buildQueue[Zerg_Spire] =                        (s >= 40 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
            buildQueue[Zerg_Hydralisk_Den] =                (s >= 80);
            buildQueue[Zerg_Evolution_Chamber] =            (s >= 84);
        }

        // Composition
        if (com(Zerg_Hydralisk_Den) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.65;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Hydralisk] =           0.30;
            armyComposition[Zerg_Scourge] =             0.05;
        }
    }

    void ZvP()
    {
        defaultZvP();

        // Builds
        if (currentBuild == "HatchPool")
            ZvP_HP();
        if (currentBuild == "PoolHatch")
            ZvP_PH();

        // Reactions
        if (!inTransition) {
            if (Spy::enemyProxy() && Util::getTime() < Time(2, 00)) {
                currentBuild = "PoolHatch";
                currentOpener = "Overpool";
                currentTransition = "2HatchMuta";
            }

            if (Spy::getEnemyBuild() != "FFE" && (currentTransition == "3HatchHydra" || currentTransition == "6HatchHydra") && com(Zerg_Spawning_Pool) > 0)
                currentTransition = "4HatchHydra";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta")
                ZvP2HatchMuta();
            if (currentTransition == "3HatchMuta")
                ZvP3HatchMuta();
            if (currentTransition == "3HatchHydra")
                ZvP3HatchHydra();
            if (currentTransition == "4HatchHydra")
                ZvP4HatchHydra();
            if (currentTransition == "6HatchHydra")
                ZvP6HatchHydra();
        }
    }
}