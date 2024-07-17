﻿#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {
        int initialLings = 0;
    }

    void defaultZvP() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wallNat =                                   false;
        wallMain =                                  false;
        wallThird =                                 true;
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
        static map<UnitType, double> trackables ={ {Protoss_Zealot, 2.0}, {Protoss_Dragoon, 2.0}, {Protoss_Dark_Templar, 5.0} };
        if (Players::hasUpgraded(PlayerState::Enemy, Singularity_Charge, 1))
            trackables[Protoss_Dragoon] = 2.2;
        if (Players::hasUpgraded(PlayerState::Enemy, Leg_Enhancements, 1))
            trackables[Protoss_Zealot] = 2.2;

        auto inBoundUnit = [&](auto &u) {
            if (!Terrain::getEnemyMain())
                return true;
            const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();

            // Check if we know they weren't at home and are missing on the map for 35 seconds
            if (!u->isThreatening() && (!Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), u->getPosition()) || Util::getTime() < Time(6, 00)) && !Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), u->getPosition()))
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 35);
            return false;
        };

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end() && (inOpening || unit.isThreatening())) {
                arrivalValue += idx->second;
                if (inBoundUnit(u) || vis(Zerg_Zergling) < 6)
                    arrivalValue += idx->second / 1.5;
            }
        }

        // Make less if we have some other units outside our opening
        //arrivalValue -= vis(Zerg_Hydralisk) * 1.5;
        arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 3.0;
        //arrivalValue -= vis(Zerg_Mutalisk);

        return int(arrivalValue);
    }

    int timingUnits_ZvP() {
        if (Spy::getEnemyTransition() == "4Gate")
            return Util::getTime().minutes * 3;
        if (Spy::getEnemyBuild() == "2Gate" && Util::getTime() > Time(3, 30))
            return Util::getTime().minutes * 3;
        if (Spy::getEnemyBuild() == "1GateCore" && Util::getTime() > Time(3, 30))
            return Util::getTime().minutes * 2;
        return 0;
    }

    int lingsNeeded_ZvP() {
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        auto initialValue = 4;
        if (currentBuild == "HatchPool")
            initialValue = 2;

        // 2Gate
        if (Spy::getEnemyBuild() == "2Gate") {
            initialValue = 12;
            if (Spy::getEnemyOpener() == "Proxy9/9" || Spy::getEnemyOpener() == "9/9" || Util::getTime() > Time(4, 45))
                initialValue = 16;
            if (Spy::getEnemyOpener() == "10/12")
                initialValue = 10;
            if (Spy::getEnemyOpener() == "10/15")
                initialValue = 4;
        }

        // FFE
        if (Spy::getEnemyBuild() == "FFE") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "Gateway" || hideTech)
                initialValue = 6;
            if (Util::getTime() > Time(5, 30))
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

        initialLings = min(initialValue, 6);

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // Specifically for proxy defense, we can allow pretty much any amount of lings after we've droned
        if (vis(Zerg_Drone) >= 18 && Spy::getEnemyOpener() == "Proxy9/9" && Spy::getEnemyBuild() == "2Gate")
            return 24;

        // If this is a 2h build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("2Hatch") != string::npos)
            return 0;

        // Timing units based on best case scenario for minimum needed lings
        auto timing = timingUnits_ZvP();
        if (total(Zerg_Zergling) < timing && vis(Zerg_Zergling) < 64)
            return timing;

        // Timing units based on arrival, typical case scenario
        auto inbound = inboundUnits_ZvP();
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 64)
            return inbound;
        return 0;
    }

    void ZvP2HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/2_Hatch_Muta_(vs._Protoss)'
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     true;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        focusUnit =                                     Zerg_Mutalisk;
        reserveLarva =                                  (Spy::getEnemyBuild() == "FFE" || Spy::enemyFastExpand()) ? 6 : 0;
        hideTech =                                      true;

        // Tech
        unitOrder ={ Zerg_Mutalisk, Zerg_Zergling };

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Hatchery] =                     2 + (vis(Zerg_Drone) >= 22) + (vis(Zerg_Drone) >= 24) + (vis(Zerg_Drone) >= 27);
        buildQueue[Zerg_Extractor] =                    (s >= 20 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (hatchCount() >= 3 && vis(Zerg_Drone) >= 22);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Spire) > 0;
        upgradeQueue[Zerg_Flyer_Carapace] =             total(Zerg_Mutalisk) >= 12;

        // Pumping
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling) || (hatchCount() >= 4 && !gas(100));
        pumpScourge = false;// com(Zerg_Spire) == 1 && total(Zerg_Scourge) < 4 && Spy::getEnemyBuild() != "FFE" && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0;
        pumpMutas = !pumpScourge && com(Zerg_Spire) > 0 && gas(80);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 45;
        }

        // All-in
        if ((Spy::getEnemyBuild() == "2Gate" || Spy::getEnemyBuild() == "1GateCore") && Spy::enemyFastExpand()) {
            static Time expandTiming = Util::getTime();
            if (expandTiming < Time(4, 00) && vis(Zerg_Zergling) >= initialLings)
                activeAllinType = AllinType::Z_4HatchSpeedling;
            else if (total(Zerg_Mutalisk) >= 6 && Spy::getEnemyTransition() != "4Gate" && (Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0))
                activeAllinType = AllinType::Z_5HatchSpeedling;
            //else if (total(Zerg_Mutalisk) >= 24)
            //    activeAllinType = AllinType::Z_6HatchCrackling;
        }
        //if (Spy::getEnemyBuild() == "FFE") {
        //    if (total(Zerg_Mutalisk) >= 18)
        //        activeAllinType = AllinType::Z_6HatchCrackling;
        //}

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "FFE") {
            if (vis(Zerg_Drone) < 10)
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && Spy::getEnemyBuild() == "2Gate" && Util::getTime() < Time(3, 45))
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && Util::getTime() < Time(3, 45))
                gasLimit = 1;
            else if (vis(Zerg_Spire) > 0 && Util::getTime() < Time(4, 30))
                gasLimit = 1;
        }
    }

    void ZvP3HatchMuta()
    {
        inTransition =                                  hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 48 && total(Zerg_Mutalisk) < 18;
        inBookSupply =                                  vis(Zerg_Overlord) < 4;

        focusUnit =                                     Zerg_Mutalisk;
        wantThird =                                     Spy::getEnemyBuild() != "2Gate" && Spy::getEnemyBuild() != "1GateCore";
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        reserveLarva =                                  2;

        // Gas first, but wait for lings
        auto firstGas = (s >= 28 && vis(Zerg_Drone) >= 12 && total(Zerg_Zergling) >= initialLings);
        auto secondGas = (Spy::getEnemyBuild() == "FFE") ? (atPercent(Zerg_Spire, 0.75)) : (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 20);
        auto thirdHatch = (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0);
        auto fourthHatch = (vis(Zerg_Drone) >= 28 && vis(Zerg_Spire) > 0);
        auto hydraSwitch = Spy::getEnemyTransition() != "4Gate" && Spy::getEnemyTransition() != "5GateGoon";

        // Tech
        unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker };
        if (!hydraSwitch)
            unitOrder ={ Zerg_Mutalisk, Zerg_Zergling };

        // Build
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch + (vis(Zerg_Drone) >= 30) + (!hydraSwitch && vis(Zerg_Drone) >= 32);
        buildQueue[Zerg_Extractor] =                    firstGas + secondGas + (hatchCount() >= 6);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3 && vis(Zerg_Drone) >= 12);
        buildQueue[Zerg_Spire] =                        (s >= 32 && com(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Hydralisk_Den] =                (hydraSwitch && total(Zerg_Mutalisk) >= 9 && vis(Zerg_Drone) >= 26);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;
        upgradeQueue[Muscular_Augments] =               hydraSwitch && com(Zerg_Hydralisk_Den) > 0 && total(Zerg_Hydralisk) >= 6;
        upgradeQueue[Grooved_Spines] =                  hydraSwitch && hydraSpeed();
        upgradeQueue[Zerg_Flyer_Carapace] =             2 * (total(Zerg_Mutalisk) >= 9 && !hydraSwitch);

        // Pumping
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);
        pumpScourge = com(Zerg_Spire) == 1 && total(Zerg_Mutalisk) < 9 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > vis(Zerg_Scourge) * 2;
        pumpMutas = !pumpScourge && com(Zerg_Spire) == 1 && gas(100) && (total(Zerg_Mutalisk) < 9 || !hydraSwitch);
        pumpHydras = !pumpMutas && hydraSwitch && vis(Zerg_Drone) >= 38;

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 45;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "FFE") {
            if (Util::getTime() < Time(4, 00))
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 12)
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 16)
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 20)
                gasLimit = 2;
        }
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

        // Tech
        unitOrder ={ Zerg_Hydralisk, Zerg_Mutalisk };

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 28 && total(Zerg_Zergling) >= initialLings);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 30 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair));
        buildQueue[Zerg_Hydralisk_Den] =                (vis(Zerg_Drone) >= 16 && s >= 38);

        // Upgrades
        upgradeQueue[Muscular_Augments] =               com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines] =                  hydraSpeed();

        // Pumping
        pumpHydras = com(Zerg_Hydralisk_Den) > 0;
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 19;
        }

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

        // Tech
        unitOrder ={ Zerg_Hydralisk, Zerg_Mutalisk };

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 28 && total(Zerg_Zergling) >= initialLings);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 60) + (s >= 74);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + (s >= 54 && vis(Zerg_Drone) >= 18) + (s >= 84 && vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Extractor] =                    (s >= 26 && vis(Zerg_Drone) >= 13 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 38 && vis(Zerg_Drone) >= 14 && gas(30));

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore";
        upgradeQueue[Metabolic_Boost] =                 total(Zerg_Zergling) >= 10 && vis(Zerg_Hydralisk_Den) > 0;
        if (total(Zerg_Hydralisk) >= 6) {
            upgradeQueue[Grooved_Spines] =              rangeFirst ? com(Zerg_Hydralisk_Den) > 0 : hydraSpeed();
            upgradeQueue[Muscular_Augments] =           rangeFirst ? hydraRange() : com(Zerg_Hydralisk_Den) > 0;
            upgradeQueue[Zerg_Carapace] =               com(Zerg_Evolution_Chamber) > 0;
        }

        // Pumping
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 2;
        pumpHydras = vis(Zerg_Drone) >= 28 || needMinimumHydras;
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);

        // All-in
        if (!needMinimumHydras && total(Zerg_Hydralisk) >= 2 && hatchCount() >= 4 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Archon) == 0) {
            if (Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "CorsairGoon")
                activeAllinType = AllinType::Z_5HatchSpeedling;
            else if (Spy::enemyGreedy())
                activeAllinType = AllinType::Z_5HatchSpeedling;
            else if (Spy::enemyFastExpand() && total(Zerg_Drone) >= 28)
                activeAllinType = AllinType::Z_6HatchCrackling;
        }

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 28;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "FFE") {
            if (pumpHydras && !needMinimumHydras)
                gasLimit = min(5, gasMax());
            else if (vis(Zerg_Hydralisk_Den) > 0 && Util::getTime() < Time(4, 30))
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 12)
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 14)
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 20)
                gasLimit = 2;
        }

        // Cap mining gas
        auto gasCap = 100;
        if (vis(Zerg_Hydralisk_Den) > 0)
            gasCap = 150;
        if (vis(Zerg_Drone) < 20 && gas(gasCap))
            gasLimit = 0;
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

        // Tech
        unitOrder ={ Zerg_Hydralisk, Zerg_Mutalisk };

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 28 && total(Zerg_Zergling) >= initialLings);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 62);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + (s >= 52) + (com(Zerg_Drone) >= 26 && s >= 64) + (com(Zerg_Drone) >= 26 && s >= 80);
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

        // All-in
        if (!needMinimumHydras && total(Zerg_Hydralisk) >= 2 && hatchCount() >= 6 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Archon) == 0) {
            if ((Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5GateGoon") && vis(Zerg_Drone) >= 30)
                activeAllinType = AllinType::Z_6HatchCrackling;
        }

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 45;
        }

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
        inBookSupply =                                  vis(Zerg_Overlord) < 4;
        unitLimits[Zerg_Zergling] =                     INT_MAX;

        wantThird =                                     Spy::getEnemyBuild() == "FFE";
        mineralThird =                                  true;
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        reserveLarva =                                  6;

        // Tech
        unitOrder ={ Zerg_Zergling, Zerg_Defiler };

        // Gas first, but wait for lings
        auto firstGas = (s >= 28 && vis(Zerg_Drone) >= 12 && total(Zerg_Zergling) >= initialLings);
        auto thirdHatch = (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0);
        auto fourthHatch = (vis(Zerg_Drone) >= 26);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch + 2 * (atPercent(Zerg_Hive, 0.5));
        buildQueue[Zerg_Extractor] =                    firstGas + (com(Zerg_Defiler_Mound) > 0);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3 && vis(Zerg_Drone) >= 12);
        buildQueue[Zerg_Queens_Nest] =                  atPercent(Zerg_Lair, 0.95);
        buildQueue[Zerg_Hive] =                         atPercent(Zerg_Queens_Nest, 0.95);
        buildQueue[Zerg_Spire] =                        com(Zerg_Defiler_Mound) > 0;
        buildQueue[Zerg_Evolution_Chamber] =            2 * vis(Zerg_Hive);
        buildQueue[Zerg_Defiler_Mound] =                hatchCount() >= 6;

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;
        upgradeQueue[Adrenal_Glands] =                  com(Zerg_Hive) > 0;
        upgradeQueue[Zerg_Carapace] =                   (com(Zerg_Evolution_Chamber) > 0) + 2 * (com(Zerg_Evolution_Chamber) > 1);
        upgradeQueue[Zerg_Melee_Attacks] =              3 * (com(Zerg_Evolution_Chamber) > 1);
        techQueue[Consume] =                            com(Zerg_Defiler_Mound) == 1;

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 400;
            unitLimits[Zerg_Drone] = 36;
        }

        // Pumping
        pumpLings = (vis(Zerg_Drone) >= 32) || lingsNeeded_ZvP() > vis(Zerg_Zergling);
        pumpScourge = com(Zerg_Spire) == 1 && total(Zerg_Scourge) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) * 2;
        pumpDefiler = com(Zerg_Defiler_Mound) == 1 && vis(Zerg_Defiler) < 2;

        // Gas
        gasLimit = vis(Zerg_Hive) > 0 ? gasMax() : 2;
        if (Spy::getEnemyBuild() != "FFE") {
            if (Util::getTime() < Time(4, 00))
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 12)
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 16)
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 20)
                gasLimit = 2;
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