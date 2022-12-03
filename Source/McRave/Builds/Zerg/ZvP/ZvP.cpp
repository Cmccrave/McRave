#include "Main/McRave.h"

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
        wantThird =                                 Spy::getEnemyBuild() == "FFE";
        proxy =                                     false;
        hideTech =                                  false;
        playPassive =                               false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        gasTrick =                                  false;
        mineralThird =                              false;

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
            if (Spy::getEnemyOpener() == "10/17")
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
                initialValue = 0;
        }

        // 1GC
        if (Spy::getEnemyBuild() == "1GateCore") {
            initialValue = 6;
        }
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
        auto arrivalValue = int(1 * count_if(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (trackables.find(u->getType()) != trackables.end()) {
                auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();
                return trackables.find(u->getType()) != trackables.end() && Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 40);
            }
            return false;
        }));
        
        arrivalValue += int(3 * count_if(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (trackables.find(u->getType()) != trackables.end()) {
                auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();
                return trackables.find(u->getType()) != trackables.end() && Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 30);
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
        if (total(Zerg_Zergling) < maxValue && vis(Zerg_Zergling) < 24)
            return maxValue;
        return 0;
    }

    void ZvP2HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/2_Hatch_Muta_(vs._Protoss)'
        inTransition =                                  total(Zerg_Mutalisk) > 0;
        inOpening =                                     Spy::getEnemyOpener() == "Proxy" ? total(Zerg_Mutalisk) < 3 : total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;
        firstUnit =                                     Zerg_Mutalisk;
        firstUpgrade =                                  (Spy::getEnemyOpener() == "Proxy" && hatchCount() >= 4 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 26 : 13;
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        playPassive =                                   false;

        wantThird =                                     Spy::enemyFastExpand() || hatchCount() >= 3 || Spy::getEnemyTransition() == "Corsair";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     Spy::enemyFastExpand() && atPercent(Zerg_Lair, 0.5) && com(Zerg_Lair) == 0 && int(Stations::getStations(PlayerState::Self).size()) < 3;

        auto thirdHatch = total(Zerg_Mutalisk) >= 6;
        if (Spy::getEnemyTransition() == "4Gate") {
            wantThird = false;
            thirdHatch = false;
            planEarly = false;
        }
        else if (Spy::getEnemyOpener() == "Proxy") {
            thirdHatch = total(Zerg_Mutalisk) >= 3;
            planEarly = false;
        }
        else if (Spy::enemyFastExpand() && vis(Zerg_Drone) >= 20 && vis(Zerg_Spire) == 0) {
            thirdHatch = true;
        }

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 24 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
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
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : 15 - hatchCount();
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        playPassive =                                   false;

        wantThird =                                     Spy::getEnemyOpener() != "9/9" && Spy::getEnemyTransition() != "ZealotRush";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     hatchCount() < 3 && s >= 26;

        auto spireOverlords = (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "NeoBisu") ? (4 * (s >= 66)) : (3 * (s >= 66)) + (s >= 82);

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Drone) >= 11);
        buildQueue[Zerg_Extractor] =                    (s >= 30 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
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
        inTransition =                                  true;
        inOpening =                                     total(Zerg_Hydralisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;
        firstUnit =                                     Zerg_Hydralisk;
        firstUpgrade =                                  UpgradeTypes::Muscular_Augments;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) == 0 ? 13 : 19;
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP() + (6 * vis(Zerg_Hydralisk_Den));
        playPassive =                                   false;
        gasLimit =                                      (vis(Zerg_Drone) >= 14) ? 3 : 0;

        wantThird =                                     Spy::getEnemyOpener() != "9/9" && Spy::getEnemyTransition() != "ZealotRush";
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 26;
        mineralThird =                                  true;

        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Drone) >= 11) + (total(Zerg_Hydralisk) >= 9 && vis(Zerg_Larva) < 3);
        buildQueue[Zerg_Extractor] =                    (s >= 30 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair));
        buildQueue[Zerg_Hydralisk_Den] =                (vis(Zerg_Drone) >= 14 && s >= 38);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);

        // Composition
        if (s <= 80 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
            armyComposition[Zerg_Hydralisk] =           0.00;
        }
        else if (com(Zerg_Hydralisk_Den) == 1 && vis(Zerg_Drone) >= 16) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Hydralisk] =           1.00;
        }
        else {
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Drone] =               1.00;
            armyComposition[Zerg_Hydralisk] =           0.00;
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

        unitLimits[Zerg_Drone] =                        INT_MAX;
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        unitLimits[Zerg_Scourge] =                      max(2, Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) * 2);

        wantThird =                                     Spy::getEnemyOpener() != "9/9" && Spy::getEnemyTransition() != "ZealotRush";
        mineralThird =                                  false;
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 28;

        playPassive =                                   false;
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
        if (s <= 80 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
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
            if (Spy::getEnemyOpener() == "9/9")
                currentTransition = "2HatchMuta";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta")
                ZvP2HatchMuta();
            if (currentTransition == "3HatchMuta")
                ZvP3HatchMuta();
            if (currentTransition == "3HatchHydra")
                ZvP3HatchHydra();
            if (currentTransition == "6HatchHydra")
                ZvP6HatchHydra();
        }
    }
}