#include "Commands.h"

#include "Info/Player/Players.h"
#include "Macro/Planning/Planning.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Micro/Combat/Combat.h"
#include "Micro/Worker/Workers.h"
#include "Strategy/Actions/Actions.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UnitCommandTypes;

namespace McRave::Command {

    vector<pair<double, Position>> positionsByCost;

    struct ScoreContext {
        Position p;
        WalkPosition w;
        std::shared_ptr<UnitInfo> unit;
        std::shared_ptr<UnitInfo> target;
        std::shared_ptr<UnitInfo> commander;
    };

    namespace {

        double grouping(ScoreContext &context)
        {
            // if (context.unit->isFlying()) {
            //    if (context.commander && context.unit->attemptingAvoidance()) {
            //        return 1.0 + pow(max(0.0, context.p.getDistance(context.commander->getCommandPosition()) - 32.0), 2.0);
            //    }
            //}
            return 1.0;
        }

        double angle(ScoreContext &context)
        {
            //
            return 1.0;
        }

        double distance(ScoreContext &context) //
        {
            return max(10.0, context.p.getDistance(context.unit->getNavigation()));
        }

        double mobility(ScoreContext &context) //
        {
            return context.unit->isFlying() ? 1.0 : Util::log10(1 + Grids::getMobility(context.w));
        }

        double threat(ScoreContext &context)
        {
            if (context.unit->isTransport()) {
                if (context.p.getDistance(context.unit->getNavigation()) < 32.0)
                    return max(0.01f, Grids::getGroundThreat(context.w, PlayerState::Enemy) + Grids::getAirThreat(context.w, PlayerState::Enemy));
                return max(0.01f, Grids::getAirThreat(context.w, PlayerState::Enemy));
            }

            if (context.unit->isHidden()) {
                if (!Actions::overlapsDetection(context.unit->unit(), context.p, PlayerState::Enemy))
                    return 0.01;
            }
            return max(0.01f, context.unit->isFlying() ? (Grids::getAirThreat(context.w, PlayerState::Enemy) * Grids::getAirThreat(context.w, PlayerState::Enemy))
                                                       : (Grids::getGroundThreat(context.w, PlayerState::Enemy)));
        }

        double altitude(ScoreContext &context)
        {
            return 1.0;
            // unit.isFlying() ? Util::log10(1 + mapBWEM.GetMiniTile(w).Altitude()) : 1.0;
        }

        Position findViablePosition(UnitInfo &unit, Position pstart, function<double(ScoreContext &)> scoreFunc)
        {
            auto nearestEdge   = Terrain::getClosestMapEdge(unit.getPosition());
            auto nearestCorner = Terrain::getClosestMapCorner(unit.getPosition());
            priority_queue<pair<double, Position>> posQueue;

            // Create the score context
            ScoreContext context;
            context.unit      = unit.shared_from_this();
            context.target    = unit.hasTarget() ? unit.getTarget().lock() : nullptr;
            context.commander = unit.hasCommander() ? unit.getCommander().lock() : nullptr;

            // Check if this is a viable position for movement
            const auto viablePosition = [&](Position p) {
                if (!unit.getType().isFlyer()) {
                    if (Planning::overlapsPlan(unit, p) || !Util::findWalkable(unit, p))
                        return false;
                }
                if (Actions::isInDanger(unit, p))
                    return false;
                return true;
            };

            // Get cost for movement to this position
            const auto scorePosition = [&](Position &p) {
                context.p    = p;
                context.w    = WalkPosition(p);
                auto current = scoreFunc(context);

                if (unit.isLightAir()) {
                    auto edgePush   = clamp(nearestEdge.getDistance(p) / 96.0, 1.0, 5.00);
                    auto cornerPush = clamp(nearestCorner.getDistance(p) / 160.0, 1.0, 5.00);
                    current         = current * cornerPush * edgePush;
                }
                return current;
            };

            // Clear the vector and keep the space reserved
            auto radius           = 20;
            const auto vectorSize = size_t(radius * radius);
            positionsByCost.clear();
            positionsByCost.reserve(vectorSize);

            // Grab the expected circle representation of this radius
            for (auto &walk : Util::getWalkCircle(radius)) {
                const auto w = WalkPosition(walk) + WalkPosition(unit.getPosition());
                if (w.isValid()) {
                    auto p   = Position(w) + Position(4, 4);
                    double s = scorePosition(p);
                    posQueue.push({s, p});
                }
            }

            while (!posQueue.empty()) {
                auto [c, pos] = posQueue.top();
                posQueue.pop();

                if (viablePosition(pos)) {
                    return pos;
                }
            }
            return Positions::Invalid;
        }
    } // namespace

    bool misc(UnitInfo &unit)
    {
        if (unit.getType() == Protoss_Reaver && unit.unit()->getScarabCount() < (5 + (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Reaver_Capacity) * 5)))
            unit.unit()->train(Protoss_Scarab);
        if (unit.getType() == Protoss_Carrier && unit.unit()->getInterceptorCount() < (4 + (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Carrier_Capacity) * 4)))
            unit.unit()->train(Protoss_Interceptor);

        // Unstick a unit
        if (unit.isStuck()) {
            unit.unit()->stop();
            unit.commandText = "Unstuck";
            return true;
        }

        // Run irradiated units away
        else if (unit.unit()->isIrradiated() && unit.getType() != Zerg_Ultralisk) {
            unit.setCommand(Move, Terrain::getClosestMapCorner(unit.getPosition()));
            unit.commandText = "IrradiateSplit";
            return true;
        }

        // If unit has a transport, load into it if we need to
        else if (unit.hasTransport()) {
            if (unit.isRequestingPickup()) {
                unit.setCommand(Right_Click_Unit, *unit.getTransport().lock());
                unit.commandText = "LoadingTransport";
                return true;
            }
        }

        // If unit is potentially stuck, try to find a manner pylon
        else if (unit.getRole() == Role::Worker && unit.hasResource() && unit.isWithinGatherRange() && (unit.framesHoldingResource() >= 100 || unit.framesHoldingResource() <= -200) &&
                 Terrain::inTerritory(PlayerState::Self, unit.getPosition())) {
            auto pylon = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) { return u->getType() == UnitTypes::Protoss_Pylon; });
            if (pylon && pylon->unit() && pylon->unit()->exists() && pylon->getPosition().getDistance(unit.getPosition()) < 128.0) {
                unit.setCommand(Attack_Unit, *pylon);
                unit.commandText = "AttackMannerPylon";
                return true;
            }
        }

        // Run into the reaver when targeted by it
        else if (!unit.isFlying() && unit.isTargetedByType(Protoss_Scarab)) {
            if (unit.hasTarget(); auto target = unit.getTarget().lock()) {
                unit.setCommand(Move, target->getPosition());
                return true;
            }
        }

        // Block vulture runby
        // else if (unit.getType() == Zerg_Zergling && Combat::State::isStaticRetreat(Zerg_Zergling)) {
        //    Visuals::drawLine(unit.getPosition(), unit.getFormation(), Colors::Green);
        //    if (unit.hasTarget(); auto target = unit.getTarget().lock()) {
        //        if (target->getType() == Terran_Vulture && target->isThreatening() && !target->isWithinRange(unit)) {
        //            unit.setCommand(Hold_Position);
        //            return true;
        //        }
        //    }
        //}
        return false;
    }

    bool poke(UnitInfo &unit)
    {
        // if (!unit.isLightAir() || unit.hasCommander())
        //    return false;

        // auto closest = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) { return u->unit()->exists() && !u->isHidden(); });

        // if (closest && unit.isWithinAngle(*closest) && unit.isWithinRange(*closest)) {
        //    unit.setCommand(Attack_Unit, *closest);
        //    unit.commandText = "Poke";
        //    Util::debug("Poke tested");
        //    return true;
        //}
        return false;
    }

    bool attack(UnitInfo &unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        if (!target.unit()->exists() || target.isHidden() || !unit.canStartAttack())
            return false;

        const auto shouldAttack = [&]() {
            // Combat will attack when in range unless surrounding
            if (unit.getRole() == Role::Combat) {
                if (unit.attemptingSurround() || unit.attemptingTrap())
                    return false;
                if (unit.isMelee() && !unit.isHovering() && unit.isWithinReach(target) && unit.getLocalState() == LocalState::Attack)
                    return true;
                if (unit.isLightAir() && !unit.isWithinAngle(target) && unit.getPosition().getDistance(target.getPosition()) > 48.0)
                    return false;
                return unit.isWithinRange(target) && unit.getLocalState() == LocalState::Attack;
            }

            // Workers will poke damage if near a build job or is threatning our gathering
            if (unit.getRole() == Role::Worker) {
                if (unit.getBuildPosition().isValid()) {
                    const auto topLeft  = Position(unit.getBuildPosition());
                    const auto botRight = topLeft + Position(unit.getBuildType().tileWidth() * 32, unit.getBuildType().tileHeight() * 32);
                    return Util::rectangleIntersect(topLeft, botRight, target.getPosition());
                }

                if (auto resource = unit.getResource().lock()) {
                    if (unit.isWithinRange(target) && Util::boxDistance(unit.getType(), unit.getPosition(), Resource_Mineral_Field, resource->getPosition()) <= 16.0) {
                        unit.circle(Colors::Blue);
                        return true;
                    }
                }
            }

            // Defenders will attack when in range
            if (unit.getRole() == Role::Defender)
                return unit.isWithinRange(target);
            return false;
        };

        // If unit can move and should attack
        if (shouldAttack()) {
            unit.setCommand(Attack_Unit, target);
            unit.commandText = "Attack";
            return true;
        }
        return false;
    }

    bool approach(UnitInfo &unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        const auto shouldApproach = [&]() {
            // Don't approach if
            if (unit.getSpeed() <= 0.0 || unit.canStartAttack() || unit.isCapitalShip() || target.isSuicidal() || unit.isSpellcaster() || unit.getLocalState() == LocalState::Retreat ||
                (unit.getType() == Zerg_Lurker && !unit.isBurrowed()) || (!unit.isTargetedBySplash() && !unit.isTargetedBySuicide() && (target.isSplasher() || target.isSuicidal())) ||
                (unit.getGroundRange() <= 32.0 && unit.getAirRange() <= 32.0 && !unit.isHovering()) || (unit.getType() == Zerg_Hydralisk) ||
                (unit.getType().isWorker() && target.getType().isWorker()) || (unit.isHovering() && Util::boxDistance(unit, target) < 16.0) ||
                (unit.isFlying() && Util::boxDistance(unit, target) < 32.0))
                return false;

            auto unitRange         = (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange        = (unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange());
            auto interceptDistance = unit.getInterceptPosition().getDistance(unit.getPosition());

            return (unit.isTargetedBySuicide()) ||
                   (unit.getInterceptPosition().isValid() && interceptDistance > unit.getPosition().getDistance(target.getPosition())) // Approach units that are moving away from us
                   || (!unit.isLightAir() && unit.getSimValue() >= 5.0 && target.getGroundRange() > 32.0)                              // If crushing victory, push forward
                   || (!unit.isLightAir() && unitRange < enemyRange && !target.getType().isBuilding());                                // If this units range is lower and target isn't a building
        };

        // If unit can move and should approach
        if (shouldApproach()) {
            if (!unit.isSuicidal() && !unit.getType().isWorker() && target.getSpeed() < unit.getSpeed()) {
                unit.setCommand(Right_Click_Position, target.getPosition());
                unit.commandText = "Approach";
                return true;
            }
            else {
                unit.setCommand(Move, target.getPosition());
                unit.commandText = "Approach";
                return true;
            }
        }
        return false;
    }

    bool move(UnitInfo &unit)
    {
        auto atHome    = Terrain::isAtHome(unit.getPosition());
        auto sim       = unit.hasSimTarget() ? unit.getSimTarget().lock()->getPosition() : Positions::Invalid;
        auto current   = unit.isFlying() ? Grids::getAirThreat(unit.getPosition(), PlayerState::Enemy) : Grids::getGroundThreat(unit.getPosition(), PlayerState::Enemy);
        auto harassing = unit.isLightAir() && !unit.getGoal().isValid() && unit.getDestination() == Combat::getHarassPosition() && unit.attemptingHarass() && unit.getLocalState() == LocalState::None;

        const auto safeMovement = (unit.getRole() == Role::Worker && !atHome) || (unit.getType() == Zerg_Queen);

        auto target         = unit.hasTarget() ? unit.getTarget().lock() : nullptr;
        const auto surround = target && unit.attemptingSurround() && unit.isWithinReach(*target);

        const auto scoreFunction = [&](ScoreContext &context) {
            auto score = 0.0;

            if (target && surround)
                score = mobility(context) * grouping(context) * Util::fastReciprocal(distance(context));
            else if (safeMovement)
                score = mobility(context) * grouping(context) * Util::fastReciprocal(distance(context) * threat(context));
            else
                score = mobility(context) * grouping(context) * Util::fastReciprocal(distance(context));
            return score;
        };

        auto shouldMove = [&]() {
            // Impossible to move
            if (unit.isBurrowed() || unit.isStunned() || unit.getSpeed() == 0.0)
                return false;

            // Special case: Mutalisks want to maintain attack angle to maintain speed
            if (unit.hasTarget()) {
                auto &target = *unit.getTarget().lock();
                if (unit.getType() == Zerg_Mutalisk && unit.canStartAttack() && !unit.isWithinAngle(target) && unit.getPosition().getDistance(target.getPosition()) > 48.0)
                    return true;
            }

            // Combats should move if we're not retreating
            if (unit.getRole() == Role::Combat) {

                if (unit.attemptingSurround() || unit.attemptingTrap())
                    return true;

                if (unit.hasTarget()) {
                    auto &target   = *unit.getTarget().lock();
                    auto doNothing = !unit.targetsFriendly() && target.unit()->exists() && unit.isWithinRange(target);

                    if (doNothing && !unit.attemptingSurround() && !unit.isLightAir() && !unit.isSuicidal())
                        return false;
                }
                return unit.getLocalState() != LocalState::Retreat;
            }

            // Workers should move if they need to get to a new gather or construction job
            if (unit.getRole() == Role::Worker) {
                return unit.getDestination().isValid();
            }

            // Scouts and Transports should always move
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        // If unit can move and should move
        if (shouldMove()) {

            auto clickToTrap = unit.attemptingTrap() && unit.getPosition().getDistance(unit.getTrapPosition()) < 160.0;
            if (clickToTrap) {
                unit.setCommand(Move, unit.getTrapPosition());
                Visuals::drawCircle(unit.getTrapPosition(), 8, Colors::Purple, true);
                unit.commandText = "Move_T";
                return true;
            }

            auto clickToSurround = unit.attemptingSurround() && unit.getPosition().getDistance(unit.getSurroundPosition()) < 160.0;
            if (clickToSurround) {
                unit.setCommand(Move, unit.getSurroundPosition());
                Visuals::drawLine(unit.getPosition(), unit.getSurroundPosition(), Colors::Orange);
                unit.commandText = "Move_S";
                return true;
            }

            // Find the best position to move to
            auto bestPosition = findViablePosition(unit, unit.getPosition(), scoreFunction);
            if (bestPosition.isValid()) {
                unit.setCommand(Move, bestPosition);
                unit.commandText = "Move_B";
                return true;
            }
            else {
                bestPosition = unit.getDestination();
                unit.setCommand(Move, bestPosition);
                unit.commandText = "Move_C";
                return true;
            }
        }
        return false;
    }

    bool kite(UnitInfo &unit)
    {
        // If we don't have a target, we can't kite it
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        // Get a position away from the target
        auto kiteTowards = Positions::Invalid;
        if (unit.isLightAir()) {
            kiteTowards = Util::shiftTowards(target.getPosition(), unit.getPosition(), 160.0);
        }

        // Find the best possible position to kite towards
        if (unit.isLightAir() && unit.hasCommander() && Grids::getAirThreat(unit.getPosition(), PlayerState::Enemy) > 0.0) {

            auto maxRange       = 160.0;
            auto commander      = unit.getCommander().lock();
            auto commanderAngle = BWEB::Map::getAngle(commander->getPosition(), target.getPosition());

            const auto kiteRange = unit.getType().groundWeapon().damageCooldown() * unit.getType().topSpeed() + 64.0;

            const auto angleDiff = [&](double a, double b) {
                double d = fmod(a - b + M_PI, 2.0 * M_PI);
                if (d < 0)
                    d += 2.0 * M_PI;
                return abs(d - M_PI);
            };

            const auto threatCalc = [&](auto &p) {
                auto threat  = double(Grids::getAirThreat(p, PlayerState::Enemy));
                auto density = double(Grids::getAirDensity(p, PlayerState::Self));
                if (unit.isTargetedByType(Terran_Valkyrie)) {
                    auto angle = BWEB::Map::getAngle(p, target.getPosition());
                    return threat / angleDiff(angle, commanderAngle);
                }
                if (unit.isRangedByType(Protoss_Corsair) || unit.isReachedByType(Protoss_Archon)) {
                    return p.getDistance(unit.getFormation());
                }
                return double(Grids::getAirThreat(p, PlayerState::Enemy));
            };
            auto calcPair = Util::findPointOnCircle(unit.getPosition(), target.getPosition(), maxRange, threatCalc);
            kiteTowards   = calcPair.second;
            Util::shiftTowards(target.getPosition(), calcPair.second, kiteRange);
        }

        // Get current distance
        auto currentDistance = Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()) / 8.0;

        const auto scoreFunction = [&](ScoreContext &context) {
            auto score = 0.0;
            if (kiteTowards.isValid())
                score = (mobility(context) * grouping(context) * angle(context)) * Util::fastReciprocal(context.p.getDistance(kiteTowards) * (threat(context)) * altitude(context));
            else
                score = (mobility(context) * grouping(context) * angle(context) * context.p.getDistance(target.getPosition())) * Util::fastReciprocal((threat(context)) * altitude(context));
            return score;
        };

        const auto canKite = [&]() {
            // Special Case: Carriers
            if (unit.getType() == Protoss_Carrier) {
                auto leashRange = 320;
                for (auto &interceptor : unit.unit()->getInterceptors()) {
                    if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() &&
                        interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted())
                        return false;
                }
                if (unit.getPosition().getDistance(target.getPosition()) <= leashRange)
                    return true;
                return false;
            }

            // Special Case: Casters
            if (unit.getPosition().getDistance(target.getPosition()) <= 400.0) {
                if (unit.getType() == Protoss_High_Templar)
                    return !unit.canStartCast(TechTypes::Psionic_Storm, target.getPosition());
                if (unit.getType() == Zerg_Queen) {
                    return !unit.canStartCast(TechTypes::Spawn_Broodlings, target.getPosition());
                }
                if (unit.getType() == Zerg_Defiler) {
                    if (target.getPlayer() == Broodwar->self())
                        return false;
                    else
                        return !unit.canStartCast(TechTypes::Dark_Swarm, target.getPosition()) && !unit.canStartCast(TechTypes::Plague, target.getPosition()) &&
                               unit.getPosition().getDistance(target.getPosition()) <= 400.0;
                }
            }

            if (unit.getSpeed() <= 0.0 || unit.getType() == Zerg_Lurker)
                return false;
            return true;
        };

        const auto shouldKite = [&]() {
            auto allyRange     = (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange    = (unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange());
            auto targetKitable = allyRange > enemyRange && enemyRange != 0 && allyRange != 0;
            auto boxDist       = Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition());

            // Special Case: workers trying to not die
            if (unit.getType().isWorker() && Spy::getEnemyTransition() == U_WorkerRush && vis(Zerg_Sunken_Colony) == 0 && !unit.getUnitsInReachOfThis().empty() && unit.getHealth() < unit.getType().maxHitPoints() && !unit.unit()->isCarryingMinerals())
                return true;

            if (unit.getRole() == Role::Combat || unit.getRole() == Role::Scout) {

                // Special Case: early "duels"
                if (unit.getType() == Zerg_Zergling && target.unit()->exists()) {
                    const auto selfFasterSpeed = (Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1) &&
                                                  !Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost, 1));
                    const auto defenders       = com(Zerg_Sunken_Colony) > 0 && Combat::State::isStaticRetreat(unit.getType());

                    if (Players::ZvP() && target.getType() == Protoss_Zealot) {
                        if (Util::getTime() < Time(4, 30) && Players::ZvP() && unit.getHealth() <= 16 && com(Zerg_Sunken_Colony) > 0)
                            return true;
                        if (Util::getTime() < Time(3, 30) && Players::ZvP() && unit.getHealth() <= 16)
                            return true;
                    }

                    if (Util::getTime() < Time(3, 30) && !target.isThreatening() && !Combat::holdAtChoke() && target.isWithinReach(unit) && target.getType() == Zerg_Zergling && unit.getHealth() <= 10)
                        return true;
                }

                // Special Case: runby units try to stay alive
                if (unit.getType() == Zerg_Zergling && (Players::ZvT() || Players::ZvP())) {
                    if (unit.attemptingRunby() && unit.getHealth() < 16 && !unit.getUnitsTargetingThis().empty())
                        return true;
                }

                // Special Case: Hydras only kite when units almost in range due to GrdAttkInit being long
                if (unit.getType() == Zerg_Hydralisk && !target.isFlying()) {
                    if (Util::getTime() < Time(7, 00) && targetKitable && boxDist <= enemyRange + 48.0 && !unit.getUnitsTargetingThis().empty())
                        return true;
                }

                // Special Case: Kite suicidal units
                if (Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) == 0) {
                    if (unit.isLightAir() && unit.isTargetedBySuicide() && Util::boxDistance(unit, target) < 72.0)
                        return true;
                    if (unit.isLightAir() && target.isSuicidal() && Util::boxDistance(unit, target) < 48.0)
                        return true;
                }

                // Situation: Unit is targeted by a stationary unit, so we could kite out of range of it
                // if (!unit.isMelee()) {
                //    auto targetedByStationary = std::any_of(unit.getUnitsTargetingThis().begin(), unit.getUnitsTargetingThis().end(), [&](auto &t) {
                //        auto targeter = t.lock();
                //        return targeter->getType().isBuilding() || targeter->getSpeed() <= 0.0;
                //    });
                //    if (targetedByStationary)
                //        return true;
                //}

                if (unit.isTargetedBySuicide() && !unit.isFlying())
                    return false;

                if (unit.inDanger)
                    return true;

                if (unit.canStartAttack())
                    return false;

                if (target.isSuicidal() && !unit.isFlying()) // Do kite when the target is a suicidal unit
                    return true;

                if (!target.canAttackGround() && !target.canAttackAir() && !unit.getType().isFlyer()) // Don't kite non attackers unless we're a flying unit
                    return false;

                if (unit.getGroundRange() <= 32.0 && unit.getAirRange() <= 32.0)
                    return false;

                // If we're already outside the range of the unit, no point in kiting, do full damage
                if (!unit.isFlying() && !target.isWithinRange(unit) && unit.getType() == Zerg_Hydralisk)
                    return false;

                if (unit.getType() == UnitTypes::Protoss_Reaver                                                                    // Reavers: Always kite after shooting
                    || unit.isLightAir()                                                                                           // Do kite when this is a light air unit
                    || unit.isCapitalShip()                                                                                        // Do kite when this unit is a capital ship
                    || targetKitable || ((!unit.getUnitsTargetingThis().empty() || !unit.isHealthy()) && allyRange == enemyRange)) // Do kite when this unit is being attacked or isn't healthy
                    return true;
            }
            return false;
        };

        if (shouldKite() && canKite()) {

            //// HACK: Drilling with workers. Should add some sort of getClosestResource or fix how PlayerState::Neutral units are stored (we don't store resources in them)
            // if (unit.getType().isWorker() && unit.getRole() == Role::Combat) {
            //    auto closestMineral = Broodwar->getClosestUnit(target.getPosition(), Filter::IsMineralField, 32);

            //    if (closestMineral && closestMineral->exists()) {
            //        unit.unit()->gather(closestMineral);
            //        return true;
            //    }
            //    if (unit.hasResource()) {
            //        unit.unit()->gather(unit.getResource().lock()->unit());
            //        return true;
            //    }
            //    return false;
            //}

            // If we found a valid position, move to it
            auto bestPosition = findViablePosition(unit, unit.getPosition(), scoreFunction);
            Visuals::drawLine(unit.getPosition(), bestPosition, Colors::Purple);
            if (bestPosition.isValid()) {
                unit.setCommand(Move, bestPosition);
                unit.commandText = "Kite";
                return true;
            }
        }
        return false;
    }

    bool defend(UnitInfo &unit)
    {
        const auto canDefend    = unit.getRole() == Role::Combat;
        const auto shouldDefend = unit.getFormation().isValid() && Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && unit.getGlobalState() != GlobalState::Attack &&
                                  unit.getLocalState() != LocalState::Attack && !unit.isLightAir() && !unit.attemptingRunby();

        if (canDefend && shouldDefend) {
            unit.setCommand(Move, unit.getFormation());
            unit.commandText = "Defend";
            return true;
        }
        return false;
    }

    bool explore(UnitInfo &unit)
    {
        const auto scoreFunction = [&](ScoreContext &context) {
            auto score = mobility(context) * grouping(context) * Util::fastReciprocal(distance(context));
            return score;
        };

        const auto canExplore    = unit.getRole() == Role::Scout && unit.getNavigation().isValid() && unit.getSpeed() > 0.0;
        const auto shouldExplore = unit.getLocalState() != LocalState::Attack;

        if (canExplore && shouldExplore) {

            if (unit.getPosition().getDistance(unit.getDestination()) < 32.0) {
                unit.setCommand(Move, unit.getDestination());
                unit.commandText = "Explore";
                return true;
            }

            auto bestPosition = findViablePosition(unit, unit.getPosition(), scoreFunction);

            if (bestPosition.isValid()) {
                unit.setCommand(Move, bestPosition);
                unit.commandText = "Explore";
                return true;
            }
            else {
                unit.setCommand(Move, unit.getDestination());
                unit.commandText = "Explore";
                return true;
            }
        }
        return false;
    }

    bool retreat(UnitInfo &unit)
    {
        const auto scoreFunction = [&](ScoreContext &context) {
            auto score = 0.0;
            score      = (mobility(context) * grouping(context)) * Util::fastReciprocal(distance(context));
            return score;
        };

        const auto canRetreat = [&]() {
            if (unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        const auto shouldRetreat = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.getLocalState() == LocalState::Retreat)
                    return true;
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        if (canRetreat() && shouldRetreat()) {
            auto bestPosition = findViablePosition(unit, unit.getNavigation(), scoreFunction);
            if (bestPosition.isValid()) {
                unit.setCommand(Move, bestPosition);
                unit.commandText = "Retreat";
                return true;
            }
            else {
                unit.setCommand(Move, Stations::getClosestRetreatStation(unit)->getBase()->Center());
                unit.commandText = "Retreat";
                return true;
            }
        }
        return false;
    }

    bool escort(UnitInfo &unit)
    {
        const auto scoreFunction = [&](ScoreContext &context) {
            auto score = 1.0 * Util::fastReciprocal(threat(context) * distance(context));
            return score;
        };

        // Escorting
        auto shouldEscort = (unit.getRole() == Role::Support || unit.getRole() == Role::Transport);
        if (!shouldEscort)
            return false;

        // Try to save on APM
        if (unit.getPosition().getDistance(unit.getDestination()) < 32.0)
            return false;

        if (unit.getPosition().getDistance(unit.getDestination()) < 64.0) {
            unit.setCommand(Right_Click_Position, unit.getDestination());
            unit.commandText = "Escort";
            return true;
        }

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, unit.getPosition(), scoreFunction);
        if (bestPosition.isValid()) {
            unit.setCommand(Move, bestPosition);
            unit.commandText = "Escort";
            return true;
        }
        return false;
    }

    bool transport(UnitInfo &unit)
    {
        auto closestRetreat = Stations::getClosestRetreatStation(unit);

        const auto scoreFunction = [&](ScoreContext &context) {
            const auto grdDist     = max(1.0, BWEB::Map::getGroundDistance(context.p, unit.getDestination()));
            const auto distRetreat = context.p.getDistance(closestRetreat->getBase()->Center());

            if (grdDist == DBL_MAX)
                return 0.0;

            double score = 0.0;
            if (unit.getTransportState() == TransportState::Retreating)
                score = 1.0 * Util::fastReciprocal(threat(context) * distRetreat);
            else
                score = 1.0 * Util::fastReciprocal(grdDist);

            for (auto &c : unit.getAssignedCargo()) {
                if (auto &cargo = c.lock()) {

                    // If we're trying to load
                    if (unit.getTransportState() == TransportState::Loading)
                        score = 1.0 * Util::fastReciprocal(grdDist);
                }
            }

            return score;
        };

        auto bestPosition = findViablePosition(unit, unit.getNavigation(), scoreFunction);
        if (bestPosition.isValid()) {
            unit.setCommand(Move, bestPosition);
            unit.commandText = "Transport";
            return true;
        }
        return false;
    }
} // namespace McRave::Command