#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {
        int transitionLings = 0;
    }

    void defaultZvP() {
        inOpening =                                 true;
        inBookSupply =                              true;

        wantNatural =                               (Spy::getEnemyOpener() != "Proxy9/9" || hatchCount() >= 3);
        wantThird =                                 !wantNatural || (Util::getTime() < Time(3, 30) && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) == 0 && Spy::getEnemyBuild() == "Unknown") || Spy::getEnemyBuild() == "FFE" || (Spy::enemyFastExpand() && hatchCount() >= 3);

        wallNat =                                   (Spy::getEnemyOpener() == "Proxy9/9" && Stations::getStations(PlayerState::Self).size() >= 2) || (Spy::getEnemyBuild() == "FFE" && hatchCount() >= 4);
        wallMain =                                  Terrain::isPocketNatural() && !wantThird;
        wallThird =                                 Stations::getStations(PlayerState::Self).size() >= 3;

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
            trackables[Protoss_Dragoon] += 0.2;
        if (Players::hasUpgraded(PlayerState::Enemy, Leg_Enhancements, 1))
            trackables[Protoss_Zealot] += 0.2;
        if (Players::hasUpgraded(PlayerState::Enemy, Protoss_Ground_Weapons, 1))
            trackables[Protoss_Zealot] += 0.5;

        auto inBoundUnit = [&](auto &u) {
            if (!Terrain::getEnemyMain())
                return true;
            const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();

            // Check if we know they weren't at home and are missing on the map for 45 seconds
            if (!u->isThreatening() && (!Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), u->getPosition()) || Util::getTime() < Time(5, 00))
                && (!Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), u->getPosition()) || Util::getTime() < Time(3, 30)))
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 45);
            return false;
        };

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end() && (inOpening || unit.isThreatening())) {
                arrivalValue += idx->second / 1.25;
                if (inBoundUnit(u) || vis(Zerg_Zergling) < 6)
                    arrivalValue += idx->second / 1.15;
            }
        }

        // Make less if we have some other units outside our opening
        if (total(Zerg_Zergling) >= 16 && vis(Zerg_Zergling) >= 10) {
            arrivalValue -= vis(Zerg_Hydralisk);
            arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 6.0;
        }
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

        auto initialValue = 6;

        // 2Gate
        if (Spy::getEnemyBuild() == "2Gate") {
            initialValue = 10;
            if (Spy::getEnemyOpener() == "Proxy9/9")
                initialValue = 16;
            if (Spy::getEnemyOpener() == "9/9")
                initialValue = 14;
            if (Spy::getEnemyOpener() == "10/12")
                initialValue = 12;
            if (Spy::getEnemyOpener() == "10/15")
                initialValue = 4;
        }

        // FFE
        if (Spy::getEnemyBuild() == "FFE") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "Gateway" || hideTech)
                initialValue = 6;
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

        // If they're aggressive, delay remaining build until we've made a transitional count (at 10 lings, we have minerals)
        transitionLings = min(initialValue, 10);
        if (Spy::getEnemyOpener() == "Proxy9/9")
            transitionLings = 16;

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
        pressure =                                      (total(Zerg_Mutalisk) >= 6);
        wantThird =                                     false;
        wallNat =                                       hatchCount() >= 2;

        // Tech
        unitOrder ={ Zerg_Mutalisk, Zerg_Zergling };

        auto firstGas = (s >= 20 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10);
        auto secondGas = (hatchCount() >= 3 && vis(Zerg_Drone) >= 20) || (com(Zerg_Spire) > 0);

        auto thirdHatch = (total(Zerg_Mutalisk) >= 6 && vis(Zerg_Drone) >= 20) || (Spy::getEnemyBuild() == "FFE" && vis(Zerg_Spire) > 0);
        auto fourthHatch = (total(Zerg_Mutalisk) >= 10 && vis(Zerg_Drone) >= 26);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch + (vis(Zerg_Drone) >= 30);
        buildQueue[Zerg_Extractor] =                    firstGas + secondGas;
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Spire) > 0;
        upgradeQueue[Zerg_Flyer_Carapace] =             total(Zerg_Mutalisk) >= 12;

        // Pumping
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling) || (hatchCount() >= 4 && !gas(100));
        pumpScourge = com(Zerg_Spire) == 1 && total(Zerg_Mutalisk) < 5 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > vis(Zerg_Scourge);
        pumpMutas = !pumpScourge && com(Zerg_Spire) > 0 && gas(80);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 45;
        }

        // All-in
        if ((Spy::getEnemyBuild() == "2Gate" || Spy::getEnemyBuild() == "1GateCore") && Spy::enemyFastExpand()) {
            static Time expandTiming = Util::getTime();
            if (expandTiming < Time(4, 00) && vis(Zerg_Zergling) >= transitionLings)
                activeAllinType = AllinType::Z_5HatchSpeedling;
            //else if (total(Zerg_Mutalisk) >= 24)
            //    activeAllinType = AllinType::Z_8HatchCrackling;
        }
        //if (Spy::getEnemyBuild() == "FFE") {
        //    if (total(Zerg_Mutalisk) >= 18)
        //        activeAllinType = AllinType::Z_8HatchCrackling;
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
        inOpening =                                     total(Zerg_Hydralisk) < 64 && total(Zerg_Mutalisk) < 18;
        inBookSupply =                                  vis(Zerg_Overlord) < 4;

        focusUnit =                                     Zerg_Mutalisk;
        planEarly =                                     wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        reserveLarva =                                  2;

        // Gas first, but wait for lings
        auto hydraOpen = (Spy::getEnemyBuild() != "2Gate" && Spy::getEnemyBuild() != "1GateCore" && Spy::getEnemyTransition() != "5GateGoon" && Spy::getEnemyTransition() != "CorsairGoon") || total(Zerg_Hydralisk) > 0;
        auto mutaOpen = !hydraOpen || Spy::getEnemyBuild() == "FFE" || Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0 || total(Zerg_Mutalisk) > 0;
        auto hydraDone = !hydraOpen || total(Zerg_Hydralisk) >= 32;
        auto mutaDone = !mutaOpen || total(Zerg_Mutalisk) >= 5;

        // Gas timings
        auto firstGas = (Util::getTime() > Time(2, 32)) || (s >= 40);
        auto secondGas = (Spy::getEnemyBuild() == "FFE") ? (com(Zerg_Hydralisk_Den) > 0 || total(Zerg_Mutalisk) >= 9) : (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 20);
        auto thirdGas = (mutaOpen && hatchCount() >= 5 && Util::getTime() > Time(8, 00)) || (mutaOpen && !hydraOpen && total(Zerg_Mutalisk) >= 9) || (hydraOpen && hydraDone);

        // Hatch timings
        auto thirdHatch = (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0 && total(Zerg_Zergling) >= transitionLings) || (s >= 34 && total(Zerg_Zergling) >= transitionLings);
        auto fourthHatch = (vis(Zerg_Drone) >= 28 && vis(Zerg_Spire) > 0) || (total(Zerg_Scourge) >= 6);

        // Overlord timings
        auto thirdOverlord = Spy::getEnemyBuild() == "2Gate" ? (s >= 34) : (s >= 32);
        auto fourthOverlord = (s >= 52 || vis(Zerg_Spire) > 0);

        // Tech
        unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker };
        if (!hydraOpen)
            unitOrder ={ Zerg_Mutalisk, Zerg_Zergling };
        if (!mutaOpen)
            unitOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Mutalisk };

        // Build
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + thirdOverlord + fourthOverlord;
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch + (vis(Zerg_Drone) >= 30) + (vis(Zerg_Drone) >= 38);
        buildQueue[Zerg_Extractor] =                    firstGas + secondGas + thirdGas;
        buildQueue[Zerg_Lair] =                         (Util::getTime() > Time(3, 20) && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.9) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Hydralisk_Den] =                (hydraOpen && mutaDone && vis(Zerg_Drone) >= 26);
        buildQueue[Zerg_Evolution_Chamber] =            (vis(Zerg_Hydralisk_Den) > 0);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;
        upgradeQueue[Muscular_Augments] =               hydraOpen && com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Zerg_Missile_Attacks] =            hydraOpen && com(Zerg_Evolution_Chamber) > 0 && com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines] =                  hydraOpen && hydraSpeed();
        upgradeQueue[Zerg_Flyer_Carapace] =             2 * (mutaDone && !hydraOpen);

        // Hydra pump - 26 then any
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && hydraOpen && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) * 2 + 2;
        auto firstHydraPump = com(Zerg_Hydralisk_Den) > 0 && hydraOpen && mutaDone && vis(Zerg_Drone) >= 30 && total(Zerg_Hydralisk) < 26 && hatchCount() >= 5;
        auto secondHydraPump = com(Zerg_Hydralisk_Den) > 0 && hydraOpen && mutaDone && vis(Zerg_Drone) >= 38;

        // Muta pump - 5 then any
        auto firstMutaPump = com(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 30 && mutaOpen && total(Zerg_Mutalisk) < 5;
        auto secondMutaPump = com(Zerg_Spire) > 0 && !hydraOpen && mutaOpen && vis(Zerg_Drone) >= 45;

        // Scourge pump
        auto firstScourgePump = com(Zerg_Spire) > 0 && total(Zerg_Hydralisk) < 9 && total(Zerg_Mutalisk) < 5 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > vis(Zerg_Scourge);

        // Ling pump
        auto needMinimumLings = (Util::getTime() > Time(6, 00) && vis(Zerg_Zergling) < 2);
        auto firstLingPump = lingsNeeded_ZvP() > vis(Zerg_Zergling) && ((Util::getTime() < Time(6, 00) && Spy::getEnemyBuild() == "FFE") || (Util::getTime() < Time(9, 00) && Spy::getEnemyBuild() != "FFE"));
        auto secondLingPump = !hydraOpen && vis(Zerg_Drone) >= 45;
        auto thirdLingPump = !hydraOpen && vis(Zerg_Drone) >= 45;

        pumpLings = firstLingPump || secondLingPump || thirdLingPump || needMinimumLings;
        pumpScourge = firstScourgePump;
        pumpMutas = firstMutaPump || secondMutaPump;
        pumpHydras = firstHydraPump || secondHydraPump || needMinimumHydras;
        pumpLurker = Broodwar->self()->hasResearched(Lurker_Aspect) && total(Zerg_Lurker) < 4;

        // All-in
        if (!needMinimumHydras && hatchCount() >= 3 && !Terrain::isPocketNatural()) {
            if (Spy::enemyGreedy() || Spy::getEnemyOpener() == "Nexus")
                activeAllinType = AllinType::Z_5HatchSpeedling;
        }

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 45;
            unitLimits[Zerg_Hydralisk] = 200;
            unitLimits[Zerg_Mutalisk] = 100;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "FFE" && !Spy::enemyFastExpand()) {
            if (Util::getTime() < Time(4, 00))
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 10)
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) < 16)
                gasLimit = 1;
            else if (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) < 20)
                gasLimit = 2;
        }
    }

    void ZvP3HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 48;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;

        focusUnit =                                     Zerg_Hydralisk;
        planEarly =                                     wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        mineralThird =                                  false;
        hideTech =                                      true;
        reserveLarva =                                  0;

        rush =                                          hydraRange() && total(Zerg_Hydralisk) >= 12;
        pressure =                                      rush;

        // Tech
        unitOrder ={ Zerg_Hydralisk, Zerg_Mutalisk };

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 26 && total(Zerg_Zergling) >= transitionLings) || (s >= 40);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + (total(Zerg_Hydralisk) >= 9 && vis(Zerg_Drone) >= 22) * 2;
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 3);
        buildQueue[Zerg_Hydralisk_Den] =                (vis(Zerg_Drone) >= 14 && s >= 34);

        // Upgrades
        upgradeQueue[Muscular_Augments] =               com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines] =                  hydraSpeed();

        // Hydra pump
        auto firstHydraPump = (com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Drone) >= 19);
        auto secondHydraPump = (hatchCount() >= 5 && vis(Zerg_Drone) >= 36);

        // Pumping
        pumpHydras = firstHydraPump || secondHydraPump;
        pumpLings = (com(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) < 6);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = (firstHydraPump ? 19 : 36);
        }

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 11)
            gasLimit = 3;
    }

    void ZvP4HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 24 && total(Zerg_Zergling) < 100;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;

        focusUnit =                                     Zerg_Hydralisk;
        planEarly =                                     wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        pressure =                                      Util::getTime() > Time(7, 00) && !isPreparingAllIn();

        // Tech
        unitOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Mutalisk };
        if (Spy::getEnemyTransition() == "4Gate")
            unitOrder ={ Zerg_Hydralisk, Zerg_Mutalisk, Zerg_Lurker };

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 28 && total(Zerg_Zergling) >= transitionLings) || (s >= 40);
        auto fourthHatch = (s >= 54 && vis(Zerg_Drone) >= 18 && total(Zerg_Hydralisk) > 0);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 60) + (s >= 74);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch + (s >= 100 && vis(Zerg_Drone) >= 28);
        buildQueue[Zerg_Extractor] =                    (s >= 26 && vis(Zerg_Drone) >= 13 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 38 && vis(Zerg_Drone) >= 14 && gas(30));
        buildQueue[Zerg_Lair] =                         (hatchCount() >= 5);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore";
        upgradeQueue[Metabolic_Boost] =                 total(Zerg_Zergling) >= 18 && com(Zerg_Hydralisk_Den) > 0;
        if (total(Zerg_Hydralisk) >= 6) {
            upgradeQueue[Grooved_Spines] =              rangeFirst ? com(Zerg_Hydralisk_Den) > 0 : hydraSpeed();
            upgradeQueue[Muscular_Augments] =           rangeFirst ? hydraRange() : com(Zerg_Hydralisk_Den) > 0;
            upgradeQueue[Zerg_Carapace] =               com(Zerg_Evolution_Chamber) > 0 && total(Zerg_Hydralisk) >= 18;
        }

        // Pumping
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 2;
        pumpHydras = (vis(Zerg_Drone) >= 28 || needMinimumHydras) && gas(25);
        pumpLings = (!needMinimumHydras && lingsNeeded_ZvP() > vis(Zerg_Zergling));

        // All-in
        if (!needMinimumHydras && total(Zerg_Hydralisk) >= 2 && hatchCount() >= 3) {
            if (Spy::enemyGreedy() || Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5GateGoon")
                activeAllinType = AllinType::Z_5HatchSpeedling;
        }

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 200;
            unitLimits[Zerg_Drone] = 28;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "FFE") {
            if (pumpHydras)
                gasLimit = min(5, gasMax());
            else if (vis(Zerg_Hydralisk_Den) > 0 && Util::getTime() < Time(4, 30))
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 11)
                gasLimit = 0;
            else if (vis(Zerg_Drone) < 18)
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 28)
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
        mineralThird =                                  false;
        planEarly =                                     wantNatural && wantThird && hatchCount() < 3 && s >= 24;

        auto pressureTime = (Spy::getEnemyTransition() == "Speedlot" || Spy::getEnemyTransition() == "Sairlot") ? Time(9, 00) : Time(7, 30);
        pressure = Util::getTime() > pressureTime && !isPreparingAllIn() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0;

        // Tech
        unitOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Zergling, Zerg_Defiler };

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 28 && total(Zerg_Zergling) >= transitionLings) || (s >= 40);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 62);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + (s >= 52) + (com(Zerg_Drone) >= 26 && s >= 64) + (com(Zerg_Drone) >= 40 && s >= 120);
        buildQueue[Zerg_Extractor] =                    (s >= 30 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 30 && s >= 70) + (hatchCount() >= 6 && Researching::haveOrResearching(Lurker_Aspect));
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 60);
        buildQueue[Zerg_Evolution_Chamber] =            (s >= 70) + (s >= 150);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2 || Spy::getEnemyBuild() == "1GateCore";
        if (total(Zerg_Hydralisk) > 0) {
            upgradeQueue[Grooved_Spines] =              rangeFirst ? (vis(Zerg_Hydralisk_Den) && gas(150)) : hydraSpeed();
            upgradeQueue[Muscular_Augments] =           rangeFirst ? hydraRange() : (vis(Zerg_Hydralisk_Den) && gas(150));
            upgradeQueue[Zerg_Missile_Attacks] =        (com(Zerg_Evolution_Chamber) > 0) + (com(Zerg_Evolution_Chamber) > 1);
            upgradeQueue[Zerg_Carapace] =               (com(Zerg_Evolution_Chamber) > 1);
            techQueue[Burrowing] =                      com(Zerg_Drone) >= 36;
            techQueue[Lurker_Aspect] =                  hydraRange() && hydraSpeed();
        }
        upgradeQueue[Metabolic_Boost] =             total(Zerg_Hydralisk) >= 2;

        // Pumping
        auto firstPump = (Util::getTime() > Time(6, 30) && hatchCount() >= 5 && total(Zerg_Hydralisk) < 26);
        auto secondPump = (total(Zerg_Drone) >= 44 && (hydraRange() || hydraSpeed()));
        auto needMinimumHydras = (com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) * 2 + 2);

        // Lings
        auto needMinimumLings = (Util::getTime() > Time(6, 00) && vis(Zerg_Zergling) < 2);
        auto firstLingPump = lingsNeeded_ZvP() > vis(Zerg_Zergling) && com(Zerg_Hydralisk_Den) == 0;

        pumpHydras = (needMinimumHydras || firstPump || secondPump);
        pumpLings = firstLingPump || needMinimumLings;
        pumpLurker = Broodwar->self()->hasResearched(Lurker_Aspect) && total(Zerg_Lurker) < 4;

        // All-in
        if (Spy::getEnemyOpener() == "Nexus")
            activeAllinType = AllinType::Z_5HatchSpeedling;
        if (!needMinimumHydras && total(Zerg_Hydralisk) >= 12 && hatchCount() >= 5) {
            if (Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5GateGoon")
                activeAllinType = AllinType::Z_8HatchCrackling;
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
        inTransition =                                  vis(Zerg_Lair) > 0 || vis(Zerg_Hive) > 0;
        inOpening =                                     true;
        inBookSupply =                                  vis(Zerg_Overlord) < 4;
        unitLimits[Zerg_Zergling] =                     INT_MAX;

        wantThird =                                     false;
        mineralThird =                                  true;
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        reserveLarva =                                  6;

        // Tech
        unitOrder ={ Zerg_Zergling, Zerg_Defiler };

        // Gas first, but wait for lings
        auto firstGas = (s >= 28 && vis(Zerg_Drone) >= 12 && total(Zerg_Zergling) >= transitionLings) || (s >= 40);
        auto thirdHatch = (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0);
        auto fourthHatch = (vis(Zerg_Drone) >= 26);
        auto fifthHatch = (hatchCount() >= 4 && vis(Zerg_Queens_Nest) > 0);
        auto sixHatch = (hatchCount() >= 5);

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48);
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch + fifthHatch + sixHatch + 2 * (vis(Zerg_Defiler_Mound) > 0);
        buildQueue[Zerg_Extractor] =                    firstGas + (com(Zerg_Defiler_Mound) > 0);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3 && vis(Zerg_Drone) >= 12);
        buildQueue[Zerg_Queens_Nest] =                  atPercent(Zerg_Lair, 0.95);
        buildQueue[Zerg_Hive] =                         atPercent(Zerg_Queens_Nest, 0.95);
        buildQueue[Zerg_Evolution_Chamber] =            2 * (vis(Zerg_Drone) >= 26 && vis(Zerg_Hive) > 0);
        buildQueue[Zerg_Defiler_Mound] =                hatchCount() >= 6;

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;
        upgradeQueue[Adrenal_Glands] =                  com(Zerg_Hive) > 0;
        upgradeQueue[Zerg_Carapace] =                   (com(Zerg_Evolution_Chamber) > 0) + 2 * (com(Zerg_Evolution_Chamber) > 1);
        upgradeQueue[Zerg_Melee_Attacks] =              3 * (com(Zerg_Evolution_Chamber) > 1);
        techQueue[Plague] =                             com(Zerg_Defiler_Mound) == 1;
        techQueue[Consume] =                            Researching::haveOrResearching(Plague);

        // Limits
        if (com(Zerg_Spawning_Pool) > 0) {
            unitLimits[Zerg_Zergling] = 400;
            unitLimits[Zerg_Drone] = 45;
        }

        // Ling pump
        auto firstLingPump = (vis(Zerg_Drone) >= 32 && hatchCount() <= 6) || lingsNeeded_ZvP() > vis(Zerg_Zergling);
        auto secondLingPump = (vis(Zerg_Drone) >= 45 && hatchCount() >= 8);

        pumpLings = firstLingPump || secondLingPump;
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
                currentTransition = "4HatchHydra";
            }

            if (Spy::getEnemyBuild() != "Unknown" && Spy::getEnemyBuild() != "FFE" && (currentTransition == "6HatchHydra" || currentTransition == "3HatchHydra"))
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