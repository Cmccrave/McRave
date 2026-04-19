#include "Builds/Zerg/ZergBuildOrder.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
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

    int inboundUnits_ZvZ()
    {
        static map<BWAPI::UnitType, double> trackables = {{Zerg_Zergling, 1.0}};
        if (Players::hasUpgraded(PlayerState::Enemy, Zerg_Melee_Attacks, 1))
            trackables[Zerg_Zergling] += 0.2;

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end()) {
                arrivalValue += idx->second * 0.9;
                if (Units::inBoundUnit(unit))
                    arrivalValue += idx->second * 0.4;
            }
        }

        // Make less if we have some other units outside our opening
        if (com(Zerg_Sunken_Colony) > 0) {
            arrivalValue -= vis(Zerg_Hydralisk);
            arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 6.0;
        }

        // Make more if we have a drone lead
        auto droneDiff = vis(Zerg_Drone) > Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone);
        if (droneDiff >= 3) {
            arrivalValue *= 1.25;
        }
        return int(arrivalValue);
    }

    int lingsNeeded_ZvZ()
    {
        auto initialValue = 10;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        auto macroHatch      = (currentBuild == Z_PoolHatch || currentBuild == Z_HatchPool) ? 1 : 0;
        auto enemyDrones     = Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone);
        auto enemyProduction = max(1, Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery, Zerg_Lair, Zerg_Hive) - 1);
        auto minDrones       = max(enemyDrones + 1, (Stations::getGasingStationsCount() * 8) + int(macroHatch * 3));

        // 1Hatch builds can't make too many lings
        if (currentTransition == Z_1HatchMuta && total(Zerg_Mutalisk) == 0) {
            initialValue = 16;
            if (Spy::enemyTurtle())
                initialValue = 8;
            else if (Spy::getEnemyTransition() == Z_1HatchMuta)
                initialValue = 12;
            else if (Spy::getEnemyTransition() == Z_2HatchSpeedling || Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 2)
                initialValue = 12;
            else if (Spy::getEnemyTransition() == Z_2HatchMuta || Spy::getEnemyTransition() == Z_3HatchSpeedling)
                initialValue = 8;
        }

        // 2Hatch builds can make more lings
        if (currentTransition == Z_2HatchMuta && total(Zerg_Mutalisk) == 0) {
            initialValue = 16;
            if (Spy::getEnemyTransition() == Z_1HatchMuta)
                initialValue = 12;
            if (Spy::enemyTurtle())
                initialValue = 12;
            if (Spy::getEnemyBuild() == Z_12Hatch || Spy::getEnemyTransition() == Z_3HatchSpeedling || Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3)
                initialValue = 6;
        }

        // Make less if they decided to defend
        initialValue -= Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) * 4;

        transitionLings = min(initialValue, 12);
        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // Doesn't really make sense to rely on this until our overlord scouting is really good
        auto inbound = max(6, inboundUnits_ZvZ());
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 64)
            return inbound;
        return 0;
    }

    // Their spire is fast enough to be lethal
    bool enemyFasterSpire()
    {
        static auto selfSpireTiming  = Time(0, 00);
        static auto enemySpireTiming = Time(0, 00);
        if (vis(Zerg_Spire) > 0 && selfSpireTiming == Time(0, 00))
            selfSpireTiming = Util::getTime();
        if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) > 0 && enemySpireTiming == Time(0, 00))
            enemySpireTiming = Util::getTime();

        auto faster = selfSpireTiming - enemySpireTiming > Time(0, 25);
        if (faster)
            LOG_ONCE("Enemy faster spire");

        return enemySpireTiming != Time(0, 00) && selfSpireTiming - enemySpireTiming > Time(0, 25);
    }

    // Our spire is fast enough to be lethal
    bool selfFasterSpire()
    {
        static auto selfSpireTiming  = Time(0, 00);
        static auto enemySpireTiming = Time(0, 00);
        if (vis(Zerg_Spire) > 0 && selfSpireTiming == Time(0, 00))
            selfSpireTiming = Util::getTime();
        if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) > 0 && enemySpireTiming == Time(0, 00))
            enemySpireTiming = Util::getTime();

        auto faster = selfSpireTiming - enemySpireTiming > Time(0, 25);
        if (faster)
            LOG_ONCE("Enemy faster spire");

        return enemySpireTiming == Time(0, 00) || enemySpireTiming - selfSpireTiming > Time(0, 25);
    }

    void defaultZvZ()
    {
        inOpening       = true;
        inBookSupply    = true;
        wallNat         = false;
        wallMain        = false;
        wantNatural     = (Spy::getEnemyOpener() != "4Pool" && Spy::getEnemyOpener() != "7Pool") || hatchCount() >= 3 || Spy::getEnemyTransition() == Z_2HatchMuta;
        wantThird       = false;
        proxy           = false;
        hideTech        = false;
        rush            = false;
        pressure        = false;
        transitionReady = false;
        gasTrick        = false;
        reserveLarva    = 0;

        gasLimit  = gasMax();
        focusUnit = UnitTypes::None;
    }

    void ZvZ_1HatchMuta()
    {
        inOpening    = total(Zerg_Mutalisk) < 3 && total(Zerg_Scourge) < 6;
        inTransition = vis(Zerg_Lair) > 0;
        inBookSupply = false;

        focusUnit                   = Zerg_Mutalisk;
        reserveLarva                = 3;
        unitPressure[Zerg_Mutalisk] = Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony, Zerg_Mutalisk) == 0 && Players::getDeadCount(PlayerState::Enemy, Zerg_Drone) < 8;
        wantNatural                 = (Spy::enemyTurtle() && Spy::getEnemyTransition() != Z_2HatchHydra) || Spy::getEnemyTransition() == Z_2HatchMuta;

        auto mirrorBuild  = (Spy::getEnemyTransition() == Z_1HatchMuta && atPercent(Zerg_Spire, 0.5) && vis(Zerg_Drone) >= 12); // We have a mirror, so take a greedy extra hatch
        auto catchupHatch = (Spy::getEnemyTransition() == Z_2HatchMuta && atPercent(Zerg_Lair, 0.5));                           // We need to catch up on production
        auto resourceRich = (vis(Zerg_Drone) >= 12 && vis(Zerg_Larva) == 0 && minerals(250) && !atPercent(Zerg_Spire, 0.5));    // We can afford a hatch

        auto lethalTiming = (selfFasterSpire() && unitPressure[Zerg_Mutalisk]); // We have a lethal spire timing

        auto secondHatch     = !lethalTiming && (mirrorBuild || catchupHatch || resourceRich);
        auto enemyHydraBuild = Spy::getEnemyTransition().find("Hydra") != string::npos || Spy::getEnemyTransition().find("Lurker") != string::npos;
        auto enemyMutaBuild  = Spy::getEnemyTransition().find("Muta") != string::npos;

        auto speedFirst = currentOpener == Z_9Pool && !Spy::enemyTurtle();

        // Build
        buildQueue[Zerg_Lair]     = (!speedFirst || lingSpeed()) && gas(100) && vis(Zerg_Zergling) >= 6 && vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Spire]    = lingSpeed() && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 9;
        buildQueue[Zerg_Hatchery] = 1 + secondHatch;

        // Upgrades
        upgradeQueue[Metabolic_Boost] = (speedFirst || vis(Zerg_Lair) > 0) && (total(Zerg_Zergling) >= 6 && gas(100));

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 18 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] = lingsNeeded_ZvZ() > vis(Zerg_Zergling);
        zergUnitPump[Zerg_Scourge]  = com(Zerg_Spire) == 1 && hatchCount() >= 2 && enemyFasterSpire();
        zergUnitPump[Zerg_Mutalisk] = !zergUnitPump[Zerg_Scourge] && com(Zerg_Spire) == 1 && gas(80) && vis(Zerg_Drone) >= 8;

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyTurtle() && !Spy::enemyFastExpand()) {
            if (lingSpeed() && total(Zerg_Lair) == 0)
                gasLimit = 2;
            else if (lingSpeed() && !atPercent(Zerg_Lair, 0.7))
                gasLimit = 1;
            else if (vis(Zerg_Drone) < 8 && com(Zerg_Spire) == 0)
                gasLimit = 0;
        }
    }

    void ZvZ_2HatchMuta()
    {
        inOpening    = total(Zerg_Mutalisk) < 3 && total(Zerg_Scourge) < 6;
        inTransition = vis(Zerg_Lair) > 0;
        inBookSupply = vis(Zerg_Overlord) < 3;

        focusUnit = Zerg_Mutalisk;
        unitPressure[Zerg_Mutalisk] = Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony, Zerg_Mutalisk) == 0 && Players::getDeadCount(PlayerState::Enemy, Zerg_Drone) < 8;

        reserveLarva = 3;
        if (Spy::enemyPressure() || (Spy::getEnemyTransition() == Z_1HatchMuta && !Spy::enemyTurtle()))
            reserveLarva = 0;
        if (Spy::getEnemyTransition() == Z_2HatchMuta)
            reserveLarva = 6;

        auto speedFirst = (currentOpener == Z_9Pool || currentOpener == Z_Overpool) && !Spy::enemyTurtle();
        auto secondOvie = (vis(Zerg_Extractor) + Spy::enemyGasSteal() >= 1) || (s >= 32);

        // Build
        buildQueue[Zerg_Extractor] = (s >= 22 && Util::getTime() > Time(1, 45)) + (vis(Zerg_Drone) >= 16 && vis(Zerg_Spire) > 0);
        buildQueue[Zerg_Lair]      = (!speedFirst || lingSpeed()) && gas(100) && vis(Zerg_Drone) >= 8 && (vis(Zerg_Larva) == 0 || vis(Zerg_Drone) >= 14);
        buildQueue[Zerg_Spire]     = lingSpeed() && atPercent(Zerg_Lair, 0.95) && com(Zerg_Drone) >= 12 && vis(Zerg_Larva) <= 1;
        buildQueue[Zerg_Overlord]  = 1 + secondOvie + (s >= 32) + (s >= 46);

        // Upgrades
        upgradeQueue[Metabolic_Boost] = (speedFirst || vis(Zerg_Lair) > 0) && (total(Zerg_Zergling) >= 6 && gas(100));

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 24 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] = lingsNeeded_ZvZ() > vis(Zerg_Zergling);
        zergUnitPump[Zerg_Scourge]  = com(Zerg_Spire) == 1 && Spy::getEnemyTransition() == Z_1HatchMuta && !Spy::enemyTurtle() && gas(75) && total(Zerg_Scourge) < 24;
        zergUnitPump[Zerg_Mutalisk] = !zergUnitPump[Zerg_Scourge] && com(Zerg_Spire) == 1 && gas(80) && vis(Zerg_Drone) >= 8;
        
        // Cap gas at 100 until lair and speed, then put all on
        gasLimit = capGas(100);

        // After lair starts, no gas for a bit
        if (lingSpeed() && vis(Zerg_Lair) > 0) {
            gasLimit = 0;
            if (atPercent(Zerg_Lair, 0.5))
                gasLimit = 2;
        }

        // AFter lair completes, full gas mining
        if (com(Zerg_Lair) > 0)
            gasLimit = gasMax();

        if (!Spy::enemyTurtle()) {
            auto dropGasLowDrone  = vis(Zerg_Drone) + vis(Zerg_Extractor) < 8;
            auto dropGasEarly     = Spy::Zerg::enemyFasterPool() && Util::getTime() < Time(3, 20);
            auto dropGasAfterLair = vis(Zerg_Lair) > 0 && Spy::getEnemyTransition() != Z_1HatchMuta && Spy::getEnemyTransition() != Z_2HatchMuta && Util::getTime() < Time(4, 00);
            auto dropGasRush      = (vis(Zerg_Lair) > 0 || gas(100)) && Spy::getEnemyTransition() == Z_2HatchSpeedling && Util::getTime() < Time(4, 30);
            auto dropGasPressure  = (vis(Zerg_Spire) > 0 || gas(150)) && Spy::enemyPressure() && Util::getTime() < Time(5, 00);

            if (dropGasLowDrone || dropGasEarly || dropGasAfterLair || dropGasRush || dropGasPressure)
                gasLimit = 0;
        }
    }

    void ZvZ_2HatchHydra()
    {
        inOpening    = total(Zerg_Hydralisk) < 6;
        inTransition = vis(Zerg_Hydralisk_Den) > 0;
        inBookSupply = vis(Zerg_Overlord) < 3;

        focusUnit = Zerg_Hydralisk;

        // Build
        buildQueue[Zerg_Overlord]      = 2 + (s >= 32) + (s >= 46);
        buildQueue[Zerg_Extractor]     = 1;
        buildQueue[Zerg_Hatchery]      = 2 + (vis(Zerg_Drone) >= 11 && total(Zerg_Zergling) >= 10) + (vis(Zerg_Drone) >= 18);
        buildQueue[Zerg_Hydralisk_Den] = hatchCount() >= 4;

        // Upgrades
        upgradeQueue[UpgradeTypes::Muscular_Augments] = vis(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[UpgradeTypes::Grooved_Spines]    = vis(Zerg_Hydralisk_Den) > 0 && hydraSpeed();

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 20 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = vis(Zerg_Zergling) < lingsNeeded_ZvZ() || (!zergUnitPump[Zerg_Drone]);
        zergUnitPump[Zerg_Hydralisk] = Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 && com(Zerg_Hydralisk_Den) > 0 &&
                                       vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) + 6;

        // Gas
        gasLimit = 3;
        if (!Spy::enemyTurtle() && !Spy::enemyFastExpand()) {
            if (vis(Zerg_Hydralisk_Den) > 0)
                gasLimit = gasMax();
            else if (lingSpeed())
                gasLimit = 1;
        }
    }

    void ZvZ()
    {
        defaultZvZ();

        // Builds
        if (currentBuild == Z_PoolHatch)
            ZvZ_PH();
        if (currentBuild == Z_PoolLair)
            ZvZ_PL();

        // Reactions
        if (!inTransition) {
            if (Spy::enemyGasSteal())
                currentTransition = Z_2HatchMuta;
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == Z_1HatchMuta)
                ZvZ_1HatchMuta();
            if (currentTransition == Z_2HatchMuta)
                ZvZ_2HatchMuta();
            if (currentTransition == Z_2HatchHydra)
                ZvZ_2HatchHydra();
        }
    }
} // namespace McRave::BuildOrder::Zerg