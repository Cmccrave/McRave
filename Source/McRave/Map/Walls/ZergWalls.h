#pragma once
#include "BWEB.h"
#include "Builds/All/BuildOrder.h"
#include "Builds/All/Learning.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls::Zerg {

    // ZvP Ground
    int ZvP_Opener(BWEB::Wall &wall)
    {
        // 1GateCore
        if (Spy::getEnemyBuild() == P_1GateCore || (Spy::getEnemyBuild() == "Unknown" && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) >= 1)) {
            if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) >= 3)
                return 2;
            return (Util::getTime() > Time(4, 00));
        }

        // 2Gate
        if (Spy::getEnemyBuild() == P_2Gate) {
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 10)) + (Util::getTime() > Time(4, 30));

            // 10-15
            if (Spy::getEnemyOpener() == P_10_15)
                return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 10)) + (Util::getTime() > Time(5, 00));

            // 10-12
            if (Spy::getEnemyOpener() == P_10_12 || Spy::getEnemyOpener() == "Unknown")
                return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 45));

            // 9-9
            if (Spy::getEnemyOpener() == P_9_9)
                return 1 + (Util::getTime() > Time(4, 30));

            // Proxy 9-9
            if (Spy::getEnemyOpener() == P_Proxy_9_9)
                return 1;

            // Horror 9-9
            if (Spy::getEnemyOpener() == P_Horror_9_9)
                return 1;

            return (Util::getTime() > Time(3, 15));
        }

        // FFE
        if (Spy::getEnemyBuild() == P_FFE) {
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 6)
                return 2;
            return Util::getTime() > Time(6, 00);
        }

        // CannonRush
        if (Spy::getEnemyBuild() == P_CannonRush) {
            return 1;
        }

        // Always make one that is a safety measure vs unknown builds
        return Util::getTime() > Time(3, 45);
    }

    int ZvP_Transition(BWEB::Wall &wall)
    {
        // 1 base transitions
        if (Spy::getEnemyBuild() == P_2Gate || Spy::getEnemyBuild() == P_1GateCore) {
            auto initial = (Spy::getEnemyBuild() == P_2Gate) ? (2 * (Util::getTime() > Time(4, 00))) : (Util::getTime() > Time(4, 00));

            // 4Gate
            if (Spy::getEnemyTransition() == P_4Gate) {
                return initial + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 00)) + (Util::getTime() > Time(5, 15)) + (Util::getTime() > Time(5, 30)) +
                       (Util::getTime() > Time(5, 45)) + (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 15)) + (Util::getTime() > Time(6, 30));
            }

            // DT
            if (Spy::getEnemyTransition() == P_DT) {
                return initial + (Util::getTime() > Time(4, 45)) + 2 * (Util::getTime() > Time(5, 15)) + 2 * (Util::getTime() > Time(6, 00));
            }

            // Corsair
            if (Spy::getEnemyTransition() == P_Corsair || Spy::getEnemyTransition() == P_Robo || Spy::getEnemyTransition() == P_CorsairGoon) {
                return initial + (Util::getTime() > Time(5, 15)) + (Util::getTime() > Time(6, 15)) + (Util::getTime() > Time(6, 45)) + (Util::getTime() > Time(7, 15));
            }

            // Speedlot
            if (Spy::getEnemyTransition() == P_Speedlot) {
                return initial + (Util::getTime() > Time(4, 45)) + 2 * (Util::getTime() > Time(5, 15)) + 2 * (Util::getTime() > Time(6, 15));
            }

            // Zealot flood
            if (Spy::getEnemyTransition() == P_Rush) {
                return initial + (Util::getTime() > Time(4, 25)) + (Util::getTime() > Time(4, 50)) + (Util::getTime() > Time(5, 15)) + (Util::getTime() > Time(5, 40)) +
                       (Util::getTime() > Time(6, 05)) - Spy::enemyProxy();
            }

            // Worker rush with zealots
            if (Spy::getEnemyTransition() == U_WorkerRush) {
                return initial + (Util::getTime() > Time(4, 40)) + (Util::getTime() > Time(5, 20)) + (Util::getTime() > Time(5, 40)) + (Util::getTime() > Time(6, 20));
            }

            if (Util::getTime() < Time(7, 00)) {
                static map<BWAPI::UnitType, double> trackables = {{Protoss_Zealot, 0.4}, {Protoss_Dragoon, 0.5}, {Protoss_Dark_Templar, 1.0}};
                auto arrivalValue = 0.0;
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    auto &unit = *u;

                    auto idx = trackables.find(unit.getType());
                    if (idx != trackables.end()) {
                        if ((Units::inBoundUnit(unit, 25) || vis(Zerg_Spire) > 0)) {
                            arrivalValue += idx->second;
                        }
                    }
                }
                return ceil(arrivalValue);
            }
        }

        // FFE transitions
        if (Spy::getEnemyBuild() == P_FFE) {
            if (Spy::getEnemyTransition() == P_5GateGoon)
                return (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 30)) + (Util::getTime() > Time(6, 45)) + (Util::getTime() > Time(7, 00)) + (Util::getTime() > Time(7, 15)) +
                       (Util::getTime() > Time(7, 30));

            if (Spy::getEnemyTransition() == P_CorsairGoon)
                return (Util::getTime() > Time(7, 00));

            if (Spy::getEnemyTransition() == P_Speedlot)
                return (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 20)) + (Util::getTime() > Time(6, 40));

            if (Spy::getEnemyTransition() == P_Sairlot)
                return 2 * (Util::getTime() > Time(6, 50));

            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0)
                return (Util::getTime() > Time(6, 00));

            return (Util::getTime() > Time(7, 15));
        }

        return 0;
    }

    int ZvP_GroundDefenses(BWEB::Wall &wall)
    {
        // Determine how much we have traded
        auto unitsKilled     = Players::getDeadCount(PlayerState::Enemy, Protoss_Zealot, Protoss_Dragoon, Protoss_Dark_Templar, Protoss_High_Templar);
        auto buildingsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Gateway);

        auto mutaBuild  = BuildOrder::getCurrentTransition().find("Muta") != string::npos;
        auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
        auto expected   = max(ZvP_Opener(wall), ZvP_Transition(wall));
        auto reduction  = (unitsKilled / 8) + buildingsKilled;
        auto minimum    = int(expected > 0);

        // 3h builds make roughly half as many
        if (threeHatch && expected > 1 && Spy::getEnemyBuild() != P_FFE && Spy::getEnemyBuild() != P_CannonRush && Spy::getEnemyTransition() != P_4Gate)
            expected = int(floor(double(expected) / 1.5));

        // Non natural walls are limited to 1 total
        if (!wall.getStation()->isNatural() && expected > 0 && Spy::getEnemyBuild() != P_FFE) {
            expected = 1;
            minimum  = 1;
        }

        // Make minimum sunkens if criteria fulfilled
        if (expected > 0 || Util::getTime() > Time(8, 00))
            minimum = 1;
        if (Spy::getEnemyBuild() != P_FFE && Spy::getEnemyBuild() != P_CannonRush && Util::getTime() < Time(8, 00)) {
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 || Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Singularity_Charge, 1) ||
                Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 4)
                minimum = 2;
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0)
                expected--;
        }

        // Against turtle builds we don't need any for a while
        if (Spy::enemyTurtle() && Util::getTime() < Time(5, 00))
            return 0;

        return max(minimum, expected - reduction);
    }

    // ZvP Air
    int ZvP_AirDefenses(BWEB::Wall &wall) { return 0; }

    // ZvT Ground
    int ZvT_Opener(BWEB::Wall &wall)
    {
        // 8Rax / Proxy
        if (Spy::enemyProxy() || Spy::getEnemyOpener() == T_8Rax || Spy::getEnemyOpener() == T_Proxy_8Rax)
            return 1 + (Util::getTime() > Time(4, 30));

        // 2Rax
        if (Spy::getEnemyBuild() == T_2Rax) {

            if (Spy::enemyRush() || Spy::getEnemyOpener() == T_BBS)
                return (Util::getTime() > Time(2, 50)) + (Util::getTime() > Time(4, 30));

            if (Spy::getEnemyOpener() == T_11_13)
                return (Util::getTime() > Time(3, 30));

            if (Spy::getEnemyOpener() == T_11_18)
                return (Util::getTime() > Time(4, 00));

            if (!Spy::enemyFastExpand())
                return 1 + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 45));
        }

        // RaxCC
        if (Spy::getEnemyBuild() == T_RaxCC) {
            return (Util::getTime() > Time(5, 00));
        }

        // RaxFact
        if (Spy::getEnemyBuild() == T_RaxFact) {
            return Util::getTime() > Time(3, 30);
        }

        //
        if (Scouts::enemyDeniedScout() || Spy::enemyWalled() || Spy::getEnemyBuild() == "Unknown")
            return 1;

        // Fall through unknown opener
        if (!Spy::enemyFastExpand() && !Spy::enemyRush())
            return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 00));

        return (Util::getTime() > Time(3, 15));
    }

    int ZvT_Transition(BWEB::Wall &wall)
    {
        if (Spy::getEnemyTransition() == U_WorkerRush)
            return 0;

        // MarineRush
        if (Spy::getEnemyTransition() == T_Rush)
            return 2 + (Util::getTime() > Time(4, 20)) + (Util::getTime() > Time(4, 40)) + (Util::getTime() > Time(5, 00)) + (Util::getTime() > Time(5, 45)) + (Util::getTime() > Time(6, 30));

        // 2Rax Acad
        if (Spy::getEnemyTransition() == T_Academy)
            return 5 * (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 45));

        // T_4Rax
        if (Spy::getEnemyTransition() == T_4Rax)
            return 3 * (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 15)) + (Util::getTime() > Time(6, 30));

        // 3FactGoliath
        if (Spy::getEnemyTransition() == T_3FactGoliath && Stations::getStations(PlayerState::Self).size() >= 3)
            return 1 + (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 30));

        // T_2PortWraith
        if (Spy::getEnemyTransition() == T_2PortWraith)
            return (Util::getTime() > Time(5, 30));

        // T_2FactVulture
        if (Spy::getEnemyTransition() == T_2FactVulture)
            return 1;

        // T_1FactTanks
        if (Spy::getEnemyTransition() == T_1FactTanks)
            return (Util::getTime() > Time(7, 45)) + (Util::getTime() > Time(8, 15)) + (Util::getTime() > Time(8, 30)) + (Util::getTime() > Time(8, 45));

        // No expand or spire visible
        if (!Spy::enemyFastExpand() || vis(Zerg_Spire) > 0)
            return 2 * (Util::getTime() > Time(6, 00));

        return 0;
    }

    int ZvT_GroundDefenses(BWEB::Wall &wall)
    {
        // Determine how much we have traded
        auto bioKilled       = Players::getDeadCount(PlayerState::Enemy, Terran_Marine, Terran_Firebat, Terran_Medic);
        auto mechKilled      = Players::getDeadCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode, Terran_Siege_Tank_Tank_Mode, Terran_Goliath);
        auto buildingsKilled = Players::getDeadCount(PlayerState::Enemy, Terran_Barracks);

        auto minimum   = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 ? 1 : 0;
        auto expected  = max(ZvT_Opener(wall), ZvT_Transition(wall));
        auto reduction = (bioKilled / 16) + (mechKilled / 3) + buildingsKilled;

        if (expected > 0)
            minimum = 1;
        if (Util::getTime() > Time(10, 00) && expected >= 2)
            minimum = 2;

        return max(minimum, expected - reduction);
    }

    // ZvT Air
    int ZvT_AirDefenses(BWEB::Wall &wall) { return 0; }

    // ZvZ Ground
    int ZvZ_GroundDefenses(BWEB::Wall &wall)
    {
        if (Spy::getEnemyTransition() == Z_1HatchMuta || Spy::getEnemyTransition() == Z_2HatchMuta)
            return 0;

        // 3 Hatch
        if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 || Spy::getEnemyTransition() == Z_3HatchSpeedling)
            return 1 + (Util::getTime() > Time(4, 15));
        if (Spy::getEnemyTransition() == Z_2HatchSpeedling || (Stations::getStations(PlayerState::Enemy).size() <= 1 && Players::getTotalCount(PlayerState::Enemy, Zerg_Hatchery) >= 2))
            return 1 + (vis(Zerg_Spire) > 0);

        // Early hatch
        if (BuildOrder::getCurrentOpener() == Z_12Pool) {
            if (Spy::getEnemyOpener() == Z_12Hatch || Spy::getEnemyOpener() == Z_10Hatch)
                return 1;
        }

        // 1Hatch Hydra/Lurker
        if (Spy::getEnemyTransition() == Z_1HatchLurker) {
            return 2 * (Util::getTime() > Time(4, 30));
        }

        return 0;
    }

    // ZvZ Air
    int ZvZ_AirDefenses(BWEB::Wall &wall) { return 0; }

    // ZvFFA Ground
    int ZvFFA_GroundDefenses(BWEB::Wall &wall) { return 1 + (Util::getTime() > Time(5, 20)) + (Util::getTime() > Time(5, 40)); }

    // ZvFFA Air
    int ZvFFA_AirDefenses(BWEB::Wall &wall) { return 0; }
} // namespace McRave::Walls::Zerg
