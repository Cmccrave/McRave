#include "Builds/Zerg/ZergBuildOrder.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
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

    int inboundUnits_ZvZ()
    {
        static map<BWAPI::UnitType, double> trackables = {{Zerg_Zergling, 0.7}};

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end()) {
                arrivalValue += idx->second;
                if (Units::inBoundUnit(unit, 25))
                    arrivalValue += idx->second / 2.0;
            }
        }

        // Make less if we have some other units outside our opening
        if (com(Zerg_Sunken_Colony) > 0) {
            arrivalValue -= vis(Zerg_Hydralisk);
            arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 4.0;
        }

        return int(arrivalValue);
    }

    int lingsNeeded_ZvZ()
    {
        auto initialValue = 10;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        auto macroHatch  = (currentBuild == Z_PoolHatch || currentBuild == Z_HatchPool) ? 1 : 0;
        auto enemyDrones = Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone);
        auto minDrones   = max(enemyDrones + 1, (Stations::getGasingStationsCount() * 8) + int(macroHatch * 3));

        // 1Hatch builds can't make too many lings
        if (currentTransition == Z_1HatchMuta && total(Zerg_Mutalisk) == 0) {
            initialValue = 24;
            if (Spy::enemyTurtle())
                initialValue = 12;
            else if (Spy::getEnemyTransition() == Z_1HatchMuta)
                initialValue = 30;
            else if (Spy::getEnemyTransition() == Z_2HatchSpeedling || Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 2)
                initialValue = 24;
            else if (Spy::getEnemyTransition() == Z_2HatchMuta || Spy::getEnemyTransition() == Z_3HatchSpeedling)
                initialValue = 18;
        }

        // 2Hatch builds can make more lings
        if (currentTransition == Z_2HatchMuta && total(Zerg_Mutalisk) == 0) {
            initialValue = 36;
            if (Spy::getEnemyTransition() == Z_1HatchMuta)
                initialValue = 24;
            if (Spy::enemyTurtle())
                initialValue = 18;
            if (Spy::getEnemyBuild() == Z_12Hatch || Spy::getEnemyTransition() == Z_3HatchSpeedling || Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3)
                initialValue = 8;
        }

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // If this is a 1Hatch build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("1Hatch") != string::npos)
            return 0;

        // Doesn't really make sense to rely on this until our overlord scouting is really good
        auto inbound = max(6, inboundUnits_ZvZ());
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 64)
            return inbound;
        return 0;
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

        gasLimit = gasMax();

        desiredDetection = Zerg_Overlord;
        focusUnit        = UnitTypes::None;
    }

    void ZvZ_1HatchMuta()
    {
        inOpening    = total(Zerg_Mutalisk) < 6 && total(Zerg_Scourge) < 12;
        inTransition = vis(Zerg_Lair) > 0;
        inBookSupply = false;

        focusUnit    = Zerg_Mutalisk;
        reserveLarva = 3;

        auto secondHatch = (Spy::getEnemyTransition() == Z_1HatchMuta && atPercent(Zerg_Spire, 0.5) && vis(Zerg_Drone) >= 12) || (Spy::enemyTurtle() && atPercent(Zerg_Spire, 0.5));

        auto speedFirst = !Spy::enemyTurtle();

        // Build
        buildQueue[Zerg_Lair]     = (!speedFirst || lingSpeed()) && gas(100) && vis(Zerg_Zergling) >= 6 && vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Spire]    = lingSpeed() && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 9;
        buildQueue[Zerg_Hatchery] = 1 + secondHatch;

        // Upgrades
        upgradeQueue[Metabolic_Boost] = (speedFirst || vis(Zerg_Lair) > 0) && (total(Zerg_Zergling) >= 6 && gas(100));

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 12 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] = lingsNeeded_ZvZ() > vis(Zerg_Zergling) || (com(Zerg_Spire) == 1 && !gas(100)) || vis(Zerg_Drone) >= 12;
        zergUnitPump[Zerg_Scourge]  = com(Zerg_Spire) == 1 && hatchCount() >= 2 && total(Zerg_Scourge) < 6 && Spy::enemyTurtle();
        zergUnitPump[Zerg_Mutalisk] = !zergUnitPump[Zerg_Scourge] && com(Zerg_Spire) == 1 && gas(100) && vis(Zerg_Drone) >= 8;

        // Reactions
        if (Spy::enemyTurtle() && Spy::getEnemyTransition() != Z_2HatchHydra) {
            wantNatural                 = true;
            zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling);
            zergUnitPump[Zerg_Drone]    = vis(Zerg_Drone) < 16;
        }

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
        inOpening    = total(Zerg_Mutalisk) < 6 && total(Zerg_Scourge) < 24;
        inTransition = vis(Zerg_Lair) > 0;
        inBookSupply = vis(Zerg_Overlord) < 3;

        focusUnit = Zerg_Mutalisk;

        auto speedFirst = !Spy::enemyTurtle();

        // Build
        buildQueue[Zerg_Extractor] = (s >= 22 && Util::getTime() > Time(1, 45)) + (vis(Zerg_Drone) >= 16 && vis(Zerg_Spire) > 0);
        buildQueue[Zerg_Lair]      = (!speedFirst || lingSpeed()) && gas(100) && vis(Zerg_Drone) >= 8 && (vis(Zerg_Larva) == 0 || vis(Zerg_Drone) >= 14);
        buildQueue[Zerg_Spire]     = lingSpeed() && atPercent(Zerg_Lair, 0.95) && com(Zerg_Drone) >= 12 && vis(Zerg_Larva) <= 1;
        buildQueue[Zerg_Overlord]  = 1 + (vis(Zerg_Extractor) + Spy::enemyGasSteal() >= 1) + (s >= 32) + (s >= 46);

        // Upgrades
        upgradeQueue[Metabolic_Boost] = (speedFirst || vis(Zerg_Lair) > 0) && (total(Zerg_Zergling) >= 6 && gas(100));

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 24 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] = lingsNeeded_ZvZ() > vis(Zerg_Zergling) || (com(Zerg_Spire) == 1 && !gas(100) && !Spy::enemyTurtle()) || (vis(Zerg_Drone) >= 24 && !gas(100)) ||
                                      (vis(Zerg_Drone) >= 24 && com(Zerg_Spire) == 0);

        // Reactions
        if (Spy::getEnemyTransition() == Z_1HatchMuta && !Spy::enemyTurtle()) {
            zergUnitPump[Zerg_Scourge]  = com(Zerg_Spire) == 1 && gas(75) && total(Zerg_Scourge) < 24;
            zergUnitPump[Zerg_Mutalisk] = !zergUnitPump[Zerg_Scourge] && com(Zerg_Spire) == 1 && gas(100) && vis(Zerg_Drone) >= 8;
            reserveLarva                = 0;
        }
        else if (Spy::enemyPressure()) {
            zergUnitPump[Zerg_Mutalisk] = com(Zerg_Spire) == 1 && gas(100) && vis(Zerg_Drone) >= 8;
            reserveLarva                = 0;
        }
        else {
            zergUnitPump[Zerg_Mutalisk] = com(Zerg_Spire) == 1 && gas(100) && vis(Zerg_Drone) >= 8;
            reserveLarva                = 3;
        }

        // Gas
        gasLimit = gasMax() - gas(50);
        if (!Spy::enemyTurtle() && !Spy::enemyFastExpand()) {
            if (Spy::enemyPressure() && Util::getTime() < Time(5, 00))
                gasLimit = 0;
            else if ((Spy::getEnemyOpener() == Z_9Pool || Spy::getEnemyOpener() == Z_Overpool) && Util::getTime() < Time(3, 20))
                gasLimit = 0;
            else if (lingSpeed() && vis(Zerg_Lair) > 0 && Util::getTime() < Time(6, 00))
                gasLimit = 2;
            else if ((lingSpeed() || vis(Zerg_Lair) > 0) && Util::getTime() < Time(6, 00))
                gasLimit = 1;
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
        zergUnitPump[Zerg_Zergling]  = vis(Zerg_Zergling) < lingsNeeded_ZvZ();
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