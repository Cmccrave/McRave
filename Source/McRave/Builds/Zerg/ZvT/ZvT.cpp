#include "Builds/Zerg/ZergBuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Macro/Researching/Researching.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Main/Common.h"
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

    void defaultZvT()
    {
        inOpening    = true;
        inBookSupply = true;

        wallNat  = Spy::getEnemyBuild() == T_RaxFact || Spy::enemyWalled();
        wallMain = false;

        wantNatural = hatchCount() >= 3 || (Spy::getEnemyTransition() != U_WorkerRush);
        wantThird   = hatchCount() >= 3 || (Spy::getEnemyBuild() == T_RaxCC);

        mineralThird    = false;
        proxy           = false;
        hideTech        = false;
        rush            = false;
        pressure        = false;
        transitionReady = false;
        planEarly       = false;
        gasTrick        = false;
        reserveLarva    = 0;

        gasLimit  = gasMax();
        focusUnit = UnitTypes::None;
    }

    int inboundUnits_ZvT()
    {
        static map<BWAPI::UnitType, double> trackables = {{Terran_Marine, 1.0}, {Terran_Medic, 2.0}, {Terran_Firebat, 2.0}, {Terran_Vulture, 1.0}, {Terran_Goliath, 2.0}};

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end()) {
                arrivalValue += idx->second;
                if (Units::inBoundUnit(unit))
                    arrivalValue += idx->second / 2.0;
            }
        }

        // Make less if we have some other units outside our opening
        if (total(Zerg_Zergling) >= 10 && vis(Zerg_Zergling) >= 6) {
            arrivalValue -= vis(Zerg_Mutalisk) * 4;
            arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 6.0;
        }

        return int(arrivalValue);
    }

    int lingsNeeded_ZvT()
    {
        auto initialValue = 4;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        // 2Rax
        if (Spy::getEnemyBuild() == T_2Rax) {
            initialValue = 6;
            if (Spy::getEnemyOpener() == T_BBS)
                initialValue = 10;
            if (Spy::getEnemyOpener() == T_11_13)
                initialValue = 4;
        }

        // RaxCC
        if (Spy::getEnemyBuild() == T_RaxCC) {
            initialValue = 2;
        }

        // RaxFact
        if (Spy::getEnemyBuild() == T_RaxFact || Spy::enemyWalled() || Spy::getEnemyBuild() == "Unknown") {
            initialValue = 4;
            if (Spy::getEnemyOpener() == T_1FactFE || Spy::getEnemyOpener() == T_2FactFE || Util::getTime() > Time(4, 00))
                initialValue = 6;
        }

        // TODO: Fix T spy
        if (Spy::getEnemyOpener() == T_8Rax || Spy::enemyProxy())
            initialValue = 10;
        if (Spy::getEnemyTransition() == U_WorkerRush)
            initialValue = 24;

        // If they're aggressive, delay remaining build until we've made a transitional count (at 8 lings, we have minerals)
        transitionLings = min(initialValue, 8);

        // Minimum lings
        auto minimumLings = (Util::getTime() > Time(4, 00) && Spy::getEnemyBuild() == T_RaxFact) ? 6 : 2;
        if (vis(Zerg_Zergling) < minimumLings)
            return minimumLings;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // If this is a 2Hatch build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("2Hatch") != string::npos)
            return 0;

        auto inbound = inboundUnits_ZvT();
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 36)
            return inbound;
        return 0;
    }

    void ZvT_1HatchLurker()
    {
        inTransition = true;
        inOpening    = true;
        inBookSupply = total(Zerg_Overlord) < 3;

        focusUnit = Zerg_Lurker;
        pressure  = com(Zerg_Hydralisk_Den) > 0;

        // Order
        unitOrder = {};

        // Buildings
        buildQueue[Zerg_Hatchery]      = 1;
        buildQueue[Zerg_Extractor]     = vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Lair]          = (com(Zerg_Hydralisk_Den) > 0 && gas(80));
        buildQueue[Zerg_Hydralisk_Den] = (s >= 20 && gas(50));

        // Research
        techQueue[TechTypes::Lurker_Aspect] = com(Zerg_Hydralisk_Den) && com(Zerg_Lair);

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 11 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = false;
        zergUnitPump[Zerg_Hydralisk] = com(Zerg_Hydralisk_Den) > 0;
        zergUnitPump[Zerg_Lurker]    = com(Zerg_Hydralisk) > 0 && Players::hasResearched(PlayerState::Self, TechTypes::Lurker_Aspect);

        // Gas
        gasLimit = gasMax();
    }

    void ZvT_2HatchMuta()
    {
        inTransition                = vis(Zerg_Lair) > 0;
        inOpening                   = total(Zerg_Mutalisk) <= 12;
        inBookSupply                = total(Zerg_Overlord) < 3;
        unitPressure[Zerg_Mutalisk] = (Spy::getEnemyTransition() == T_2FactVulture && Util::getTime() < Time(10, 00)) || Players::getTotalCount(PlayerState::Enemy, Terran_Missile_Turret) == 0;

        focusUnit    = Zerg_Mutalisk;
        reserveLarva = 6;

        auto thirdHatch  = (com(Zerg_Spire) == 0 && s >= 48 && vis(Zerg_Drone) >= 20) || (com(Zerg_Spire) == 1 && total(Zerg_Mutalisk) >= 6 && vis(Zerg_Drone) >= 20);
        auto fourthHatch = com(Zerg_Mutalisk) > 0;

        auto firstGas  = (hatchCount() >= 2 && vis(Zerg_Drone) >= 10 && vis(Zerg_Spawning_Pool) > 0);
        auto secondGas = (Spy::enemyFastExpand() && vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 20) || (atPercent(Zerg_Spire, 0.5) && vis(Zerg_Drone) >= 18);

        // Order
        unitOrder = mutalingdefiler;
        if (Spy::Terran::enemyMech())
            unitOrder = mutalingqueen;
        if (Spy::Terran::enemyBio())
            unitOrder = Spy::enemyFastExpand() ? ultraling : mutalurk;

        // Buildings
        buildQueue[Zerg_Hatchery]          = 2 + thirdHatch + fourthHatch;
        buildQueue[Zerg_Extractor]         = firstGas + secondGas;
        buildQueue[Zerg_Overlord]          = 1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Lair]              = (s >= 24 && gas(80));
        buildQueue[Zerg_Spire]             = (vis(Zerg_Drone) >= 16 && atPercent(Zerg_Lair, 0.95));
        buildQueue[Zerg_Hydralisk_Den]     = com(Zerg_Mutalisk) > 0 && unitOrder == mutalurk;
        buildQueue[Zerg_Evolution_Chamber] = (unitOrder == ultraling && hatchCount() >= 3) || (unitOrder == mutalingdefiler && total(Zerg_Mutalisk) >= 9);

        techQueue[Lurker_Aspect] = com(Zerg_Hydralisk_Den) > 0;

        auto softDroneCap     = 28;
        auto firstScourgePump = com(Zerg_Spire) > 0 && total(Zerg_Mutalisk) < 5 && Players::getVisibleCount(PlayerState::Enemy, Terran_Wraith) > vis(Zerg_Scourge);
        auto firstMutaPump    = com(Zerg_Spire) > 0 && !firstScourgePump && (total(Zerg_Mutalisk) < 9 || unitPressure[Zerg_Mutalisk]);
        auto secondMutaPump   = com(Zerg_Spire) > 0 && vis(Zerg_Drone) >= softDroneCap;
        auto firstHydraPump   = com(Zerg_Hydralisk_Den) > 0 && total(Zerg_Hydralisk) < 2 && Researching::haveOrResearching(Lurker_Aspect);

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < softDroneCap && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        zergUnitPump[Zerg_Scourge]   = firstScourgePump;
        zergUnitPump[Zerg_Mutalisk]  = firstMutaPump || secondMutaPump;
        zergUnitPump[Zerg_Hydralisk] = firstHydraPump;
        zergUnitPump[Zerg_Lurker]    = true;

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyFastExpand()) {
            auto dropGasLowDrone  = vis(Zerg_Drone) + vis(Zerg_Extractor) < 10;
            auto dropGasEarly     = (Spy::enemyProxy() || Spy::getEnemyOpener() == T_BBS) && Util::getTime() < Time(3, 00);
            auto dropGasAfterLair = vis(Zerg_Lair) > 0 && Spy::getEnemyBuild() == T_2Rax && Util::getTime() < Time(4, 00);
            auto dropGasRush      = (vis(Zerg_Lair) > 0 || gas(100)) && Spy::getEnemyTransition() == T_Rush && Util::getTime() < Time(4, 30);
            auto dropGasPressure  = (vis(Zerg_Spire) > 0 || gas(150)) && Spy::getEnemyTransition() == T_Academy && !Spy::enemyFastExpand() && Util::getTime() < Time(5, 00);
            if (dropGasLowDrone || dropGasEarly || dropGasAfterLair || dropGasRush || dropGasPressure)
                gasLimit = 0;
        }
    }

    void ZvT_3HatchMuta()
    {
        // 13h 12g
        inTransition                = vis(Zerg_Lair) > 0;
        inOpening                   = total(Zerg_Mutalisk) <= 12;
        inBookSupply                = vis(Zerg_Overlord) < 4;
        unitPressure[Zerg_Mutalisk] = (Spy::getEnemyTransition() == T_2FactVulture && Util::getTime() < Time(10, 00)) || Players::getTotalCount(PlayerState::Enemy, Terran_Missile_Turret) == 0;

        focusUnit    = Zerg_Mutalisk;
        reserveLarva = (Spy::enemyFastExpand() ? 9 : 6);

        auto thirdHatch  = (s >= 26 && vis(Zerg_Drone) >= 11 && total(Zerg_Zergling) >= transitionLings);
        auto fourthHatch = (Spy::getEnemyBuild() == T_RaxFact || !Spy::enemyFastExpand()) ? com(Zerg_Mutalisk) > 0 : (vis(Zerg_Spire) > 0 && s >= 66);

        auto secondGas = (Spy::enemyFastExpand() && vis(Zerg_Drone) >= 21) || (com(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);

        if (!Spy::enemyProxy() && Spy::getEnemyBuild() == T_RaxFact)
            wantThird = true;

        // Order
        unitOrder = mutalingdefiler;
        if (Spy::Terran::enemyMech())
            unitOrder = mutalingqueen;
        if (Spy::Terran::enemyBio())
            unitOrder = Spy::enemyFastExpand() ? ultraling : mutalurk;

        // Buildings
        buildQueue[Zerg_Hatchery]          = 2 + thirdHatch + fourthHatch;
        buildQueue[Zerg_Extractor]         = (s >= 26 && hatchCount() >= 3) + secondGas;
        buildQueue[Zerg_Overlord]          = 1 + (s >= 18) + (s >= 32) + (s >= 48);
        buildQueue[Zerg_Lair]              = (s >= 24 && gas(80));
        buildQueue[Zerg_Spire]             = (vis(Zerg_Drone) >= 20 && atPercent(Zerg_Lair, 0.95));
        buildQueue[Zerg_Hydralisk_Den]     = com(Zerg_Mutalisk) > 0 && unitOrder == mutalurk;
        buildQueue[Zerg_Evolution_Chamber] = (unitOrder == ultraling && hatchCount() >= 3) || (unitOrder == mutalingdefiler && total(Zerg_Mutalisk) >= 9);

        techQueue[Lurker_Aspect] = com(Zerg_Hydralisk_Den) > 0;

        // Upgrades
        upgradeQueue[Metabolic_Boost]    = Spy::getEnemyBuild() != T_RaxFact && vis(Zerg_Zergling) >= 12 && gas(80);
        upgradeQueue[Zerg_Carapace]      = com(Zerg_Evolution_Chamber) > 0;
        upgradeQueue[Zerg_Flyer_Attacks] = total(Zerg_Mutalisk) >= 9;

        auto softDroneCap     = 35;
        auto firstLingPump    = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        auto firstScourgePump = com(Zerg_Spire) > 0 && total(Zerg_Mutalisk) < 5 && Players::getVisibleCount(PlayerState::Enemy, Terran_Wraith) > vis(Zerg_Scourge);
        auto firstMutaPump    = com(Zerg_Spire) > 0 && !firstScourgePump && (total(Zerg_Mutalisk) < 9 || unitPressure[Zerg_Mutalisk]);
        auto secondMutaPump   = com(Zerg_Spire) > 0 && vis(Zerg_Drone) >= softDroneCap;
        auto firstHydraPump   = com(Zerg_Hydralisk_Den) > 0 && total(Zerg_Hydralisk) < 2 && Researching::haveOrResearching(Lurker_Aspect);
        auto firstLurkerPump  = com(Zerg_Hydralisk) > 0 && Researching::haveResearch(Lurker_Aspect);

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < softDroneCap && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling]  = firstLingPump;
        zergUnitPump[Zerg_Scourge]   = firstScourgePump;
        zergUnitPump[Zerg_Mutalisk]  = firstMutaPump || secondMutaPump;
        zergUnitPump[Zerg_Hydralisk] = firstHydraPump;
        zergUnitPump[Zerg_Lurker]    = firstLurkerPump;

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyFastExpand()) {
            auto dropGasEarly     = vis(Zerg_Drone) + vis(Zerg_Extractor) < 10;
            auto dropGasImmediate = (Spy::enemyProxy() || Spy::getEnemyOpener() == T_BBS) && Util::getTime() < Time(3, 30);
            auto dropGasAfterLair = vis(Zerg_Lair) > 0 && Spy::getEnemyBuild() == T_2Rax && Util::getTime() < Time(4, 30);
            auto dropGasRush      = (vis(Zerg_Lair) > 0 || gas(100)) && Spy::getEnemyTransition() == T_Rush && Util::getTime() < Time(5, 00);
            auto dropGasPressure  = (vis(Zerg_Spire) > 0 || gas(150)) && Spy::getEnemyTransition() == T_Academy && !Spy::enemyFastExpand() && Util::getTime() < Time(5, 30);
            if (dropGasEarly || dropGasImmediate || dropGasAfterLair || dropGasRush || dropGasPressure)
                gasLimit = 0;
        }
    }

    void ZvT()
    {
        defaultZvT();

        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyTransition() == U_WorkerRush) {
                currentBuild      = Z_PoolHatch;
                currentOpener     = Z_Overpool;
                currentTransition = Z_2HatchMuta;
            }
        }

        // Builds
        if (currentBuild == Z_HatchPool)
            ZvT_HP();
        if (currentBuild == Z_PoolHatch)
            ZvT_PH();
        if (currentBuild == Z_PoolLair)
            ZvT_PL();

        // Transitions
        if (transitionReady) {
            if (currentTransition == Z_2HatchMuta)
                ZvT_2HatchMuta();
            if (currentTransition == Z_3HatchMuta)
                ZvT_3HatchMuta();
            if (currentTransition == Z_1HatchLurker)
                ZvT_1HatchLurker();
        }
    }
} // namespace McRave::BuildOrder::Zerg