#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave::Targets {

    namespace {

        void getBestTarget(UnitInfo& unit, PlayerState pState)
        {
            map<UnitType, int> targetedByType;
            double distBest = DBL_MAX;
            double scoreBest = 0.0;
            auto oldTarget = unit.hasTarget() ? unit.getTarget().shared_from_this() : nullptr;
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myStrength = Players::getStrength(PlayerState::Self);

            // Checks if this target is valid for targeting this frame
            const auto isValid = [&](UnitInfo &target) {
                if (!target.unit()
                    || target.unit()->isInvincible()
                    || !target.getWalkPosition().isValid()
                    || (target.getType().isBuilding() && !target.canAttackGround() && Terrain::isInAllyTerritory(target.getTilePosition()) && Broodwar->getFrameCount() < 10000 && !target.isThreatening())) // Don't attack proxy structures that can't hit us (cannon rush)
                    return false;
                return true;
            };

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
                    || (!enemyHasGround && !enemyHasAir)
                    || Util::getTime() > Time(10, 0);

                if (unit.getRole() == Role::Combat) {
                    if ((unit.getType() == Protoss_Zealot && target.getType() == Terran_Vulture_Spider_Mine && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)                 // Zealot: Don't target mines without +2
                        || (unit.getType() == Protoss_Zealot && (target.getSpeed() > unit.getSpeed() || target.getType().isBuilding()) && !target.getType().isWorker() && BuildOrder::isRush())             // Zealot: Don't target faster units or buildings when we are rushing
                        || (unit.getType() == Protoss_Dark_Templar && target.getType() == Terran_Vulture && !unit.isWithinRange(target))                                                                    // DT: Don't target Vultures not in range
                        || !unitCanAttack || !targetMatters                                                                                                                                                 // Don't target units that don't matter or unit can't attack it
                        || (target.getType() == Terran_Vulture_Spider_Mine && int(target.getTargetedBy().size()) >= 4 && !target.isBurrowed())                                                              // Don't attack enemy spider mines with more than 4 units
                        || (target.getType() == Protoss_Interceptor && unit.isFlying())                                                                                                                     // Don't target interceptors as a flying unit
                        || (target.unit()->exists() && target.unit()->isStasised())                                                                                                                         // Don't target stasis units
                        || (target.getType().isWorker() && Strategy::getEnemyTransition() != "WorkerRush" && unit.getGroundRange() <= 32.0 && !target.isThreatening() && (!atHome || Players::ZvZ() || !target.getTargetedBy().empty()) && !atEnemy && Util::getTime() < Time(5, 00))                            // Don't target non threatening workers in our territory
                        || (target.isHidden() && (!targetCanAttack || (!Players::hasDetection(PlayerState::Self) && Players::PvP())) && !unit.getType().isDetector())                                       // Don't target if invisible and can't attack this unit or we have no detectors in PvP
                        || (target.isFlying() && !unit.isFlying() && !target.unit()->exists() && Grids::getMobility(target.getPosition()) < 1)                                                              // Don't target flyers that we can't reach
                        || (unit.getType() == Zerg_Defiler && unit.targetsFriendly() && target.getType() != Zerg_Zergling)                                                                                  // Don't target important units for consume
                        || (unit.getType() == Zerg_Defiler && target.isFlying())
                        || (unit.getType() == Zerg_Mutalisk && Players::ZvZ() && !enemyHasAir && !target.getType().isWorker() && !target.canAttackAir() && !target.canAttackGround() && !target.isThreatening() && Util::getTime() < Time(8, 00))
                        || (unit.getType() == Zerg_Mutalisk && Players::ZvZ() && !enemyHasAir && target.getType().isBuilding() && Util::getTime() < Time(8, 00))
                        || (unit.getType() == Zerg_Scourge && !Stations::getEnemyStations().empty() && !target.canAttackAir() && !target.canAttackGround())
                        || (unit.getType().isWorker() && !target.unit()->isCompleted() && target.getType() == Terran_Bunker)
                        || (unit.isLightAir() && Util::getTime() < Time(12, 00) && Players::getStrength(PlayerState::Enemy).airToAir == 0 && target.getType().isBuilding() && !target.canAttackAir() && (enemyHasGround || enemyHasAir)))
                        return false;
                }
                if (unit.getRole() == Role::Scout) {
                    if (target.getType().isBuilding() && Terrain::isInEnemyTerritory(target.getTilePosition()))
                        return false;
                }
                if (unit.getRole() == Role::Worker) {
                    if (targetMatters && unitCanAttack)
                        return true;
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

                // Workers that aren't fighting don't count
                if (target.getType().isWorker() && !target.isThreatening())
                    return;

                // Set sim position
                double dist = unit.getPosition().getDistance(target.getPosition()) * (1 + int(!targetCanAttack));
                if ((unitCanAttack || targetCanAttack) && dist < distBest) {
                    unit.setSimPosition(target.getPosition());
                    unit.setSimTarget(&target);
                    distBest = dist;
                }
            };

            const auto checkBest = [&](UnitInfo& target) {

                if (!isSuitable(target))
                    return;

                // Scoring parameters
                const auto range = target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
                const auto reach = target.getType().isFlyer() ? unit.getAirReach() : unit.getGroundReach();
                const auto boxDistance = double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));

                const auto healthRatio = unit.isLightAir() ? 1.0 : (1.0 - target.getPercentTotal());
                const auto health = (range > 32.0 || unit.isWithinRange(target)) ? 1.0 + (0.5*healthRatio) : 1.0;
                const auto dist = exp(0.0125 * boxDistance);

                // Calculate is focus firing is a good idea here
                auto focusFire = 1.0;
                if (range <= 32.0 && boxDistance <= range) {
                    if (target.getTargetedBy().size() < 3 || Players::ZvZ())
                        focusFire = exp(int(target.getTargetedBy().size()) + 1);
                    else
                        focusFire = 1.0;
                }
                else if (range > 32.0 && boxDistance <= reach)
                    focusFire = exp(int(target.getTargetedBy().size()) + 1);

                auto priority = focusFire * target.getPriority() * health;

                if ((!target.canAttackAir() && unit.isFlying()) || (!target.canAttackGround() && !unit.isFlying()))
                    priority /= 2.0;

                auto thisUnit = 0.0;

                // Detector targeting
                if ((target.isBurrowed() || target.unit()->isCloaked()) && ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Terran_Comsat_Station)) {
                    auto closest = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u != unit && u.getRole() == Role::Combat && target.getType().isFlyer() ? u.getAirRange() > 0 : u.getGroundRange() > 0;
                    });

                    // Detectors want to stay close to their target if we have a friendly nearby
                    if (closest && closest->getPosition().getDistance(target.getPosition()) < 480.0)
                        thisUnit = priority / (1.0 + (dist * closest->getPosition().getDistance(target.getPosition())));
                    else
                        thisUnit = -1.0;
                }

                // DT want to kill any workers they're in reach of
                else if ((unit.getType() == Protoss_Dark_Templar || (BuildOrder::isRush() && unit.getGroundRange() <= 32)) && (target.getType().isWorker() || target.isSiegeTank() || target.getType().isDetector()) && unit.isWithinReach(target) && !Actions::overlapsDetection(target.unit(), target.getPosition(), PlayerState::Enemy))
                    thisUnit = 10000.0 / dist;

                else if (unit.getType() == Protoss_Dark_Templar && (target.getType() == Protoss_Observatory || target.getType() == Protoss_Robotics_Facility))
                    thisUnit = 1000.0 / dist;

                // Cluster targeting for AoE units
                else if (unit.getType() == Protoss_High_Templar || unit.getType() == Protoss_Arbiter) {
                    if (!target.getType().isBuilding() && target.getType() != Terran_Vulture_Spider_Mine) {

                        double eGrid = Grids::getEGroundCluster(target.getWalkPosition()) + Grids::getEAirCluster(target.getWalkPosition());
                        double aGrid = 1.0 + Grids::getAGroundCluster(target.getWalkPosition()) + Grids::getAAirCluster(target.getWalkPosition());
                        double score = eGrid / aGrid;

                        thisUnit = priority * score / dist;
                    }
                }

                // Proximity targeting
                else if (unit.getType() == Protoss_Reaver || unit.getType() == Zerg_Ultralisk) {
                    if (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir())
                        thisUnit = 0.1 / dist;
                    else
                        thisUnit = 1.0 / dist;
                }

                // Priority targeting
                else if (unit.isThreatening()) {
                    auto closestStation = Stations::getClosestStation(PlayerState::Self, target.getPosition());
                    thisUnit = closestStation ? priority / (dist * closestStation->getBase()->Center().getDistance(target.getPosition())) : thisUnit = priority / dist;
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

                for (auto &targeter : target.getTargetedBy()) {
                    if (targeter.lock())
                        targetedByType[targeter.lock()->getType()]++;
                }

                checkClosest(target);
                checkBest(target);
            }

            // If unit is close, add this unit to the targeted by vector
            if (unit.hasTarget() && unit.isWithinReach(unit.getTarget()) && unit.getRole() == Role::Combat)
                unit.getTarget().getTargetedBy().push_back(unit.weak_from_this());

            if (unit.hasTarget() && oldTarget != unit.getTarget().shared_from_this() && !unit.isWithinRange(unit.getTarget()) && !BuildOrder::isRush())
                unit.setLocalState(LocalState::Retreat);
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngagePosition(Positions::None);
                return;
            }

            // Have to set it to at least 64 or units can't path sometimes to engage position
            auto range = max(64.0, unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            if (!unit.getTargetPath().getTiles().empty() && (!unit.isWithinRange(unit.getTarget()) || unit.unit()->isLoaded())) {
                auto usedType = BWEB::Map::isUsed(TilePosition(unit.getTarget().getTilePosition()));
                auto engagePosition = Util::findPointOnPath(unit.getTargetPath(), [&](Position p) {
                    auto usedHere = BWEB::Map::isUsed(TilePosition(p));
                    return usedHere == UnitTypes::None && Util::boxDistance(unit.getTarget().getType(), unit.getTarget().getPosition(), unit.getType(), p) <= range;
                });

                if (engagePosition.isValid()) {
                    unit.setEngagePosition(engagePosition);
                    return;
                }
            }

            auto targetPosition = unit.getTarget().getPosition();
            auto distance = unit.getPosition().getDistance(targetPosition);
            auto direction = (unit.getPosition() - targetPosition) * int(round((distance - int(range)) / distance));

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

            if (!unit.getTargetPath().isReachable() && !unit.getTarget().getType().isBuilding() && !unit.getType().isFlyer() && !unit.getTarget().getType().isFlyer() && Grids::getMobility(unit.getTarget().getPosition()) >= 4) {
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
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                unit.setEngDist(0.0);
                unit.setTargetPath(newPath);
                unit.setQuickPath({});
                return;
            }

            // If no quick path, grab one
            if (unit.getQuickPath().empty() && !unit.getTarget().isFlying() && !unit.getType().isFlyer())
                unit.setQuickPath(mapBWEM.GetPath(unit.getPosition(), unit.getTarget().getPosition()));

            // Don't generate a target path in these cases
            if (unit.getTarget().isFlying() || unit.isFlying() || unit.getTilePosition() == unit.getTarget().getTilePosition()) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                unit.setTargetPath(newPath);
                unit.setQuickPath({});
                return;
            }

            // Create a pathpoint
            auto pathPoint = unit.getTarget().getPosition();
            if (unit.getTarget().getType().isBuilding()) {
                auto closest = DBL_MAX;
                for (int x = unit.getTarget().getTilePosition().x - 4; x < unit.getTarget().getTilePosition().x + unit.getTarget().getType().tileWidth() + 3; x++) {
                    for (int y = unit.getTarget().getTilePosition().y - 4; y < unit.getTarget().getTilePosition().y + unit.getTarget().getType().tileHeight() + 3; y++) {
                        auto center = Position(TilePosition(x, y)) + Position(16, 16);
                        auto dist = center.getDistance(unit.getPosition());
                        if (dist < closest) {
                            closest = dist;
                            pathPoint = center;
                        }
                    }
                }
            }

            // If should create path, grab one from BWEB
            if (unit.getTargetPath().getTarget() != TilePosition(pathPoint)) {
                BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());
                newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
                unit.setTargetPath(newPath);
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
        auto pState = unit.targetsFriendly() ? PlayerState::Self : PlayerState::Enemy;

        if (unit.getRole() == Role::Combat || unit.getRole() == Role::Defender || unit.getRole() == Role::Worker) {
            getBestTarget(unit, pState);
            getPathToTarget(unit);
            getEngagePosition(unit);
            getEngageDistance(unit);
            getBackupTarget(unit);
        }
    }
}