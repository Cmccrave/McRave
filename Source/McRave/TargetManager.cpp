#include "McRave.h"

using namespace std;
using namespace BWAPI;

namespace McRave::Targets {

    namespace {

        void getBestTarget(UnitInfo& unit, PlayerState pState)
        {
            double distBest = DBL_MAX;
            double scoreBest = 0.0;

            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myStrength = Players::getStrength(PlayerState::Self);

            const auto isSuitable = [&](UnitInfo& target) {

                bool targetCanAttack = ((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == UnitTypes::Terran_Vulture_Spider_Mine));
                bool unitCanAttack = ((target.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!target.getType().isFlyer() && unit.canAttackGround()) || (unit.getType() == UnitTypes::Protoss_Carrier));

                // HACK: Check for a flying building
                if (target.unit()->exists() && target.unit()->isFlying() && unit.getAirDamage() <= 0.0)
                    unitCanAttack = false;

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

                    // Don't target critters
                    || (target.getPlayer()->isNeutral() && (!target.unit()->exists() || target.getSpeed() > 0.0 || target.getPosition().getDistance(unit.getPosition()) > unit.getGroundReach()))

                    // Scout roles should only target non buildings
                    || (unit.getRole() == Role::Scout && target.getType().isBuilding() && Terrain::isInEnemyTerritory(target.getTilePosition()))

                    // Don't try to attack flyers that we can't reach
                    || (target.getType().isFlyer() && Grids::getMobility(target.getPosition()) < 1 && !unit.getType().isFlyer() && !target.unit()->exists())

                    // If target is an egg, larva, scarab or spell
                    || (target.getPlayer() != Broodwar->neutral() && (target.getType() == UnitTypes::Zerg_Egg || target.getType() == UnitTypes::Zerg_Larva || target.getType() == UnitTypes::Protoss_Scarab || target.getType().isSpell()))

                    // If unit can't attack and unit is not a detector
                    || (!unit.getType().isDetector() && unit.getType() != UnitTypes::Terran_Comsat_Station && !unitCanAttack)

                    // If target is stasised
                    || (target.unit()->exists() && target.unit()->isStasised())

                    // Zealot: Don't attack mines without +2
                    || (target.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)

                    // If target is invisible and can't attack this unit
                    || ((target.isHidden() || unit.isHidden()) && !targetCanAttack && !unit.getType().isDetector())

                    // Don't attack units that don't matter
                    || !targetMatters

                    || (BuildOrder::getCurrentOpener() == "Proxy" && !target.getType().isWorker() && target.getSpeed() > unit.getSpeed() && Broodwar->getFrameCount() < 8000)

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

            const auto checkClosest = [&](UnitInfo& target) {
                if (!target.getPlayer()->isEnemy(Broodwar->self()))
                    return;

                bool targetCanAttack = ((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == UnitTypes::Terran_Vulture_Spider_Mine));
                bool unitCanAttack = ((target.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!target.getType().isFlyer() && unit.canAttackGround()) || (unit.getType() == UnitTypes::Protoss_Carrier));

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
                const auto dist = exp(0.0125 * (unit.getPosition().getDistance(target.getPosition()) - range));
                const auto clusterTarget = unit.getType() == UnitTypes::Protoss_High_Templar || unit.getType() == UnitTypes::Protoss_Arbiter;
                const auto priority = target.getPriority() * health;

                auto thisUnit = 0.0;

                // Detector targeting
                if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == UnitTypes::Terran_Comsat_Station) {

                    if (target.isBurrowed() || target.unit()->isCloaked()) {

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

                // DT want to kill any workers they're in reach of
                else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && target.getType().isWorker() && unit.isWithinReach(target))
                    thisUnit = 10000.0 / dist;

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
                        thisUnit = 1.0 / dist;
                }

                // Priority targeting
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
                    return unit.getType() == UnitTypes::Protoss_Reaver ? BWEB::Map::getGroundDistance(center, unit.getTarget().getPosition()) <= range : center.getDistance(unit.getTarget().getPosition()) <= range;
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

    void getBackupTarget(UnitInfo& unit)
    {
        if (unit.getRole() == Role::Combat && !unit.getType().isFlyer()) {
            if (!unit.hasTarget() || Terrain::isIslandMap()) {
                getBestTarget(unit, PlayerState::Neutral);
                getPathToTarget(unit);
                getEngagePosition(unit);
                getEngageDistance(unit);
            }
        }
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