#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Command {

    namespace {

        void updateActions()
        {
            // Clear cache
            neutralActions.clear();
            myActions.clear();
            allyActions.clear();
            enemyActions.clear();

            // Check bullet based abilities, store as neutral PlayerState as it affects both sides
            for (auto &b : Broodwar->getBullets()) {
                if (b && b->exists()) {
                    if (b->getType() == BulletTypes::Psionic_Storm)
                        addAction(nullptr, b->getPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);

                    if (b->getType() == BulletTypes::EMP_Missile)
                        addAction(nullptr, b->getTargetPosition(), TechTypes::EMP_Shockwave, PlayerState::Neutral);
                }
            }

            // Check nuke dots, store as neutral PlayerState as it affects both sides
            for (auto &dot : Broodwar->getNukeDots())
                addAction(nullptr, dot, TechTypes::Nuclear_Strike, PlayerState::Neutral);

            // Check enemy Actions, store outgoing bullet based abilities as neutral PlayerState
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;

                if (!unit.unit() || (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())))
                    continue;

                if (unit.getType().isDetector())
                    addAction(unit.unit(), unit.getPosition(), unit.getType(), PlayerState::Enemy);

                if (unit.unit()->exists()) {
                    Order enemyOrder = unit.unit()->getOrder();
                    Position enemyTarget = unit.unit()->getOrderTargetPosition();

                    if (enemyOrder == Orders::CastEMPShockwave)
                        addAction(unit.unit(), enemyTarget, TechTypes::EMP_Shockwave, PlayerState::Neutral);
                    if (enemyOrder == Orders::CastPsionicStorm)
                        addAction(unit.unit(), enemyTarget, TechTypes::Psionic_Storm, PlayerState::Neutral);
                }

                if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                    addAction(unit.unit(), unit.getPosition(), TechTypes::Spider_Mines, PlayerState::Enemy);
            }

            // Check my Actions
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;

                if (unit.getType().isDetector())
                    addAction(unit.unit(), unit.getPosition(), unit.getType(), PlayerState::Self);
            }
        }

        double defaultGrouping(UnitInfo& unit, WalkPosition w) {
            return unit.getType().isFlyer() ? 1.0 / max(0.1f, Grids::getAAirCluster(w)) : log(50.0 + Grids::getAGroundCluster(w));
        }

        double defaultDistance(UnitInfo& unit, WalkPosition w) {
            const auto p = Position(w) + Position(4, 4);
            return (unit.getType().isFlyer() || Terrain::isIslandMap()) ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination());
        }

        double defaultVisited(UnitInfo& unit, WalkPosition w) {
            return log(clamp(double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w)), 100.0, 1000.0));
        }

        double defaultVisible(UnitInfo& unit, WalkPosition w) {
            return log(clamp(double(Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(w))), 100.0, 1000.0));
        }

        double defaultMobility(UnitInfo& unit, WalkPosition w) {
            return unit.getType().isFlyer() ? 1.0 : log(40.0 + double(Grids::getMobility(w)));
        }

        double defaultThreat(UnitInfo& unit, WalkPosition w)
        {
            if (unit.isTransport()) {
                const auto p = Position(w) + Position(4, 4);
                if (p.getDistance(unit.getDestination()) < 32.0)
                    return max(MIN_THREAT, Grids::getEGroundThreat(w) + Grids::getEAirThreat(w));
                return max(MIN_THREAT, Grids::getEAirThreat(w));
            }
            return max(MIN_THREAT, unit.getType().isFlyer() ? Grids::getEAirThreat(w) : Grids::getEGroundThreat(w));
        }

        Position findViablePosition(UnitInfo& unit, function<double(WalkPosition)> score)
        {
            const auto viablePosition = [&](WalkPosition here, Position p) {
                // If not a flyer and position blocks a building, has collision or a splash threat
                if (!unit.getType().isFlyer() &&
                    (Buildings::overlapsQueue(unit, unit.getTilePosition()) /*|| Grids::getESplash(here) > 0*/))
                    return false;

                auto unitOnChokeGeo = unit.getWalkPosition().isValid() && !unit.getType().isFlyer() && !mapBWEM.GetArea(unit.getWalkPosition());
                auto validAreas = unit.getWalkPosition().isValid() && here.isValid() && mapBWEM.GetArea(here) && mapBWEM.GetArea(unit.getWalkPosition()) && mapBWEM.GetArea(here)->AccessibleFrom(mapBWEM.GetArea(unit.getWalkPosition()));
                auto distanceFromUnit = (unit.getType().isFlyer() || validAreas || unitOnChokeGeo) ? p.getDistance(unit.getPosition()) : BWEB::Map::getGroundDistance(p, unit.getPosition());

                // If too far of a command, is in danger or isn't walkable
                if (distanceFromUnit >= 96
                    || (!unit.getType().isFlyer() && !Broodwar->isWalkable(here))
                    || isInDanger(unit, p)
                    || !Util::isWalkable(unit, here))
                    return false;
                return true;
            };

            // Find the best TilePosition in 5x5 so we can iterate the WalkPositions in the TilePosition
            multimap<double, TilePosition> sortedTiles;
            auto bestPosition = Positions::Invalid;
            auto best = 0.0;
            auto t = unit.getTilePosition() + TilePosition(1, 1); //TilePosition(unit.getPosition().x % 2 >= 16 ? 1 : 0, unit.getPosition().y % 2 >= 16 ? 1 : 0);

            vector<TilePosition> directions{ {t.x, t.y - 1},{t.x - 1, t.y},{t.x, t.y + 1}, {t.x + 1, t.y} };
            set<TilePosition> allowedDirections;

            const auto unitCanFitThrough = [&](TilePosition side1, TilePosition side2, UnitType b1, UnitType b2) {
                auto dim1 = side1.x != t.x ?
                    (side1.x < t.x ? b1.dimensionRight() + b2.dimensionLeft() : b1.dimensionLeft() + b2.dimensionRight()) :
                    (side1.y < t.y ? b1.dimensionDown() + b2.dimensionUp() : b1.dimensionUp() + b2.dimensionDown());
                auto dim2 = side2.x != t.x ?
                    (side2.x < t.x ? b2.dimensionRight() + b1.dimensionLeft() : b2.dimensionLeft() + b1.dimensionRight()) :
                    (side2.y < t.y ? b2.dimensionDown() + b1.dimensionUp() : b2.dimensionUp() + b1.dimensionDown());

                auto dim1Ok = (side1.x != t.x && dim1 >= unit.getType().width()) || (side1.y != t.y && dim1 >= unit.getType().height());
                auto dim2Ok = (side2.x != t.x && dim2 >= unit.getType().width()) || (side2.y != t.y && dim2 >= unit.getType().height());

                return dim1Ok || dim2Ok;
            };

            // Search each Tile to see if it's walkable
            for (vector<TilePosition>::iterator itr = directions.begin(); itr < directions.end(); itr++) {
                auto side1 = *itr;
                auto side2 = itr == directions.end() - 1 ? directions.at(0) : *(itr + 1);
                auto corner = side1 + side2 - t;

                auto side1Type = BWEB::Map::isUsed(side1);
                auto side2Type = BWEB::Map::isUsed(side2);
                auto cornerType = BWEB::Map::isUsed(corner);

                if ((unit.getType().isFlyer() || side1Type == UnitTypes::None || side2Type == UnitTypes::None || unitCanFitThrough(side1, side2, side1Type, side2Type)) && cornerType == UnitTypes::None && (unit.getType().isFlyer() || Util::isWalkable(corner)))
                    allowedDirections.insert(corner);
                if (unit.getType().isFlyer() || (side1Type == UnitTypes::None && Util::isWalkable(side1)))
                    allowedDirections.insert(side1 + (side1 - t));
                if (unit.getType().isFlyer() || (side2Type == UnitTypes::None && Util::isWalkable(side2)))
                    allowedDirections.insert(side2 + (side2 - t));
            }

            // Score each Walkposition to see which one is best
            for (auto &tile : allowedDirections) {
                WalkPosition w(tile);

                // Iterate the WalkPositions within the TilePosition
                for (int x = w.x; x < w.x + 4; x++) {
                    for (int y = w.y; y < w.y + 4; y++) {
                        WalkPosition w(x, y);
                        if (!w.isValid())
                            continue;

                        auto current = score(w);
                        if (current > best && viablePosition(w, Position(w))) {
                            best = current;
                            bestPosition = Position(w);
                        }
                    }
                }
            }
            return bestPosition;
        }
    }

    bool misc(UnitInfo& unit)
    {
        // Unstick a unit
        if (unit.isStuck() && unit.unit()->isMoving()) {
            unit.unit()->stop();
            return true;
        }

        // DT vs spider mines
        else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
            if (unit.canStartAttack()) {
                unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
                return true;
            }
            return false;
        }

        // If unit has a transport, load into it if we need to
        else if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
            if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.canStartAttack()) {
                unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
                return true;
            }
            else if (unit.getType() == UnitTypes::Protoss_High_Templar && !unit.canStartCast(TechTypes::Psionic_Storm)) {
                unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
                return true;
            }
        }

        // Units targeted by splash need to move away from the army
        // TODO: Maybe move this to their respective functions
        else if (unit.hasTarget() && Units::getSplashTargets().find(unit.unit()) != Units::getSplashTargets().end()) {
            if (unit.hasTransport())
                unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
            else if (unit.canStartAttack() && unit.getTarget().unit()->exists())
                unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
            else
                unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition(), true);
            return true;
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
                    if (interceptor->getOrder() == Orders::InterceptorReturn && interceptor->isCompleted()) {
                        unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
                        return true;
                    }
                }
                if (unit.getPosition().getApproxDistance(unit.getTarget().getPosition()) >= leashRange) {
                    unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
                    unit.circleBlue();
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
                return unit.withinRange(unit.getTarget()) && (unit.getLocalState() == LocalState::Attack || !threatAtEngagement);

            if (unit.getRole() == Role::Scout)
                return unit.withinRange(unit.getTarget()) && (unit.getPercentTotal() >= unit.getTarget().getPercentTotal());

            return false;
        };

        // If unit should and can be attacking
        if (shouldAttack() && canAttack()) {
            unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
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
                    if (unit.getTarget().getType().isFlyer() || unit.getTarget().getAirRange() == 0.0)
                        return true;
                    return false;
                }

                // HACK: Dragoons shouldn't approach Vultures before range upgrade
                if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                    return false;
            }

            // If this units range is lower and target isn't a building
            if (unitRange < enemyRange && !unit.getTarget().getType().isBuilding())
                return true;
            return false;
        };

        if (shouldApproach() && canApproach()) {
            unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition(), false);
            return true;
        }
        return false;
    }

    bool move(UnitInfo& unit)
    {
        const auto scoreFunction = [&](WalkPosition w) {
            const auto distance =   defaultDistance(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      (grouping * mobility) / distance;
            return score;
        };

        auto canMove = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        auto shouldMove = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.hasTarget() && unit.withinRange(unit.getTarget()) && !unit.getTarget().isHidden() && unit.getTarget().unit()->exists() && unit.getType() != UnitTypes::Zerg_Lurker)
                    return false;
                if (unit.getLocalState() == LocalState::Attack)
                    return true;
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Worker || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        // If unit has a transport, move to it or load into it
        if (unit.hasTransport() && unit.getTransport().unit()->exists() && unit.getPosition().getDistance(unit.getEngagePosition()) > 64.0) {
            unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
            return true;
        }

        // HACK: Drilling with workers. Should add some sort of getClosestResource or fix how PlayerState::Neutral units are stored (we don't store resources in them)
        if (unit.getType().isWorker() && unit.getRole() == Role::Combat && unit.hasTarget()) {
            auto closestMineral = Broodwar->getClosestUnit(unit.getTarget().getPosition(), Filter::IsMineralField, 32);

            if (closestMineral && closestMineral->exists()) {
                unit.unit()->gather(closestMineral);
                return true;
            }
        }

        // If unit can move and should move
        if (canMove() && shouldMove()) {

            auto bestPosition = Positions::Invalid;

            Visuals::displayPath(unit.getAttackPath().getTiles());

            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Brown);

            // Move to the first point that is at least 5 tiles away if possible
            if (!unit.getAttackPath().getTiles().empty() && unit.getAttackPath().isReachable()) {
                bestPosition = Util::findPointOnPath(unit.getAttackPath(), [&](Position here) {
                    return here.getDistance(unit.getPosition()) >= 256.0;
                });

                if (bestPosition.isValid()) {
                    Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Green);
                    unit.command(UnitCommandTypes::Move, bestPosition, true);
                    return true;
                }
            }

            // Find the best position to move to
            bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Red);
                unit.command(UnitCommandTypes::Move, bestPosition, true);
                return true;
            }
            else {
                bestPosition = unit.getDestination();
                unit.command(UnitCommandTypes::Move, bestPosition, true);
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
            const auto score =      (grouping * mobility) / (threat * distance);
            return score;
        };

        const auto canKite = [&]() {

            if (unit.getRole() == Role::Worker)
                return true;

            // Special Case: Carriers
            if (unit.getType() == UnitTypes::Protoss_Carrier) {
                auto leashRange = 320;
                for (auto &interceptor : unit.unit()->getInterceptors()) {
                    if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->isCompleted())
                        return false;
                }
                if (unit.getPosition().getApproxDistance(unit.getTarget().getPosition()) >= leashRange)
                    return false;
                return true;
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

            if (unit.getRole() == Role::Worker)
                return true;

            if (unit.getRole() == Role::Combat || unit.getRole() == Role::Scout) {
                auto intercept = Util::getInterceptPosition(unit);
                auto interceptDistance = intercept.getDistance(unit.getPosition());

                // Don't kite units that are moving away from us
                if (interceptDistance > unit.getPosition().getDistance(unit.getTarget().getPosition()))
                    return false;

                // Don't kite if we can start an attack
                if (unit.canStartAttack())
                    return false;

                // Don't kite buildings unless we're a flying unit
                if (unit.getTarget().getType().isBuilding() && !unit.getType().isFlyer())
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
                    || unit.getType() == UnitTypes::Terran_Vulture
                    || allyRange >= enemyRange
                    || unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                    return true;
            }

            // If unit is being targeted and should drop being targeted if possible
            if (!unit.getTargetedBy().empty() && enemyRange <= allyRange)
                return true;

            // If unit isn't attacking at maximum range and unit outranges the target
            if (enemyRange < allyRange && unit.unit()->getDistance(unit.getTarget().getPosition()) <= allyRange - enemyRange) {
                unit.circleRed();
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
                unit.command(UnitCommandTypes::Move, Terrain::getDefendPosition(), true);
                return true;
            }

            // If we found a valid position, move to it
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(UnitCommandTypes::Move, bestPosition, true);
                return true;
            }
        }
        return false;
    }

    bool defend(UnitInfo& unit)
    {
        bool defendingExpansion = unit.getDestination().isValid() && !Terrain::isInEnemyTerritory((TilePosition)unit.getDestination());
        bool closeToDefend = Terrain::getDefendPosition().getDistance(unit.getPosition()) < 640.0 || Terrain::isInAllyTerritory(unit.getTilePosition());

        if (!closeToDefend
            || unit.getLocalState() == LocalState::Attack
            || (unit.hasTarget() && unit.getTarget().isHidden() && unit.getTarget().withinReach(unit)))
            return false;

        // Probe Cannon surround
        if (unit.getType().isWorker() && vis(UnitTypes::Protoss_Photon_Cannon) > 0) {
            auto &cannon = Util::getClosestUnitGround(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                return u.getType() == UnitTypes::Protoss_Photon_Cannon;
            });

            if (cannon) {

                auto cannonDist = BWEB::Map::getGroundDistance(mapBWEM.Center(), cannon->getPosition());
                auto distBest = DBL_MAX;
                auto walkBest = WalkPositions::Invalid;
                auto start = cannon->getWalkPosition();
                for (int x = start.x - 10; x < start.x + 18; x++) {
                    for (int y = start.y - 10; y < start.y + 18; y++) {
                        WalkPosition w(x, y);
                        Position p(w);
                        double dist = BWEB::Map::getGroundDistance(mapBWEM.Center(), p);

                        // This comes from our concave check, need a solution other than this
                        if (!w.isValid()
                            || p.getDistance(cannon->getPosition()) < 64.0
                            || Command::overlapsActions(unit.unit(), p, unit.getType(), PlayerState::Self, 24)
                            || Command::isInDanger(unit, p)
                            || Grids::getMobility(p) <= 6
                            || Buildings::overlapsQueue(unit, TilePosition(w))
                            || dist >= cannonDist + 64)
                            continue;

                        if (dist < distBest) {
                            distBest = dist;
                            walkBest = w;
                        }
                    }
                }
                if (walkBest.isValid()) {
                    unit.command(UnitCommandTypes::Move, Position(walkBest), false);
                    return true;
                }
            }
        }

        // HACK: Flyers defend above a base
        // TODO: Choose a base instead of closest to enemy, sometimes fly over a base I dont own
        if (unit.getType().isFlyer()) {
            if (Terrain::getEnemyStartingPosition().isValid() && BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition()) < BWEB::Map::getNaturalPosition().getDistance(Terrain::getEnemyStartingPosition()))
                unit.command(UnitCommandTypes::Move, BWEB::Map::getMainPosition(), true);
            else
                unit.command(UnitCommandTypes::Move, BWEB::Map::getNaturalPosition(), true);
        }
        else {
            // Estimate concave radii
            auto meleeRadius = Terrain::isDefendNatural() && Terrain::getNaturalWall() && Players::getSupply(PlayerState::Self) >= 40 && Players::vZ() ? 32 : min(160, Units::getNumberMelee() * 16);
            auto rangedRadius = !Terrain::isDefendNatural() && Terrain::getMainWall() ? 128.0 : max(int(unit.getGroundRange()), meleeRadius + 32);

            // Find a concave position at the desired radius
            auto radius = unit.getGroundRange() > 32.0 ? rangedRadius : meleeRadius;
            auto defendArea = Terrain::isDefendNatural() ? BWEB::Map::getNaturalArea() : BWEB::Map::getMainArea();

            auto bestPosition = Util::getConcavePosition(unit, radius, defendArea, Terrain::getDefendPosition());
            unit.command(UnitCommandTypes::Move, bestPosition, false);
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
            const auto visited =    unit.getRole() == Role::Scout && threat > 0 ? 1.0 : defaultVisible(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      mobility * visited / (threat * distance * grouping);

            if (threat == MIN_THREAT
                || (unit.unit()->isCloaked() && !overlapsDetection(unit.unit(), p, PlayerState::Enemy))
                || unit.getRole() == Role::Scout)
                return score;
            return 0.0;
        };

        const auto canHunt = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        const auto shouldHunt = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.isLightAir() && Players::getStrength(PlayerState::Enemy).airToAir <= 0.0)
                    return true;
            }
            if (unit.getRole() == Role::Transport || unit.getRole() == Role::Scout)
                return true;
            return false;
        };

        if (shouldHunt() && canHunt()) {
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(UnitCommandTypes::Move, bestPosition, true);
                return true;
            }
        }
        return false;
    }

    bool retreat(UnitInfo& unit)
    {
        // Low distance, low threat, high clustering
        const auto scoreFunction = [&](WalkPosition w) {
            const auto p =          Position(w) + Position(4, 4);
            const auto distRatio =  unit.hasTarget() ? max(0.01, (SIM_RADIUS - p.getDistance(unit.getTarget().getPosition()))) / SIM_RADIUS : 1.0;

            // Distance is a mix of kiting and retreating
            const auto distance =   defaultDistance(unit, w);
            const auto threat =     defaultThreat(unit, w);
            const auto grouping =   defaultGrouping(unit, w);
            const auto mobility =   defaultMobility(unit, w);
            const auto score =      1.0 / (threat * distance * grouping);
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

            auto bestPosition = Positions::Invalid;

            // Find the best position to move to
            if (unit.getType().isFlyer()) {
                auto bestPosition = findViablePosition(unit, scoreFunction);
                if (bestPosition.isValid()) {
                    unit.command(UnitCommandTypes::Move, bestPosition, true);
                    return true;
                }
            }
            else {

                // Create a retreat path if needed
                auto closestRetreat = Combat::getClosestRetreatPosition(unit.getPosition());
                if (unit.canCreateRetreatPath(closestRetreat)) {
                    BWEB::Path newRetreatPath;
                    newRetreatPath.createUnitPath(unit.getPosition(), closestRetreat);
                    unit.setRetreatPath(newRetreatPath);
                }

                // Move to the first point that is at least 5 tiles away if possible
                if (!unit.getRetreatPath().getTiles().empty() && unit.getRetreatPath().isReachable()) {
                    bestPosition = Util::findPointOnPath(unit.getRetreatPath(), [&](Position here) {
                        return here.getDistance(unit.getPosition()) >= 256.0;
                    });

                    if (bestPosition.isValid()) {
                        unit.command(UnitCommandTypes::Move, bestPosition, true);
                        return true;
                    }
                }
            }

            bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(UnitCommandTypes::Move, bestPosition, true);
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
            const auto distance = unit.hasTarget() ? p.getDistance(unit.getDestination()) * log(p.getDistance(unit.getTarget().getPosition())) : p.getDistance(unit.getDestination());
            const auto score = 1.0 / (threat * distance);

            // Try to keep the unit alive if it's cloaked inside detection
            if (unit.unit()->isCloaked() && threat > MIN_THREAT && Command::overlapsDetection(unit.unit(), p, PlayerState::Enemy))
                return 0.0;
            return score;
        };

        // Escorting
        auto shouldEscort = unit.getRole() == Role::Support;
        if (!shouldEscort)
            return false;

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.command(UnitCommandTypes::Move, bestPosition, true);
            return true;
        }
        return false;
    }

    bool transport(UnitInfo& unit)
    {
        auto cluster = Positions::Invalid;

        const auto scoreFunction = [&](WalkPosition w) {
            auto p = Position(w);

            if (!w.isValid()
                || (unit.getTransportState() == TransportState::Engaging && Broodwar->getGroundHeight(TilePosition(w)) != Broodwar->getGroundHeight(TilePosition(unit.getDestination())) && p.getDistance(unit.getDestination()) < 64.0))
                return 0.0;

            // If we just dropped units, we need to make sure not to leave them
            if (unit.getTransportState() == TransportState::Monitoring) {
                for (auto &c : unit.getAssignedCargo()) {
                    if (auto &cargo = c.lock()) {

                        if (!cargo->unit()->isLoaded() && cargo->getPosition().getDistance(p) > 64.0)
                            return 0.0;
                        //if (unit.getTransportState() == TransportState::Engaging && !Util::isWalkable(*cargo, w) && p.getDistance(cargo->getDestination()) < 64.0)
                        //    return 0.0;
                    }
                }
            }

            double threat =     unit.getTransportState() == TransportState::Engaging ? 1.0 : defaultThreat(unit, w);
            double distance =   p.getDistance(unit.getDestination()) * p.getDistance(cluster);
            double visited =    1.0;// defaultVisited(unit, w);
            double score =      visited / (threat * distance);

            return score;
        };

        auto highestCluster = 0.0;
        for (auto &[score, position] : Combat::getCombatClusters()) {
            if (score > highestCluster) {
                highestCluster = score;
                cluster = position;
            }
        }

        //Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);

        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid() && bestPosition != unit.getDestination()) {
            unit.command(UnitCommandTypes::Move, bestPosition, true);
            return true;
        }
        return false;
    }

    bool isInDanger(UnitInfo& unit, Position here)
    {
        int halfWidth = int(ceil(unit.getType().width() / 2));
        int halfHeight = int(ceil(unit.getType().height() / 2));

        if (here == Positions::Invalid)
            here = unit.getPosition();

        auto uTopLeft = here + Position(-halfWidth, -halfHeight);
        auto uTopRight = here + Position(halfWidth, -halfHeight);
        auto uBotLeft = here + Position(-halfWidth, halfHeight);
        auto uBotRight = here + Position(halfWidth, halfHeight);

        const auto checkCorners = [&](Position cTopLeft, Position cBotRight) {
            if (Util::rectangleIntersect(cTopLeft, cBotRight, uTopLeft)
                || Util::rectangleIntersect(cTopLeft, cBotRight, uTopRight)
                || Util::rectangleIntersect(cTopLeft, cBotRight, uBotLeft)
                || Util::rectangleIntersect(cTopLeft, cBotRight, uBotRight))
                return true;
            return false;
        };

        // Check that we're not in danger of Storm, DWEB, EMP
        for (auto &command : myActions) {
            if (command.tech == TechTypes::Psionic_Storm
                || command.tech == TechTypes::Disruption_Web
                || command.tech == TechTypes::EMP_Shockwave
                || command.tech == TechTypes::Spider_Mines) {
                auto cTopLeft = command.pos - Position(48, 48);
                auto cBotRight = command.pos + Position(48, 48);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }

            if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
                auto cTopLeft = command.pos - Position(320, 320);
                auto cBotRight = command.pos + Position(320, 320);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }
        }

        for (auto &command : enemyActions) {
            if (command.tech == TechTypes::Psionic_Storm
                || command.tech == TechTypes::Disruption_Web
                || command.tech == TechTypes::EMP_Shockwave
                || command.tech == TechTypes::Spider_Mines) {
                auto cTopLeft = command.pos - Position(48, 48);
                auto cBotRight = command.pos + Position(48, 48);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }
            if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
                auto cTopLeft = command.pos - Position(320, 320);
                auto cBotRight = command.pos + Position(320, 320);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }
        }
        return false;
    }

    bool overlapsDetection(Unit unit, Position here, PlayerState player) {
        vector<Action>* actions;

        if (player == PlayerState::Enemy)
            actions = &enemyActions;
        if (player == PlayerState::Self)
            actions = &myActions;
        if (player == PlayerState::Ally)
            actions = &allyActions;
        if (player == PlayerState::Neutral)
            actions = &neutralActions;

        if (!actions)
            return false;

        // Detection checks use a radial check
        for (auto &action : *actions) {
            if (action.type == UnitTypes::Spell_Scanner_Sweep) {
                if (action.pos.getDistance(here) < 420.0)
                    return true;
            }
            else if (action.type.isDetector()) {
                double range = action.type.isBuilding() ? 256.0 : 360.0;
                if (action.pos.getDistance(here) < range)
                    return true;
            }
        }
        return false;
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateActions();
        Visuals::endPerfTest("Commands");
    }

    vector <Action>& getActions(PlayerState player) {
        if (player == PlayerState::Enemy)
            return enemyActions;
        if (player == PlayerState::Self)
            return myActions;
        if (player == PlayerState::Ally)
            return allyActions;
    }
}
