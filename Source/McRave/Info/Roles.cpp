#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Roles {

    namespace {
        map<Role, int> myRoles;
        map<Role, int> forcedRoles;
        set<UnitInfo*> oldCombatWorkers;
        set<UnitType> proxyTargeting ={ Protoss_Pylon, Protoss_Photon_Cannon, Terran_Barracks, Terran_Bunker, Zerg_Sunken_Colony };

        void forceCombatWorker(int count, Position here, LocalState lState = LocalState::Attack, GlobalState gState = GlobalState::Attack)
        {
            auto needed = count - forcedRoles[Role::Combat];
            if (needed <= 0)
                return;

            const auto invalid = [&](auto &unit) {
                return (unit->getType() == Protoss_Probe && unit->getShields() <= 4)
                    || (unit->getType() == Zerg_Drone && unit->getHealth() < (Spy::getEnemyTransition() == "WorkerRush" ? 6 : 20));
            };

            // Only pull the closest worker
            for (int i = 0; i < needed; i++) {
                auto closestWorker = Util::getClosestUnit(here, PlayerState::Self, [&](auto &unit) {
                    if (invalid(unit)) {
                        oldCombatWorkers.erase(&*unit);
                        return false;
                    }

                    if ((unit->getRole() != Role::Worker)
                        || (!oldCombatWorkers.empty() && oldCombatWorkers.find(&*unit) == oldCombatWorkers.end()))
                        return false;
                    return true;
                });

                if (closestWorker) {
                    closestWorker->circle(Colors::Purple);
                    closestWorker->setRole(Role::Combat);
                    closestWorker->setLocalState(lState);
                    closestWorker->setGlobalState(gState);
                    closestWorker->setDestination(here);

                    forcedRoles[Role::Combat]++;

                    if (closestWorker->hasResource())
                        Workers::removeUnit(*closestWorker);
                    if (oldCombatWorkers.find(closestWorker) != oldCombatWorkers.end())
                        oldCombatWorkers.erase(closestWorker);
                }
            }
        }

        void pPullWorker()
        {
            if (Broodwar->self()->getRace() != Races::Protoss)
                return;

            int completedDefenders = com(Protoss_Photon_Cannon) + com(Protoss_Zealot);
            int visibleDefenders = vis(Protoss_Photon_Cannon) + vis(Protoss_Zealot);

            // If trying to FFE, pull based on Cannon/Zealot numbers, or lack of scouting information
            if (BuildOrder::getCurrentBuild() == "FFE") {
                if (Spy::getEnemyOpener() == "4Pool" && visibleDefenders >= 1)
                    forceCombatWorker(8 - 2 * completedDefenders, Position(Terrain::getNaturalChoke()->Center()), LocalState::None, GlobalState::Retreat);
                else if (Spy::enemyRush() && Spy::getEnemyOpener() == "9Pool" && Util::getTime() > Time(3, 15) && completedDefenders < 3)
                    forceCombatWorker(3, Position(Terrain::getNaturalChoke()->Center()), LocalState::None, GlobalState::Retreat);
                else if (!Terrain::getEnemyStartingPosition().isValid() && Spy::getEnemyBuild() == "Unknown" && completedDefenders < 1 && visibleDefenders > 0)
                    forceCombatWorker(1, Position(Terrain::getNaturalChoke()->Center()), LocalState::None, GlobalState::Retreat);
            }
        }

        void tPullWorker()
        {
            if (Broodwar->self()->getRace() != Races::Terran)
                return;

            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.isCompleted()) {
                    auto damaged = unit.getHealth() != unit.getType().maxHitPoints();
                    auto threatened = Units::getImmThreat() > 0.0 || !unit.getUnitsInReachOfThis().empty();

                    if (damaged || threatened) {
                        if (unit.getType() == Terran_Missile_Turret)
                            forceCombatWorker(3, unit.getPosition(), LocalState::Retreat, GlobalState::Retreat);
                        if (unit.getType() == Terran_Bunker)
                            forceCombatWorker(3, unit.getPosition(), LocalState::Retreat, GlobalState::Retreat);
                        if (unit.getType().isMechanical() && damaged && Terrain::inTerritory(PlayerState::Self, unit.getPosition()))
                            forceCombatWorker(1, unit.getPosition(), LocalState::Retreat, GlobalState::Retreat);
                    }
                }
            }
        }

        void zPullWorker()
        {
            if (Broodwar->self()->getRace() != Races::Zerg)
                return;

            auto proxyBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->isProxy() && u->getType().isBuilding() && !u->canAttackGround() && !u->canAttackAir();
            });
            auto proxyDangerousBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->isProxy() && proxyTargeting.find(u->getType()) != proxyTargeting.end() && (u->canAttackGround() || u->canAttackAir() || Terrain::inTerritory(PlayerState::Self, u->getPosition()));
            });
            auto proxyWorker = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->getType().isWorker() && u->isProxy();
            });
            auto proxyBuildingWorker = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->getType().isWorker() &&
                    (u->isThreatening() || (proxyBuilding && u->getPosition().getDistance(proxyBuilding->getPosition()) < 160.0) || (proxyDangerousBuilding && u->getPosition().getDistance(proxyDangerousBuilding->getPosition()) < 160.0));
            });
            auto proxyCombatUnit = Util::getClosestUnit(Position(Terrain::getNaturalChoke()->Center()), PlayerState::Enemy, [&](auto &u) {
                return u->isProxy() && !u->getType().isWorker() && !u->getType().isBuilding() && u->canAttackGround();
            });


            static bool likelyProxy = likelyProxy || (proxyWorker && Util::getTime() < Time(2, 00));

            // ZvZ
            if (Players::ZvZ() && Util::getTime() < Time(6, 00) && !Spy::enemyTurtle()) {
                if ((Spy::getEnemyOpener() == "9Pool" || Spy::getEnemyOpener() == "OverPool") && Combat::isDefendNatural() && Util::getTime() > Time(2, 45) && BuildOrder::getCurrentOpener() == "12Pool" && total(Zerg_Zergling) < 16 && int(Stations::getStations(PlayerState::Self).size()) >= 2)
                    forceCombatWorker(3, Position(Terrain::getNaturalChoke()->Center()), LocalState::None, GlobalState::Retreat);
            }

            // ZvP
            if (Players::ZvP() && Players::getCompleteCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0 && Util::getTime() < Time(6, 00)) {

                // 2Gate or Cannon proxy, 4 drones
                if (proxyDangerousBuilding && Players::getVisibleCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0 && (Spy::getEnemyBuild() == "CannonRush" || Spy::getEnemyBuild() == "2Gate") && com(Zerg_Zergling) <= 6)
                    forceCombatWorker(4, proxyDangerousBuilding->getPosition());

                // Probe actively building proxy, 2 drones
                else if (proxyBuilding && proxyBuildingWorker)
                    forceCombatWorker(2, proxyBuildingWorker->getPosition());

                // Probe arrived early, 1 drone
                else if (proxyWorker && Util::getTime() < Time(3, 00) && vis(Zerg_Spawning_Pool) == 0)
                    forceCombatWorker(1, proxyWorker->getPosition());

                // Proxy building, 1 drone
                else if (proxyBuilding && Spy::getEnemyBuild() == "CannonRush" && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, proxyBuilding->getPosition());

                // Likely proxy, worker arrived way too early
                else if (likelyProxy && Util::getTime() < Time(3, 00))
                    forceCombatWorker(1, proxyWorker->getPosition());

                // We know it's likely a proxy, watch the natural for now
                else if (Spy::enemyPossibleProxy() && Util::getTime() < Time(2, 00)) {
                    if (proxyWorker)
                        forceCombatWorker(1, proxyWorker->getPosition());
                    else
                        forceCombatWorker(1, Position(Terrain::getNaturalChoke()->Center()), LocalState::Retreat, GlobalState::Retreat);
                }
            }

            // ZvT
            if (Players::ZvT() && Util::getTime() < Time(3, 30)) {
                auto count = (Players::getCompleteCount(PlayerState::Enemy, Terran_Marine) * 3);
                if (proxyDangerousBuilding)
                    count += 3;

                // Choose the location we expect to be fighting at to get closest worker, descending priority
                auto location = Position(Terrain::getNaturalChoke()->Center());
                if (proxyCombatUnit)
                    location = proxyCombatUnit->getPosition();
                else if (proxyDangerousBuilding)
                    location = proxyDangerousBuilding->getPosition();
                else if (proxyWorker)
                    location = proxyWorker->getPosition();

                // Bunker being built, 3 drones per marine and 3 extra for the bunker
                if (proxyDangerousBuilding && !proxyDangerousBuilding->isCompleted() && com(Zerg_Zergling) <= 2 && total(Zerg_Zergling) <= 8)
                    forceCombatWorker(count, location);

                // Proxy, 3 drones per marine
                else if (proxyCombatUnit && com(Zerg_Zergling) <= 2 && total(Zerg_Zergling) <= 8)
                    forceCombatWorker(count, location);

                // All-in with marines
                else if (Spy::getEnemyTransition() == "WorkerRush" && total(Zerg_Zergling) < 12 && Players::getCompleteCount(PlayerState::Enemy, Terran_Marine) > 0)
                    forceCombatWorker(count, location);

                // Likely proxy, worker arrived way too early
                else if (likelyProxy && Util::getTime() < Time(3, 00))
                    forceCombatWorker(1, location);

                // We know it's likely a proxy, watch the natural for now
                else if (Spy::enemyPossibleProxy() && Util::getTime() < Time(2, 00))
                    forceCombatWorker(1, Position(Terrain::getNaturalChoke()->Center()), LocalState::Retreat, GlobalState::Retreat);

                // Some SCV poking
                else if (proxyWorker && com(Zerg_Spawning_Pool) == 0)
                    forceCombatWorker(1, proxyWorker->getPosition());
            }

            // Misc
            //if (Spy::getWorkersNearUs() > 2 && com(Zerg_Zergling) < Spy::getWorkersNearUs() && Util::getTime() < Time(6, 00) && Units::getImmThreat() > 0.0f)
            //    forceCombatWorker(Spy::getWorkersNearUs() + 2, Position(Terrain::getMainChoke()->Center()), LocalState::Attack, GlobalState::Attack);
        }

        void updateForcedRoles()
        {
            forcedRoles.clear();
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
                    else if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Protoss_Arbiter || unit.getType() == Terran_Comsat_Station)
                        unit.setRole(Role::Support);
                    else if ((unit.getType().isBuilding() && !unit.canAttackGround() && !unit.canAttackAir() && unit.getType() != Zerg_Creep_Colony) || unit.getType() == Zerg_Larva || unit.getType() == Zerg_Egg)
                        unit.setRole(Role::Production);
                    else if (unit.getType().isBuilding() && (unit.canAttackGround() || unit.canAttackAir() || unit.getType() == Zerg_Creep_Colony))
                        unit.setRole(Role::Defender);
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

                // Check if a worker was forced to fight
                if (unit.getType().isWorker() && unit.getRole() == Role::Combat) {
                    unit.setRole(Role::Worker);
                    oldCombatWorkers.insert(&unit);
                }

                // Check for desired floating buildings
                if (unit.getType() == Terran_Engineering_Bay) {
                    if (!Players::TvZ() || (Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Terran_Infantry_Armor, 3) && Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Terran_Infantry_Weapons, 3)))
                        unit.setRole(Role::Support);
                }
                if (unit.getType() == Terran_Science_Facility) {
                    if (!Players::TvZ() || Players::hasResearched(PlayerState::Self, TechTypes::Irradiate))
                        unit.setRole(Role::Support);
                }
                if (unit.getType() == Terran_Barracks) {
                    if (!Players::TvZ() && total(Terran_Marine) >= 4)
                        unit.setRole(Role::Support);
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