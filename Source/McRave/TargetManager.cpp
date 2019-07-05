#include "McRave.h"

using namespace std;
using namespace BWAPI;

namespace McRave::Targets {

    namespace {

        void getBestTarget(UnitInfo& unit)
        {
            double closest = DBL_MAX;
            double bestScore = 0.0;
            double bestDistance = DBL_MAX;

            auto &unitList = unit.targetsFriendly() ? Units::getUnits(PlayerState::Self) : Units::getUnits(PlayerState::Enemy);
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myStrength = Players::getStrength(PlayerState::Self);

            const auto shouldTarget = [&](UnitInfo& target, bool unitCanAttack, bool targetCanAttack) {

                bool enemyHasGround = enemyStrength.groundToAir > 0.0 || enemyStrength.groundToGround > 0.0;
                bool enemyHasAir = enemyStrength.airToGround > 0.0 || enemyStrength.airToAir > 0.0;
                bool selfHasGround = myStrength.groundToAir > 0.0 || myStrength.groundToGround > 0.0;
                bool selfHasAir = myStrength.airToGround > 0.0 || myStrength.airToAir > 0.0 || com(UnitTypes::Protoss_Shuttle) > 0;

                bool targetMatters = (target.getAirDamage() > 0.0 && selfHasAir)
                    || (target.canAttackGround())
                    || (target.getType().isDetector() && (vis(UnitTypes::Protoss_Dark_Templar) > 0 || vis(UnitTypes::Protoss_Observer) > 0))
                    || (!target.canAttackAir() && !target.canAttackGround() && !unit.hasTransport())
                    || (target.getType().isWorker())
                    || (!enemyHasGround && !enemyHasAir);

                // Melee: Don't attack non threatening workers in our territory
                if ((unit.getRole() == Role::Combat && unit.getGroundRange() <= 32.0 && target.getType().isWorker() && !target.isThreatening() && (Players::getSupply(PlayerState::Self) < 60 || int(unit.getTargetedBy().size()) > 0) && Terrain::isInAllyTerritory(target.getTilePosition()) && !target.hasAttackedRecently() && !Terrain::isInEnemyTerritory(target.getTilePosition()))

                    // Scout roles should only target non buildings
                    || (unit.getRole() == Role::Scout && target.getType().isBuilding() && Terrain::isInEnemyTerritory(target.getTilePosition()))

                    // Don't try to attack flyers that we can't reach
                    || (target.getType().isFlyer() && Grids::getMobility(target.getPosition()) == 0 && !unit.getType().isFlyer() && !target.unit()->exists())

                    // If target is an egg, larva, scarab or spell
                    || (target.getType() == UnitTypes::Zerg_Egg || target.getType() == UnitTypes::Zerg_Larva || target.getType() == UnitTypes::Protoss_Scarab || target.getType().isSpell())

                    // If unit can't attack and unit is not a detector
                    || (!unit.getType().isDetector() && unit.getType() != UnitTypes::Terran_Comsat_Station && !unitCanAttack)

                    // If target is stasised
                    || (target.unit()->exists() && target.unit()->isStasised())

                    // Zealot: Don't attack mines without +2
                    || (target.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)

                    // If target is invisible and can't attack this unit
                    || (target.isHidden() && !targetCanAttack && !unit.getType().isDetector())

                    // Don't attack units that don't matter
                    || !targetMatters

                    // TEST: some ling stuff, don't attack Vultures
                    || (unit.getType() == UnitTypes::Zerg_Zergling && target.getType() == UnitTypes::Terran_Vulture)

                    // DT: Don't attack Vultures
                    || (unit.getType() == UnitTypes::Protoss_Dark_Templar && target.getType() == UnitTypes::Terran_Vulture)

                    // Flying units don't attack interceptors
                    || (unit.getType().isFlyer() && target.getType() == UnitTypes::Protoss_Interceptor)

                    // Don't attack enemy spider mines with more than 4 units
                    || (target.getType() == UnitTypes::Terran_Vulture_Spider_Mine && int(target.getTargetedBy().size()) >= 4) && !target.isBurrowed())
                    return false;
                return true;
            };

            const auto checkBest = [&](UnitInfo& target, double thisUnit, double health, double dist) {

                //auto visibility = log(clamp(Broodwar->getFrameCount() - double(Grids::lastVisibleFrame(target.getTilePosition())), 25.0, 1000.0));
                auto clusterTarget = unit.getType() == UnitTypes::Protoss_High_Templar || unit.getType() == UnitTypes::Protoss_Arbiter;
                auto priority = target.getPriority() * health/* / visibility*/;

                // Detector targeting
                if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == UnitTypes::Terran_Comsat_Station) {

                    // See if I have a unit close to the enemy
                    auto closest = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u != unit && u.isWithinReach(target);
                    });

                    // Detectors want to stay close to their target
                    if (closest && (target.isBurrowed() || target.unit()->isCloaked()))
                        thisUnit = priority / dist;
                    else
                        thisUnit = -1.0;
                }

                // Cluster targeting for AoE units
                else if (clusterTarget) {
                    if (!target.getType().isBuilding() && target.getType() != UnitTypes::Terran_Vulture_Spider_Mine) {

                        double eGrid = Grids::getEGroundCluster(target.getWalkPosition()) + Grids::getEAirCluster(target.getWalkPosition());
                        double aGrid = 1.0 + Grids::getAGroundCluster(target.getWalkPosition()) + Grids::getAAirCluster(target.getWalkPosition());
                        double score = eGrid / aGrid;

                        thisUnit = priority * score / dist;
                    }
                }

                // Proximity targeting
                else if (unit.getType() == UnitTypes::Protoss_Reaver) {
                    if (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir())
                        thisUnit = 0.1 / dist;
                    else
                        thisUnit = priority / dist;
                }

                // Priority targeting
                else
                    thisUnit = priority / dist;

                // If this target is more important to target, set as current target
                if (thisUnit > bestScore || (thisUnit == bestScore && dist < bestDistance)) {
                    bestScore = thisUnit;
                    unit.setTarget(&target);
                    bestDistance = dist;
                }
            };

            for (auto &t : unitList) {
                UnitInfo &target = *t;

                // Valid check;
                if (!target.unit()
                    || !target.getWalkPosition().isValid()
                    || !unit.getWalkPosition().isValid()
                    || (target.getType().isBuilding() && !target.isThreatening() && !target.canAttackGround() && Terrain::isInAllyTerritory(target.getTilePosition()) && Broodwar->getFrameCount() < 10000))
                    continue;

                bool targetCanAttack = ((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == UnitTypes::Terran_Vulture_Spider_Mine));
                bool unitCanAttack = ((target.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!target.getType().isFlyer() && unit.canAttackGround()) || (unit.getType() == UnitTypes::Protoss_Carrier));

                // HACK: Check for a flying building
                if (target.unit()->exists() && target.unit()->isFlying() && unit.getAirDamage() <= 0.0)
                    unitCanAttack = false;

                double widths = unit.getType().tileWidth() * 16.0 + target.getType().tileWidth() * 16.0;
                double dist = max(1.0, unit.getPosition().getDistance(target.getPosition()) - widths);
                double health = (targetCanAttack ? 1.0 + (1.0*(1.0 - target.getPercentTotal())) : 1.0) + (0.25*target.getTargetedBy().size());
                double thisUnit = 0.0;
                
                // Set sim position
                if ((unitCanAttack || targetCanAttack) && dist < closest) {
                    unit.setSimPosition(target.getPosition());
                    closest = dist;
                }

                // If should target, check if it's best
                if (shouldTarget(target, unitCanAttack, targetCanAttack))
                    checkBest(target, thisUnit, health, dist);
            }

            // If unit is close, increment it
            if (unit.hasTarget() && unit.isWithinRange(unit.getTarget()))
                unit.getTarget().getTargetedBy().push_back(make_shared<UnitInfo>(unit));
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngagePosition(Positions::None);
                return;
            }

            auto range = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
            if (!unit.getPath().getTiles().empty() && !unit.unit()->isLoaded() && !unit.isWithinRange(unit.getTarget())) {
                auto engagePosition = Util::findPointOnPath(unit.getPath(), [&](Position p) {
                    auto center = p + Position(16, 16);
                    return center.getDistance(unit.getTarget().getPosition()) <= range;
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

        void getEngageDistance(UnitInfo& unit) {

            // If unnecessary to get engage distance
            if (unit.getRole() != Role::Combat && unit.getRole() != Role::Scout)
                return;

            if (!unit.getPath().isReachable())
                unit.setEngDist(DBL_MAX);
            
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
            
            if (unit.getTarget().getType().isBuilding() || unit.getTarget().getType().isFlyer() || unit.getTilePosition() == unit.getTarget().getTilePosition()) {
                BWEB::Path newPath;
                unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
                unit.setPath(newPath);
                return;
            }

            // If should create path, grab one from BWEB
            if (unit.canCreatePath(unit.getTarget().getTilePosition())) {
                BWEB::Path newPath;
                newPath.createUnitPath(unit.getPosition(), unit.getTarget().getPosition());
                unit.setPath(newPath);
            }
        }
    }

    void getTarget(UnitInfo& unit)
    {
        getBestTarget(unit);
        getPathToTarget(unit);
        getEngagePosition(unit);
        getEngageDistance(unit);
    }
}