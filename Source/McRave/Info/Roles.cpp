#include "Roles.h"

#include "Builds/All/BuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Micro/Worker/Workers.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Roles {

    namespace {
        map<Role, int> myRoles;
        map<Role, int> forcedRoles;
        set<UnitType> proxyTargeting = {Protoss_Pylon, Protoss_Photon_Cannon, Terran_Barracks, Terran_Bunker, Terran_Factory, Zerg_Sunken_Colony};
        int lastCombatWorkerCount    = 0;

        void forceCombatWorker(int count, Position here, LocalState lState = LocalState::Attack, GlobalState gState = GlobalState::Attack)
        {
            auto needed = count - forcedRoles[Role::Combat];

            // Update where
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.getType().isWorker() && unit.getRole() == Role::Combat)
                    unit.setDestination(here);
            }

            if (needed <= 0)
                return;

            const auto invalid = [&](auto &unit) {
                if (unit->getBuildPosition().isValid()) {
                    return true;
                }

                auto health = 26;
                if (Players::ZvZ())
                    health = 31;
                if (Players::ZvP()) {
                    health = 11;
                    if (Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) > 0)
                        health = 22;
                }
                if (Players::ZvT()) {
                    health = 16;
                    if (Players::getTotalCount(PlayerState::Enemy, Terran_Marine) > 0)
                        health = 22;
                }

                return (unit->getType() == Protoss_Probe && unit->getShields() <= 4) || (unit->getType() == Zerg_Drone && unit->getHealth() < health);
            };

            // Only pull the closest worker
            for (int i = 0; i < needed; i++) {
                auto closestForcedWorker = Util::getClosestUnitGround(here, PlayerState::Self,
                                                                      [&](auto &unit) { return !invalid(unit) && unit->getRole() == Role::Worker && unit->getLastRole() == Role::Combat; });

                auto closestWorker = closestForcedWorker ? closestForcedWorker
                                                         : Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &unit) { return !invalid(unit) && unit->getRole() == Role::Worker; });

                if (closestWorker) {
                    closestWorker->circle(Colors::Purple);
                    closestWorker->setRole(Role::Combat);
                    closestWorker->setLocalState(lState);
                    closestWorker->setGlobalState(gState);
                    closestWorker->setDestination(here);

                    forcedRoles[Role::Combat]++;

                    if (closestWorker->hasResource())
                        Workers::removeUnit(*closestWorker);
                }
            }
        }

        void pPullWorker()
        {
            if (Broodwar->self()->getRace() != Races::Protoss)
                return;

            int completedDefenders = com(Protoss_Photon_Cannon) + com(Protoss_Zealot);
            int visibleDefenders   = vis(Protoss_Photon_Cannon) + vis(Protoss_Zealot);

            auto cannon = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &c) { return c->getType() == Protoss_Photon_Cannon; });

            // If trying to FFE, pull based on Cannon/Zealot numbers, or lack of scouting information
            if (BuildOrder::getCurrentBuild() == P_FFE) {
                if (Spy::getEnemyOpener() == Z_4Pool && visibleDefenders >= 1) {
                    auto pos = cannon ? cannon->getPosition() : Position(Terrain::getNaturalChoke()->Center());
                    forceCombatWorker(8 - 2 * completedDefenders, pos, LocalState::None, GlobalState::Retreat);
                }
                else if (Spy::enemyRush() && Spy::getEnemyOpener() == Z_9Pool && Util::getTime() > Time(3, 15) && completedDefenders < 3)
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
                    auto damaged    = unit.getHealth() != unit.getType().maxHitPoints();
                    auto threatened = Units::enemyThreatening() || !unit.getUnitsInReachOfThis().empty();

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
            if (Broodwar->self()->getRace() != Races::Zerg || BuildOrder::isRush())
                return;

            auto proxyBuilding          = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy,
                                                      [&](auto &u) { return u->isProxy() && u->getType().isBuilding() && !u->canAttackGround() && !u->canAttackAir(); });
            auto proxyDangerousBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->isProxy() && proxyTargeting.find(u->getType()) != proxyTargeting.end() &&
                       (u->canAttackGround() || u->canAttackAir() || Terrain::inTerritory(PlayerState::Self, u->getPosition()));
            });
            auto proxyWorker            = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) { return u->getType().isWorker() && u->isProxy(); });
            auto proxyBuildingWorker    = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->getType().isWorker() && (u->isThreatening() || (proxyBuilding && u->getPosition().getDistance(proxyBuilding->getPosition()) < 160.0) ||
                                                   (proxyDangerousBuilding && u->getPosition().getDistance(proxyDangerousBuilding->getPosition()) < 160.0));
            });
            auto proxyCombatUnit        = Util::getClosestUnit(Position(Terrain::getNaturalChoke()->Center()), PlayerState::Enemy,
                                                        [&](auto &u) { return u->isProxy() && !u->getType().isWorker() && !u->getType().isBuilding() && u->canAttackGround(); });
            auto selfBuildingWorker     = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self,
                                                           [&](auto &u) { return u->getType().isWorker() && u->getBuildType() == Zerg_Hatchery && Broodwar->self()->minerals() >= 200; });

            auto proxyCombatWorker   = proxyWorker && proxyWorker->hasAttackedRecently();
            auto unknownMainLocation = Position(Terrain::getOldestPosition(Terrain::getMainArea()));

            static bool sixLings = false;
            if (com(Zerg_Zergling) >= 6)
                sixLings = true;

            // Worker rush - pull 3 unless they all in
            if (Spy::getEnemyTransition() == U_WorkerRush) {
                if (Players::getTotalCount(PlayerState::Enemy, Terran_Marine) > 0)
                    return;

                if (Players::ZvP() && sixLings && com(Zerg_Sunken_Colony) == 0 && Combat::isDefendNatural() && proxyWorker) {
                    auto cnt = 3 + Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot);
                    forceCombatWorker(5, Position(Terrain::getNaturalPosition()), LocalState::None, GlobalState::Retreat);
                }
                if (Players::ZvT() && com(Zerg_Sunken_Colony) == 0 && proxyCombatWorker) {
                    forceCombatWorker(Spy::getWorkersPulled() + 1, Terrain::getMainPosition(), LocalState::Attack, GlobalState::Retreat);
                }
                return;
            }

            // ZvZ
            if (Players::ZvZ() && Util::getTime() < Time(6, 00) && !Spy::enemyTurtle() && !Spy::enemyFastExpand() && Combat::State::isStaticRetreat(Zerg_Zergling)) {

                // auto lState = Units::getImmThreat() > 0.0f ? LocalState::Attack : LocalState::None;

                if (Combat::isDefendNatural()) {
                    if ((Spy::getEnemyOpener() == Z_9Pool || Spy::getEnemyOpener() == Z_Overpool || Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > total(Zerg_Zergling)) &&
                        Util::getTime() > Time(2, 45) && Util::getTime() < Time(3, 30) && BuildOrder::getCurrentOpener() == Z_12Pool && total(Zerg_Zergling) < 20 &&
                        int(Stations::getStations(PlayerState::Self).size()) >= 2)
                        forceCombatWorker(2, Terrain::getNaturalPosition(), LocalState::None, GlobalState::Retreat);
                }
            }

            // ZvP
            if (Players::ZvP() && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) == 0 && Players::getCompleteCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0 &&
                Util::getTime() < Time(6, 00)) {

                if (Spy::getEnemyOpener() == P_Proxy_9_9 || Spy::getEnemyOpener() == P_Horror_9_9)
                    return;

                // Gateway or Cannon in territory, 6 drones
                if (proxyDangerousBuilding && Players::getVisibleCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0 && Spy::getEnemyBuild() == P_CannonRush && com(Zerg_Zergling) <= 6)
                    forceCombatWorker(6, proxyDangerousBuilding->getPosition());
                else if (proxyDangerousBuilding && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) > 0 && Spy::getEnemyBuild() == P_2Gate)
                    forceCombatWorker(6, proxyDangerousBuilding->getPosition());

                // Pylon in main or natural, 3 drones
                else if (proxyBuilding && com(Zerg_Zergling) <= 2 &&
                         (Terrain::inArea(Terrain::getMainArea(), proxyBuilding->getPosition()) || Terrain::inArea(Terrain::getMainArea(), proxyBuilding->getPosition())))
                    forceCombatWorker(3, proxyBuilding->getPosition());

                // Probe actively building dangerous proxy, 2 drones
                else if (proxyBuilding && proxyBuildingWorker && proxyDangerousBuilding)
                    forceCombatWorker(2, proxyBuildingWorker->getPosition());

                // Proxy building, 1 drone
                else if (proxyBuilding && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, proxyBuilding->getPosition());

                // Proxy worker, 1 drone
                else if (Spy::enemyPossibleProxy() && proxyWorker && proxyWorker->unit()->exists() && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, proxyWorker->getPosition());
                else if (Spy::enemyPossibleProxy() && proxyWorker && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, unknownMainLocation);
                else if (Spy::enemyPossibleProxy() && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, Position(Terrain::getNaturalChoke()->Center()), LocalState::Retreat, GlobalState::Retreat);

                else if (Spy::getEnemyBuild() == P_CannonRush && com(Zerg_Zergling) <= 2)
                    forceCombatWorker(1, Position(Terrain::getNaturalChoke()->Center()), LocalState::Retreat, GlobalState::Retreat);

                // We haven't got our hatchery down yet
                else if (vis(Zerg_Hatchery) < 2 && BuildOrder::takeNatural() && proxyWorker && proxyWorker->unit()->exists() && selfBuildingWorker &&
                         (Terrain::inArea(Terrain::getNaturalArea(), proxyWorker->getPosition()) || proxyWorker->hasAttackedRecently() || proxyWorker->isThreatening()))
                    forceCombatWorker(1, proxyWorker->getPosition());
            }

            // ZvT
            if (Players::ZvT() && Util::getTime() < Time(3, 30)) {
                auto count = (Players::getTotalCount(PlayerState::Enemy, Terran_Marine) * 2);
                if (proxyDangerousBuilding)
                    count += 3;

                auto closestMarine = Util::getClosestUnit(Position(Terrain::getNaturalChoke()->Center()), PlayerState::Enemy, [&](auto &u) { return u->getType() = Terran_Marine; });

                static bool marineEngaged = false;
                if (closestMarine && Terrain::inTerritory(PlayerState::Self, closestMarine->getPosition()))
                    marineEngaged = true;

                // Choose the location we expect to be fighting at to get closest worker, descending priority
                auto location = Position(Terrain::getNaturalChoke()->Center());
                if (closestMarine)
                    location = closestMarine->getPosition();
                else if (proxyDangerousBuilding)
                    location = proxyDangerousBuilding->getPosition();
                else if (proxyWorker)
                    location = proxyWorker->getPosition();

                if (BuildOrder::takeNatural()) {

                    // Bunker being built, 3 drones per marine and 3 extra for the bunker
                    if (proxyDangerousBuilding && !proxyDangerousBuilding->isCompleted() && com(Zerg_Zergling) <= 2 && total(Zerg_Zergling) <= 8) {
                        LOG_ONCE("Proxy bunker, pull drones");
                        forceCombatWorker(count, location);
                    }

                    // Proxy, 3 drones per marine
                    else if (marineEngaged && com(Zerg_Zergling) <= 2 && total(Zerg_Zergling) <= 8) {
                        LOG_ONCE("Marine arrived before lings, pull drones");
                        forceCombatWorker(count, location);
                    }

                    // Suspiciously early and they took gas early
                    else if (Spy::enemyPossibleProxy() && Spy::getEnemyBuild() == T_RaxFact && proxyWorker && com(Zerg_Zergling) <= 2) {
                        LOG_ONCE("Possible hidden factory");
                        forceCombatWorker(1, unknownMainLocation, LocalState::Attack, GlobalState::Attack);
                    }

                    // We haven't got our hatchery down yet
                    else if (hatchCount() < 2 && proxyWorker && selfBuildingWorker &&
                             (Terrain::inArea(Terrain::getNaturalArea(), proxyWorker->getPosition()) || proxyWorker->hasAttackedRecently() || proxyWorker->isThreatening())) {
                        LOG_ONCE("Help get natural hatchery get built");
                        forceCombatWorker(1, proxyWorker->getPosition());
                    }
                }
            }
        }

        void updateForcedRoles()
        {
            forcedRoles.clear();
            pPullWorker();
            tPullWorker();
            zPullWorker();

            if (forcedRoles[Role::Combat] != lastCombatWorkerCount) {
                lastCombatWorkerCount = forcedRoles[Role::Combat];
                LOG("forcing ", lastCombatWorkerCount, " combat workers");
            }
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
                    else if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Protoss_Arbiter || unit.getType() == Zerg_Queen ||
                             unit.getType() == Terran_Comsat_Station)
                        unit.setRole(Role::Support);
                    else if ((unit.getType().isBuilding() && !unit.canAttackGround() && !unit.canAttackAir() && unit.getType() != Zerg_Creep_Colony) || unit.getType() == Zerg_Larva ||
                             unit.getType() == Zerg_Egg)
                        unit.setRole(Role::Production);
                    else if (unit.getType().spaceProvided() > 0)
                        unit.setRole(Role::Transport);
                    else if (unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Protoss_Scarab || unit.getType().isSpell() || unit.getType() == Terran_Nuclear_Missile)
                        unit.setRole(Role::Consumable);
                    else if (unit.getType().isBuilding() && (unit.canAttackAir() || unit.canAttackGround()))
                        unit.setRole(Role::Defender);
                    else
                        unit.setRole(Role::Combat);
                }

                // Check if a worker morphed into a building
                if (unit.getRole() == Role::Worker && unit.getType().isBuilding()) {
                    if (unit.getType().isBuilding() && !unit.canAttackGround() && !unit.canAttackAir())
                        unit.setRole(Role::Production);
                    else
                        unit.setRole(Role::Defender);
                }

                // Check if a production building morphed into a defender
                if (unit.getRole() == Role::Production && (unit.canAttackGround() || unit.canAttackGround()) && unit.isCompleted())
                    unit.setRole(Role::Defender);

                // Check if a worker was forced to fight
                if (unit.getType().isWorker() && unit.getRole() == Role::Combat) {
                    unit.setRole(Role::Worker);
                }

                // Check for desired floating buildings
                if (unit.getType() == Terran_Engineering_Bay) {
                    if (!Players::TvZ() ||
                        (Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Terran_Infantry_Armor, 3) && Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Terran_Infantry_Weapons, 3)))
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
    } // namespace

    void onFrame() { updateSelf(); }

    int getMyRoleCount(Role role) { return myRoles[role]; }
} // namespace McRave::Roles