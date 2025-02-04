#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UnitCommandTypes;

namespace McRave::Command {

    vector<pair<double, Position>> positionsByCost;

    namespace {

        double grouping(UnitInfo& unit, WalkPosition w) {
            if (unit.isFlying()) {
                if (unit.hasCommander() && (unit.isTargetedBySplash() || unit.isTargetedBySuicide())) {
                    return 1.0 + Position(w).getDistance(unit.getCommander().lock()->getPosition());
                }
            }
            return 1.0;
        }

        double distance(UnitInfo& unit, WalkPosition w) {
            const auto p = Position(w) + Position(4, 4);
            return max(1.0, p.getDistance(unit.getNavigation()));
        }

        double mobility(UnitInfo& unit, WalkPosition w) {
            return unit.isFlying() ? 1.0 : Util::log10(10 + Grids::getMobility(w));
        }

        double threat(UnitInfo& unit, WalkPosition w, float current = 0.0f)
        {
            const auto p = Position(w) + Position(4, 4);
            if (unit.isTransport()) {
                if (p.getDistance(unit.getNavigation()) < 32.0)
                    return max(0.01f, Grids::getGroundThreat(w, PlayerState::Enemy) + Grids::getAirThreat(w, PlayerState::Enemy));
                return max(0.01f, Grids::getAirThreat(w, PlayerState::Enemy));
            }

            if (unit.isHidden()) {
                if (!Actions::overlapsDetection(unit.unit(), p, PlayerState::Enemy))
                    return 0.01;
            }
            return max(0.01f, unit.getType().isFlyer() ? (Grids::getAirThreat(w, PlayerState::Enemy) - current) : (Grids::getGroundThreat(w, PlayerState::Enemy) - current));
        }

        double avoidance(UnitInfo& unit, Position p, Position sim)
        {
            return (p.getDistance(sim));
        }

        double altitude(UnitInfo& unit, WalkPosition w)
        {
            if (unit.isFlying())
                return 1.0 + mapBWEM.GetMiniTile(w).Altitude();
            return 1.0;
        }

        Position findViablePosition(UnitInfo& unit, Position pstart, function<double(WalkPosition)> score)
        {
            // Check if this is a viable position for movement
            const auto viablePosition = [&](Position& p) {
                if (!unit.getType().isFlyer()) {
                    if (Planning::overlapsPlan(unit, p) || !Util::findWalkable(unit, p))
                        return false;
                }
                if (Actions::isInDanger(unit, p))
                    return false;
                return true;
            };

            // Get cost for movement to this position
            const auto cost = [&](Position& p) {
                const auto w = WalkPosition(p);
                auto current = score(w);

                if (unit.isLightAir() && unit.getLocalState() != LocalState::Attack) {
                    auto edgePush = clamp(Terrain::getClosestMapEdge(unit.getPosition()).getDistance(p) / 160.0, 0.01, 1.00);
                    auto cornerPush = clamp(Terrain::getClosestMapCorner(unit.getPosition()).getDistance(p) / 320.0, 0.01, 1.00);
                    current = current * cornerPush * edgePush;
                }

                // TODO: make things a cost rather than score
                return 1.0 / current;
            };

            // Clear the vector and keep the space reserved
            auto radius = 20;
            const auto vectorSize = size_t(radius * radius);
            positionsByCost.clear();
            positionsByCost.reserve(vectorSize);

            // Grab the expected circle representation of this radius
            for (auto &walk : Util::getWalkCircle(radius)) {
                const auto w = WalkPosition(walk) + WalkPosition(unit.getPosition());
                if (w.isValid()) {
                    auto p = Position(w) + Position(4, 4);
                    positionsByCost.push_back(make_pair(cost(p), p));
                }
            }

            // Sort above positions by cost
            sort(positionsByCost.begin(), positionsByCost.end(), [&](auto &p1, auto &p2) {
                return p1.first < p2.first;
            });

            // Iterate ascending cost until a position is viable
            for (auto &[cost, p] : positionsByCost) {
                auto position = p;
                if (viablePosition(position))
                    return position;
            }
            return Positions::Invalid;
        }
    }

    bool misc(UnitInfo& unit)
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
        else if (unit.unit()->isIrradiated()) {
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
        else if (unit.getRole() == Role::Worker && unit.hasResource() && unit.isWithinGatherRange() && (unit.framesHoldingResource() >= 100 || unit.framesHoldingResource() <= -200) && Terrain::inTerritory(PlayerState::Self, unit.getPosition())) {
            auto pylon = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->getType() == UnitTypes::Protoss_Pylon;
            });
            if (pylon && pylon->unit() && pylon->unit()->exists() && pylon->getPosition().getDistance(unit.getPosition()) < 128.0) {
                unit.setCommand(Attack_Unit, *pylon);
                unit.commandText = "AttackMannerPylon";
                return true;
            }
        }

        // Specal case: Holding a chokepoint, don't move
        if (Players::ZvZ() && Combat::holdAtChoke() && Combat::State::isStaticRetreat(unit.getType()) && unit.getPosition().getDistance(unit.getFormation()) <= 2) {
            unit.setCommand(Hold_Position);
            unit.commandText = "HoldPosition";
            return true;
        }
        return false;
    }

    bool attack(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        if (!target.unit()->exists()
            || target.isHidden()
            || !unit.canStartAttack())
            return false;

        const auto shouldAttack = [&]() {
            // Combat will attack when in range unless surrounding
            if (unit.getRole() == Role::Combat) {
                if (unit.attemptingSurround())
                    return false;
                if (unit.isMelee() && unit.isWithinReach(target) && unit.getLocalState() == LocalState::Attack)
                    return true;
                return unit.isWithinRange(target) && unit.getLocalState() == LocalState::Attack;
            }

            // Workers will poke damage if near a build job or is threatning our gathering
            if (unit.getRole() == Role::Worker) {
                if (unit.getBuildPosition().isValid()) {
                    const auto topLeft = Position(unit.getBuildPosition());
                    const auto botRight = topLeft + Position(unit.getBuildType().tileWidth() * 32, unit.getBuildType().tileHeight() * 32);
                    return Util::rectangleIntersect(topLeft, botRight, target.getPosition());
                }

                if (target.isThreatening() && (unit.getUnitsTargetingThis().empty() || unit.isHealthy()) && unit.isWithinRange(target) && unit.isWithinGatherRange() && (Players::ZvZ() || target.getType().isWorker() || Spy::enemyRush()))
                    return true;
            }

            // Defenders will attack when in range
            if (unit.getRole() == Role::Defender)
                return unit.isWithinRange(target);
            return false;
        };

        // If unit can move and should attack
        if (shouldAttack()) {
            unit.setCommand(Attack_Unit, target);
            unit.commandText = "AttackTarget";
            return true;
        }
        return false;
    }

    bool approach(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        const auto shouldApproach = [&]() {
            // Don't approach if
            if (unit.getSpeed() <= 0.0
                || unit.canStartAttack()
                || unit.isCapitalShip()
                || target.isSuicidal()
                || unit.isSpellcaster()
                || unit.getLocalState() == LocalState::Retreat
                || (unit.getType() == Zerg_Lurker && !unit.isBurrowed())
                || (!unit.isTargetedBySplash() && !unit.isTargetedBySuicide() && (target.isSplasher() || target.isSuicidal()))
                || (unit.getGroundRange() <= 32.0 && unit.getAirRange() <= 32.0 && !unit.isHovering())
                || (unit.getType() == Zerg_Hydralisk)
                || (unit.isFlying() && unit.getPosition().getDistance(target.getPosition()) < 32.0))
                return false;

            auto unitRange = (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange());
            auto interceptDistance = unit.getInterceptPosition().getDistance(unit.getPosition());

            return (unit.isTargetedBySuicide())
                || (unit.getInterceptPosition().isValid() && interceptDistance > unit.getPosition().getDistance(target.getPosition()))  // Approach units that are moving away from us
                || (!unit.isLightAir() && unit.getSimValue() >= 5.0 && target.getGroundRange() > 32.0)                                  // If crushing victory, push forward
                || (!unit.isLightAir() && unitRange < enemyRange && !target.getType().isBuilding());                                    // If this units range is lower and target isn't a building
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

    bool move(UnitInfo& unit)
    {
        auto atHome = Terrain::inTerritory(PlayerState::Self, unit.getPosition());
        auto sim = unit.hasSimTarget() ? unit.getSimTarget().lock()->getPosition() : Positions::Invalid;
        auto current = unit.isFlying() ? Grids::getAirThreat(unit.getPosition(), PlayerState::Enemy) : Grids::getGroundThreat(unit.getPosition(), PlayerState::Enemy);
        auto harassing = unit.isLightAir() && !unit.getGoal().isValid() && unit.getDestination() == Combat::getHarassPosition() && unit.attemptingHarass() && unit.getLocalState() == LocalState::None;
        auto regrouping = unit.attemptingRegroup();

        const auto safeMovement = regrouping || (unit.getRole() == Role::Worker && !atHome);

        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            auto score = 0.0;

            if (safeMovement)
                score = mobility(unit, w) * grouping(unit, w) / (distance(unit, w) * threat(unit, w));
            else
                score = mobility(unit, w) * grouping(unit, w) / (distance(unit, w));
            return score;
        };

        auto shouldMove = [&]() {

            // Impossible to move
            if (unit.isBurrowed() || unit.stunned || unit.getSpeed() == 0.0)
                return false;

            // Special case: Mutalisks want to maintain attack angle to maintain speed
            if (unit.hasTarget()) {
                auto &target = *unit.getTarget().lock();
                if (unit.getType() == Zerg_Mutalisk && unit.canStartAttack() && !unit.isWithinAngle(target))
                    return true;
            }

            // Combats should move if we're not retreating
            if (unit.getRole() == Role::Combat) {

                if (unit.attemptingSurround())
                    return true;

                if (unit.hasTarget()) {
                    auto &target = *unit.getTarget().lock();
                    auto doNothing = !unit.targetsFriendly() && target.unit()->exists() && unit.isWithinRange(target);

                    if (doNothing && !unit.attemptingSurround() && !unit.isLightAir() && !unit.isSuicidal())
                        return false;
                }
                return unit.getLocalState() != LocalState::Retreat;
            }

            // Workers should move if they need to get to a new gather or construction job
            if (unit.getRole() == Role::Worker) {
                const auto hasBuildingAssignment = unit.getBuildPosition().isValid() && unit.getBuildType() != UnitTypes::None;

                auto hasMineableResource = false;
                if (unit.hasResource()) {
                    auto &resource = unit.getResource().lock();
                    hasMineableResource = resource->getResourceState() == ResourceState::Mineable;
                }

                return ((hasBuildingAssignment && Workers::shouldMoveToBuild(unit, unit.getBuildPosition(), unit.getBuildType()))
                    || (hasMineableResource && !unit.isWithinGatherRange()))
                    || unit.getGoal().isValid();
            }

            // Scouts and Transports should always move
            if (unit.getRole() == Role::Scout
                || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        // If unit can move and should move
        if (shouldMove()) {

            auto clickToSurround = unit.attemptingSurround() && unit.getPosition().getDistance(unit.getSurroundPosition()) < 160.0;
            if (clickToSurround) {
                unit.setCommand(Move, unit.getSurroundPosition());
                unit.commandText = "Move_S";
                return true;
            }

            // Necessary for mutas to not overshoot when already in range and melee struggling to ever hit anything
            auto clickToTarget = (unit.getRole() == Role::Combat && unit.hasTarget() && unit.canStartAttack() && (unit.isLightAir() || unit.isMelee()) && unit.isWithinReach(*unit.getTarget().lock()) && unit.getLocalState() == LocalState::Attack)                
                || !unit.getDestinationPath().isReachable();

            // Just move to the destination
            if (clickToTarget) {
                auto fixedDest = unit.getDestination();
                Util::findWalkable(unit, fixedDest);
                unit.setCommand(Right_Click_Position, fixedDest);
                unit.commandText = "Move_A";
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

    bool kite(UnitInfo& unit)
    {
        // If we don't have a target, we can't kite it
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        // Find the best possible position to kite towards
        auto kiteTowards = Positions::Invalid;
        if (unit.isLightAir() && Grids::getAirThreat(unit.getPosition(), PlayerState::Enemy) > 0.0) {
            auto maxRange = max(unit.getAirRange(), unit.getGroundRange());
            const auto kiteRange = unit.getType().groundWeapon().damageCooldown() * unit.getType().topSpeed() + 64.0;
            const auto threatCalc = [&](auto &p) {
                return double(Grids::getAirThreat(p, PlayerState::Enemy));
            };
            auto calcPair = Util::findPointOnCircle(unit.getPosition(), target.getPosition(), maxRange, threatCalc);
            kiteTowards = Util::shiftTowards(target.getPosition(), calcPair.second, kiteRange);
        }

        const auto scoreFunction = [&](WalkPosition w) {
            auto score = 0.0;
            if (kiteTowards.isValid())
                score =      (mobility(unit, w) * grouping(unit, w)) / (w.getDistance(WalkPosition(kiteTowards)) * (threat(unit, w)) * altitude(unit, w));
            else
                score =      (mobility(unit, w) * grouping(unit, w)) / ((threat(unit, w)) * altitude(unit, w));
            return score;
        };

        const auto canKite = [&]() {

            // Special Case: Carriers
            if (unit.getType() == UnitTypes::Protoss_Carrier) {
                auto leashRange = 320;
                for (auto &interceptor : unit.unit()->getInterceptors()) {
                    if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() && interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted())
                        return false;
                }
                if (unit.getPosition().getDistance(target.getPosition()) <= leashRange)
                    return true;
                return false;
            }

            // Special Case: High Templars
            if (unit.getType() == UnitTypes::Protoss_High_Templar)
                return !unit.canStartCast(TechTypes::Psionic_Storm, target.getPosition());

            // Special Case: Defilers
            if (unit.getType() == UnitTypes::Zerg_Defiler) {
                if (target.getPlayer() == Broodwar->self())
                    return false;
                else
                    return !unit.canStartCast(TechTypes::Dark_Swarm, target.getPosition()) && !unit.canStartCast(TechTypes::Plague, target.getPosition()) && unit.getPosition().getDistance(target.getPosition()) <= 400.0;
            }

            if (unit.getSpeed() <= 0.0
                || unit.getType() == Zerg_Lurker)
                return false;
            return true;
        };

        const auto shouldKite = [&]() {
            auto allyRange = (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange());
            auto targetKitable = allyRange > enemyRange && enemyRange != 0 && allyRange != 0;
            auto boxDist = Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition());

            if (unit.getRole() == Role::Combat || unit.getRole() == Role::Scout) {

                // Special Case: early "duels"
                if (unit.getType() == Zerg_Zergling) {
                    const auto selfFasterSpeed = (Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1) && !Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost, 1));
                    const auto defenders = com(Zerg_Sunken_Colony) > 0 && Combat::State::isStaticRetreat(unit.getType());

                    if (Util::getTime() < Time(4, 30) && !target.isThreatening() && !unit.getUnitsTargetingThis().empty() && target.getType() == Protoss_Zealot && unit.getHealth() <= 16)
                        return true;
                    if (Util::getTime() < Time(4, 30) && !target.isThreatening() && !Combat::holdAtChoke() && target.isWithinReach(unit) && target.getType() == Zerg_Zergling && unit.getHealth() <= 10)
                        return true;
                }
                if ((unit.getType() == Zerg_Hydralisk || unit.getType() == Protoss_Dragoon) && !target.isFlying()) {
                    if (Util::getTime() < Time(7, 00) && targetKitable && boxDist <= enemyRange + 48.0 && !unit.getUnitsTargetingThis().empty())
                        return true;
                }

                // Situation: Unit is targeted by a stationary unit, so we could kite out of range of it
                if (!unit.isMelee()) {
                    auto targetedByStationary = std::any_of(unit.getUnitsTargetingThis().begin(), unit.getUnitsTargetingThis().end(), [&](auto &t) {
                        auto targeter = t.lock();
                        return targeter->getType().isBuilding() || targeter->getSpeed() <= 0.0;
                    });
                    if (targetedByStationary)
                        return true;
                }
                

                if (unit.isTargetedBySuicide() && !unit.isFlying())
                    return false;

                if (unit.inDanger)
                    return true;

                if (unit.canStartAttack())
                    return false;

                if (target.isSuicidal() && !unit.isFlying())                                                         // Do kite when the target is a suicidal unit
                    return true;

                if (!target.canAttackGround() && !target.canAttackAir() && !unit.getType().isFlyer())      // Don't kite non attackers unless we're a flying unit
                    return false;

                if (unit.getGroundRange() <= 32.0 && unit.getAirRange() <= 32.0)
                    return false;

                // If we're already outside the range of the unit, no point in kiting, do full damage
                if (!unit.isFlying() && !target.isWithinRange(unit) && unit.getType() != Protoss_Dragoon && unit.getType() != Terran_Siege_Tank_Tank_Mode && unit.getType() != Terran_Goliath)
                    return false;

                if (unit.getType() == UnitTypes::Protoss_Reaver                                                                 // Reavers: Always kite after shooting
                    || unit.isLightAir()                                                                                        // Do kite when this is a light air unit
                    || unit.isCapitalShip()                                                                                     // Do kite when this unit is a capital ship
                    || targetKitable
                    || ((!unit.getUnitsTargetingThis().empty() || !unit.isHealthy()) && allyRange == enemyRange))               // Do kite when this unit is being attacked or isn't healthy
                    return true;
            }
            return false;
        };

        if (shouldKite() && canKite()) {

            //// HACK: Drilling with workers. Should add some sort of getClosestResource or fix how PlayerState::Neutral units are stored (we don't store resources in them)
            //if (unit.getType().isWorker() && unit.getRole() == Role::Combat) {
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
            if (bestPosition.isValid()) {
                unit.setCommand(Move, bestPosition);
                unit.commandText = "Kite";
                return true;
            }
        }
        return false;
    }

    bool defend(UnitInfo& unit)
    {
        const auto canDefend = unit.getRole() == Role::Combat;
        const auto shouldDefend = unit.getFormation().isValid() && Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && unit.getGlobalState() != GlobalState::Attack
            && unit.getLocalState() != LocalState::Attack && !unit.isLightAir() && !unit.attemptingRunby();

        if (canDefend && shouldDefend) {
            unit.setCommand(Move, unit.getFormation());
            unit.commandText = "Defend";
            return true;
        }
        return false;
    }

    bool explore(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            auto score = mobility(unit, w) * grouping(unit, w) / (distance(unit, w));
            return score;
        };

        const auto canExplore = unit.getRole() == Role::Scout && unit.getNavigation().isValid() && unit.getSpeed() > 0.0;
        const auto shouldExplore = unit.getLocalState() != LocalState::Attack;

        if (canExplore && shouldExplore) {

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

    bool retreat(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            auto score = 0.0;
            score = (mobility(unit, w) * grouping(unit, w)) / (threat(unit, w) * distance(unit, w));
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

    bool escort(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            auto score = 1.0 / (threat(unit, w) * distance(unit, w));
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

    bool transport(UnitInfo& unit)
    {
        auto closestRetreat = Stations::getClosestRetreatStation(unit);

        const auto scoreFunction = [&](WalkPosition w) {
            auto p = Position(w) + Position(4, 4);

            const auto airDist =        max(1.0, p.getDistance(unit.getDestination()));
            const auto grdDist =        max(1.0, BWEB::Map::getGroundDistance(p, unit.getDestination()));
            const auto dist =           grdDist;
            const auto distRetreat =    p.getDistance(closestRetreat->getBase()->Center());

            if (grdDist == DBL_MAX)
                return 0.0;

            double score = 0.0;
            if (unit.getTransportState() == TransportState::Retreating)
                score = 1.0 / (threat(unit, w) * distRetreat);
            else
                score = 1.0 / (dist);

            for (auto &c : unit.getAssignedCargo()) {
                if (auto &cargo = c.lock()) {

                    // If we're trying to load
                    if (unit.getTransportState() == TransportState::Loading)
                        score = 1.0 / dist;
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
}