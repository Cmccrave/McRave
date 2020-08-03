#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UnitCommandTypes;

namespace McRave::Command {

    namespace {

        double defaultGrouping(UnitInfo& unit, WalkPosition w) {
            return unit.getType().isFlyer() ?
                ((unit.isNearSplash() || unit.isTargetedBySuicide()) ? clamp(Grids::getAAirCluster(w), 0.01f, 1.0f) : clamp(-1.0f / (log(Grids::getAAirCluster(w))), 0.001f, 100.0f))
                : clamp(log(Grids::getAGroundCluster(w)), 0.05f, 0.5f);
        }

        double defaultDistance(UnitInfo& unit, WalkPosition w) {
            const auto p = Position(w) + Position(4, 4);
            return max(1.0, p.getDistance(unit.getDestination()));
        }

        double defaultVisited(UnitInfo& unit, WalkPosition w) {
            return log(clamp(double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w)), 100.0, 1000.0));
        }

        double defaultVisible(UnitInfo& unit, WalkPosition w) {
            return log(clamp(double(Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(w))), 100.0, 1000.0));
        }

        double defaultMobility(UnitInfo& unit, WalkPosition w) {
            return unit.isFlying() ? 1.0 : log(100.0 + double(Grids::getMobility(w)));
        }

        double defaultThreat(UnitInfo& unit, WalkPosition w)
        {
            const auto p = Position(w) + Position(4, 4);
            if (unit.isTransport()) {
                if (p.getDistance(unit.getDestination()) < 32.0)
                    return max(MIN_THREAT, Grids::getEGroundThreat(w) + Grids::getEAirThreat(w));
                return max(MIN_THREAT, Grids::getEAirThreat(w));
            }

            if (unit.isHidden()) {
                if (!Actions::overlapsDetection(unit.unit(), p, PlayerState::Enemy))
                    return MIN_THREAT;
            }
            return max(MIN_THREAT, unit.getType().isFlyer() ? Grids::getEAirThreat(w) : Grids::getEGroundThreat(w));
        }

        Position findViablePosition(UnitInfo& unit, function<double(WalkPosition)> score)
        {
            const auto closeCorner = [&](const Position& p) {
                return p.getDistance(Position(0, 0)) < 160.0
                    || p.getDistance(Position(Broodwar->mapWidth() * 32, 0)) < 160.0
                    || p.getDistance(Position(0, Broodwar->mapHeight() * 32)) < 160.0
                    || p.getDistance(Position(Broodwar->mapWidth() * 32, Broodwar->mapHeight() * 32)) < 160.0;
            };

            const auto viablePosition = [&](const WalkPosition& w, const Position& p) {

                // If not a flyer and position blocks a building, is in splash threat or is not walkable
                if (!unit.getType().isFlyer() || unit.getRole() == Role::Transport) {
                    if (Buildings::overlapsQueue(unit, p) || Grids::getESplash(w) > 0 || Grids::getMobility(w) < 1 || !Util::isTightWalkable(unit, p))
                        return false;
                }

                // If flyer close to a corner
                if (unit.getType().isFlyer() && closeCorner(p))
                    return false;

                // If in danger
                if (Actions::isInDanger(unit, p))
                    return false;
                return true;
            };

            // Check to see if we're close to a building or bad terrain and need a bit more tiles to look at
            auto moreTiles = false;
            if (!unit.getType().isFlyer()) {
                const vector<TilePosition> directions ={ {-1,-1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}, {-2,-2}, {-2, 0}, {-2, 2}, {0, -2}, {0, 2}, {2, -2}, {2, 0}, {2, 2} };
                for (auto &direction : directions) {
                    const auto tile = unit.getTilePosition() + direction;
                    if (!tile.isValid() || BWEB::Map::isUsed(tile) != UnitTypes::None || !BWEB::Map::isWalkable(tile, unit.getType())) {
                        moreTiles = true;
                        break;
                    }
                }
            }

            // Find bounding box to score WalkPositions
            auto bestPosition = Positions::Invalid;
            auto best = 0.0;
            const auto start = unit.getWalkPosition();
            const auto radius = unit.getType().isFlyer() ? 8 : 8 + (8 * moreTiles);

            // Create bounding box, keep units outside a tile of the edge of the map if it's a flyer
            const auto left = max(0, start.x - radius)/* + (2 * unit.getType().isFlyer())*/;
            const auto right = min(Broodwar->mapWidth() * 4, start.x + radius + unit.getWalkWidth())/* - (2 * unit.getType().isFlyer())*/;
            const auto up = max(0, start.y - radius)/* + (2 * unit.getType().isFlyer())*/;
            const auto down = min(Broodwar->mapHeight() * 4, start.y + radius + unit.getWalkHeight())/* - (2 * unit.getType().isFlyer())*/;

            // Iterate
            for (auto x = left; x < right; x++) {
                for (auto y = up; y < down; y++) {
                    const WalkPosition w(x, y);
                    const Position p = Position(w) + Position(4, 4);
                    const auto current = score(w);

                    if (current > best && viablePosition(w, p)) {
                        best = current;
                        bestPosition = p;
                    }
                }
            }
            return bestPosition;
        }
    }

    bool misc(UnitInfo& unit)
    {
        // Unstick a unit
        if (unit.isStuck()) {
            unit.command(Stop, unit.getPosition());
            return true;
        }

        // Run irradiated units away
        else if (unit.unit()->isIrradiated()) {
            unit.command(Move, Terrain::getClosestMapCorner(unit.getPosition()));
            return true;
        }

        // DT vs spider mines
        else if (unit.getType() == UnitTypes::Protoss_Dark_Templar  && unit.hasTarget() && !unit.getTarget().isHidden() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
            if (unit.canStartAttack()) {
                unit.command(Attack_Unit, unit.getTarget());
                return true;
            }
            return false;
        }

        // If unit has a transport, load into it if we need to
        else if (unit.hasTransport()) {
            if (unit.isRequestingPickup()) {
                unit.command(Right_Click_Unit, unit.getTransport());
                return true;
            }
        }

        // Units targeted by splash need to move away from the army
        else if (unit.hasTarget() && Units::getSplashTargets().find(unit.unit()) != Units::getSplashTargets().end()) {
            if (unit.canStartAttack() && unit.getTarget().unit()->exists())
                unit.command(Attack_Unit, unit.getTarget());
            else
                unit.command(Move, unit.getTarget().getPosition());
            return true;
        }

        // If unit is potentially stuck, try to find a manner pylon
        else if (unit.getRole() == Role::Worker && (unit.framesHoldingResource() >= 100 || unit.framesHoldingResource() <= -200)) {
            auto &pylon = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u.getType() == UnitTypes::Protoss_Pylon;
            });
            if (pylon && pylon->unit() && pylon->unit()->exists() && pylon->getPosition().getDistance(unit.getPosition()) < 128.0) {
                unit.command(Attack_Unit, *pylon);
                return true;
            }
        }
        return false;
    }

    bool attack(UnitInfo& unit)
    {
        // If we have no target or it doesn't exist, we can't attack
        if (!unit.hasTarget()
            || !unit.getTarget().unit()->exists()
            || unit.getTarget().isHidden())
            return false;

        // Can the unit execute an attack command
        const auto canAttack = [&]() {
            return unit.canStartAttack();
        };

        // Should the unit execute an attack command
        const auto shouldAttack = [&]() {

            if (unit.getRole() == Role::Combat) {
                if (unit.isLightAir() && unit.getPosition().getDistance(Combat::getAirClusterCenter()) < 96.0 && unit.getPosition().getDistance(Combat::getAirClusterCenter()) > 32.0)
                    return false;

                if (unit.getEngagePosition().isValid()) {
                    auto threatAtEngagement = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());
                    return unit.isWithinRange(unit.getTarget()) && unit.isWithinAngle(unit.getTarget()) && (unit.getLocalState() == LocalState::Attack || !threatAtEngagement);
                }
            }

            if (unit.getRole() == Role::Scout)
                return false; //unit.getTargetedBy().size() <= 1 && (unit.isWithinRange(unit.getTarget()) || unit.getTarget().getType().isBuilding()) && unit.getPercentTotal() >= unit.getTarget().getPercentTotal();

            if (unit.getRole() == Role::Worker) {
                if (unit.getBuildPosition().isValid()) {
                    const auto topLeft = Position(unit.getBuildPosition());
                    const auto botRight = topLeft + Position(unit.getBuildType().tileWidth() * 32, unit.getBuildType().tileHeight() * 32);
                    return unit.hasTarget() && Util::rectangleIntersect(topLeft, botRight, unit.getTarget().getPosition());
                }
            }
            return false;
        };

        // If unit should and can be attacking
        if (shouldAttack() && canAttack()) {
            unit.command(Attack_Unit, unit.getTarget());
            return true;
        }
        return false;
    }

    bool approach(UnitInfo& unit)
    {
        // If we don't have a target or the targets position is invalid, we can't approach it
        if (!unit.hasTarget()
            || !unit.getTarget().getPosition().isValid())
            return false;

        // Can the unit approach its target
        const auto canApproach = [&]() {
            if (unit.getSpeed() <= 0.0
                || unit.canStartAttack()
                || unit.getType() == Zerg_Lurker)
                return false;
            return true;
        };

        // Should the unit approach its target
        const auto shouldApproach = [&]() {
            auto unitRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

            if (unit.getType().isWorker())
                return true;

            if (unit.getRole() == Role::Combat) {

                auto intercept = Util::getInterceptPosition(unit);
                auto interceptDistance = intercept.getDistance(unit.getPosition());

                // Capital ships want to stay at max range due to their slow nature
                if (unit.isCapitalShip() || unit.getTarget().isSuicidal() || unit.isSpellcaster() || unit.getLocalState() == LocalState::Retreat)
                    return false;

                if (unit.isFlying() && unit.getPosition().getDistance(unit.getTarget().getPosition()) < 32.0)
                    return false;

                // Approach units that are moving away from us
                if ((!unit.isLightAir() || unit.getTarget().isFlying()) && interceptDistance > unit.getPosition().getDistance(unit.getTarget().getPosition()))
                    return true;

                // HACK: Dragoons shouldn't approach Vultures before range upgrade
                if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                    return false;

                // If crushing victory, push forward
                if (!unit.isLightAir() && unit.getSimValue() >= 5.0 && !unit.getTarget().getType().isWorker())
                    return true;
            }

            // If this units range is lower and target isn't a building
            if (!unit.isLightAir() && unitRange < enemyRange && !unit.getTarget().getType().isBuilding())
                return true;
            return false;
        };

        if (shouldApproach() && canApproach()) {
            unit.command(Move, unit.getTarget().getPosition());
            return true;
        }
        return false;
    }

    bool move(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto threat =     defaultThreat(unit, w);
            const auto distance =   defaultDistance(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      unit.getRole() == Role::Worker ? mobility / (distance * grouping * threat) : mobility / (distance * grouping);
            return score;
        };

        auto canMove = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        auto shouldMove = [&]() {
            if (unit.getRole() == Role::Combat)
                return unit.getLocalState() != LocalState::Retreat;
            else
                return true;
            return false;
        };

        // If unit can move and should move
        if (canMove() && shouldMove()) {
            if (unit.hasTarget() && unit.canStartAttack() && unit.isWithinReach(unit.getTarget()) && unit.getLocalState() == LocalState::Attack) {
                unit.command(Move, unit.getTarget().getPosition());
                return true;
            }

            // Find the best position to move to
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(Move, bestPosition);
                return true;
            }
            else {
                bestPosition = unit.getDestination();
                unit.command(Move, bestPosition);
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

        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            const auto distance =   unit.hasTarget() ? 1.0 / log(p.getDistance(unit.getTarget().getPosition())) : p.getDistance(BWEB::Map::getMainPosition());
            const auto threat =     defaultThreat(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      mobility / (threat * distance * grouping);
            return score;
        };

        const auto canKite = [&]() {

            if (unit.getRole() == Role::Worker)
                return true;

            // Special Case: Carriers
            if (unit.getType() == UnitTypes::Protoss_Carrier) {
                auto leashRange = 320;
                for (auto &interceptor : unit.unit()->getInterceptors()) {
                    if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() && interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted())
                        return false;
                }
                if (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= leashRange)
                    return true;
                return false;
            }

            // Special Case: High Templars
            if (unit.getType() == UnitTypes::Protoss_High_Templar)
                return !unit.canStartCast(TechTypes::Psionic_Storm);

            if (unit.getSpeed() <= 0.0
                || unit.canStartAttack()
                || unit.getType() == Zerg_Lurker)                                                                           // Lurkers: Can't kite
                return false;
            return true;
        };

        const auto shouldKite = [&]() {
            auto allyRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

            if (unit.getType().isWorker() && Terrain::isDefendNatural())
                return false;

            if (unit.getRole() == Role::Combat || unit.getRole() == Role::Scout) {
                auto intercept = Util::getInterceptPosition(unit);
                auto interceptDistance = intercept.getDistance(unit.getPosition());

                if (unit.getTarget().isSuicidal())                                                                             // Do kite when the target is a suicidal unit
                    return true;

                if (!unit.getTarget().canAttackGround() && !unit.getTarget().canAttackAir() && !unit.getType().isFlyer())      // Don't kite buildings unless we're a flying unit
                    return false;

                if (unit.getType() == UnitTypes::Protoss_Reaver                                                                 // Reavers: Always kite after shooting
                    || unit.isLightAir()                                                                                        // Do kite when this is a light air unit
                    || allyRange > enemyRange                                                                                   // Do kite when this units range is higher than its target
                    || unit.isCapitalShip()                                                                                     // Do kite when this unit is a capital ship
                    || ((!unit.getTargetedBy().empty() || !unit.isHealthy()) && allyRange == enemyRange))                       // Do kite when this unit is being attacked or isn't healthy
                    return true;
            }
            return false;
        };

        if (shouldKite() && canKite()) {

            // HACK: Drilling with workers. Should add some sort of getClosestResource or fix how PlayerState::Neutral units are stored (we don't store resources in them)
            if (unit.getType().isWorker() && unit.getRole() == Role::Combat) {
                auto closestMineral = Broodwar->getClosestUnit(unit.getTarget().getPosition(), Filter::IsMineralField, 32);

                if (closestMineral && closestMineral->exists()) {
                    unit.unit()->gather(closestMineral);
                    return true;
                }
                if (unit.hasResource()) {
                    unit.unit()->gather(unit.getResource().unit());
                    return true;
                }
            }

            // If we aren't defending the choke, we just want to move to our mineral hold position
            if (!Strategy::defendChoke()) {
                unit.command(Move, Terrain::getDefendPosition());
                return true;
            }

            // If we found a valid position, move to it
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(Move, bestPosition);
                return true;
            }
        }
        return false;
    }

    bool defend(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto distance =   defaultDistance(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      mobility / distance;
            return score;
        };

        bool closeToMainChoke = Position(BWEB::Map::getMainChoke()->Center()).getDistance(unit.getPosition()) < 320.0;
        bool closeToNaturalChoke = Position(BWEB::Map::getNaturalChoke()->Center()).getDistance(unit.getPosition()) < 320.0;
        bool defendingExpansion = unit.getDestination().isValid() && !Terrain::isInEnemyTerritory((TilePosition)unit.getDestination());
        bool closeToDefend = Terrain::isInAllyTerritory(unit.getTilePosition()) || (closeToMainChoke || closeToNaturalChoke) || unit.getType().isWorker();

        if (!closeToDefend
            || unit.getLocalState() == LocalState::Attack
            || unit.getGlobalState() == GlobalState::Attack
            || (unit.hasTarget() && unit.getTarget().isHidden() && unit.getTarget().isWithinReach(unit))
            || (unit.hasTarget() && unit.getTarget().isSiegeTank()))
            return false;

        // Probe Cannon surround
        if (unit.getType().isWorker() && vis(UnitTypes::Protoss_Photon_Cannon) > 0) {
            auto &cannon = Util::getClosestUnitGround(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                return u.getType() == UnitTypes::Protoss_Photon_Cannon;
            });

            if (cannon) {
                auto distBest = DBL_MAX;
                auto walkBest = WalkPositions::Invalid;
                auto start = cannon->getWalkPosition();
                for (int x = start.x - 4; x < start.x + 12; x++) {
                    for (int y = start.y - 4; y < start.y + 12; y++) {
                        WalkPosition w(x, y);
                        Position p(w);
                        double dist = p.getDistance(mapBWEM.Center());

                        // This comes from our concave check, need a solution other than this
                        if (!w.isValid()
                            || Actions::overlapsActions(unit.unit(), p, unit.getType(), PlayerState::Self, 16)
                            || Buildings::overlapsQueue(unit, p)
                            || !Util::isTightWalkable(unit, p))
                            continue;

                        if (dist < distBest) {
                            distBest = dist;
                            walkBest = w;
                        }
                    }
                }
                if (walkBest.isValid()) {
                    unit.command(Move, Position(walkBest));
                    return true;
                }
            }
        }

        // Flyers defend above closest base to enemy
        if (unit.getType().isFlyer()) {
            if (Terrain::getEnemyStartingPosition().isValid() && BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition()) < BWEB::Map::getNaturalPosition().getDistance(Terrain::getEnemyStartingPosition()))
                unit.command(Move, BWEB::Map::getMainPosition());
            else
                unit.command(Move, BWEB::Map::getNaturalPosition());
            return true;
        }

        else if (Strategy::defendChoke()) {

            // Find the best position to move to
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(Move, bestPosition);
                return true;
            }
            else {
                bestPosition = unit.getDestination();
                unit.command(Move, bestPosition);
                return true;
            }
        }
        else {
            unit.command(Move, Terrain::getDefendPosition());
            return true;
        }
        return false;
    }

    bool explore(UnitInfo& unit)
    {
        auto unitThreat = defaultThreat(unit, unit.getWalkPosition());

        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            const auto threat =     defaultThreat(unit, w);
            const auto distance =   defaultDistance(unit, w);
            const auto visible =    defaultVisited(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);

            const auto score =      mobility * visible / (exp(threat) * distance * grouping);

            return score;
        };

        const auto canHunt = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        const auto shouldHunt = [&]() {
            if (unit.getRole() == Role::Combat)
                return false;
            else
                return true;
            return false;
        };

        if (shouldHunt() && canHunt()) {

            // See if there's a way to mineral walk
            if (unit.getType().isWorker() && Terrain::isIslandMap()) {
                for (auto &b : Resources::getMyBoulders()) {
                    auto &boulder = *b;

                    if (boulder.unit()->exists() && boulder.getPosition().getDistance(unit.getPosition()) > 64.0 && boulder.getPosition().getDistance(unit.getDestination()) < unit.getPosition().getDistance(unit.getDestination()) - 64.0) {
                        unit.unit()->gather(boulder.unit());
                        return true;
                    }
                }
            }

            if (unit.getType() == Zerg_Overlord && !Terrain::foundEnemy() && Players::getStrength(PlayerState::Enemy).groundToAir <= 0.0 && Players::getStrength(PlayerState::Enemy).airDefense <= 0.0) {
                unit.command(Move, unit.getDestination());
                return true;
            }

            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(Move, bestPosition);
                return true;
            }
            else {
                unit.command(Move, unit.getDestination());
                return true;
            }
        }
        return false;
    }

    bool retreat(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            const auto distance =   defaultDistance(unit, w);
            const auto threat =     defaultThreat(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      mobility / (threat * grouping * distance);
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
                if (Util::getTime() < Time(4, 00)) {
                    auto possibleDamage = 0;
                    for (auto &attacker : unit.getTargetedBy()) {
                        if (attacker.lock())
                            possibleDamage+= int(attacker.lock()->getGroundDamage());
                    }
                    if (possibleDamage >= unit.getHealth() + unit.getShields())
                        return true;
                }
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        if (canRetreat() && shouldRetreat()) {

            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(Move, bestPosition);
                return true;
            }
            else if (Combat::getClosestRetreatPosition(unit.getPosition()).isValid()) {
                unit.command(Move, Combat::getClosestRetreatPosition(unit.getPosition()));
                return true;
            }
        }
        return false;
    }

    bool escort(UnitInfo& unit)
    {
        auto closestDefense = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
            return u.canAttackAir();
        });

        const auto scoreFunction = [&](WalkPosition w) {
            const auto p = Position(w) + Position(4, 4);
            const auto threat = defaultThreat(unit, w);
            const auto distance = defaultDistance(unit, w);
            auto score = (closestDefense && closestDefense->getPosition().getDistance(p) < 160.0) ? 1.0 / distance : 1.0 / (threat * distance);
            return score;
        };

        // Escorting
        auto shouldEscort = unit.getRole() == Role::Support;
        if (!shouldEscort)
            return false;

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.command(Move, bestPosition);
            return true;
        }
        return false;
    }

    bool transport(UnitInfo& unit)
    {
        auto cluster = Positions::Invalid;
        auto closestRetreat = Combat::getClosestRetreatPosition(unit.getPosition());

        const auto scoreFunction = [&](WalkPosition w) {
            auto p = Position(w) + Position(4, 4);

            const auto airDist =        max(1.0, p.getDistance(unit.getDestination()));
            const auto grdDist =        max(1.0, BWEB::Map::getGroundDistance(p, unit.getDestination()));
            const auto dist =           grdDist;
            const auto distRetreat =    p.getDistance(closestRetreat);

            if (grdDist == DBL_MAX)
                return 0.0;

            const auto threat =     unit.getTransportState() == TransportState::Retreating ? defaultThreat(unit, w) : 1.0;
            const auto distance =   unit.getTransportState() == TransportState::Retreating ? distRetreat : dist;
            double score =          1.0 / (threat * distance);

            for (auto &c : unit.getAssignedCargo()) {
                if (auto &cargo = c.lock()) {

                    // If we just dropped units, we need to make sure not to leave them
                    if (unit.getTransportState() == TransportState::Monitoring) {
                        if (!cargo->unit()->isLoaded() && cargo->getPosition().getDistance(p) > 64.0)
                            return 0.0;
                    }

                    // If we're trying to load
                    if (unit.getTransportState() == TransportState::Loading)
                        score = 1.0 / distance;
                }
            }

            return score;
        };

        auto highestCluster = 0.0;
        for (auto &[score, position] : Combat::getCombatClusters()) {
            if (score > highestCluster) {
                highestCluster = score;
                cluster = position;
            }
        }

        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.command(Move, bestPosition);
            return true;
        }
        return false;
    }
}