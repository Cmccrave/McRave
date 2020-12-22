#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave::Targets {

    namespace {

        bool isValidTarget(UnitInfo& unit, UnitInfo& target)
        {
            if (!target.unit()
                || target.unit()->isInvincible()
                || !target.getWalkPosition().isValid())
                return false;
            return true;
        }

        bool isSuitableTarget(UnitInfo& unit, UnitInfo& target)
        {
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myStrength = Players::getStrength(PlayerState::Self);

            bool targetCanAttack = !unit.isHidden() && (((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine)));
            bool unitCanAttack = !target.isHidden() && ((target.isFlying() && unit.getAirDamage() > 0.0) || (!target.isFlying() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));
            bool enemyHasGround = enemyStrength.groundToAir > 0.0 || enemyStrength.groundToGround > 0.0 || enemyStrength.groundDefense > 0.0;
            bool enemyHasAir = enemyStrength.airToGround > 0.0 || enemyStrength.airToAir > 0.0 || enemyStrength.airDefense > 0.0;
            bool selfHasGround = myStrength.groundToAir > 0.0 || myStrength.groundToGround > 0.0;
            bool selfHasAir = myStrength.airToGround > 0.0 || myStrength.airToAir > 0.0;

            bool atHome = Terrain::isInAllyTerritory(target.getTilePosition());
            bool atEnemy = Terrain::isInEnemyTerritory(target.getTilePosition());

            bool enemyCanDefendUnit = unit.isFlying() ? enemyStrength.airDefense > 0.0 : enemyStrength.groundDefense > 0.0;

            // Check if the target is important right now to attack
            bool targetMatters = target.getType().isSpell()
                || (target.canAttackAir() && selfHasAir)
                || (target.canAttackGround())
                || (target.getType().spaceProvided() > 0)
                || (target.getType().isDetector())
                || (!target.canAttackAir() && !target.canAttackGround() && !unit.hasTransport())
                || (target.getType().isWorker())
                || (!enemyHasGround && !enemyHasAir)
                || (Players::ZvZ() && enemyCanDefendUnit)
                || (Players::ZvZ() && Strategy::enemyFastExpand())
                || Util::getTime() > Time(10, 0);

            if (unit.getRole() == Role::Combat) {
                if ((unit.getType() == Protoss_Zealot && target.getType() == Terran_Vulture_Spider_Mine && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)                 // Zealot: Don't target mines without +2
                    || (unit.getType() == Protoss_Zealot && (target.getSpeed() > unit.getSpeed() || target.getType().isBuilding()) && !target.getType().isWorker() && BuildOrder::isRush())             // Zealot: Don't target faster units or buildings when we are rushing
                    || (unit.getType() == Protoss_Dark_Templar && target.getType() == Terran_Vulture && !unit.isWithinRange(target))                                                                    // DT: Don't target Vultures not in range
                    || !unitCanAttack || !targetMatters                                                                                                                                                 // Don't target units that don't matter or unit can't attack it
                    || (target.getType() == Terran_Vulture_Spider_Mine && int(target.getTargetedBy().size()) >= 4 && !target.isBurrowed())                                                              // Don't attack enemy spider mines with more than 4 units
                    || (target.getType() == Protoss_Interceptor && unit.isFlying())                                                                                                                     // Don't target interceptors as a flying unit
                    || (target.unit()->exists() && target.unit()->isStasised())                                                                                                                         // Don't target stasis units
                    || (target.getType().isWorker() && !unit.getType().isWorker() && Strategy::getEnemyTransition() != "WorkerRush" && !target.hasAttackedRecently() && unit.getGroundRange() <= 32.0 && !target.isThreatening() && (!atHome || !target.getTargetedBy().empty() || Players::ZvZ()) && !atEnemy && Util::getTime() < Time(8, 00))                            // Don't target non threatening workers in our territory
                    || (target.isHidden() && (!targetCanAttack || (!Players::hasDetection(PlayerState::Self) && Players::PvP())) && !unit.getType().isDetector())                                       // Don't target if invisible and can't attack this unit or we have no detectors in PvP
                    || (target.isFlying() && !unit.isFlying() && !target.unit()->exists() && Grids::getMobility(target.getPosition()) < 1)                                                              // Don't target flyers that we can't reach
                    || (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir() && Terrain::isInAllyTerritory(target.getTilePosition()) && Broodwar->getFrameCount() < 10000 && !target.isThreatening()) // Don't attack proxy structures that can't hit us (cannon rush)
                    || (!target.canAttackGround() && Strategy::getEnemyBuild() == "2Gate" && Strategy::enemyProxy() && target.getType() != Protoss_Pylon)
                    || (unit.isSpellcaster() && (target.getType() == Terran_Vulture_Spider_Mine || target.getType().isBuilding()))
                    || (unit.getType() == Zerg_Mutalisk && !enemyHasAir && !target.getType().isWorker() && !target.canAttackAir() && !target.isThreatening() && Util::getTime() < Time(10, 00) && (enemyHasGround || enemyHasAir))
                    || (unit.getType() == Zerg_Mutalisk && target.getType().isBuilding() && !target.canAttackAir() && !target.canAttackGround() && Util::getTime() < Time(10, 00))
                    || (unit.getType() == Zerg_Mutalisk && Grids::getAAirCluster(unit.getPosition()) <= 8.0f && (target.getType() == Terran_Missile_Turret || target.getType() == Protoss_Photon_Cannon || target.getType() == Zerg_Spore_Colony))
                    || (unit.getType() == Zerg_Mutalisk && Grids::getAAirCluster(unit.getPosition()) <= 12.0f && (target.getType() == Terran_Bunker || target.getType() == Protoss_Dragoon || target.getType() == Protoss_Zealot) && !target.isThreatening())
                    || (unit.getType() == Zerg_Scourge && !Stations::getEnemyStations().empty() && (target.getType() == Zerg_Overlord || target.getType().isBuilding() || target.getType() == Protoss_Interceptor))
                    || (unit.getType() == Zerg_Zergling && Strategy::getEnemyBuild() == "2Gate" && Strategy::enemyProxy() && Util::getTime() < Time(5, 00) && !target.getType().isWorker() && !target.isThreatening())
                    || (unit.getType() == Zerg_Zergling && target.getType().isBuilding() && Players::ZvZ() && !target.canAttackGround() && !target.canAttackAir() && Util::getTime() < Time(5, 00) && !Strategy::enemyFastExpand())
                    || (unit.getType() == Zerg_Defiler && (target.getType().isBuilding() || target.getType().isWorker()))
                    || (unit.getType() == Zerg_Defiler && unit.targetsFriendly() && target.getType() != Zerg_Zergling)                                                                                  // Don't target important units for consume
                    || (unit.getType() == Zerg_Defiler && target.isFlying())
                    || (unit.getType().isWorker() && target.getType().isWorker() && !target.isThreatening() && !target.isProxy())
                    || (unit.isLightAir() && target.getType() == Protoss_Interceptor)
                    || (unit.getType().isWorker() && target.getType() == Terran_Bunker && !selfHasGround))
                    return false;
            }
            if (unit.getRole() == Role::Scout) {
                if (target.getType().isBuilding() && !target.isThreatening())
                    return false;
            }
            if (unit.getRole() == Role::Worker) {

            }
            if (unit.getRole() == Role::Support) {

            }
            return true;
        }

        double scoreTarget(UnitInfo& unit, UnitInfo& target)
        {
            // Scoring parameters
            const auto range =          target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
            const auto reach =          target.getType().isFlyer() ? unit.getAirReach() : unit.getGroundReach();
            const auto boxDistance =    double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));
            const auto dist =           exp(0.0125 * boxDistance);

            const auto bonusScore = [&]() {
                // Add bonus for Observers that are vulnerable
                if (target.getType() == Protoss_Observer && !target.isHidden())
                    return 20.0;

                // Add penalty for a Terran building that's being constructed
                if (target.getType().isBuilding() && target.getType().getRace() == Races::Terran && target.unit()->isBeingConstructed() && !target.isThreatening())
                    return 0.0;

                // Add bonus for an SCV repairing a turret or building a turret
                if (target.getType().isWorker() && ((target.unit()->isConstructing() && target.unit()->getBuildUnit() && target.unit()->getBuildUnit()->getType() == Terran_Missile_Turret) || (target.unit()->isRepairing() && target.hasTarget() && target.getTarget().getType() == Terran_Missile_Turret)))
                    return 2.0;

                // Add bonus for a DT to kill important counters
                if (unit.getType() == Protoss_Dark_Templar && unit.isWithinReach(target) && !Actions::overlapsDetection(target.unit(), target.getPosition(), PlayerState::Enemy)
                    && (target.getType().isWorker() || target.isSiegeTank() || target.getType().isDetector() || target.getType() == Protoss_Observatory || target.getType() == Protoss_Robotics_Facility))
                    return 20.0;
                return 1.0;
            };

            const auto healthScore = [&]() {
                const auto healthRatio = unit.isLightAir() ? 1.0 : (1.0 - target.getPercentTotal());
                if (range > 32.0 && target.unit()->isCompleted())
                    return 1.0 + (0.1*healthRatio);
                return 1.0;
            };

            const auto focusScore = [&]() {
                if (!unit.isLightAir() && (range <= 32.0 || target.getType() == Protoss_Interceptor) && boxDistance <= range) {
                    if (target.getTargetedBy().size() < 3 || Players::ZvZ())
                        return exp(int(target.getTargetedBy().size()) + 1);
                    else
                        return 1.0;
                }

                else if ((!Players::ZvZ() || target.isFlying()) && range > 32.0 && boxDistance <= reach) {
                    if (unit.isLightAir()) {
                        int actual = 0;
                        for (auto &t : target.getTargetedBy()) {
                            if (auto &targeter = t.lock())
                                actual += targeter->isLightAir();
                        }
                        return exp(actual + 1);
                    }
                    else {
                        return exp(int(target.getTargetedBy().size()) + 1);
                    }
                }
                return 1.0;
            };

            const auto priorityScore = [&]() {
                if (!target.getType().isWorker() && ((!target.canAttackAir() && unit.isFlying()) || (!target.canAttackGround() && !unit.isFlying())))
                    return target.getPriority() / 4.0;
                if (target.getType().isWorker() && unit.isLightAir() && Grids::getEAirThreat(unit.getPosition()) > 0.0f)
                    return target.getPriority() / 4.0;
                return target.getPriority();
            };

            const auto targetScore = healthScore() * focusScore() * priorityScore();


            if (unit.unit()->isSelected() && unit.getType() == Protoss_Reaver)
                Broodwar->drawTextMap(target.getPosition(), "%.2f", 1.0 / dist);

            // Detector targeting (distance to nearest ally added)
            if ((target.isBurrowed() || target.unit()->isCloaked()) && ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Terran_Comsat_Station)) {
                auto closest = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u != unit && u.getRole() == Role::Combat && target.getType().isFlyer() ? u.getAirRange() > 0 : u.getGroundRange() > 0;
                });

                // Detectors want to stay close to their target if we have a friendly nearby
                if (closest && closest->getPosition().getDistance(target.getPosition()) < 480.0)
                    return priorityScore() / (dist * closest->getPosition().getDistance(target.getPosition()));
                return 0.0;
            }

            // Cluster targeting (grid score added)
            else if (unit.getType() == Protoss_High_Templar || unit.getType() == Protoss_Arbiter) {
                const auto eGrid =      Grids::getEGroundCluster(target.getWalkPosition()) + Grids::getEAirCluster(target.getWalkPosition());
                const auto aGrid =      1.0 + Grids::getAGroundCluster(target.getWalkPosition()) + Grids::getAAirCluster(target.getWalkPosition());
                const auto gridScore =  eGrid / aGrid;
                return gridScore * targetScore / dist;
            }

            // Defender targeting (distance not used)
            else if (unit.getRole() == Role::Defender && boxDistance - range <= 16.0)
                return priorityScore();

            // Proximity targeting (targetScore not used)
            else if (unit.getType() == Protoss_Reaver || unit.getType() == Zerg_Ultralisk) {
                if (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir())
                    return 0.1 / dist;
                return 1.0 / dist;
            }

            // Priority targeting
            else if (unit.isThreatening()) {
                auto closestStation = Stations::getClosestStation(PlayerState::Self, target.getPosition());
                if (closestStation)
                    return targetScore / (dist * closestStation->getBase()->Center().getDistance(target.getPosition()));
            }
            return targetScore / dist;
        }

        void getSimTarget(UnitInfo& unit, PlayerState pState)
        {
            double distBest = DBL_MAX;
            for (auto &t : Units::getUnits(pState)) {
                UnitInfo &target = *t;
                auto targetCanAttack = ((unit.getType().isFlyer() && target.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine));
                auto unitCanAttack = ((target.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!target.getType().isFlyer() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));

                // HACK: Check for a flying building
                if (target.unit()->exists() && target.unit()->isFlying() && unit.getAirDamage() <= 0.0)
                    unitCanAttack = false;

                if (!unitCanAttack && !targetCanAttack)
                    continue;

                // Set sim position
                auto dist = unit.getPosition().getDistance(target.getPosition()) * (1 + int(!targetCanAttack));
                if (dist < distBest) {
                    unit.setSimTarget(&target);
                    distBest = dist;
                }
            }
        }

        void getBestTarget(UnitInfo& unit, PlayerState pState)
        {
            auto scoreBest = 0.0;
            for (auto &t : Units::getUnits(pState)) {
                UnitInfo &target = *t;

                if (!isValidTarget(unit, target)
                    || !isSuitableTarget(unit, target))
                    continue;

                if (unit.getType() == Protoss_Reaver)
                    target.circlePurple();

                // If this target is more important to target, set as current target
                const auto thisUnit = scoreTarget(unit, target);
                if (thisUnit > scoreBest) {
                    scoreBest = thisUnit;
                    unit.setTarget(&target);
                }
            }

            // If unit is close, add this unit to the targeted by vector
            if (unit.hasTarget() && unit.isWithinReach(unit.getTarget()) && unit.getRole() == Role::Combat)
                unit.getTarget().getTargetedBy().push_back(unit.weak_from_this());
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (unit.getRole() == Role::Defender) {
                unit.setEngagePosition(unit.getPosition());
                return;
            }

            if (!unit.hasTarget() || unit.getRole() != Role::Combat) {
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

            auto distance = Util::boxDistance(unit.getType(), unit.getPosition(), unit.getTarget().getType(), unit.getTarget().getPosition());
            auto direction = ((distance - range) / distance);
            auto engageX = int((unit.getPosition().x - unit.getTarget().getPosition().x) * direction);
            auto engageY = int((unit.getPosition().y - unit.getTarget().getPosition().y) * direction);
            auto engagePosition = unit.getPosition() - Position(engageX, engageY);

            // If unit is loaded or further than their range, we want to calculate the expected engage position
            if (distance > range || unit.unit()->isLoaded())
                unit.setEngagePosition(engagePosition);            
            else
                unit.setEngagePosition(unit.getPosition());
        }

        void getEngageDistance(UnitInfo& unit)
        {
            if (unit.getRole() == Role::Defender) {
                unit.setEngDist(0.0);
                return;
            }

            if (!unit.hasTarget() || unit.getRole() != Role::Combat) {
                unit.setEngDist(0.0);
                return;
            }

            if (!unit.getTargetPath().isReachable() && !unit.getTarget().getType().isBuilding() && !unit.isFlying() && !unit.getTarget().isFlying() && Grids::getMobility(unit.getTarget().getPosition()) >= 4) {
                if (unit.hasBackupTarget()) {
                    unit.setTarget(&unit.getBackupTarget());
                    unit.setEngDist(unit.getPosition().getDistance(unit.getBackupTarget().getPosition()));
                }
                else {
                    Broodwar->drawLineMap(unit.getPosition(), unit.getTarget().getPosition(), Colors::Cyan);
                    unit.setEngDist(DBL_MAX);
                }
                return;
            }

            auto dist = unit.getPosition().getDistance(unit.getEngagePosition());
            unit.setEngDist(dist);
        }

        void getPathToTarget(UnitInfo& unit)
        {
            // If unnecessary to get path
            if (unit.getRole() != Role::Combat)
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
            if (unit.getTarget().isFlying() || unit.isFlying()) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                unit.setTargetPath(newPath);
                unit.setQuickPath({});
                return;
            }

            // Create a pathpoint
            auto pathPoint = unit.getTarget().getPosition();
            if (!BWEB::Map::isWalkable(unit.getTarget().getTilePosition(), unit.getType()) || BWEB::Map::isUsed(unit.getTarget().getTilePosition()) != UnitTypes::None) {
                auto closest = DBL_MAX;
                for (int x = unit.getTarget().getTilePosition().x - 1; x < unit.getTarget().getTilePosition().x + unit.getTarget().getType().tileWidth() + 1; x++) {
                    for (int y = unit.getTarget().getTilePosition().y - 1; y < unit.getTarget().getTilePosition().y + unit.getTarget().getType().tileHeight() + 1; y++) {
                        auto center = Position(TilePosition(x, y)) + Position(16, 16);
                        auto dist = center.getDistance(unit.getPosition());
                        if (dist < closest && BWEB::Map::isWalkable(TilePosition(x, y), unit.getType()) && BWEB::Map::isUsed(TilePosition(x, y)) == UnitTypes::None) {
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
                Visuals::displayPath(unit.getTargetPath());
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

        if (unit.getRole() == Role::Combat || unit.getRole() == Role::Support || unit.getRole() == Role::Defender || unit.getRole() == Role::Worker || unit.getRole() == Role::Scout) {
            getBestTarget(unit, pState);
            getSimTarget(unit, PlayerState::Enemy);
            getPathToTarget(unit);
            getEngagePosition(unit);
            getEngageDistance(unit);
            getBackupTarget(unit);
        }
    }
}