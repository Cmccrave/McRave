#include "Info/Player/PlayerInfo.h"
#include "Info/Player/Players.h"
#include "Info/Unit/UnitInfo.h"
#include "Map/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UpgradeTypes;

namespace McRave::Spy::Protoss {

    void enemyProtossBuilds(PlayerInfo &player, StrategySpy &theSpy)
    {
        for (auto &u : player.getUnits()) {
            UnitInfo &unit = *u;

            // Zealot timing
            if (unit.getType() == Protoss_Zealot) {
                if (unit.timeArrivesWhen().minutes > 0 && unit.timeArrivesWhen() < theSpy.rushArrivalTime)
                    theSpy.rushArrivalTime = unit.timeArrivesWhen();
            }

            // CannonRush
            if (unit.getType() == Protoss_Forge && Scouts::gotFullScout() && Terrain::getEnemyStartingPosition().isValid() &&
                unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 320.0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) == 0 &&
                Players::getVisibleCount(PlayerState::Enemy, Protoss_Nexus) <= 1) {
                theSpy.build.name     = P_CannonRush;
                theSpy.proxy.possible = true;
            }
            if ((unit.getType() == Protoss_Forge || unit.getType() == Protoss_Photon_Cannon) && unit.isProxy()) {
                theSpy.build.name     = P_CannonRush;
                theSpy.proxy.possible = true;
            }
            if (unit.getType() == Protoss_Pylon && unit.isProxy())
                theSpy.early.possible = true;

            // FFE
            if (Players::ZvP() && Util::getTime() < Time(4, 00) && Terrain::getEnemyNatural() &&
                (unit.getType() == Protoss_Photon_Cannon || unit.getType() == Protoss_Forge || unit.getType() == Protoss_Nexus)) {
                if (unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getChokepoint()->Center())) < 320.0 ||
                    unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getBase()->Center())) < 320.0) {
                    theSpy.build.name      = P_FFE;
                    theSpy.expand.possible = true;
                }
            }
        }

        if (Players::getTotalCount(PlayerState::Enemy, Protoss_Forge) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0) {

            // 1GateCore - Gas estimation
            if ((completesBy(1, Protoss_Assimilator, Time(2, 15)) && !theSpy.steal.possible) ||
                (Players::getTotalCount(PlayerState::Enemy, Protoss_Gateway) > 0 && completesBy(1, Protoss_Assimilator, Time(2, 50)) && !theSpy.steal.possible))
                theSpy.build.name = P_1GateCore;

            // 1GateCore - Core estimation
            if (completesBy(1, Protoss_Cybernetics_Core, Time(3, 25)))
                theSpy.build.name = P_1GateCore;

            // 1GateCore - Goon estimation
            if (completesBy(1, Protoss_Dragoon, Time(4, 10)) || completesBy(2, Protoss_Dragoon, Time(4, 30)))
                theSpy.build.name = P_1GateCore;

            // 1GateCore - Tech estimation
            if (completesBy(1, Protoss_Scout, Time(5, 15)) || completesBy(1, Protoss_Corsair, Time(5, 15)) || completesBy(1, Protoss_Stargate, Time(4, 10)))
                theSpy.build.name = P_1GateCore;

            // 1GateCore - Turtle estimation
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Forge) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Gateway) > 0 &&
                Players::getTotalCount(PlayerState::Enemy, Protoss_Cybernetics_Core) > 0 && Util::getTime() < Time(4, 00))
                theSpy.build.name = P_1GateCore;

            // 1GateCore - Fallback assumption
            if (Util::getTime() > Time(3, 45) && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 2)
                theSpy.build.name = P_1GateCore;

            // 2Gate Proxy - No info estimation
            auto nothing = Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Forge) == 0 &&
                           Players::getVisibleCount(PlayerState::Enemy, Protoss_Cybernetics_Core) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) == 0 &&
                           Players::getVisibleCount(PlayerState::Enemy, Protoss_Nexus) <= 1;
            if (Util::getTime() < Time(3, 30) && nothing && Scouts::gotFullScout()) {
                theSpy.build.name     = P_2Gate;
                theSpy.opener.name    = P_Proxy_9_9;
                theSpy.proxy.possible = true;
            }

            // 2Gate Proxy - 2 in base pylons
            if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Pylon) >= 2 && Scouts::gotFullScout() && nothing) {
                theSpy.build.name     = P_2Gate;
                theSpy.opener.name    = P_Proxy_9_9;
                theSpy.proxy.possible = true;
            }

            // 2Gate - Zealot estimation
            if ((Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 5 && Util::getTime() < Time(4, 00)) ||
                (Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 3 && Util::getTime() < Time(3, 30)) ||
                (theSpy.proxy.possible == true && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) > 0) || arrivesBy(2, Protoss_Zealot, Time(3, 25)) ||
                arrivesBy(3, Protoss_Zealot, Time(4, 00)) || arrivesBy(4, Protoss_Zealot, Time(4, 20)) || completesBy(2, Protoss_Gateway, Time(2, 55)) || completesBy(3, Protoss_Zealot, Time(3, 20))) {
                theSpy.build.name = P_2Gate;
            }
        }

        // 1GateCore - fallback when they're playing something terrible that makes no sense
        if (Util::getTime() > Time(5, 00) && !theSpy.proxy.likely)
            theSpy.build.name = P_1GateCore;
    }

    void enemyProtossOpeners(PlayerInfo &player, StrategySpy &theSpy)
    {
        // 2Gate Openers
        if (theSpy.build.name == P_2Gate) {

            // Horror/Proxy 9/9
            if (theSpy.proxy.possible) {
                auto closestGate = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) { return u->getType() == Protoss_Gateway; });
                if (closestGate && Terrain::inTerritory(PlayerState::Self, closestGate->getPosition()))
                    theSpy.opener.name = P_Horror_9_9;
                else
                    theSpy.opener.name = P_Proxy_9_9;
            }

            // Proxy 9/9
            else if (arrivesBy(1, Protoss_Zealot, Time(2, 50)) || arrivesBy(2, Protoss_Zealot, Time(3, 10)) || arrivesBy(4, Protoss_Zealot, Time(3, 40))) {
                theSpy.opener.name    = P_Proxy_9_9;
                theSpy.proxy.possible = true;
            }

            // 9/9
            else if (arrivesBy(1, Protoss_Zealot, Time(3, 00)) || arrivesBy(2, Protoss_Zealot, Time(3, 15)) || arrivesBy(3, Protoss_Zealot, Time(3, 20)) || arrivesBy(4, Protoss_Zealot, Time(3, 30)) ||
                     arrivesBy(5, Protoss_Zealot, Time(3, 35)) || completesBy(2, Protoss_Zealot, Time(2, 45)) || completesBy(3, Protoss_Zealot, Time(2, 50)) ||
                     completesBy(4, Protoss_Zealot, Time(3, 10)) || completesBy(5, Protoss_Zealot, Time(3, 15)) || completesBy(2, Protoss_Gateway, Time(2, 15)))
                theSpy.opener.name = P_9_9;

            // 10/12
            else if (arrivesBy(3, Protoss_Zealot, Time(4, 05)) || arrivesBy(4, Protoss_Zealot, Time(4, 20)))
                theSpy.opener.name = P_10_12;

            // 10/15
            else if (arrivesBy(3, Protoss_Zealot, Time(4, 20)) || arrivesBy(2, Protoss_Dragoon, Time(5, 00)) || completesBy(1, Protoss_Cybernetics_Core, Time(3, 40)))
                theSpy.opener.name = P_10_15;
        }

        // FFE Openers - need timings for when Nexus/Forge/Gate complete
        if (theSpy.build.name == P_FFE) {

            // Forge
            if (completesBy(1, Protoss_Photon_Cannon, Time(3, 30)) || completesBy(1, Protoss_Forge, Time(2, 55)))
                theSpy.opener.name = P_Forge;

            // Nexus
            else if (completesBy(1, Protoss_Nexus, Time(3, 30)) && Spy::enemyFastExpand())
                theSpy.opener.name = P_Nexus;

            // Gateway
            else if (completesBy(1, Protoss_Gateway, Time(2, 40)) || arrivesBy(1, Protoss_Zealot, Time(3, 45)))
                theSpy.opener.name = P_Gateway;
        }

        // 1Gate Openers -  need timings for when Core completes
        if (theSpy.build.name == P_1GateCore) {

            // NZCore
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) == 0)
                theSpy.opener.name = P_NZCore;

            // ZCore
            else if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) == 1)
                theSpy.opener.name = P_ZCore;

            // ZZCore
            else if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 2)
                theSpy.opener.name = P_ZZCore;
        }

        if (theSpy.build.name == P_CannonRush) {
            auto closestBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) { return u->getType() == Protoss_Pylon || u->getType() == Protoss_Forge; });

            if (closestBuilding) {
                if (Terrain::inArea(Terrain::getMainArea(), closestBuilding->getPosition()))
                    theSpy.opener.name = P_Main;
                else if (Terrain::inArea(Terrain::getNaturalArea(), closestBuilding->getPosition()))
                    theSpy.opener.name = P_Natural;
                else
                    theSpy.opener.name = P_Contain;
            }
        }
    }

    void enemyProtossTransitions(PlayerInfo &player, StrategySpy &theSpy)
    {
        // Rush detection
        if (theSpy.build.name == P_2Gate) {
            if ((Players::getVisibleCount(PlayerState::Enemy, Protoss_Assimilator) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 8 && Util::getTime() < Time(4, 00)) ||
                (Players::getVisibleCount(PlayerState::Enemy, Protoss_Assimilator) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) == 0 &&
                 Players::getVisibleCount(PlayerState::Enemy, Protoss_Cybernetics_Core) == 0 && Util::getTime() > Time(6, 00)) ||
                (Players::getVisibleCount(PlayerState::Enemy, Protoss_Assimilator) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) == 0 &&
                 Players::getVisibleCount(PlayerState::Enemy, Protoss_Cybernetics_Core) == 0 && completesBy(3, Protoss_Gateway, Time(4, 30))) ||
                completesBy(3, Protoss_Gateway, Time(3, 30)) || theSpy.opener.name == P_Proxy_9_9)
                theSpy.transition.name = P_Rush;
        }

        if (theSpy.workersPulled >= 4 && Util::getTime() < Time(4, 00))
            theSpy.transition.name = U_WorkerRush;

        // 2Gate
        if (theSpy.build.name == P_2Gate || theSpy.build.name == P_1GateCore) {

            // DT
            if (!theSpy.expand.possible) {
                if ((!Players::ZvP() && completesBy(1, Protoss_Citadel_of_Adun, Time(5, 00)) && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) > 0) ||
                    completesBy(1, Protoss_Templar_Archives, Time(6, 00)) || arrivesBy(1, Protoss_Dark_Templar, Time(7, 00)) ||
                    (!Players::ZvP() && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 2 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 &&
                     Util::getTime() > Time(6, 45)))
                    theSpy.transition.name = P_DT;
            }

            // Speedlot
            if (completesBy(1, UpgradeTypes::Leg_Enhancements, Time(7, 00)))
                theSpy.transition.name = P_Speedlot;

            // Corsair
            if ((theSpy.build.name == P_2Gate && completesBy(1, Protoss_Stargate, Time(6, 30))) || (theSpy.build.name == P_1GateCore && completesBy(1, Protoss_Stargate, Time(5, 00))) ||
                (theSpy.build.name == P_2Gate && completesBy(1, Protoss_Scout, Time(7, 00))) || (theSpy.build.name == P_2Gate && completesBy(1, Protoss_Corsair, Time(7, 00))) ||
                (theSpy.build.name == P_2Gate && arrivesBy(1, Protoss_Corsair, Time(7, 30))) || (theSpy.build.name == P_1GateCore && completesBy(1, Protoss_Scout, Time(5, 15))) ||
                (theSpy.build.name == P_1GateCore && completesBy(1, Protoss_Corsair, Time(5, 15))) || (theSpy.build.name == P_1GateCore && arrivesBy(1, Protoss_Corsair, Time(5, 45))))
                theSpy.transition.name = P_Corsair;

            // Robo
            if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Shuttle) > 0 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Reaver) > 0 ||
                Players::getVisibleCount(PlayerState::Enemy, Protoss_Observer) > 0 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Robotics_Facility) > 0 ||
                Players::getVisibleCount(PlayerState::Enemy, Protoss_Robotics_Support_Bay) > 0 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Observatory) > 0)
                theSpy.transition.name = P_Robo;

            // 4Gate
            if (Players::PvP()) {
                if ((Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 7 && Util::getTime() < Time(6, 30)) ||
                    (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 11 && Util::getTime() < Time(7, 15)) ||
                    (Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) >= 4 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 &&
                     Util::getTime() < Time(5, 30)))
                    theSpy.transition.name = P_4Gate;
            }

            // DT Drop
            if (completesBy(1, Protoss_Robotics_Facility, Time(5, 00)) && completesBy(1, Protoss_Templar_Archives, Time(5, 30)))
                theSpy.transition.name = P_DTDrop;


            if (Players::ZvP()) {

                if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Stargate) == 0) {
                    if ((!theSpy.expand.possible && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 4 && Util::getTime() < Time(6, 00)) ||                        
                        (Spy::enemyTurtle() && completesBy(1, Singularity_Charge, Time(6, 30))) || (arrivesBy(3, Protoss_Dragoon, Time(5, 15))) || (arrivesBy(5, Protoss_Dragoon, Time(6, 05))) ||
                        (arrivesBy(9, Protoss_Dragoon, Time(6, 40))) || (arrivesBy(12, Protoss_Dragoon, Time(7, 25))) ||
                        (completesBy(4, Protoss_Gateway, Time(5, 30)) &&
                         Players::getVisibleCount(PlayerState::Enemy, Protoss_Cybernetics_Core) + Players::getVisibleCount(PlayerState::Enemy, Protoss_Assimilator) >= 1))
                        theSpy.transition.name = P_4Gate;
                }

                if ((theSpy.typeUpgrading.find(Protoss_Cybernetics_Core) != theSpy.typeUpgrading.end() && Util::getTime() < Time(4, 15)) || (completesBy(1, Singularity_Charge, Time(6, 00)))) {
                    if (completesBy(1, Protoss_Stargate, Time(5, 00)) || completesBy(1, Protoss_Corsair, Time(5, 30)) || arrivesBy(1, Protoss_Corsair, Time(5, 50)))
                        theSpy.transition.name = P_CorsairGoon;
                } 
                
                // CorsairGoon
                if (completesBy(1, UpgradeTypes::Singularity_Charge, Time(4, 45)) && arrivesBy(1, Protoss_Corsair, Time(6, 00)))
                    theSpy.transition.name = P_CorsairGoon;
            }

            // 5ZealotExpand
            if (Players::PvP() && theSpy.expand.possible && (Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 5) &&
                Util::getTime() < Time(4, 45))
                theSpy.transition.name = P_5ZealotExpand;
        }

        // FFE transitions
        if (Players::ZvP() && theSpy.build.name == P_FFE) {

            // 5GateGoon
            auto noStargateTech = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) == 0 &&
                                  Players::getTotalCount(PlayerState::Enemy, Protoss_Carrier) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter) == 0 &&
                                  Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Fleet_Beacon) == 0 &&
                                  Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter_Tribunal) == 0;

            if (noStargateTech && (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 4 || completesBy(1, UpgradeTypes::Singularity_Charge, Time(7, 00))))
                theSpy.transition.name = P_5GateGoon;

            // NeoBisu
            else if (completesBy(1, Protoss_Stargate, Time(5, 45)) && completesBy(1, Protoss_Citadel_of_Adun, Time(5, 45)) && theSpy.typeUpgrading.find(Protoss_Forge) != theSpy.typeUpgrading.end())
                theSpy.transition.name = P_NeoBisu;

            // Speedlot
            else if ((completesBy(1, Protoss_Citadel_of_Adun, Time(5, 30)) && completesBy(2, Protoss_Gateway, Time(5, 45))) ||
                     (Util::getTime() < Time(8, 30) && completesBy(1, UpgradeTypes::Leg_Enhancements, Time(7, 00))))
                theSpy.transition.name = P_Speedlot;

            // Sairlot
            else if (Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 && Util::getTime() < Time(8, 30) && completesBy(1, UpgradeTypes::Leg_Enhancements, Time(8, 00)) &&
                     completesBy(1, UpgradeTypes::Protoss_Ground_Weapons, Time(7, 45)))
                theSpy.transition.name = P_Sairlot;

            // ZealotArchon
            else if (completesBy(1, Protoss_Templar_Archives, Time(7, 30)) || (arrivesBy(1, Protoss_Archon, Time(8, 15)) && completesBy(1, UpgradeTypes::Leg_Enhancements, Time(8, 00))))
                theSpy.transition.name = P_ZealotArchon;

            // CorsairReaver
            else if (completesBy(1, Protoss_Stargate, Time(5, 45)) && completesBy(1, Protoss_Robotics_Facility, Time(6, 30)))
                theSpy.transition.name = P_CorsairReaver;

            // CorsairGoon
            else if (completesBy(1, UpgradeTypes::Singularity_Charge, Time(9, 00)) && completesBy(1, Protoss_Corsair, Time(7, 00)))
                theSpy.transition.name = P_CorsairGoon;

            // 2Stargate
            else if (completesBy(2, Protoss_Stargate, Time(7, 00)))
                theSpy.transition.name = P_2Stargate;

            // Carriers
            else if (completesBy(1, Protoss_Fleet_Beacon, Time(9, 30)) || completesBy(1, Protoss_Carrier, Time(12, 00)))
                theSpy.transition.name = P_Carrier;
        }
    }

    void enemyProtossMisc(PlayerInfo &player, StrategySpy &theSpy)
    {
        // Turtle detection
        if (!theSpy.expand.likely && theSpy.build.name != P_CannonRush) {
            if (completesBy(2, Protoss_Photon_Cannon, Time(3, 30)) || completesBy(3, Protoss_Photon_Cannon, Time(3, 45)))
                theSpy.turtle.possible = true;
        }
    }

    void updateProtoss(StrategySpy &theSpy)
    {
        for (auto &p : Players::getPlayers()) {
            PlayerInfo &player = p.second;
            if (player.isEnemy() && player.getCurrentRace() == Races::Protoss) {
                if (!theSpy.build.confirmed)
                    enemyProtossBuilds(player, theSpy);
                if (!theSpy.opener.confirmed)
                    enemyProtossOpeners(player, theSpy);
                if (!theSpy.transition.confirmed)
                    enemyProtossTransitions(player, theSpy);
                enemyProtossMisc(player, theSpy);
            }
        }
    }
} // namespace McRave::Spy::Protoss