#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitCommandTypes;

namespace McRave::Command {

    namespace {

        double defaultGrouping(UnitInfo& unit, WalkPosition w) {
            return unit.getType().isFlyer() ? 1.0f / max(0.1f, log(Grids::getAAirCluster(w))) : max(0.1f, log(Grids::getAGroundCluster(w)));
        }

        double defaultDistance(UnitInfo& unit, WalkPosition w) {
            const auto p = Position(w) + Position(4, 4);
            return p.getDistance(unit.getDestination());
            //return unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination());
        }

        double defaultVisited(UnitInfo& unit, WalkPosition w) {
            return log(clamp(double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w)), 100.0, 1000.0));
        }

        double defaultVisible(UnitInfo& unit, WalkPosition w) {
            return log(clamp(double(Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(w))), 100.0, 1000.0));
        }

        double defaultMobility(UnitInfo& unit, WalkPosition w) {
            return unit.getType().isFlyer() ? 1.0 : log(100.0 + double(Grids::getMobility(w)));
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
            const auto viablePosition = [&](WalkPosition here, Position p) {
                // If not a flyer and position blocks a building, has collision or a splash threat
                if (!unit.getType().isFlyer() &&
                    (Buildings::overlapsQueue(unit, unit.getTilePosition()) || Grids::getESplash(here) > 0))
                    return false;

                // If too far of a command, is in danger or isn't walkable
                if ((!unit.getType().isFlyer() && Grids::getMobility(here) < 1)
                    || !Util::isTightWalkable(unit, here)
                    || Actions::isInDanger(unit, p))
                    return false;
                return true;
            };

            auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0);
            auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0);
            auto start = unit.getWalkPosition();
            auto bestPosition = Positions::Invalid;
            auto best = 0.0;

            // Check to see if we're close to a building and need a bit more tiles to look at
            auto moreTiles = false;
            if (!unit.getType().isFlyer()) {
                vector<TilePosition> directions ={ {-1,-1}, {-1, 1}, {1, -1}, {1,1} };
                for (auto &direction : directions) {
                    auto tile = unit.getTilePosition() + direction;
                    if (!tile.isValid() || BWEB::Map::isUsed(tile) != UnitTypes::None || !BWEB::Map::isWalkable(tile)) {
                        moreTiles = true;
                        break;
                    }
                }
            }

            // Find bounding box to score WalkPositions
            const auto radius = unit.getType().isFlyer() ? 24 : 8 + (16 * moreTiles);
            const auto left = max(0, start.x - radius);
            const auto right = min(Broodwar->mapWidth() * 4, start.x + radius + walkWidth);
            const auto up = max(0, start.y - radius);
            const auto down = min(Broodwar->mapHeight() * 4, start.y + radius + walkHeight);

            // Iterate
            for (auto x = left; x < right; x++) {
                for (auto y = up; y < down; y++) {
                    const WalkPosition w(x, y);
                    const Position p = Position(w) + Position(4, 4);

                    if ((!unit.getType().isFlyer() || unit.getRole() == Role::Transport) && Grids::getMobility(w) < 1)
                        continue;
                    if (!viablePosition(w, p))
                        continue;

                    const auto current = score(w);

                    if (current > best) {
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

        // Check if we should remove a building assignment
        if (unit.getBuildPosition().isValid() && (unit.isStuck() || BuildOrder::buildCount(unit.getBuildType()) <= vis(unit.getBuildType()) || !Buildings::isBuildable(unit.getBuildType(), unit.getBuildPosition()))) {
            unit.setBuildingType(UnitTypes::None);
            unit.setBuildPosition(TilePositions::Invalid);
            unit.move(Stop, unit.getPosition());
            return true;
        }

        // Unstick a unit
        else if (unit.isStuck() && unit.unit()->isMoving()) {
            unit.move(Stop, unit.getPosition());
            return true;
        }

        // DT vs spider mines
        else if (unit.getType() == UnitTypes::Protoss_Dark_Templar  && unit.hasTarget() && !unit.getTarget().isHidden() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
            if (unit.canStartAttack()) {
                unit.click(Attack_Unit, unit.getTarget());
                return true;
            }
            return false;
        }

        // If unit has a transport, load into it if we need to
        else if (unit.hasTransport()) {

            // Check if we Unit being attacked by multiple bullets
            auto bulletCount = 0;
            for (auto &bullet : Broodwar->getBullets()) {
                if (bullet && bullet->exists() && bullet->getPlayer() != Broodwar->self() && bullet->getTarget() == unit.unit())
                    bulletCount++;
            }

            if (unit.isRequestingPickup() || bulletCount >= 6 || Units::getSplashTargets().find(unit.unit()) != Units::getSplashTargets().end()) {
                unit.click(Right_Click_Unit, unit.getTransport());
                return true;
            }
        }

        // Units targeted by splash need to move away from the army
        // TODO: Maybe move this to their respective functions
        else if (unit.hasTarget() && Units::getSplashTargets().find(unit.unit()) != Units::getSplashTargets().end()) {
            if (unit.canStartAttack() && unit.getTarget().unit()->exists()) {
                unit.click(Attack_Unit, unit.getTarget());
            }
            else
                unit.move(Move, unit.getTarget().getPosition());
            return true;
        }


        // If unit is potentially stuck, try to find a manner pylon
        // TODO: Use units target? Check if it's actually targeting pylon?
        else if (unit.getRole() == Role::Worker && (unit.framesHoldingResource() >= 100 || unit.framesHoldingResource() <= -200)) {
            auto &pylon = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u.getType() == UnitTypes::Protoss_Pylon;
            });
            if (pylon && pylon->unit() && pylon->unit()->exists() && pylon->getPosition().getDistance(unit.getPosition()) < 128.0) {
                unit.click(Attack_Unit, *pylon);
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

            // Special Case: Carriers
            if (unit.getType() == UnitTypes::Protoss_Carrier) {
                auto leashRange = 320;
                for (auto &interceptor : unit.unit()->getInterceptors()) {
                    if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() && interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted()) {
                        unit.click(Attack_Unit, unit.getTarget());
                        return true;
                    }
                }
                if (unit.getPosition().getDistance(unit.getTarget().getPosition()) >= leashRange) {
                    unit.click(Attack_Unit, unit.getTarget());
                    return true;
                }
                return false;
            }

            if (unit.getType() == UnitTypes::Zerg_Lurker && !unit.isBurrowed())
                return false;
            return unit.canStartAttack();
        };

        // Should the unit execute an attack command
        const auto shouldAttack = [&]() {

            auto threatAtEngagement = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());
            if (unit.getRole() == Role::Combat)
                return unit.isWithinRange(unit.getTarget()) && (unit.getLocalState() == LocalState::Attack || !threatAtEngagement);

            if (unit.getRole() == Role::Scout)
                return unit.getTargetedBy().size() <= 1 && (unit.isWithinRange(unit.getTarget()) || unit.getTarget().getType().isBuilding()) && unit.getPercentTotal() >= unit.getTarget().getPercentTotal();

            if (unit.getRole() == Role::Worker && unit.getBuildPosition().isValid()) {
                const auto topLeft = Position(unit.getBuildPosition());
                const auto botRight = topLeft + Position(unit.getBuildType().tileWidth() * 32, unit.getBuildType().tileHeight() * 32);
                return unit.hasTarget() && Util::rectangleIntersect(topLeft, botRight, unit.getTarget().getPosition());
            }
            return false;
        };

        // If unit should and can be attacking
        if (shouldAttack() && canAttack()) {
            unit.click(Attack_Unit, unit.getTarget());
            return true;
        }
        return false;
    }

    bool approach(UnitInfo& unit)
    {
        // If we don't have a target or the targets position is invalid, we can't approach it
        if (!unit.hasTarget() || !unit.getTarget().getPosition().isValid())
            return false;

        // Can the unit approach its target
        const auto canApproach = [&]() {

            // Special Case: High Templars
            if (unit.getType() == UnitTypes::Protoss_High_Templar)
                return !unit.canStartCast(TechTypes::Psionic_Storm);

            if (unit.getSpeed() <= 0.0 || unit.canStartAttack())
                return false;
            return true;
        };

        // Should the unit approach its target
        const auto shouldApproach = [&]() {
            auto unitRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

            if (unit.getRole() == Role::Combat) {

                auto intercept = Util::getInterceptPosition(unit);
                auto interceptDistance = intercept.getDistance(unit.getPosition());

                // Capital ships want to stay at max range due to their slow nature
                if (unit.isCapitalShip() || unit.getTarget().isSuicidal() || unit.isSpellcaster())
                    return false;

                // Approach units that are moving away from us
                if (interceptDistance > unit.getPosition().getDistance(unit.getTarget().getPosition()))
                    return true;

                // Light air wants to approach air targets or anything that cant attack air
                if (unit.isLightAir()) {
                    if (!unit.getTarget().getType().isBuilding() && (unit.getTarget().getType().isFlyer() || unit.getTarget().getAirRange() == 0.0))
                        return true;
                    return false;
                }

                // HACK: Dragoons shouldn't approach Vultures before range upgrade
                if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                    return false;

                // If crushing victory, push forward
                if (unit.getSimValue() >= 10.0 && !unit.getTarget().getType().isWorker())
                    return true;
            }

            // If this units range is lower and target isn't a building
            if (unitRange < enemyRange && !unit.getTarget().getType().isBuilding())
                return true;
            return false;
        };

        if (shouldApproach() && canApproach()) {
            unit.move(Move, unit.getTarget().getPosition());
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
            const auto score =      unit.getRole() == Role::Worker ? mobility / (distance * grouping * exp(threat)) : mobility / (distance * grouping);
            return score;
        };

        auto canMove = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        auto shouldMove = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.hasTarget() && unit.isWithinRange(unit.getTarget()) && !unit.getTarget().isHidden() && unit.getTarget().unit()->exists() && unit.getType() != UnitTypes::Zerg_Lurker)
                    return false;
                if (unit.getLocalState() == LocalState::Attack)
                    return true;
            }
            else
                return true;
            return false;
        };

        // HACK: Drilling with workers. Should add some sort of getClosestResource or fix how PlayerState::Neutral units are stored (we don't store resources in them)
        if (false && unit.getType().isWorker() && unit.getRole() == Role::Combat && unit.hasTarget()) {
            auto closestMineral = Broodwar->getClosestUnit(unit.getTarget().getPosition(), Filter::IsMineralField, 32);

            if (closestMineral && closestMineral->exists()) {
                unit.unit()->gather(closestMineral);
                return true;
            }
        }

        // If unit can move and should move
        if (canMove() && shouldMove()) {

            // Find the best position to move to
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.move(Move, bestPosition);
                return true;
            }
            else {
                bestPosition = unit.getDestination();
                unit.move(Move, bestPosition);
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
            const auto distance =   unit.hasTarget() ? 1.0 / (p.getDistance(unit.getTarget().getPosition())) : p.getDistance(BWEB::Map::getMainPosition());
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

            if (unit.getSpeed() <= 0.0 || unit.canStartAttack())
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

                // Don't kite units that are moving away from us
                if (interceptDistance > unit.getPosition().getDistance(unit.getTarget().getPosition()))
                    return false;

                // Don't kite buildings unless we're a flying unit
                if (!unit.getTarget().canAttackGround() && !unit.getTarget().canAttackAir() && !unit.getType().isFlyer())
                    return false;

                // Capital ships want to stay at max range due to their slow nature
                if (unit.isCapitalShip() || unit.getTarget().isSuicidal())
                    return true;

                // Light air shouldn't kite flyers or units that can't attack air
                if (unit.isLightAir()) {
                    if (unit.getTarget().getType().isFlyer() || unit.getTarget().getAirRange() == 0.0)
                        return false;
                    return true;
                }

                // HACK: Dragoons should always kite Vultures before range upgrade
                if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                    return true;

                if (unit.getType() == UnitTypes::Protoss_Reaver
                    || allyRange > enemyRange
                    || (!unit.getTargetedBy().empty() && allyRange == enemyRange)
                    || unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                    return true;

                // Kite instead of retreating if we are far enough into a game
                if (unit.getGlobalState() == GlobalState::Attack && unit.getLocalState() == LocalState::Retreat)
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
                unit.move(Move, Terrain::getDefendPosition());
                return true;
            }

            // If we found a valid position, move to it
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.move(Move, bestPosition);
                return true;
            }
        }
        return false;
    }

    bool defend(UnitInfo& unit)
    {
        bool closeToMainChoke = Position(BWEB::Map::getMainChoke()->Center()).getDistance(unit.getPosition()) < 320.0;
        bool closeToNaturalChoke = Position(BWEB::Map::getNaturalChoke()->Center()).getDistance(unit.getPosition()) < 320.0;
        bool defendingExpansion = unit.getDestination().isValid() && !Terrain::isInEnemyTerritory((TilePosition)unit.getDestination());
        bool closeToDefend = Terrain::isInAllyTerritory(unit.getTilePosition()) || (closeToMainChoke || closeToNaturalChoke) || unit.getType().isWorker();

        if (!closeToDefend
            || unit.getLocalState() == LocalState::Attack
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
                            || Buildings::overlapsQueue(unit, TilePosition(w))
                            || !Util::isTightWalkable(unit, w))
                            continue;

                        if (dist < distBest) {
                            distBest = dist;
                            walkBest = w;
                        }
                    }
                }
                if (walkBest.isValid()) {
                    unit.move(Move, Position(walkBest));
                    return true;
                }
            }
        }

        // HACK: Flyers defend above a base
        // TODO: Choose a base instead of closest to enemy, sometimes fly over a base I dont own
        if (unit.getType().isFlyer()) {
            if (Terrain::getEnemyStartingPosition().isValid() && BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition()) < BWEB::Map::getNaturalPosition().getDistance(Terrain::getEnemyStartingPosition()))
                unit.move(Move, BWEB::Map::getMainPosition());
            else
                unit.move(Move, BWEB::Map::getNaturalPosition());
        }
        else if (Strategy::defendChoke()) {
            const BWEM::Area * defendArea = nullptr;
            const BWEM::ChokePoint * defendChoke = Terrain::isDefendNatural() ? BWEB::Map::getNaturalChoke() : BWEB::Map::getMainChoke();

            // Decide which area is within my territory, useful for maps with small adjoining areas like Andromeda
            auto &[a1, a2] = defendChoke->GetAreas();
            if (a1 && Terrain::isInAllyTerritory(a1))
                defendArea = a1;
            if (a2 && Terrain::isInAllyTerritory(a2))
                defendArea = a2;

            auto bestPosition = Util::getConcavePosition(unit, defendArea, Terrain::getDefendPosition());
            unit.move(Move, bestPosition);
            return true;
        }
        else {
            unit.move(Move, Terrain::getDefendPosition());
            return true;
        }
        return false;
    }

    bool hunt(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            const auto threat =     defaultThreat(unit, w);
            const auto distance =   defaultDistance(unit, w);
            const auto visited =    defaultVisited(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      mobility * visited / (threat * distance * grouping);

            return score;
        };

        const auto canHunt = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        const auto shouldHunt = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.isLightAir())
                    return Players::getStrength(PlayerState::Enemy).airToAir <= 0.0 && unit.getLocalState() == LocalState::Retreat;
            }
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

            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.move(Move, bestPosition);
                return true;
            }
            else {
                unit.move(Move, unit.getDestination());
                return true;
            }
        }
        return false;
    }

    bool retreat(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            const auto distance =   (unit.hasTarget() && unit.getGlobalState() == GlobalState::Attack) ? defaultDistance(unit, w) / (p.getDistance(unit.getTarget().getPosition())) : defaultDistance(unit, w);
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
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        if (canRetreat() && shouldRetreat()) {

            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.move(Move, bestPosition);
                return true;
            }
            else if (Combat::getClosestRetreatPosition(unit.getPosition()).isValid()) {
                unit.move(Move, Combat::getClosestRetreatPosition(unit.getPosition()));
                return true;
            }
        }
        return false;
    }

    bool escort(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto p = Position(w) + Position(4, 4);
            const auto threat = defaultThreat(unit, w);
            const auto distance = defaultDistance(unit, w);
            auto score = 1.0 / (threat * distance);

            //// Try to keep the unit alive if it's cloaked inside detection
            //if (unit.unit()->isCloaked() && threat > MIN_THREAT && Actions::overlapsDetection(unit.unit(), p, PlayerState::Enemy))
            //    score = 1.0 / (threat * distance);
            //if (!unit.isHealthy() && threat > MIN_THREAT)
            //    score = 1.0 / (threat * distance);
            return score;
        };

        // Escorting
        auto shouldEscort = unit.getRole() == Role::Support;
        if (!shouldEscort)
            return false;

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.move(Move, bestPosition);
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
            unit.move(Move, bestPosition);
            return true;
        }
        return false;
    }
}
