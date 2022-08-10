#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Roles {

    namespace {

        //int lastCombatWorkerRoleChange = 0;
        map<Role, int> myRoles;

        void updateCombatWorkerRole(UnitInfo& unit)
        {
            // Can't change role to combat if not a worker or we did one this frame
            if (!unit.getType().isWorker()
                //|| lastCombatWorkerRoleChange == Broodwar->getFrameCount()
                || unit.getBuildType() != None
                || unit.getGoal().isValid()
                || unit.getBuildPosition().isValid())
                return;

            // Only proactively pull the closest worker to our defend position
            auto closestWorker = Util::getClosestUnit(Terrain::getDefendPosition(), PlayerState::Self, [&](auto &u) {
                return u->getRole() == Role::Worker && !u->getGoal().isValid() && (!u->hasResource() || !u->getResource().lock()->getType().isRefinery()) && !unit.getBuildPosition().isValid();
            });

            auto combatCount = Roles::getMyRoleCount(Role::Combat) - (unit.getRole() == Role::Combat ? 1 : 0);
            auto combatWorkersCount =  Roles::getMyRoleCount(Role::Combat) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Zerg_Zergling) - (unit.getRole() == Role::Combat ? 1 : 0);

            const auto healthyWorker = [&] {

                // Don't pull low shield probes
                if (unit.getType() == Protoss_Probe && unit.getShields() <= 4)
                    return false;

                // Don't pull low health drones
                if (unit.getType() == Zerg_Drone && unit.getHealth() < 20)
                    return false;
                return true;
            };

            // Proactive pulls will result in the worker defending
            const auto proactivePullWorker = [&]() {

                // If this isn't the closest mineral worker to the defend position, don't pull it
                if (unit.getRole() == Role::Worker && closestWorker && unit != *closestWorker)
                    return false;

                // Protoss
                if (Broodwar->self()->getRace() == Races::Protoss) {
                    int completedDefenders = com(Protoss_Photon_Cannon) + com(Protoss_Zealot);
                    int visibleDefenders = vis(Protoss_Photon_Cannon) + vis(Protoss_Zealot);

                    // If trying to hide tech, pull 1 probe with a Zealot
                    if (!BuildOrder::isRush() && BuildOrder::isHideTech() && combatCount < 2 && completedDefenders > 0)
                        return true;

                    // If trying to FFE, pull based on Cannon/Zealot numbers, or lack of scouting information
                    if (BuildOrder::getCurrentBuild() == "FFE") {
                        if (Spy::enemyRush() && Spy::getEnemyOpener() == "4Pool" && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 1)
                            return true;
                        if (Spy::enemyPressure() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 2)
                            return true;
                        if (Spy::enemyRush() && Spy::getEnemyOpener() == "9Pool" && Util::getTime() > Time(3, 15) && completedDefenders < 3)
                            return combatWorkersCount < 3;
                        if (!Terrain::getEnemyStartingPosition().isValid() && Spy::getEnemyBuild() == "Unknown" && combatCount < 2 && completedDefenders < 1 && visibleDefenders > 0)
                            return true;
                    }

                    // If trying to 2Gate at our natural, pull based on Zealot numbers
                    else if (BuildOrder::getCurrentBuild() == "2Gate" && BuildOrder::getCurrentOpener() == "Natural") {
                        if (Spy::enemyRush() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 1)
                            return true;
                        if (Spy::enemyPressure() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 2)
                            return true;
                    }

                    // If trying to 1GateCore and scouted 2Gate late, pull workers to block choke when we are ready
                    else if (BuildOrder::getCurrentBuild() == "1GateCore" && Spy::getEnemyBuild() == "2Gate" && BuildOrder::getCurrentTransition() != "Defensive" && Combat::defendChoke()) {
                        if (Util::getTime() < Time(3, 30) && combatWorkersCount < 2)
                            return true;
                    }
                }

                // Terran

                // Zerg
                if (Broodwar->self()->getRace() == Races::Zerg) {
                    if (BuildOrder::getCurrentOpener() == "12Pool" && Spy::getEnemyOpener() == "9Pool" && total(Zerg_Zergling) < 16 && int(Stations::getStations(PlayerState::Self).size()) >= 2)
                        return combatWorkersCount < 3;
                }
                return false;
            };

            // Reactive pulls will cause the worker to attack aggresively
            const auto reactivePullWorker = [&]() {

                auto proxyDangerousBuilding = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u->isProxy() && u->getType().isBuilding() && u->canAttackGround();
                });
                auto proxyBuildingWorker = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u->getType().isWorker() && (u->isThreatening() || (proxyDangerousBuilding && u->getType().isWorker() && u->getPosition().getDistance(proxyDangerousBuilding->getPosition()) < 160.0));
                });

                // HACK: Don't pull workers reactively versus vultures
                if (Players::getVisibleCount(PlayerState::Enemy, Terran_Vulture) > 0)
                    return false;
                if (Spy::getEnemyBuild() == "2Gate" && Spy::enemyProxy())
                    return false;

                // If we have immediate threats
                if (Players::ZvT() && proxyDangerousBuilding && com(Zerg_Zergling) <= 2)
                    return combatWorkersCount < 6;
                if (Players::ZvP() && proxyDangerousBuilding && Spy::getEnemyBuild() == "CannonRush" && com(Zerg_Zergling) <= 2)
                    return combatWorkersCount < (4 * Players::getVisibleCount(PlayerState::Enemy, proxyDangerousBuilding->getType()));
                if (Spy::getWorkersNearUs() > 2 && com(Zerg_Zergling) < Spy::getWorkersNearUs())
                    return Spy::getWorkersNearUs() >= combatWorkersCount - 3;
                if (BuildOrder::getCurrentOpener() == "12Hatch" && Spy::getEnemyOpener() == "8Rax" && com(Zerg_Zergling) < 2)
                    return combatWorkersCount <= com(Zerg_Drone) - 4;

                // If we're trying to make our expanding hatchery and the drone is being harassed
                if (Players::ZvP() && vis(Zerg_Hatchery) == 1 && Util::getTime() < Time(3, 00) && BuildOrder::isOpener() && Units::getImmThreat() > 0.0f && combatCount == 0)
                    return combatWorkersCount < 1;
                if (Players::ZvP() && Util::getTime() < Time(4, 00) && int(Stations::getStations(PlayerState::Self).size()) < 2 && BuildOrder::getBuildQueue()[Zerg_Hatchery] >= 2 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Probe) > 0)
                    return combatWorkersCount < 1;

                if (Players::ZvZ() && Spy::getEnemyOpener() == "4Pool" && Units::getImmThreat() > 0.0f && com(Zerg_Zergling) == 0)
                    return combatWorkersCount < 10;

                // If we suspect a cannon rush is coming
                if (Players::ZvP() && Spy::enemyPossibleProxy() && Util::getTime() < Time(3, 00))
                    return combatWorkersCount < 1;
                return false;
            };

            // Check if workers should fight or work
            if (unit.getType().isWorker()) {
                auto react = reactivePullWorker();
                auto proact = proactivePullWorker();

                // Pull a worker if needed
                if (unit.getRole() == Role::Worker && !unit.unit()->isCarryingMinerals() && !unit.unit()->isCarryingGas() && healthyWorker() && (react || proact)) {
                    unit.setRole(Role::Combat);
                    unit.setBuildingType(None);
                    unit.setBuildPosition(TilePositions::Invalid);
                    //lastCombatWorkerRoleChange = Broodwar->getFrameCount();
                }

                // Return a worker if not needed
                else if (unit.getRole() == Role::Combat && ((!react && !proact) || !healthyWorker())) {
                    unit.setRole(Role::Worker);
                    //lastCombatWorkerRoleChange = Broodwar->getFrameCount();
                }

                // HACK: Check if this was a reactive pull, set worker to always engage
                if (unit.getRole() == Role::Combat) {
                    combatCount--;
                    combatWorkersCount--;
                    react = reactivePullWorker();
                    if (react && !unit.hasTarget()) {
                        unit.setLocalState(LocalState::Attack);
                        unit.setGlobalState(GlobalState::Attack);
                    }
                }
            }
        }

        void updateRole(UnitInfo& unit)
        {
            // Don't assign a role to uncompleted units
            if (!unit.unit()->isCompleted() && !unit.getType().isBuilding() && unit.getType() != Zerg_Egg) {
                unit.setRole(Role::None);
                return;
            }

            // Update default role
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

            // Check if we need to pull or release a worker from combat
            if (unit.getType().isWorker())
                updateCombatWorkerRole(unit);
        }

        void updateSelf()
        {
            myRoles.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                updateRole(unit);
                myRoles[unit.getRole()]++;
            }

        }
    }

    void onFrame()
    {
        updateSelf();
    }

    int getMyRoleCount(Role role) { return myRoles[role]; }
}