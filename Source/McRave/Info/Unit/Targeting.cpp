#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave::Targets {

    namespace {

        set<UnitType> cancelPriority ={ Terran_Missile_Turret, Terran_Barracks, Terran_Bunker, Terran_Factory, Terran_Starport, Terran_Armory, Terran_Bunker };
        set<UnitType> proxyTargeting ={ Protoss_Pylon, Terran_Barracks, Terran_Bunker, Zerg_Sunken_Colony };
        map<UnitInfo*, int> meleeSpotsAvailable;

        enum class Priority {
            Ignore, Trivial, Minor, Normal, Major, Critical
        };

        bool enemyHasGround;
        bool enemyHasAir;
        bool enemyCanHitAir;
        bool enemyCanHitGround;

        bool selfHasGround;
        bool selfHasAir;
        bool selfCanHitAir;
        bool selfCanHitGround;

        bool allowWorkerTarget(UnitInfo& unit, UnitInfo& target) {
            if (unit.getType().isWorker()) {
                return Spy::getEnemyTransition() == "WorkerRush"
                    || target.hasAttackedRecently()
                    || target.hasRepairedRecently()
                    || target.isThreatening()
                    || target.getUnitsTargetingThis().empty();
            }

            if (Util::getTime() < Time(8, 00)) {
                return unit.getType().isWorker()
                    || unit.getGroundRange() > 32.0
                    || unit.attemptingRunby()
                    || target.hasAttackedRecently()
                    || target.hasRepairedRecently()
                    || target.isThreatening()
                    || BuildOrder::isHideTech()
                    || (target.getUnitsTargetingThis().empty() && !Players::ZvZ())
                    || Spy::getEnemyTransition() == "WorkerRush"
                    || Terrain::inTerritory(PlayerState::Enemy, target.getPosition());
            }
            return int(target.getUnitsTargetingThis().size()) <= 4;
        }

        Priority getPriority(UnitInfo &unit, UnitInfo& target)
        {
            bool targetCanAttack = !unit.isHidden() && (((unit.getType().isFlyer() && target.canAttackAir()) || (!unit.getType().isFlyer() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine)));
            bool unitCanAttack = !target.isHidden() && ((target.isFlying() && unit.canAttackAir()) || (!target.isFlying() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));

            if (unit.getRole() != Role::Support && (target.movedFlag || !unitCanAttack))
                return Priority::Ignore;

            // Check if the target is important right now to attack
            bool targetMatters = (target.canAttackAir() && selfHasAir)
                || (target.canAttackGround() && selfHasGround)
                || (target.getType().spaceProvided() > 0)
                || (target.getType().isDetector())
                || (!target.canAttackAir() && !target.canAttackGround() && !unit.hasTransport())
                || (!enemyHasGround && !enemyHasAir)
                || (target.getType() == Terran_Bunker && !target.isCompleted())
                || (Players::ZvZ() && Spy::enemyFastExpand())
                || Util::getTime() > Time(6, 00)
                || Spy::enemyGreedy();

            // Support Role
            if (unit.getRole() == Role::Support) {
                if (target.isHidden() && (unit.getType().isDetector() || unit.getType() == Terran_Comsat_Station))
                    return Priority::Critical;
                if (unit.getType() == Terran_Comsat_Station)
                    return Priority::Ignore;
            }

            // Combat Role
            if (unit.getRole() == Role::Combat) {

                // Generic trivial
                if (!targetMatters)
                    return Priority::Trivial;

                // Proxy worker
                if (target.isProxy() && target.getType().isWorker() && target.unit()->exists()) {
                    if (target.unit()->isConstructing())
                        return Priority::Critical;
                    else
                        return Priority::Major;
                }

                // Proxy priority
                if (target.isProxy() && target.getType().isBuilding() && Spy::enemyProxy() && unit.getType() != Zerg_Mutalisk) {
                    Visuals::drawCircle(target.getPosition(), 10, Colors::Yellow, true);
                    if (target.unit()->getBuildUnit() || (target.getType().getRace() == Races::Terran && !target.isCompleted()))
                        return Priority::Minor;
                    else if (proxyTargeting.find(target.getType()) != proxyTargeting.end() && ((Players::getVisibleCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0 && Players::getVisibleCount(PlayerState::Enemy, Terran_Marine) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) == 0) || !unit.getType().isWorker()))
                        return Priority::Critical;
                    else if (target.canAttackGround())
                        return Priority::Critical;
                    else
                        return Priority::Ignore;
                }

                // Generic major priority
                if (!unit.getType().isWorker() && target.isThreatening() && !target.getType().isWorker() && Terrain::inTerritory(PlayerState::Self, unit.getPosition()))
                    return Priority::Critical;

                // Generic ignore
                if (target.getType().isSpell()
                    || (target.getType() == Terran_Vulture_Spider_Mine && int(target.getUnitsTargetingThis().size()) >= 4 && !target.isBurrowed())                          // Don't over target spider mines
                    || (target.getType() == Protoss_Interceptor && unit.isFlying())                                                                                         // Don't target interceptors as a flying unit
                    || (target.getType().isWorker() && !allowWorkerTarget(unit, target))
                    || (target.getType() == Protoss_Corsair && !unit.isFlying() && target.getUnitsTargetingThis().size() > 2 && !unit.isWithinRange(target))
                    || (target.isHidden() && (!targetCanAttack || (!Players::hasDetection(PlayerState::Self) && Players::PvP())) && !unit.getType().isDetector())           // Don't target if invisible and can't attack this unit or we have no detectors in PvP
                    || (target.isFlying() && !unit.isFlying() && !BWEB::Map::isWalkable(target.getTilePosition(), unit.getType()) && !unit.isWithinRange(target))           // Don't target flyers that we can't reach
                    || (unit.isFlying() && target.getType() == Protoss_Interceptor))
                    return Priority::Ignore;

                // Zergling
                if (unit.getType() == Zerg_Zergling) {
                    if (Util::getTime() < Time(5, 00)) {
                        if (Players::ZvZ() && !target.canAttackGround() && !Spy::enemyFastExpand())                                                                 // Avoid non ground hitters to try and kill drones
                            return Priority::Ignore;
                    }
                    const auto targetSize = max(target.getType().width(), target.getType().height());
                    const auto targetingCount = count_if(target.getUnitsTargetingThis().begin(), target.getUnitsTargetingThis().end(), [&](auto &u) { return u.lock()->getType() == Zerg_Zergling; });
                    if (!target.getType().isBuilding() && targetingCount >= targetSize / 4)
                        return Priority::Minor;
                    //if (unit.attemptingRunby() && (!target.getType().isWorker() || !Terrain::inTerritory(PlayerState::Enemy, target.getPosition())))
                    //    return Priority::Ignore;
                }

                // Mutalisk
                if (unit.getType() == Zerg_Mutalisk) {
                    auto anythingTime = Util::getTime() > (Players::ZvZ() ? Time(7, 00) : Time(12, 00));
                    auto anythingSupply = !Players::ZvZ() && Players::getSupply(PlayerState::Enemy, Races::None) < 20;
                    auto defendExpander = BuildOrder::shouldExpand() && unit.getGoal().isValid();

                    if (unit.canOneShot(target))
                        return Priority::Major;

                    if (Players::ZvP()) {

                        // Always kill cannons
                        //if (target.getType() == Protoss_Photon_Cannon && unit.attemptingHarass() && target.getPosition().getDistance(Combat::getHarassPosition()) < 160.0)
                        //    return Priority::Critical;
                        if (target.getType() == Protoss_Photon_Cannon && unit.isWithinReach(target) && target.unit()->exists() && !target.isCompleted())
                            return Priority::Critical;
                        if (target.getType() == Protoss_Photon_Cannon && unit.isWithinReach(target) && (unit.canOneShot(target) || unit.canTwoShot(target)))
                            return Priority::Critical;

                        // Clean Zealots up vs rushes
                        if (Util::getTime() < Time(9, 00) && target.getType() == Protoss_Zealot && Spy::getEnemyTransition() == "ZealotRush")
                            return Priority::Major;

                        // Try to push for only worker kills to try and end the game
                        if (Spy::getEnemyBuild() == "FFE" && Util::getTime() < Time(8, 00) && !target.getType().isWorker() && target.isCompleted() && !target.isThreatening())
                            return Priority::Ignore;
                    }
                    if (Players::ZvZ() && Util::getTime() < Time(8, 00) && target.getType() == Zerg_Zergling && Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) > vis(Zerg_Zergling))
                        return Priority::Major;

                    // Low priority targets, ignore when we haven't found the enemy
                    auto priorityAfterInfo = Terrain::foundEnemy() ? Priority::Minor : Priority::Trivial;
                    if (!anythingTime && !defendExpander && !target.isThreatening()) {
                        if (!Players::ZvZ() && !unit.canOneShot(target) && !unit.canTwoShot(target) && !target.isFlying() && !target.getType().isBuilding() && !target.getType().isWorker())
                            return priorityAfterInfo;
                        if ((enemyCanHitAir || enemyCanHitGround) && !target.canAttackAir() && !target.canAttackGround())
                            return priorityAfterInfo;
                    }
                }

                // Scourge
                if (unit.getType() == Zerg_Scourge) {
                    if ((!Stations::getStations(PlayerState::Enemy).empty() && target.getType().isBuilding())                                                                            // Avoid targeting buildings if the enemy has a base still
                        || target.getType() == Zerg_Overlord                                                                                                                // Don't target overlords
                        || target.getType() == Protoss_Interceptor)                                                                                                         // Don't target interceptors
                        return Priority::Ignore;
                }

                // Defiler
                if (unit.getType() == Zerg_Defiler) {
                    if (target.getType().isBuilding()
                        || target.getType().isWorker()
                        || target.isFlying()
                        || (unit.targetsFriendly() && target.getType() != Zerg_Zergling)
                        || (unit.targetsFriendly() && target.getRole() != Role::Combat))
                        return Priority::Ignore;
                }

                // Zealot
                if (unit.getType() == Protoss_Zealot) {
                    if ((target.getType() == Terran_Vulture_Spider_Mine && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)                     // Avoid mines without +2 to oneshot them
                        || ((target.getSpeed() > unit.getSpeed() || target.getType().isBuilding()) && !target.getType().isWorker() && BuildOrder::isRush()))                // Avoid faster units when we're rushing
                        return Priority::Ignore;
                }

                // Dark Templar
                if (unit.getType() == Protoss_Dark_Templar) {
                    if (target.getType() == Terran_Vulture && !unit.isWithinRange(target))                                                                                  // Avoid vultures if not in range already
                        return Priority::Ignore;
                }

                // Ghost
                if (unit.getType() == Terran_Ghost) {
                    if (!target.getType().isResourceDepot())
                        return Priority::Ignore;
                }

                // Medic
                if (unit.getType() == Terran_Medic) {
                    if (target.getType() == Terran_Marine || target.getType() == Terran_Firebat || (target.getType() == Terran_Medic && target.getHealth() < target.getType().maxHitPoints()))
                        return Priority::Critical;
                    else if (target.getType() == Terran_SCV)
                        return Priority::Major;
                    else if (target.getType().isOrganic())
                        return Priority::Minor;
                    return Priority::Ignore;
                }
            }
            return Priority::Normal;
        }

        double scoreTarget(UnitInfo& unit, UnitInfo& target)
        {
            // Scoring parameters
            auto range =          target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
            auto reach =          target.getType().isFlyer() ? unit.getAirReach() : unit.getGroundReach();
            auto enemyRange =     unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange();
            auto enemyReach =     unit.getType().isFlyer() ? target.getAirReach() : target.getGroundReach();

            const auto boxDistance =    double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));
            const auto useGrd =         !unit.getType().isWorker() && !unit.isFlying() && !target.isFlying()
                && mapBWEM.GetArea(unit.getTilePosition()) && mapBWEM.GetArea(target.getTilePosition())
                && mapBWEM.GetArea(unit.getTilePosition())->AccessibleFrom(mapBWEM.GetArea(target.getTilePosition()))
                && boxDistance < unit.getEngageRadius()
                && boxDistance > reach;
            const auto actualDist =     useGrd ? BWEB::Map::getGroundDistance(unit.getPosition(), target.getPosition()) : boxDistance;
            const auto dist =           exp(0.0125 * actualDist);

            if (unit.isSiegeTank()) {
                range = 384.0;
                reach = range + (unit.getSpeed() * 16.0) + double(unit.getType().width() / 2) + 64.0;
            }

            const auto bonusScore = [&]() {

                // Add bonus for Observers that are vulnerable
                if (target.getType() == Protoss_Observer && !target.isHidden())
                    return 20.0;

                // Add penalty for a Terran building that's being constructed
                if (target.getType().isBuilding() && target.getType().getRace() == Races::Terran && !target.unit()->isCompleted() && !target.isThreatening() && !target.unit()->getBuildUnit())
                    return 0.1;

                // Add penalty for a Terran building that has been repaired recently
                if (target.getType().isBuilding() && target.hasRepairedRecently())
                    return 0.1;

                // Add bonus for a building warping in
                if (Util::getTime() > Time(8, 00) && target.unit()->exists() && !target.unit()->isCompleted() && (target.getType() == Protoss_Photon_Cannon || (target.getType() == Terran_Missile_Turret && !target.unit()->getBuildUnit())))
                    return 25.0;

                // Add bonus for a DT to kill important counters
                if (unit.getType() == Protoss_Dark_Templar && unit.isWithinReach(target) && !Actions::overlapsDetection(target.unit(), target.getPosition(), PlayerState::Enemy)
                    && (target.getType().isWorker() || target.isSiegeTank() || target.getType().isDetector() || target.getType() == Protoss_Observatory || target.getType() == Protoss_Robotics_Facility))
                    return 20.0;

                // Add bonus for being able to one shot a unit
                if (!Players::ZvZ() && unit.isLightAir() && unit.canOneShot(target) && Util::getTime() < Time(10, 00))
                    return 4.0;

                // Add bonus for being able to two shot a unit
                if (!Players::ZvZ() && unit.isLightAir() && unit.canTwoShot(target) && Util::getTime() < Time(10, 00))
                    return 2.0;
                return 1.0;
            };

            const auto healthScore = [&]() {
                if (range > 32.0 && target.unit()->isCompleted())
                    return 1.0 + (0.1 * (1.0 - target.getPercentTotal()));
                return 1.0;
            };

            const auto focusScore = [&]() {
                if (unit.isLightAir() || Players::ZvZ())
                    return 1.0;

                const auto withinReachHigherRange = range > 32.0 && range >= enemyRange && boxDistance <= reach;
                const auto withinRangeLessRange = range > 32.0 && range < enemyRange && boxDistance <= range;
                const auto withinRangeMelee = range <= 32.0 && boxDistance <= 64.0;

                if (withinReachHigherRange || withinRangeLessRange || withinRangeMelee)
                    return (1.0 + double());
                return 1.0;
            };

            const auto priorityScore = [&]() {
                //if (!target.getType().isWorker() && !target.isThreatening() && ((!target.canAttackAir() && unit.isFlying()) || (!target.canAttackGround() && !unit.isFlying())))
                //    return target.getPriority() / 4.0;
                //if (target.getType().isWorker() && !Spy::enemyProxy() && !Spy::enemyPossibleProxy() && !Terrain::inTerritory(PlayerState::Enemy, target.getPosition()))
                //    return target.getPriority() / 10.0;
                return target.getPriority();
            };

            const auto effectiveness = [&]() {
                auto weapon = target.isFlying() ? unit.getType().airWeapon() : unit.getType().groundWeapon();
                if (weapon.damageType() == DamageTypes::Explosive) {
                    if (target.getType().size() == UnitSizeTypes::Small)
                        return 0.5;
                    if (target.getType().size() == UnitSizeTypes::Medium)
                        return 0.75;
                }
                else if (weapon.damageType() == DamageTypes::Concussive) {
                    if (target.getType().size() == UnitSizeTypes::Medium)
                        return 0.5;
                    if (target.getType().size() == UnitSizeTypes::Large)
                        return 0.25;
                }
                return 1.0;
            };

            const auto targetScore = healthScore() * focusScore() * priorityScore() * bonusScore();

            // Detector targeting (distance to nearest ally added)
            if ((target.isBurrowed() || target.unit()->isCloaked()) && ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Terran_Comsat_Station)) {
                auto closest = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                    return *u != unit && u->getRole() == Role::Combat && target.getType().isFlyer() ? u->getAirRange() > 0 : u->getGroundRange() > 0;
                });

                // Detectors want to stay close to their target if we have a friendly nearby
                if (closest && closest->getPosition().getDistance(target.getPosition()) < 480.0)
                    return priorityScore() / (dist * closest->getPosition().getDistance(target.getPosition()));
                return 0.0;
            }

            // Cluster targeting (grid score added)
            else if (unit.getType() == Protoss_High_Templar || unit.getType() == Protoss_Arbiter) {
                const auto eGrid =      Grids::getGroundDensity(target.getPosition(), PlayerState::Enemy) + Grids::getAirDensity(target.getPosition(), PlayerState::Enemy);
                const auto aGrid =      1.0 + Grids::getGroundDensity(target.getPosition(), PlayerState::Self) + Grids::getAirDensity(target.getPosition(), PlayerState::Self);
                const auto gridScore =  eGrid / aGrid;
                return gridScore * targetScore / dist;
            }

            // Defender targeting (distance not used)
            else if (unit.getRole() == Role::Defender && boxDistance - range <= 16.0)
                return healthScore() * priorityScore() * effectiveness();

            // Lurker burrowed targeting (only distance matters)
            else if (unit.getType() == Zerg_Lurker && unit.isBurrowed())
                return 1.0 / dist;

            // Proximity targeting (targetScore not used)
            else if (unit.getType() == Protoss_Reaver || unit.getType() == Zerg_Ultralisk) {
                if (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir())
                    return 0.1 / dist;
                return 1.0 / dist;
            }
            return targetScore / dist;
        }

        void getSimTarget(UnitInfo& unit, PlayerState pState)
        {
            double distBest = DBL_MAX;
            for (auto &t : Units::getUnits(pState)) {
                UnitInfo &target = *t;
                auto targetCanAttack = ((unit.isFlying() && target.getAirDamage() > 0.0) || (!unit.isFlying() && target.canAttackGround()) || (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine));
                auto unitCanAttack = ((target.isFlying() && unit.getAirDamage() > 0.0) || (!target.isFlying() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));

                if (!targetCanAttack && (!unit.hasTarget() || target != unit.getTarget()))
                    continue;

                if (!target.isWithinReach(unit))
                    continue;

                if (!allowWorkerTarget(unit, target))
                    continue;

                // Set sim position
                auto dist = unit.getPosition().getDistance(target.getPosition()) * (1 + int(!targetCanAttack));
                if (dist < distBest) {
                    unit.setSimTarget(&target);
                    distBest = dist;
                }
            }

            if (!unit.hasSimTarget() && unit.hasTarget())
                unit.setSimTarget(&*unit.getTarget().lock());
        }

        void getBestTarget(UnitInfo& unit, PlayerState pState)
        {
            const auto checkGroundAccess = [&](UnitInfo& target) {
                if (unit.isFlying() || unit.unit()->isLoaded())
                    return true;

                if (target.isFlying() && BWEB::Map::isWalkable(target.getTilePosition(), Zerg_Ultralisk)) {
                    auto grdDist = BWEB::Map::getGroundDistance(unit.getPosition(), target.getPosition());
                    auto airDist = unit.getPosition().getDistance(target.getPosition());
                    if (grdDist > airDist * 2)
                        return false;
                }
                return true;
            };

            const auto isValidTarget = [&](auto &target) {
                if (!target.unit()
                    || (unit.targetsFriendly() && !target.isCompleted())
                    || target.isInvincible()
                    || unit == target
                    || !target.getWalkPosition().isValid())
                    return false;
                return true;
            };

            auto scoreBest = 0.0;
            auto priorityBest = Priority::Ignore;

            for (auto &t : Units::getUnits(pState)) {
                UnitInfo &target = *t;

                if (!isValidTarget(target))
                    continue;

                auto priority = getPriority(unit, target);
                if (priority == Priority::Ignore || priority < priorityBest)
                    continue;

                if (priority > priorityBest) {
                    scoreBest = 0.0;
                    priorityBest = priority;
                }

                // If this target is more important to target, set as current target
                const auto thisUnit = scoreTarget(unit, target);
                if (thisUnit > scoreBest && checkGroundAccess(target)) {
                    scoreBest = thisUnit;
                    unit.setTarget(&target);
                    unit.setSimTarget(&target);
                }
            }

            // Check for any self targets marked for death
            for (auto &t : Units::getUnits(PlayerState::Self)) {
                UnitInfo &target = *t;

                if (!isValidTarget(target)
                    || !target.isMarkedForDeath()
                    || unit.isLightAir())
                    continue;

                auto priority = getPriority(unit, target);
                if (priority == Priority::Ignore || priority < priorityBest)
                    continue;

                if (priority > priorityBest) {
                    scoreBest = 0.0;
                    priorityBest = priority;
                }

                // If this target is more important to target, set as current target
                const auto thisUnit = scoreTarget(unit, target);
                if (thisUnit > scoreBest && checkGroundAccess(target)) {
                    scoreBest = thisUnit;
                    unit.setTarget(&target);
                }
            }

            // Check for any neutral targets marked for death
            for (auto &t : Units::getUnits(PlayerState::Neutral)) {
                UnitInfo &target = *t;

                if (!isValidTarget(target)
                    || !target.isMarkedForDeath()
                    || unit.isLightAir())
                    continue;

                auto priority = getPriority(unit, target);
                if (priority == Priority::Ignore || priority < priorityBest)
                    continue;

                if (priority > priorityBest) {
                    scoreBest = 0.0;
                    priorityBest = priority;
                }

                // If this target is more important to target, set as current target
                const auto thisUnit = scoreTarget(unit, target);
                if (thisUnit > scoreBest && checkGroundAccess(target)) {
                    scoreBest = thisUnit;
                    unit.setTarget(&target);
                }
            }

            // Add this unit to the targeted by vector
            if (unit.hasTarget() && unit.getRole() == Role::Combat)
                unit.getTarget().lock()->getUnitsTargetingThis().push_back(unit.weak_from_this());
        }

        void getTarget(UnitInfo& unit)
        {
            auto pState = unit.targetsFriendly() ? PlayerState::Self : PlayerState::Enemy;

            // Spider mines have a set order target, possibly scarabs too
            if (unit.getType() == Terran_Vulture_Spider_Mine) {
                if (unit.unit()->getOrderTarget())
                    unit.setTarget(&*Units::getUnitInfo(unit.unit()->getOrderTarget()));
            }

            // Self units are assigned targets
            if (unit.getRole() == Role::Combat || unit.getRole() == Role::Support || unit.getRole() == Role::Defender || unit.getRole() == Role::Worker || unit.getRole() == Role::Scout) {
                getBestTarget(unit, pState);
                getSimTarget(unit, PlayerState::Enemy);
            }

            // Enemy units are assumed targets or order targets
            if (unit.getPlayer()->isEnemy(Broodwar->self())) {
                auto &targetInfo = unit.unit()->getOrderTarget() ? Units::getUnitInfo(unit.unit()->getOrderTarget()) : nullptr;
                if (targetInfo) {
                    unit.setTarget(&*targetInfo);
                    targetInfo->getUnitsTargetingThis().push_back(unit.weak_from_this());
                }
                else if (unit.getType() != Terran_Vulture_Spider_Mine) {
                    auto closest = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                        return (u->isFlying() && unit.getAirDamage() > 0.0) || (!u->isFlying() && unit.getGroundDamage() > 0.0);
                    });
                    if (closest)
                        unit.setTarget(&*closest);
                }
            }
        }

        void updateTargets()
        {
            // Sort my units by distance to closest enemy
            multimap<double, UnitInfo&> sortedUnits;
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo& unit = *u;
                auto closestEnemy = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return (u->isFlying() && unit.canAttackAir()) || (!u->isFlying() && unit.canAttackGround()) || (unit.getRole() == Role::Support);
                });
                if (closestEnemy) {
                    const auto dist = unit.getPosition().getDistance(closestEnemy->getPosition());
                    sortedUnits.emplace(dist, unit);
                }
            }

            // TODO: This results in lings seeing 0 empty spots when in range of the target
            // Huge winrate decrease in ZvZ, maybe need to scrap this concept entirely

            //// Check how many available melee spots exist on each enemy
            //meleeSpotsAvailable.clear();
            //for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            //    UnitInfo& unit = *u;
            //    auto width = unit.getType().isBuilding() ? unit.getType().tileWidth() * 16 : unit.getType().width();
            //    auto height = unit.getType().isBuilding() ? unit.getType().tileHeight() * 16 : unit.getType().height();
            //    for (double x = -1.0; x <= 1.0; x += 1.0 / double(unit.getType().tileWidth())) {
            //        auto p = (unit.getPosition()) + Position(int(x * width), int(-1.0 * height));
            //        auto q = (unit.getPosition()) + Position(int(x * width), int(1.0 * height));
            //        if (Util::findWalkable(Position(-16, -16), Zerg_Zergling, p))
            //            meleeSpotsAvailable[&unit]+=2;
            //        if (Util::findWalkable(Position(-16, -16), Zerg_Zergling, q))
            //            meleeSpotsAvailable[&unit]+=2;
            //    }
            //    for (double y = -1.0; y <= 1.0; y += 1.0 / double(unit.getType().tileHeight())) {
            //        if (y <= -0.99 || y >= 0.99)
            //            continue;
            //        auto p = (unit.getPosition()) + Position(int(-1.0 * width), int(y * height));
            //        auto q = (unit.getPosition()) + Position(int(1.0 * width), int(y * height));
            //        if (Util::findWalkable(Position(-16, -16), Zerg_Zergling, p))
            //            meleeSpotsAvailable[&unit]+=2;
            //        if (Util::findWalkable(Position(-16, -16), Zerg_Zergling, q))
            //            meleeSpotsAvailable[&unit]+=2;
            //    }
            //    Broodwar->drawTextMap(unit.getPosition(), "%d", meleeSpotsAvailable[&unit]);
            //}

            for (auto &u : sortedUnits) {
                UnitInfo& unit = u.second;
                getTarget(unit);
            }
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo& unit = *u;
                getTarget(unit);
            }
        }

        void updateStatistics()
        {
            auto &enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto &myStrength = Players::getStrength(PlayerState::Self);

            enemyHasGround = true;
            enemyHasAir = enemyStrength.airToGround > 0.0 || enemyStrength.airToAir > 0.0 || enemyStrength.airDefense > 0.0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Overlord) > 0;
            enemyCanHitAir = enemyStrength.groundToAir > 0.0 || enemyStrength.airToAir > 0.0 || enemyStrength.airDefense > 0.0;
            enemyCanHitGround = enemyStrength.groundToGround > 0.0 || enemyStrength.airToGround > 0.0 || enemyStrength.groundDefense > 0.0;

            selfHasGround = true;
            selfHasAir = myStrength.airToGround > 0.0 || myStrength.airToAir > 0.0 || total(Zerg_Overlord) > 0;
            selfCanHitAir = myStrength.groundToAir > 0.0 || myStrength.airToAir > 0.0 || myStrength.airDefense > 0.0;
            selfCanHitGround = myStrength.groundToGround > 0.0 || myStrength.airToGround > 0.0 || myStrength.groundDefense > 0.0;
        }
    }

    void onFrame()
    {
        updateStatistics();
        updateTargets();
    }
}