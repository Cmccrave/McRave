#include "Builds/Zerg/ZergBuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Macro/Researching/Researching.h"
#include "Main/Common.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

namespace McRave::BuildOrder::Zerg {

    namespace {
        int transitionLings = 0;
    } // namespace

    void defaultZvP()
    {
        inOpening    = true;
        inBookSupply = true;

        wantNatural = true;
        if (Spy::getEnemyOpener() == P_Horror_9_9 || (Spy::getEnemyOpener() == P_Proxy_9_9 && currentOpener == Z_12Hatch))
            wantNatural = hatchCount() >= 3;

        wantThird = (Spy::enemyFastExpand() && hatchCount() >= 4) || hatchCount() >= 5 || Spy::getEnemyBuild() == P_FFE || Spy::getEnemyBuild() == "Unknown";
        if (!wantNatural)
            wantThird = false;

        wallNat = wantNatural && ((Spy::getEnemyOpener() == P_Proxy_9_9 && Stations::getStations(PlayerState::Self).size() >= 2) || (Spy::getEnemyBuild() == P_FFE && hatchCount() >= 4) ||
                                  (Spy::getEnemyBuild() == P_2Gate && hatchCount() >= 2) || Spy::getEnemyTransition() == P_Speedlot || Spy::getEnemyTransition() == P_Rush);

        wallMain  = Terrain::isPocketNatural() && !wantThird;
        wallThird = Stations::getStations(PlayerState::Self).size() >= 3;

        proxy           = false;
        hideTech        = false;
        rush            = false;
        pressure        = false;
        transitionReady = false;
        planEarly       = false;
        gasTrick        = false;
        mineralThird    = false;
        reserveLarva    = 0;

        gasLimit = gasMax();

        desiredDetection = Zerg_Overlord;
        focusUnit        = UnitTypes::None;
    }

    int inboundUnits_ZvP()
    {
        static map<BWAPI::UnitType, double> trackables = {{Protoss_Zealot, 2.0}, {Protoss_Dragoon, 2.0}, {Protoss_Dark_Templar, 5.0}};
        if (Players::hasUpgraded(PlayerState::Enemy, Singularity_Charge, 1))
            trackables[Protoss_Dragoon] += 0.2;
        if (Players::hasUpgraded(PlayerState::Enemy, Leg_Enhancements, 1))
            trackables[Protoss_Zealot] += 0.2;
        if (Players::hasUpgraded(PlayerState::Enemy, Protoss_Ground_Weapons, 1))
            trackables[Protoss_Zealot] += 0.5;

        auto softCap = vis(Zerg_Drone) < 28 ? 24 : 64;

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end() && (inOpening || unit.isThreatening())) {
                arrivalValue += idx->second / 1.25;
                if (Units::inBoundUnit(unit) && com(Zerg_Zergling) < softCap)
                    arrivalValue += idx->second / 1.15;
            }
        }

        // Make less if we have some other units outside our opening
        if (total(Zerg_Zergling) >= transitionLings || com(Zerg_Sunken_Colony) > 0) {
            arrivalValue -= vis(Zerg_Hydralisk);
            arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 6.0;
        }

        return int(arrivalValue);
    }

    int timingUnits_ZvP()
    {
        if (Spy::getEnemyTransition() == P_4Gate)
            return Util::getTime().minutes * 3;
        if (Spy::getEnemyBuild() == P_2Gate && Util::getTime() > Time(3, 30))
            return Util::getTime().minutes * 3;
        if (Spy::getEnemyBuild() == P_1GateCore && Util::getTime() > Time(3, 30))
            return Util::getTime().minutes * 2;
        return 0;
    }

    int lingsNeeded_ZvP()
    {
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        auto initialValue = 6;
        if (Spy::enemyTurtle())
            initialValue = 2;

        // 2Gate
        if (Spy::getEnemyBuild() == P_2Gate) {
            initialValue = 6;
            if (Spy::getEnemyOpener() == P_Horror_9_9)
                initialValue = 20;
            if (Spy::getEnemyOpener() == P_Proxy_9_9)
                initialValue = 14;
            if (Spy::getEnemyOpener() == P_9_9)
                initialValue = 12;
            if (Spy::getEnemyOpener() == P_10_12)
                initialValue = 10;
            if (Spy::getEnemyOpener() == P_10_15)
                initialValue = 2;

            if (currentOpener == Z_12Hatch || currentOpener == Z_12Pool)
                initialValue += 4;
        }

        // FFE
        if (Spy::getEnemyBuild() == P_FFE) {
            initialValue = 2;
            if (Spy::getEnemyOpener() == P_Gateway || hideTech)
                initialValue = 6;
        }

        // 1GC
        if (Spy::getEnemyBuild() == P_1GateCore) {
            initialValue = 4;
            if (Spy::getEnemyOpener() == P_ZCore || Spy::getEnemyOpener() == P_ZZCore || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) > 0)
                initialValue = 6;
        }

        // CannonRush
        if (Spy::getEnemyBuild() == P_CannonRush)
            initialValue = 12;
        if (Spy::getEnemyTransition() == U_WorkerRush)
            initialValue = 18;

        // If they're aggressive, delay remaining build until we've made a transitional count (at 8 lings, we have minerals)
        transitionLings = min(initialValue, 8);

        // Minimum lings
        auto minimumLings = 2;
        if (vis(Zerg_Zergling) < minimumLings)
            return minimumLings;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

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
        inTransition = vis(Zerg_Lair) > 0;
        inOpening    = total(Zerg_Mutalisk) < 36;
        inBookSupply = vis(Zerg_Overlord) < 3;

        focusUnit    = Zerg_Mutalisk;
        reserveLarva = (Spy::getEnemyBuild() == P_FFE || Spy::enemyFastExpand()) ? 6 : 0;
        hideTech     = true;
        pressure     = (total(Zerg_Mutalisk) >= 6 && Players::getDeadCount(PlayerState::Enemy, Protoss_Probe) < 10 && Spy::getEnemyBuild() != P_1GateCore && Spy::getEnemyBuild() != P_2Gate);
        wantThird    = hatchCount() >= 4;
        wallNat      = hatchCount() >= 2;

        // Order
        unitOrder = mutalurk;

        auto firstGas  = (s >= 20 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10);
        auto secondGas = (hatchCount() >= 3 && vis(Zerg_Drone) >= 20) || (com(Zerg_Spire) > 0) || (vis(Zerg_Drone) >= 22);

        auto thirdHatch  = (com(Zerg_Spire) == 0 && s >= 50 && vis(Zerg_Drone) >= 22 && Spy::enemyFastExpand()) || (com(Zerg_Spire) == 1 && total(Zerg_Mutalisk) >= 6 && vis(Zerg_Drone) >= 22);
        auto fourthHatch = (total(Zerg_Mutalisk) >= 10 && vis(Zerg_Drone) >= 26);

        // Buildings
        buildQueue[Zerg_Overlord]  = 1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Hatchery]  = 2 + thirdHatch + fourthHatch + (vis(Zerg_Drone) >= 30);
        buildQueue[Zerg_Extractor] = firstGas + secondGas;
        buildQueue[Zerg_Lair]      = (s >= 24 && gas(80) && total(Zerg_Zergling) >= transitionLings);
        buildQueue[Zerg_Spire]     = (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);

        // Upgrades
        upgradeQueue[Metabolic_Boost]     = hatchCount() >= 3;
        upgradeQueue[Zerg_Flyer_Carapace] = total(Zerg_Mutalisk) >= 12;

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 30 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] = lingsNeeded_ZvP() > vis(Zerg_Zergling) || (hatchCount() >= 4 && vis(Zerg_Drone) >= 30);
        zergUnitPump[Zerg_Scourge]  = com(Zerg_Spire) == 1 && total(Zerg_Mutalisk) < 5 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > vis(Zerg_Scourge);
        zergUnitPump[Zerg_Mutalisk] = !zergUnitPump[Zerg_Scourge] && com(Zerg_Spire) > 0;

        // All-in
        if (Spy::getEnemyOpener() == P_Nexus)
            activeAllinType = AllinType::Z_3HatchSpeedling;
        if ((Spy::getEnemyBuild() == P_2Gate || Spy::getEnemyBuild() == P_1GateCore) && Spy::enemyFastExpand()) {
            static Time expandTiming = Util::getTime();
            if (expandTiming < Time(4, 00) && vis(Zerg_Zergling) >= transitionLings)
                activeAllinType = AllinType::Z_5HatchSpeedling;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "Unknown" && Spy::getEnemyBuild() != P_FFE && !Spy::enemyFastExpand()) {
            if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 10)
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && Spy::getEnemyBuild() == P_2Gate && Util::getTime() < Time(3, 45))
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && Util::getTime() < Time(3, 45))
                gasLimit = 1;
            else if (vis(Zerg_Spire) > 0 && Util::getTime() < Time(4, 30))
                gasLimit = 1;
        }
        if (gasLimit > 0 && (Spy::getEnemyBuild() == "Unknown" || Spy::getEnemyBuild() == P_2Gate || Spy::getEnemyBuild() == P_1GateCore) && !atPercent(Zerg_Lair, 0.5))
            gasLimit = capGas(100);
    }

    void ZvP3HatchMuta()
    {
        inTransition = hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpening    = total(Zerg_Hydralisk) < 64 && total(Zerg_Mutalisk) < 18;
        inBookSupply = vis(Zerg_Overlord) < 4;

        focusUnit    = Zerg_Mutalisk;
        planEarly    = wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        reserveLarva = 2;

        // Gas first, but wait for lings
        auto hydraOpen = (Spy::getEnemyBuild() == P_FFE && Spy::getEnemyTransition() != P_5GateGoon && Spy::getEnemyTransition() != P_CorsairGoon && Spy::getEnemyTransition() != P_Speedlot) || //
                         (Spy::getEnemyBuild() == P_1GateCore && Spy::getEnemyTransition() != P_4Gate) ||                                                                                        //
                         total(Zerg_Hydralisk) > 0;
        auto mutaOpen  = !hydraOpen;
        auto hydraDone = !hydraOpen || total(Zerg_Hydralisk) >= 32;
        auto mutaDone  = !mutaOpen || total(Zerg_Mutalisk) >= 5;

        auto mutaLingOpen = mutaOpen && Spy::getEnemyBuild() == P_2Gate && Spy::getEnemyBuild() == P_Rush;

        // Gas timings
        auto firstGas  = (Util::getTime() > Time(2, 32)) || (s >= 40);
        auto secondGas = (vis(Zerg_Evolution_Chamber) > 0 && vis(Zerg_Drone) >= 32) || (!wantThird && vis(Zerg_Drone) >= 26) || (mutaOpen && vis(Zerg_Drone) >= 30);
        auto thirdGas  = (mutaOpen && hatchCount() >= 5 && Util::getTime() > Time(8, 00)) || (hydraOpen && hydraDone);

        // Hatch timings
        auto thirdHatch  = (s >= 28 && vis(Zerg_Drone) >= 13 && vis(Zerg_Extractor) > 0 && total(Zerg_Zergling) >= transitionLings) || (s >= 34);
        auto fourthHatch = (vis(Zerg_Drone) >= 28 && vis(Zerg_Spire) > 0) || (total(Zerg_Scourge) >= 6);
        auto fifthHatch  = (vis(Zerg_Drone) >= 30);
        auto sixHatch    = (vis(Zerg_Drone) >= 38);

        // Overlord timings
        auto thirdOverlord  = Spy::getEnemyBuild() == P_2Gate ? (s >= 34) : (s >= 32);
        auto fourthOverlord = (s >= 50 || vis(Zerg_Spire) > 0);

        // Order
        unitOrder = mutalurk;
        if (!mutaOpen)
            unitOrder = hydralurk;

        // Build
        buildQueue[Zerg_Overlord]          = 1 + (s >= 18) + thirdOverlord + fourthOverlord;
        buildQueue[Zerg_Hatchery]          = 2 + thirdHatch + fourthHatch + fifthHatch + sixHatch;
        buildQueue[Zerg_Extractor]         = firstGas + secondGas + thirdGas;
        buildQueue[Zerg_Lair]              = (Util::getTime() > Time(3, 20) && gas(80) && vis(Zerg_Drone) >= 14);
        buildQueue[Zerg_Spire]             = (s >= 32 && atPercent(Zerg_Lair, 0.9) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Hydralisk_Den]     = (hydraOpen && mutaDone && vis(Zerg_Drone) >= 32);
        buildQueue[Zerg_Evolution_Chamber] = (vis(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Drone) >= 32) || (vis(Zerg_Drone) >= 26 && mutaLingOpen);

        // Upgrades
        upgradeQueue[Metabolic_Boost]      = (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 15);
        upgradeQueue[Muscular_Augments]    = hydraOpen && com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Zerg_Missile_Attacks] = hydraOpen && com(Zerg_Evolution_Chamber) > 0 && com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines]       = hydraOpen && hydraSpeed();
        upgradeQueue[Zerg_Flyer_Carapace]  = 2 * (mutaDone && !hydraOpen);
        upgradeQueue[Zerg_Melee_Attacks]   = mutaLingOpen;

        // Hydra pump - 26 then any
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && hydraOpen && vis(Zerg_Hydralisk) < 4;
        auto firstHydraPump    = com(Zerg_Hydralisk_Den) > 0 && hydraOpen && mutaDone && vis(Zerg_Drone) >= 30 && total(Zerg_Hydralisk) < 26 && hatchCount() >= 5;
        auto secondHydraPump   = com(Zerg_Hydralisk_Den) > 0 && hydraOpen && mutaDone && vis(Zerg_Drone) >= 38;

        // Muta pump - 5 then any
        auto needMinimumMutas = com(Zerg_Spire) > 0 && mutaOpen && (hydraOpen ? total(Zerg_Mutalisk) < 5 : vis(Zerg_Mutalisk) < 12);
        auto firstMutaPump    = com(Zerg_Spire) > 0 && !hydraOpen && mutaOpen && vis(Zerg_Drone) >= 30;

        // Scourge pump
        auto firstScourgePump = com(Zerg_Spire) > 0 && total(Zerg_Hydralisk) < 9 && total(Zerg_Mutalisk) < 5 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > vis(Zerg_Scourge);

        // Ling pump
        auto needMinimumLings = (Util::getTime() > Time(6, 00) && vis(Zerg_Zergling) < 2);
        auto firstLingPump    = lingsNeeded_ZvP() > vis(Zerg_Zergling) &&
                             ((Util::getTime() < Time(6, 00) && Spy::getEnemyBuild() == P_FFE) || (Util::getTime() < Time(8, 00) && Spy::getEnemyBuild() != P_FFE));
        auto secondLingPump = !hydraOpen && vis(Zerg_Drone) >= 45;
        auto thirdLingPump  = !hydraOpen && vis(Zerg_Drone) >= 45;

        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 45 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = firstLingPump || secondLingPump || thirdLingPump || needMinimumLings;
        zergUnitPump[Zerg_Scourge]   = firstScourgePump;
        zergUnitPump[Zerg_Mutalisk]  = firstMutaPump || needMinimumMutas;
        zergUnitPump[Zerg_Hydralisk] = firstHydraPump || secondHydraPump || needMinimumHydras;
        zergUnitPump[Zerg_Lurker]    = Broodwar->self()->hasResearched(Lurker_Aspect) && total(Zerg_Lurker) < 4;

        // All-in
        if (Spy::getEnemyOpener() == P_Nexus)
            activeAllinType = AllinType::Z_3HatchSpeedling;
        if (!needMinimumHydras && hatchCount() >= 3 && !Terrain::isPocketNatural()) {
            if (Spy::enemyGreedy())
                activeAllinType = AllinType::Z_5HatchSpeedling;
        }
        if (mutaDone) {
            if (Spy::getEnemyTransition() == P_5GateGoon)
                activeAllinType = AllinType::Z_9HatchCrackling;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "Unknown" && Spy::getEnemyBuild() != P_FFE && !Spy::enemyFastExpand()) {
            if (Spy::getEnemyBuild() == P_2Gate && Util::getTime() < Time(3, 30))
                gasLimit = 0;
            else if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 13)
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) < 16)
                gasLimit = 1;
            else if (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) < 20)
                gasLimit = 2;
        }
        if (gasLimit > 0 && (Spy::getEnemyBuild() == "Unknown" || Spy::getEnemyBuild() == P_2Gate) && !atPercent(Zerg_Lair, 0.25))
            gasLimit = capGas(100);
    }

    void ZvP3HatchHydra()
    {
        inTransition = vis(Zerg_Hydralisk_Den) > 0;
        inOpening    = total(Zerg_Hydralisk) < 48;
        inBookSupply = vis(Zerg_Overlord) < 5;

        focusUnit    = Zerg_Hydralisk;
        planEarly    = wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);
        mineralThird = false;
        hideTech     = true;
        reserveLarva = 0;

        rush     = hydraRange() && total(Zerg_Hydralisk) >= 12 && Players::getCompleteCount(PlayerState::Enemy, Protoss_Photon_Cannon) < 3;
        pressure = rush;

        // Order
        unitOrder = hydralurk;

        // Third hatch first, but wait for lings
        auto thirdHatch  = (s >= 26 && total(Zerg_Zergling) >= transitionLings && Spy::getEnemyBuild() != "Unknown") || (s >= 36);
        auto fourthHatch = (total(Zerg_Hydralisk) >= 9 && vis(Zerg_Drone) >= 22);

        // Buildings
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);
        buildQueue[Zerg_Hatchery]      = 2 + thirdHatch + fourthHatch * 2;
        buildQueue[Zerg_Extractor]     = (hatchCount() >= 3);
        buildQueue[Zerg_Hydralisk_Den] = (vis(Zerg_Drone) >= 14 && s >= 34);

        // Upgrades
        upgradeQueue[Muscular_Augments] = com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines]    = hydraSpeed();

        // Hydra pump
        auto firstHydraPump  = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Drone) >= 19 && (rush || total(Zerg_Hydralisk) < 9);
        auto secondHydraPump = hatchCount() >= 5 && vis(Zerg_Drone) >= 36;

        // Pumping
        zergUnitPump[Zerg_Drone] |= (vis(Zerg_Drone) < 19 && com(Zerg_Spawning_Pool) > 0) || (!rush && vis(Zerg_Drone) < 36);
        zergUnitPump[Zerg_Zergling] |= (com(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) < 6) || (lingsNeeded_ZvP() > vis(Zerg_Zergling) && com(Zerg_Hydralisk_Den) == 0);
        zergUnitPump[Zerg_Hydralisk] |= firstHydraPump || secondHydraPump;

        // Gas
        gasLimit = gasMax();
        if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 11)
            gasLimit = 0;
    }

    void ZvP4HatchHydra()
    {
        inTransition = vis(Zerg_Hydralisk_Den) > 0;
        inOpening    = total(Zerg_Hydralisk) < 24 && total(Zerg_Zergling) < 100;
        inBookSupply = vis(Zerg_Overlord) < 5;

        focusUnit = Zerg_Hydralisk;
        planEarly = wantNatural && wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);

        // Order
        unitOrder = hydralurk;
        if (Spy::getEnemyTransition() == P_4Gate)
            unitOrder = mutalurk;

        // Third hatch first, but wait for lings
        auto thirdHatch  = (s >= 28 && vis(Zerg_Drone) >= 13 && total(Zerg_Zergling) >= transitionLings && Spy::getEnemyBuild() != "Unknown") || (s >= 36);
        auto fourthHatch = Spy::getEnemyBuild() == P_2Gate ? (s >= 54 && vis(Zerg_Drone) >= 18) : (total(Zerg_Hydralisk) > 0);

        // Buildings
        buildQueue[Zerg_Overlord]          = 1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 60) + (s >= 74);
        buildQueue[Zerg_Hatchery]          = 2 + thirdHatch + fourthHatch + (s >= 100 && vis(Zerg_Drone) >= 28) + (s >= 100 && vis(Zerg_Drone) >= 30);
        buildQueue[Zerg_Extractor]         = (s >= 26 && vis(Zerg_Drone) >= 13 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Hydralisk_Den]     = (s >= 38 && vis(Zerg_Drone) >= 14 && gas(30));
        buildQueue[Zerg_Lair]              = (hydraSpeed() && hydraRange());
        buildQueue[Zerg_Evolution_Chamber] = (vis(Zerg_Lair) > 0 && s >= 100);

        // Upgrades
        auto rangeFirst   = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == P_1GateCore;
        auto upgradeStart = total(Zerg_Hydralisk) >= 6 || (gas(150) && minerals(150) && vis(Zerg_Larva) <= 1);

        upgradeQueue[Metabolic_Boost]   = total(Zerg_Zergling) >= 18 && com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines]    = rangeFirst ? upgradeStart : hydraSpeed();
        upgradeQueue[Muscular_Augments] = rangeFirst ? hydraRange() : upgradeStart;
        upgradeQueue[Zerg_Carapace]     = com(Zerg_Evolution_Chamber) > 0 && total(Zerg_Hydralisk) >= 18;

        // Pumping
        auto minHydras         = (Spy::getEnemyTransition() == P_Robo ? (4 * (Util::getTime() > Time(5, 30))) : Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 1);
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < minHydras;
        auto firstHydraPump    = (vis(Zerg_Drone) >= 30 && total(Zerg_Hydralisk) < 18);
        auto secondHydraPump   = (vis(Zerg_Drone) >= 45);

        auto firstLingPump = lingsNeeded_ZvP() > vis(Zerg_Zergling) && com(Zerg_Hydralisk_Den) == 0;

        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 45 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = firstLingPump;
        zergUnitPump[Zerg_Hydralisk] = firstHydraPump || secondHydraPump || needMinimumHydras;

        // All-in
        if (!needMinimumHydras && total(Zerg_Hydralisk) >= 2 && hatchCount() >= 3) {
            if (Spy::enemyGreedy() || Spy::getEnemyTransition() == P_4Gate || Spy::getEnemyTransition() == P_CorsairGoon ||
                Spy::getEnemyTransition() == P_5GateGoon)
                activeAllinType = AllinType::Z_5HatchSpeedling;
        }

        // Gas
        gasLimit = gasMax();
        if (Spy::getEnemyBuild() != "Unknown" && Spy::getEnemyBuild() != P_FFE) {
            if (zergUnitPump[Zerg_Hydralisk])
                gasLimit = min(5, gasMax());
            else if (vis(Zerg_Hydralisk_Den) > 0 && Util::getTime() < Time(4, 30))
                gasLimit = 1;
            else if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 11)
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
        if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 20 && gas(gasCap))
            gasLimit = 0;
    }

    void ZvP6HatchHydra()
    {
        //'https://liquipedia.net/starcraft/6_hatch_(vs._Protoss)' - Soulkey style (early 4th hatch, early den, delay spire)
        inTransition = Util::getTime() > Time(4, 00);
        inOpening    = total(Zerg_Hydralisk) < 64;
        inBookSupply = vis(Zerg_Overlord) < 5;
        focusUnit    = Zerg_Hydralisk;
        mineralThird = false;
        planEarly    = wantNatural && wantThird && hatchCount() < 3 && s >= 24;

        auto pressureTime = (Spy::getEnemyTransition() == P_Speedlot || Spy::getEnemyTransition() == P_Sairlot) ? Time(9, 00) : Time(7, 30);
        pressure          = Util::getTime() > pressureTime && !isPreparingAllIn() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0;

        // Order
        unitOrder = hydralurk;

        // Third hatch first, but wait for lings
        auto thirdHatch = (s >= 28 && total(Zerg_Zergling) >= transitionLings) || (s >= 36);

        // Buildings
        buildQueue[Zerg_Overlord]          = 1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 62);
        buildQueue[Zerg_Hatchery]          = 2 + thirdHatch + (s >= 52) + (com(Zerg_Drone) >= 26 && s >= 64) + (com(Zerg_Drone) >= 40 && s >= 120);
        buildQueue[Zerg_Extractor]         = (s >= 30 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 30 && s >= 70) + (hatchCount() >= 6 && Researching::haveOrResearching(Lurker_Aspect));
        buildQueue[Zerg_Lair]              = (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Hydralisk_Den]     = (s >= 60);
        buildQueue[Zerg_Evolution_Chamber] = (s >= 70) + (s >= 150);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2 || Spy::getEnemyBuild() == P_1GateCore;

        upgradeQueue[Grooved_Spines]       = rangeFirst ? (total(Zerg_Hydralisk) > 0 && gas(150)) : hydraSpeed();
        upgradeQueue[Muscular_Augments]    = rangeFirst ? hydraRange() : (total(Zerg_Hydralisk) > 0 && gas(150));
        upgradeQueue[Zerg_Missile_Attacks] = (com(Zerg_Evolution_Chamber) > 0) + (com(Zerg_Evolution_Chamber) > 1);
        upgradeQueue[Zerg_Carapace]        = (com(Zerg_Evolution_Chamber) > 1);
        upgradeQueue[Metabolic_Boost]      = total(Zerg_Hydralisk) >= 2;

        // Research
        techQueue[Burrowing]     = com(Zerg_Drone) >= 36;
        techQueue[Lurker_Aspect] = hydraRange() && hydraSpeed();

        // Pumping
        auto firstPump         = (Util::getTime() > Time(6, 30) && hatchCount() >= 5 && total(Zerg_Hydralisk) < 26);
        auto secondPump        = (total(Zerg_Drone) >= 44 && (hydraRange() || hydraSpeed()));
        auto needMinimumHydras = (com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) * 2 + 2);

        // Lings
        auto needMinimumLings = (Util::getTime() > Time(6, 00) && vis(Zerg_Zergling) < 2);
        auto firstLingPump    = lingsNeeded_ZvP() > vis(Zerg_Zergling) && com(Zerg_Hydralisk_Den) == 0;

        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 45 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = needMinimumLings || firstLingPump;
        zergUnitPump[Zerg_Hydralisk] = needMinimumHydras || firstPump || secondPump;
        zergUnitPump[Zerg_Lurker]    = Broodwar->self()->hasResearched(Lurker_Aspect) && total(Zerg_Lurker) < 4;

        // All-in
        if (Spy::getEnemyOpener() == P_Nexus)
            activeAllinType = AllinType::Z_3HatchSpeedling;
        if (!needMinimumHydras && hatchCount() >= 5) {
            if (Spy::getEnemyTransition() == P_CorsairGoon || Spy::getEnemyTransition() == P_5GateGoon)
                activeAllinType = AllinType::Z_9HatchCrackling;
        }

        // Gas
        gasLimit = gasMax();
        if (vis(Zerg_Hydralisk_Den) == 0 && vis(Zerg_Lair) > 0)
            gasLimit = 1;
        else if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 16)
            gasLimit = 0;
    }

    void ZvP()
    {
        defaultZvP();

        // Builds
        if (currentBuild == Z_HatchPool)
            ZvP_HP();
        if (currentBuild == Z_PoolHatch)
            ZvP_PH();

        // Reactions
        if (!inTransition) {
            if (Spy::enemyProxy())
                currentTransition = Z_3HatchMuta;

            if (Spy::getEnemyBuild() == P_1GateCore && (currentTransition == Z_6HatchHydra || currentTransition == Z_3HatchHydra))
                currentTransition = Z_4HatchHydra;
            if (Spy::getEnemyBuild() == P_2Gate && (currentTransition == Z_6HatchHydra || currentTransition == Z_3HatchHydra))
                currentTransition = Z_3HatchMuta;

            if (Spy::getEnemyBuild() == P_FFE && currentTransition == Z_4HatchHydra)
                currentTransition = Z_6HatchHydra;
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == Z_2HatchMuta)
                ZvP2HatchMuta();
            if (currentTransition == Z_3HatchMuta)
                ZvP3HatchMuta();
            if (currentTransition == Z_3HatchHydra)
                ZvP3HatchHydra();
            if (currentTransition == Z_4HatchHydra)
                ZvP4HatchHydra();
            if (currentTransition == Z_6HatchHydra)
                ZvP6HatchHydra();
        }
    }
} // namespace McRave::BuildOrder::Zerg