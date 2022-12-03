#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Roles {

    namespace {
        map<Role, int> myRoles;
        map<Role, int> forcedRoles;

        void forceCombatWorker(int count, LocalState lState, GlobalState gState)
        {
            auto needed = count - forcedRoles[Role::Combat];
            if (needed <= 0)
                return;

            // Only pull the closest worker to our defend position
            for (int i = 0; i < needed; i++) {
                auto closestWorker = Util::getClosestUnit(Terrain::getDefendPosition(), PlayerState::Self, [&](auto &u) {
                    if (u->getType() == Protoss_Probe && u->getShields() <= 4) // Don't pull low shield probes
                        return false;
                    if (u->getType() == Zerg_Drone && u->getHealth() < 20) // Don't pull low health drones
                        return false;
                    return u->getRole() == Role::Worker && !u->getGoal().isValid() && (!u->hasResource() || !u->getResource().lock()->getType().isRefinery()) && !u->getBuildPosition().isValid();
                });

                if (closestWorker) {
                    closestWorker->setRole(Role::Combat);
                    closestWorker->setLocalState(lState);
                    closestWorker->setGlobalState(gState);
                    forcedRoles[Role::Combat]++;
                }
            }
        }

        void pPullWorker()
        {
            if (Broodwar->self()->getRace() != Races::Protoss)
                return;

            int completedDefenders = com(Protoss_Photon_Cannon) + com(Protoss_Zealot);
            int visibleDefenders = vis(Protoss_Photon_Cannon) + vis(Protoss_Zealot);

            // If trying to hide tech, pull 2 probes with a Zealot
            if (!BuildOrder::isRush() && BuildOrder::isHideTech() && com(Protoss_Zealot) == 1 && total(Protoss_Zealot) == 1 && com(Protoss_Dragoon) == 0 && total(Protoss_Dragoon) == 0)
                forceCombatWorker(2, LocalState::None, GlobalState::Retreat);

            // If trying to FFE, pull based on Cannon/Zealot numbers, or lack of scouting information
            else if (BuildOrder::getCurrentBuild() == "FFE") {
                if (Spy::enemyRush() && Spy::getEnemyOpener() == "4Pool" && visibleDefenders >= 1)
                    forceCombatWorker(8 - 2 * completedDefenders, LocalState::None, GlobalState::Retreat);
                else if (Spy::enemyRush() && Spy::getEnemyOpener() == "9Pool" && Util::getTime() > Time(3, 15) && completedDefenders < 3)
                    forceCombatWorker(3, LocalState::None, GlobalState::Retreat);
                else if (!Terrain::getEnemyStartingPosition().isValid() && Spy::getEnemyBuild() == "Unknown" && completedDefenders < 1 && visibleDefenders > 0)
                    forceCombatWorker(1, LocalState::None, GlobalState::Retreat);
            }

            // If trying to 1GateCore and scouted 2Gate late, pull workers to block choke when we are ready
            else if (BuildOrder::getCurrentBuild() == "1GateCore" && Spy::getEnemyBuild() == "2Gate" && Combat::defendChoke()) {
                if (Util::getTime() < Time(3, 30))
                    forceCombatWorker(2, LocalState::None, GlobalState::Retreat);
            }
        }

        void tPullWorker()
        {

        }

        void zPullWorker()
        {
            if (Broodwar->self()->getRace() != Races::Zerg)
                return;

            auto proxyDangerousBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->isProxy() && u->getType().isBuilding() && u->canAttackGround();
            });
            auto proxyBuildingWorker = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->getType().isWorker() && (u->isThreatening() || (proxyDangerousBuilding && u->getType().isWorker() && u->getPosition().getDistance(proxyDangerousBuilding->getPosition()) < 160.0));
            });

            // ZvZ
            if (Players::ZvZ()) {
                if (Spy::getEnemyOpener() == "9Pool" && BuildOrder::getCurrentOpener() == "12Pool" && total(Zerg_Zergling) < 16 && int(Stations::getStations(PlayerState::Self).size()) >= 2)
                    forceCombatWorker(3, LocalState::None, GlobalState::Retreat);
                else if (Spy::getEnemyOpener() == "4Pool" && Units::getImmThreat() > 0.0f && com(Zerg_Zergling) == 0)
                    forceCombatWorker(10, LocalState::None, GlobalState::Retreat);
            }

            // ZvP
            if (Players::ZvP()){

                // If we suspect a cannon rush is coming
                if (Spy::enemyPossibleProxy() && Util::getTime() < Time(3, 00))
                    forceCombatWorker(1, LocalState::Attack, GlobalState::Attack);
                else if (proxyDangerousBuilding && Spy::getEnemyBuild() == "CannonRush" && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, LocalState::Attack, GlobalState::Attack);

                // If we're trying to make our expanding hatchery and the drone is being harassed
                else if (vis(Zerg_Hatchery) == 1 && Util::getTime() < Time(3, 00) && BuildOrder::isOpener() && Units::getImmThreat() > 0.0f && com(Zerg_Zergling) == 0)
                    forceCombatWorker(1, LocalState::Attack, GlobalState::Attack);
                else if (Util::getTime() < Time(4, 00) && int(Stations::getStations(PlayerState::Self).size()) < 2 && BuildOrder::getBuildQueue()[Zerg_Hatchery] >= 2 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Probe) > 0)
                    forceCombatWorker(1, LocalState::Attack, GlobalState::Attack);
            }

            // ZvT
            if (Players::ZvT()) {

                // Proxy fighting
                if (proxyDangerousBuilding && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(6, LocalState::Attack, GlobalState::Attack);
                else if (BuildOrder::getCurrentOpener() == "12Hatch" && Spy::getEnemyOpener() == "8Rax" && com(Zerg_Zergling) < 2)
                    forceCombatWorker(8, LocalState::Attack, GlobalState::Attack);

                // If we're trying to make our expanding hatchery and the drone is being harassed
                else if (vis(Zerg_Hatchery) == 1 && Util::getTime() < Time(3, 00) && BuildOrder::isOpener() && Units::getImmThreat() > 0.0f && com(Zerg_Zergling) == 0)
                    forceCombatWorker(1, LocalState::Attack, GlobalState::Attack);
                else if (Util::getTime() < Time(4, 00) && int(Stations::getStations(PlayerState::Self).size()) < 2 && BuildOrder::getBuildQueue()[Zerg_Hatchery] >= 2 && Players::getVisibleCount(PlayerState::Enemy, Terran_SCV) > 0)
                    forceCombatWorker(1, LocalState::Attack, GlobalState::Attack);
            }

            // Misc
            if (Spy::getWorkersNearUs() > 2 && com(Zerg_Zergling) < Spy::getWorkersNearUs())
                forceCombatWorker(Spy::getWorkersNearUs() + 2, LocalState::Attack, GlobalState::Attack);
        }

        void updateForcedRoles()
        {
            forcedRoles.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.getRole() == Role::Combat && unit.getType().isWorker())
                    unit.setRole(Role::Worker);
            }

            pPullWorker();
            tPullWorker();
            zPullWorker();
        }

        void updateDefaultRoles()
        {
            myRoles.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;

                // Don't assign a role to uncompleted units
                if (!unit.unit()->isCompleted() && !unit.getType().isBuilding() && unit.getType() != Zerg_Egg) {
                    unit.setRole(Role::None);
                    continue;
                }

                // Update default role for each unit
                if (unit.getRole() == Role::None) {
                    if (unit.getType().isWorker())
                        unit.setRole(Role::Worker);
                    else if ((unit.getType().isBuilding() && !unit.canAttackGround() && !unit.canAttackAir() && unit.getType() != Zerg_Creep_Colony) || unit.getType() == Zerg_Larva || unit.getType() == Zerg_Egg)
                        unit.setRole(Role::Production);
                    else if (unit.getType().isBuilding() && (unit.canAttackGround() || unit.canAttackAir() || unit.getType() == Zerg_Creep_Colony))
                        unit.setRole(Role::Defender);
                    else if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Protoss_Arbiter)
                        unit.setRole(Role::Support);
                    else if (unit.getType().spaceProvided() > 0)
                        unit.setRole(Role::Transport);
                    else if (unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Protoss_Scarab || unit.getType().isSpell() || unit.getType() == Terran_Nuclear_Missile)
                        unit.setRole(Role::Consumable);
                    else
                        unit.setRole(Role::Combat);
                }

                // Check if a worker morphed into a building
                if (unit.getRole() == Role::Worker && unit.getType().isBuilding()) {
                    if (unit.getType().isBuilding() && !unit.canAttackGround() && !unit.canAttackAir() && unit.getType() != Zerg_Creep_Colony)
                        unit.setRole(Role::Production);
                    else
                        unit.setRole(Role::Defender);
                }

                // If we cancelled any buildings, we may need to re-assign the worker
                if (unit.getType().isWorker() && unit.getRole() == Role::Production)
                    unit.setRole(Role::Worker);

                myRoles[unit.getRole()]++;
            }
        }

        void updateSelf()
        {
            updateDefaultRoles();
            updateForcedRoles();
        }
    }

    void onFrame()
    {
        updateSelf();
    }

    int getMyRoleCount(Role role) { return myRoles[role]; }
}