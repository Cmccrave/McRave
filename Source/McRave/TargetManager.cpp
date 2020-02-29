#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave::Targets {

    namespace {

        void getBestTarget(UnitInfo& unit, PlayerState pState)
        {
            double distBest = DBL_MAX;
            double scoreBest = 0.0;

            auto oldTarget = unit.hasTarget() ? unit.getTarget().shared_from_this() : nullptr;

            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myStrength = Players::getStrength(PlayerState::Self);

            const auto isSuitable = [&](UnitInfo& target) {

                bool targetCanAttack = !unit.isHidden() && (((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine)));
                bool unitCanAttack = !target.isHidden() && ((target.isFlying() && unit.getAirDamage() > 0.0) || (!target.isFlying() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));
                bool enemyHasGround = enemyStrength.groundToAir > 0.0 || enemyStrength.groundToGround > 0.0;
                bool enemyHasAir = enemyStrength.airToGround > 0.0 || enemyStrength.airToAir > 0.0;
                bool selfHasGround = myStrength.groundToAir > 0.0 || myStrength.groundToGround > 0.0;
                bool selfHasAir = myStrength.airToGround > 0.0 || myStrength.airToAir > 0.0 || com(Protoss_Shuttle) > 0;

                bool atHome = Terrain::isInAllyTerritory(target.getTilePosition());
                bool atEnemy = Terrain::isInEnemyTerritory(target.getTilePosition());

                // Check if the target is important right now to attack
                bool targetMatters = target.getType().isSpell()
                    || (target.canAttackAir() && selfHasAir)
                    || (target.canAttackGround())
                    || (target.getType().isDetector() && (vis(Protoss_Dark_Templar) > 0 || vis(Protoss_Observer) > 0))
                    || (!target.canAttackAir() && !target.canAttackGround() && !unit.hasTransport())
                    || (target.getType().isWorker())
                    || (!enemyHasGround && !enemyHasAir);

                if (unit.getRole() == Role::Combat) {
                    if ((unit.getType() == Protoss_Zealot && target.getType() == Terran_Vulture_Spider_Mine && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)                 // Zealot: Don't target mines without +2
                        || (unit.getType() == Protoss_Zealot && (target.getSpeed() > unit.getSpeed() || target.getType().isBuilding()) && !target.getType().isWorker() && BuildOrder::isRush())             // Zealot: Don't target faster units or buildings when we are rushing
                        || ((unit.getType() == Protoss_Dark_Templar || unit.isLightAir()) && !target.isThreatening() && target.getType() == Terran_Vulture && !unit.isWithinRange(target))                  // DT/Air: Don't target Vultures not in range
                        || !unitCanAttack || !targetMatters                                                                                                                                                 // Don't target units that don't matter or unit can't attack it
                        || (target.getType() == Terran_Vulture_Spider_Mine && int(target.getTargetedBy().size()) >= 4 && !target.isBurrowed())                                                              // Don't attack enemy spider mines with more than 4 units
                        || (target.getType() == Protoss_Interceptor && unit.isFlying())                                                                                                                     // Don't target interceptors as a flying unit
                        || (target.unit()->exists() && target.unit()->isStasised())                                                                                                                         // Don't target stasis units
                        || (target.getType().isWorker() && unit.getGroundRange() <= 32.0 && !target.isThreatening() /*&& !atHome*/ && !atEnemy)                                                                 // Don't target non threatening workers in our territory
                        || (target.isHidden() && (!targetCanAttack || (!Players::hasDetection(PlayerState::Self) && Players::PvP())) && !unit.getType().isDetector())                                       // Don't target if invisible and can't attack this unit or we have no detectors in PvP
                        || (target.isFlying() && !unit.isFlying() && !target.unit()->exists() && Grids::getMobility(target.getPosition()) < 1)                                                              // Don't target flyers that we can't reach
                        )
                        return false;
                }
                if (unit.getRole() == Role::Scout) {
                    if (target.getType().isBuilding() && Terrain::isInEnemyTerritory(target.getTilePosition()))
                        return false;
                }
                if (unit.getRole() == Role::Support) {

                }
                return true;
            };

            const auto checkClosest = [&](UnitInfo& target) {
                if (!target.getPlayer()->isEnemy(Broodwar->self()))
                    return;

                bool targetCanAttack = ((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine));
                bool unitCanAttack = ((target.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!target.getType().isFlyer() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));

                // HACK: Check for a flying building
                if (target.unit()->exists() && target.unit()->isFlying() && unit.getAirDamage() <= 0.0)
                    unitCanAttack = false;

                // Set sim position
                double dist = unit.getPosition().getDistance(target.getPosition());
                if ((unitCanAttack || targetCanAttack) && dist < distBest) {
                    unit.setSimPosition(target.getPosition());
                    distBest = dist;
                }
            };

            const auto isValid = [&](UnitInfo &target) {
                if (!target.unit()
                    || target.unit()->isInvincible()
                    || !target.getWalkPosition().isValid()
                    || !unit.getWalkPosition().isValid()
                    || (target.getType().isBuilding() && !target.canAttackGround() && Terrain::isInAllyTerritory(target.getTilePosition()) && Broodwar->getFrameCount() < 10000 && !target.isThreatening()))
                    return false;
                return true;
            };

            const auto checkBest = [&](UnitInfo& target) {

                if (!isSuitable(target))
                    return;

                // Scoring parameters
                const auto range = target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
                const auto widths = unit.getType().tileWidth() * 16.0 + target.getType().tileWidth() * 16.0;
                const auto healthRatio = (1.0 - target.getPercentTotal());
                const auto health = (range > 32.0 || unit.isWithinRange(target)) ? 1.0 + (0.5*healthRatio) : 1.0;
                const auto dist = exp(0.0125 * max(unit.getPosition().getDistance(target.getPosition()), range));
                const auto clusterTarget = unit.getType() == Protoss_High_Templar || unit.getType() == Protoss_Arbiter;
                auto priority = target.getPriority() * health * (target.isThreatening() ? log(32.0 + target.getPosition().getDistance(Terrain::getDefendPosition())) : 1.0);

                auto thisUnit = 0.0;

                // Detector targeting
                if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Terran_Comsat_Station) {

                    if (target.isBurrowed() || target.unit()->isCloaked()) {

                        // Photons should focus Observers if possible
                        if (unit.getType() == Protoss_Photon_Cannon && target.getType() == Protoss_Observer)
                            thisUnit = 1000.0 / dist;

                        else {

                            // See if I have a unit close to the enemy
                            auto closest = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                                return u != unit && u.getRole() == Role::Combat && target.getType().isFlyer() ? u.getAirRange() > 0 : u.getGroundRange() > 0;
                            });

                            // Detectors want to stay close to their target
                            if (closest && closest->getPosition().getDistance(target.getPosition()) < 480.0)
                                thisUnit = priority / (1.0 + (dist * closest->getPosition().getDistance(target.getPosition())));
                            else
                                thisUnit = -1.0;
                        }
                    }
                }

                // DT want to kill any workers they're in reach of
                else if ((unit.getType() == Protoss_Dark_Templar || (BuildOrder::isRush() && unit.getGroundRange() <= 32)) && (target.getType().isWorker() || target.isSiegeTank() || target.getType().isDetector()) && unit.isWithinReach(target) && !Actions::overlapsDetection(target.unit(), target.getPosition(), PlayerState::Enemy))
                    thisUnit = 10000.0 / dist;

                else if (unit.getType() == Protoss_Dark_Templar && (target.getType() == Protoss_Observatory || target.getType() == Protoss_Robotics_Facility))
                    thisUnit = 1000.0 / dist;

                // Cluster targeting for AoE units
                else if (clusterTarget) {
                    if (!target.getType().isBuilding() && target.getType() != Terran_Vulture_Spider_Mine) {

                        double eGrid = Grids::getEGroundCluster(target.getWalkPosition()) + Grids::getEAirCluster(target.getWalkPosition());
                        double aGrid = 1.0 + Grids::getAGroundCluster(target.getWalkPosition()) + Grids::getAAirCluster(target.getWalkPosition());
                        double score = eGrid / aGrid;

                        thisUnit = priority * score / dist;
                    }
                }

                // Proximity targeting
                else if (unit.getType() == Protoss_Reaver) {
                    if (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir())
                        thisUnit = 0.1 / dist;
                    else
                        thisUnit = 1.0 / dist;
                }

                // Priority targeting
                else if (unit.isThreatening()) {
                    auto closestStation = Stations::getClosestStation(PlayerState::Self, target.getPosition());
                    thisUnit = closestStation.isValid() ? priority / (dist * closestStation.getDistance(target.getPosition())) : thisUnit = priority / dist;
                }

                else if (unit.isLightAir()) {
                    auto targeted = exp(int(target.getTargetedBy().size()) + 1);
                    thisUnit = targeted * priority / dist;
                }
                else
                    thisUnit = priority / dist;

                // If this target is more important to target, set as current target
                if (thisUnit > scoreBest) {
                    scoreBest = thisUnit;
                    unit.setTarget(&target);
                }
            };

            for (auto &t : Units::getUnits(pState)) {
                UnitInfo &target = *t;

                if (!isValid(target))
                    continue;

                checkClosest(target);
                checkBest(target);
            }

            // If unit is close, add this unit to the targeted by vector
            if (unit.hasTarget() && unit.isWithinRange(unit.getTarget()))
                unit.getTarget().getTargetedBy().push_back(make_shared<UnitInfo>(unit));

            if (unit.hasTarget() && oldTarget != unit.getTarget().shared_from_this() && !unit.isWithinRange(unit.getTarget()) && !BuildOrder::isRush())
                unit.setLocalState(LocalState::Retreat);
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngagePosition(Positions::None);
                return;
            }

            auto range = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
            if (!unit.getPath().getTiles().empty() && (!unit.isWithinRange(unit.getTarget()) || unit.unit()->isLoaded())) {
                auto engagePosition = Util::findPointOnPath(unit.getPath(), [&](Position p) {
                    auto center = p + Position(16, 16);
                    return unit.getType() == Protoss_Reaver ? BWEB::Map::getGroundDistance(center, unit.getTarget().getPosition()) <= range : center.getDistance(unit.getTarget().getPosition()) <= range;
                }) + Position(16, 16);

                if (engagePosition.isValid()) {
                    unit.setEngagePosition(engagePosition);
                    return;
                }
            }

            auto distance = unit.getPosition().getApproxDistance(unit.getTarget().getPosition());
            auto direction = (unit.getPosition() - unit.getTarget().getPosition()) * (distance - int(range)) / distance;

            // If unit is loaded or further than their range, we want to calculate the expected engage position
            if (distance > range || unit.unit()->isLoaded())
                unit.setEngagePosition(unit.getPosition() - direction);
            else
                unit.setEngagePosition(unit.getPosition());
        }

        void getEngageDistance(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngDist(0.0);
                return;
            }

            if (!unit.getPath().isReachable() && !unit.getTarget().getType().isBuilding() && !unit.getType().isFlyer() && !unit.getTarget().getType().isFlyer() && Grids::getMobility(unit.getTarget().getPosition()) >= 4) {
                if (unit.hasBackupTarget()) {
                    unit.setTarget(&unit.getBackupTarget());
                    unit.setEngDist(unit.getPosition().getDistance(unit.getBackupTarget().getPosition()));
                }
                else
                    unit.setEngDist(DBL_MAX);
                return;
            }

            auto dist = unit.getPosition().getDistance(unit.getEngagePosition());
            unit.setEngDist(dist);
        }

        void getPathToTarget(UnitInfo& unit)
        {
            // If unnecessary to get path
            if (unit.getRole() != Role::Combat && unit.getRole() != Role::Scout)
                return;

            // If no target, no distance/path available
            if (!unit.hasTarget()) {
                BWEB::Path newPath;
                unit.setEngDist(0.0);
                unit.setPath(newPath);
                return;
            }

            if (unit.getTarget().getType().isBuilding() || unit.getType().isFlyer() || unit.getTarget().getType().isFlyer() || unit.getTilePosition() == unit.getTarget().getTilePosition()) {
                BWEB::Path newPath;
                BWEM::CPPath newQuickPath;
                unit.setPath(newPath);
                unit.setQuickPath(newQuickPath);
                return;
            }

            // If should create path, grab one from BWEB
            if (unit.canCreatePath(unit.getTarget().getPosition())) {
                BWEB::Path newPath;
                newPath.createUnitPath(unit.getPosition(), unit.getTarget().getPosition());
                unit.setPath(newPath);
                unit.setQuickPath(mapBWEM.GetPath(unit.getPosition(), unit.getTarget().getPosition()));
            }
        }
    }

    void getBackupTarget(UnitInfo& unit)
    {
        // Originally used to kill neutral buildings
        /*if (unit.getRole() == Role::Combat && !unit.getType().isFlyer()) {
            if (!unit.hasTarget() || Terrain::isIslandMap()) {
                getBestTarget(unit, PlayerState::Neutral);
                getPathToTarget(unit);
                getEngagePosition(unit);
                getEngageDistance(unit);
            }
        }*/
    }

    void getTarget(UnitInfo& unit)
    {
        getBestTarget(unit, PlayerState::Enemy);
        getPathToTarget(unit);
        getEngagePosition(unit);
        getEngageDistance(unit);
        getBackupTarget(unit);
    }
}