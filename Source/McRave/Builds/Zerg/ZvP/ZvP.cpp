﻿#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void defaultZvP() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wallNat =                                   int(Stations::getStations(PlayerState::Self).size()) >= 4 && Spy::getEnemyBuild() == "FFE";
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
        reserveLarva =                              0;

        gasLimit =                                  gasMax();
        unitLimits[Zerg_Zergling] =                 lingsNeeded_ZvP();
        unitLimits[Zerg_Drone] =                    INT_MAX;

        desiredDetection =                          Zerg_Overlord;
        focusUnit =                                 UnitTypes::None;

        pumpLings = false;
        pumpHydras = false;
        pumpMutas = false;

        armyComposition[Zerg_Drone] =               0.60;
        armyComposition[Zerg_Zergling] =            0.40;
    }

    int inboundUnits_ZvP()
    {
        static map<UnitType, double> trackables ={ {Protoss_Zealot, 2.5}, {Protoss_Dragoon, 2.5}, {Protoss_Dark_Templar, 8} };
        auto inBoundUnit = [&](auto &u) {
            if (!Terrain::getEnemyMain())
                return true;
            const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();

            // Check if we know they weren't at home and are missing on the map for 40 seconds
            if (!Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), u->getPosition()) && !Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), u->getPosition()))
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 40);
            return false;
        };

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end())
                arrivalValue += (inBoundUnit(u) ? idx->second : idx->second / 2.0);
        }
        return int(arrivalValue);
    }

    int lingsNeeded_ZvP() {
        auto initialValue = 6;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        // 2Gate
        if (Spy::getEnemyBuild() == "2Gate") {
            if (Spy::getEnemyOpener() == "Proxy9/9" || Spy::getEnemyOpener() == "9/9")
                initialValue = 16;
            else
                initialValue = 10;
        }

        // FFE
        if (Spy::getEnemyBuild() == "FFE") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "Gateway")
                initialValue = 6;
            if (Util::getTime() > Time(5, 45))
                initialValue = 12;
        }

        // 1GC
        if (Spy::getEnemyBuild() == "1GateCore") {
            initialValue = 4;
            if (Spy::getEnemyOpener() == "1Zealot" || Spy::getEnemyOpener() == "2Zealot")
                initialValue = 6;
        }

        // CannonRush
        if (Spy::getEnemyBuild() == "CannonRush")
            initialValue = 12;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // Specifically for proxy defense, we can allow pretty much any amount of lings after we've droned
        if (vis(Zerg_Drone) >= 18 && Spy::getEnemyOpener() == "Proxy9/9" && Spy::getEnemyBuild() == "2Gate")
            return 24;

        // If this is a 2Hatch build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("2Hatch") != string::npos)
            return 0;

        auto inbound = inboundUnits_ZvP();
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 36)
            return inbound;
        return 0;
    }

    void ZvP2HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/2_Hatch_Muta_(vs._Protoss)'
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        focusUnit =                                     Zerg_Mutalisk;
        wantThird =                                     Spy::enemyFastExpand() || hatchCount() >= 3 || Spy::getEnemyTransition() == "Corsair";
        reserveLarva =                                  6;

        auto thirdHatch = total(Zerg_Mutalisk) >= 6 && vis(Zerg_Drone) >= 18;
        if (Spy::getEnemyBuild() == "FFE") {
            thirdHatch = (s >= 26 && vis(Zerg_Drone) >= 11 && vis(Zerg_Lair) > 0);
            planEarly = false;
        }

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 20 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);

        // Pumping
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && gas(80);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0)
            unitLimits[Zerg_Drone] = 26;

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10)
            gasLimit = gasMax();
    }

    void ZvP3HatchMuta()
    {
        inTransition =                                  hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 48;
        inBookSupply =                                  vis(Zerg_Overlord) < 4;

        focusUnit =                                     Zerg_Mutalisk;
        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE" || hatchCount() >= 3;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        reserveLarva =                                  (Spy::getEnemyBuild() != "FFE") ? 6 : 9;

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26 && vis(Zerg_Drone) >= 11) + (vis(Zerg_Hydralisk_Den) > 0) + (hatchCount() >= 4) * 2;
        buildQueue[Zerg_Extractor] =                    (s >= 32 && vis(Zerg_Drone) >= 13) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 20) + (hatchCount() >= 6);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (vis(Zerg_Extractor) > 0 && s >= 32) + (s >= 48);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Spire] =                        (s >= 32 && com(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Hydralisk_Den] =                (total(Zerg_Mutalisk) >= 9);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Spire) > 0;
        upgradeQueue[Muscular_Augments] =               com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines] =                  hydraSpeed();

        // Pumping
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) == 1 && gas(80) && total(Zerg_Mutalisk) < 15;
        pumpHydras = (total(Zerg_Drone) >= 38 && (hydraRange() || hydraSpeed()));

        // Limits
        if (total(Zerg_Mutalisk) >= 9)
            unitLimits[Zerg_Drone] = 38;
        else if (com(Zerg_Spawning_Pool) > 0)
            unitLimits[Zerg_Drone] = 33;

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10)
            gasLimit = gasMax();
    }

    void ZvP3HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 32;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;

        focusUnit =                                     Zerg_Hydralisk;
        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 26;
        mineralThird =                                  true;
        reserveLarva =                                  9;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26 && vis(Zerg_Drone) >= 11);
        buildQueue[Zerg_Extractor] =                    (s >= 30 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair));
        buildQueue[Zerg_Hydralisk_Den] =                (vis(Zerg_Drone) >= 16 && s >= 38);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);

        // Upgrades
        upgradeQueue[Muscular_Augments] =               com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines] =                  hydraSpeed();

        // Pumping
        pumpHydras = com(Zerg_Hydralisk_Den) > 0;
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0)
            unitLimits[Zerg_Drone] = 19;

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 11)
            gasLimit = 3;
    }

    void ZvP4HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 64;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;

        focusUnit =                                     Zerg_Hydralisk;
        planEarly =                                     false;
        wantThird =                                     hatchCount() >= 4 && Spy::enemyFastExpand();

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28) + (s >= 54 && vis(Zerg_Drone) >= 18) + (s >= 84 && vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Extractor] =                    (s >= 26 && vis(Zerg_Drone) >= 13 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 38 && vis(Zerg_Drone) >= 16 && gas(30));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 60) + (s >= 74);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore";
        if (total(Zerg_Hydralisk) >= 6) {
            upgradeQueue[Grooved_Spines] =              rangeFirst ? vis(Zerg_Hydralisk_Den) > 0 : hydraSpeed();
            upgradeQueue[Muscular_Augments] =           rangeFirst ? hydraRange() : vis(Zerg_Hydralisk_Den) > 0;
            upgradeQueue[Zerg_Carapace] =               com(Zerg_Evolution_Chamber) > 0;
            upgradeQueue[Metabolic_Boost] =             1;
        }

        // Pumping
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 2;
        pumpHydras = vis(Zerg_Drone) >= 28 || needMinimumHydras;
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);

        //// All-in
        //if (total(Zerg_Zergling) < 100) {
        //    if (Spy::enemyFastExpand() && vis(Zerg_Drone) >= 18 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && hatchCount() >= 4 && !needMinimumHydras && total(Zerg_Hydralisk) >= 2) {
        //        pumpHydras = false;
        //        pumpLings = true;
        //        unitLimits[Zerg_Zergling] = INT_MAX;
        //        upgradeQueue[Metabolic_Boost] = 1;
        //    }
        //}

        // Limits
        if (com(Zerg_Spawning_Pool) > 0)
            unitLimits[Zerg_Drone] = 28;

        // Gas
        gasLimit = 0;
        if (pumpHydras)
            gasLimit = min(5, gasMax());
        else if (vis(Zerg_Hydralisk_Den) > 0)
            gasLimit = 1;
        else if (vis(Zerg_Drone) >= 11)
            gasLimit = 3;
    }

    void ZvP6HatchHydra()
    {
        //'https://liquipedia.net/starcraft/6_hatch_(vs._Protoss)' - Soulkey style (early 4th hatch, early den, delay spire)
        inTransition =                                  Util::getTime() > Time(3, 15);
        inOpening =                                     total(Zerg_Hydralisk) < 64;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;
        focusUnit =                                     Zerg_Hydralisk;
        wantThird =                                     true;
        mineralThird =                                  false;
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 24;

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 62);
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + (s >= 52) + (com(Zerg_Drone) >= 26 && s >= 64) + (com(Zerg_Drone) >= 26 && s >= 80);
        buildQueue[Zerg_Extractor] =                    (s >= 30) + (vis(Zerg_Drone) >= 32 && s >= 70) + (vis(Zerg_Spire) > 0);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 60);
        buildQueue[Zerg_Evolution_Chamber] =            (s >= 70) + (s >= 150);
        buildQueue[Zerg_Spire] =                        (s >= 140);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore";
        if (total(Zerg_Hydralisk) > 0) {
            upgradeQueue[Grooved_Spines] =              rangeFirst ? vis(Zerg_Hydralisk_Den) : hydraSpeed();
            upgradeQueue[Muscular_Augments] =           rangeFirst ? hydraRange() : vis(Zerg_Hydralisk_Den);
            upgradeQueue[Zerg_Carapace] =               (com(Zerg_Evolution_Chamber) > 0) + (com(Zerg_Evolution_Chamber) > 1);
            upgradeQueue[Zerg_Missile_Attacks] =        (com(Zerg_Evolution_Chamber) > 1);
            techQueue[Burrowing] =                      com(Zerg_Drone) >= 36;
        }
        upgradeQueue[Metabolic_Boost] =             total(Zerg_Hydralisk_Den) > 0;

        // Pumping
        auto needMinimumHydras = (com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 2)
            || (Util::getTime() > Time(6, 30) && total(Zerg_Hydralisk) < 26);
        pumpHydras = (com(Zerg_Hydralisk_Den) > 0 && needMinimumHydras) || (total(Zerg_Drone) >= 44 && (hydraRange() || hydraSpeed()));
        pumpLings = (Util::getTime() < Time(7, 00) && lingsNeeded_ZvP() > vis(Zerg_Zergling));

        //// All-in
        //if (total(Zerg_Zergling) < 150) {
        //    if ((Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5GateGoon") && (vis(Zerg_Drone) >= 30 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && !needMinimumHydras && total(Zerg_Hydralisk) >= 2)) {
        //        pumpHydras = false;
        //        pumpLings = true;
        //        unitLimits[Zerg_Zergling] = INT_MAX;
        //        upgradeQueue[Metabolic_Boost] = 1;
        //    }
        //}

        // Limits
        if (com(Zerg_Spawning_Pool) > 0)
            unitLimits[Zerg_Drone] = 45;

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Hydralisk_Den) == 0 && vis(Zerg_Lair) > 0)
            gasLimit = 1;
        else if (vis(Zerg_Drone) >= 16)
            gasLimit = gasMax();
    }

    void ZvP6HatchCrackling()
    {
        // NOT THE BEES - Build designed to counter goonbois
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     true;
        inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;
        unitLimits[Zerg_Zergling] =                     INT_MAX;

        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);

        auto spireOverlords = (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "NeoBisu") ? (4 * (s >= 66)) : (3 * (s >= 66)) + (s >= 82);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0) + (com(Zerg_Hive) > 0) + 2 * (s >= 100);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 11) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3 && vis(Zerg_Overlord) >= 3);
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Queens_Nest] =                  total(Zerg_Mutalisk) >= 9;
        buildQueue[Zerg_Hive] =                         atPercent(Zerg_Queens_Nest, 0.95);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + spireOverlords;
        buildQueue[Zerg_Evolution_Chamber] =            (s >= 70) + (s >= 150);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;
        upgradeQueue[Adrenal_Glands] =                  com(Zerg_Hive) > 0;
        upgradeQueue[Zerg_Carapace] =                   (com(Zerg_Evolution_Chamber) > 0) + 2 * (com(Zerg_Evolution_Chamber) > 1);
        upgradeQueue[Zerg_Melee_Attacks] =              3 * (com(Zerg_Evolution_Chamber) > 1);

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Hive) > 0)
            gasLimit = 3;
        else if (vis(Zerg_Drone) >= 11)
            gasLimit = gasMax();

        // Limits
        if (com(Zerg_Spawning_Pool) > 0)
            unitLimits[Zerg_Drone] = 36;

        // Pumping
        pumpLings = vis(Zerg_Drone) >= 30 || lingsNeeded_ZvP() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && total(Zerg_Mutalisk) < 12 && gas(80);
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
                currentTransition = "3HatchMuta";
            }

            if (Spy::getEnemyBuild() != "FFE" && (currentTransition == "3HatchHydra" || currentTransition == "6HatchHydra") && com(Zerg_Spawning_Pool) > 0)
                currentTransition = "4HatchHydra";
            if (Spy::getEnemyBuild() == "FFE" && currentTransition == "4HatchHydra")
                currentTransition = "6HatchHydra";
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
            if (currentTransition == "6HatchCrackling")
                ZvP6HatchCrackling();
        }
    }
}