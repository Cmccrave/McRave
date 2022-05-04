#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Pathing {

    namespace {

        Position getPathPoint(UnitInfo& unit, Position start)
        {
            // Create a pathpoint
            auto pathPoint = start;
            auto usedTile = BWEB::Map::isUsed(TilePosition(start));
            if (!BWEB::Map::isWalkable(TilePosition(start), unit.getType()) || usedTile != UnitTypes::None) {
                auto dimensions = usedTile != UnitTypes::None ? usedTile.tileSize() : TilePosition(1, 1);
                auto closest = DBL_MAX;
                for (int x = TilePosition(start).x - dimensions.x; x < TilePosition(start).x + dimensions.x + 1; x++) {
                    for (int y = TilePosition(start).y - dimensions.y; y < TilePosition(start).y + dimensions.y + 1; y++) {
                        auto center = Position(TilePosition(x, y)) + Position(16, 16);
                        auto dist = center.getDistance(unit.getPosition());
                        if (dist < closest && BWEB::Map::isWalkable(TilePosition(x, y), unit.getType()) && BWEB::Map::isUsed(TilePosition(x, y)) == UnitTypes::None) {
                            closest = dist;
                            pathPoint = center;
                        }
                    }
                }
            }
            return pathPoint;
        }

        void trySharedPaths(UnitInfo& unit)
        {
            // See if we can borrow a path from a targeter around us
            auto maxDist = Players::getSupply(PlayerState::Self, Races::None) / 2;
            auto unitTarget = unit.getTarget().lock();
            for (auto &t : unitTarget->getTargetedBy()) {
                if (auto targeter = t.lock()) {
                    if (targeter->hasTarget()) {
                        auto targeterTarget = targeter->getTarget().lock();
                        if (unit.getTarget() == targeter->getTarget() && targeter->getTargetPath().isReachable() && targeter->getPosition().getDistance(targeterTarget->getPosition()) < unit.getPosition().getDistance(unitTarget->getPosition()) && targeter->getPosition().getDistance(unit.getPosition()) < maxDist) {
                            unit.setTargetPath(targeter->getTargetPath());
                            unit.setRetreatPath(targeter->getRetreatPath());
                            return;
                        }
                    }
                }
            }
        }

        void getTargetPath(UnitInfo& unit)
        {
            // If unnecessary to get path
            if (unit.getRole() != Role::Combat || !unit.getTargetPath().getTiles().empty())
                return;

            // If no target, no distance/path available
            if (!unit.hasTarget()) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                unit.setEngDist(0.0);
                unit.setTargetPath(newPath);
                return;
            }
            auto unitTarget = unit.getTarget().lock();

            // Don't generate a target path in these cases
            if (unitTarget->isFlying() || unit.isFlying()) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                unit.setTargetPath(newPath);
                return;
            }

            // TODO: Need a custom target walkable for a building
            const auto targetWalkable = [&](const TilePosition &t) {
                return unitTarget->getType().isBuilding() && Util::rectangleIntersect(unitTarget->getPosition(), unitTarget->getPosition() + Position(unitTarget->getType().tileSize()), Position(t));
            };

            // If should create path, grab one from BWEB
            if (unit.getTargetPath().getTarget() != TilePosition(unitTarget->getPosition()) && (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(unitTarget->getPosition())) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(unitTarget->getPosition()))))) {
                BWEB::Path newPath(unit.getPosition(), unitTarget->getPosition(), unit.getType());
                newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t) || targetWalkable(t); });
                unit.setTargetPath(newPath);

                if (!newPath.isReachable() && newPath.unitWalkable(unit.getTilePosition()))
                    unitTarget->lastUnreachableFrame = Broodwar->getFrameCount();
            }
        }

        void getRetreatPath(UnitInfo& unit)
        {
            // If unnecessary to get path
            if (unit.getRole() != Role::Combat || !unit.getRetreatPath().getTiles().empty())
                return;

            // Generate a new path that obeys collision of terrain and buildings
            auto needPath = unit.getRetreatPath().getTiles().empty();
            if (!unit.isFlying() && needPath && unit.getRetreatPath().getTarget() != TilePosition(unit.getRetreat())) {
                auto pathPoint = getPathPoint(unit, unit.getRetreat());
                if (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)))) {
                    BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());
                    newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
                    unit.setRetreatPath(newPath);
                }
            }
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngagePosition(Positions::Invalid);
                return;
            }
            auto unitTarget = unit.getTarget().lock();

            if (unit.getRole() == Role::Defender || unit.isSuicidal()) {
                unit.setEngagePosition(unitTarget->getPosition());
                return;
            }

            // Have to set it to at least 64 or units can't path sometimes to engage position
            auto range = max(64.0, unitTarget->getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            if (unit.getTargetPath().isReachable() && (!unit.isWithinRange(*unitTarget) || unit.unit()->isLoaded())) {
                auto usedType = BWEB::Map::isUsed(unitTarget->getTilePosition());

                auto engagePosition = Util::findPointOnPath(unit.getTargetPath(), [&](Position p) {
                    auto usedHere = BWEB::Map::isUsed(TilePosition(p));
                    return usedHere == UnitTypes::None && Util::boxDistance(unitTarget->getType(), unitTarget->getPosition(), unit.getType(), p) <= range;
                });

                if (engagePosition.isValid()) {
                    unit.setEngagePosition(engagePosition);
                    return;
                }
            }

            auto distance = Util::boxDistance(unit.getType(), unit.getPosition(), unitTarget->getType(), unitTarget->getPosition());
            auto direction = ((distance - range) / distance);
            auto engageX = int((unit.getPosition().x - unitTarget->getPosition().x) * direction);
            auto engageY = int((unit.getPosition().y - unitTarget->getPosition().y) * direction);
            auto engagePosition = unit.getPosition() - Position(engageX, engageY);

            // If unit is loaded or further than their range, we want to calculate the expected engage position
            if (distance > range || unit.unit()->isLoaded())
                unit.setEngagePosition(engagePosition);
            else
                unit.setEngagePosition(unit.getPosition());
        }

        void getEngageDistance(UnitInfo& unit)
        {
            if (!unit.hasTarget() || (unit.getPlayer() == Broodwar->self() && unit.getRole() != Role::Combat)) {
                unit.setEngDist(0.0);
                return;
            }
            auto unitTarget = unit.getTarget().lock();

            if (unit.getRole() == Role::Defender && unit.hasTarget()) {
                auto range = unitTarget->isFlying() ? unit.getAirRange() : unit.getGroundRange();
                unit.setEngDist(Util::boxDistance(unit.getType(), unit.getPosition(), unitTarget->getType(), unitTarget->getPosition()) - range);
                return;
            }

            if (unit.getPlayer() == Broodwar->self() && !Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && !unit.getTargetPath().isReachable() && !unitTarget->getType().isBuilding() && !unit.isFlying() && !unitTarget->isFlying() && Grids::getMobility(unitTarget->getPosition()) >= 4) {
                unit.setEngDist(DBL_MAX);
                return;
            }

            auto dist = (unit.isFlying() || unitTarget->isFlying()) ? unit.getPosition().getDistance(unit.getEngagePosition()) : BWEB::Map::getGroundDistance(unit.getPosition(), unit.getEngagePosition()) - 32.0;
            if (dist == DBL_MAX)
                dist = 2.0 * unit.getPosition().getDistance(unit.getEngagePosition());

            unit.setEngDist(dist);
        }

        void getInterceptPosition(UnitInfo& unit)
        {
            if (unit.getTarget().expired())
                return;

            // If we can't see the units speed, return its current position
            auto unitTarget = unit.getTarget().lock();
            if (!unitTarget->unit()->exists()
                || unit.getSpeed() == 0.0
                || unitTarget->getSpeed() == 0.0
                || !unitTarget->getPosition().isValid()
                || !Terrain::getEnemyStartingPosition().isValid())
                return;

            unit.setInterceptPosition(Positions::Invalid);
        }

        void updateSurroundPositions()
        {
            if (Players::ZvZ())
                return;

            // Allowed types to surround
            auto allowedTypes ={ Zerg_Zergling, Protoss_Zealot };

            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo& unit = *u;

                if (unit.getTargetedBy().empty()
                    || unit.isFlying())
                    continue;

                // Figure out how to trap the unit
                auto trapTowards = Terrain::getEnemyStartingPosition();
                if (BWEB::Map::getNaturalChoke())
                    trapTowards = Position(BWEB::Map::getNaturalChoke()->Center());
                if (BWEB::Map::getMainChoke() && (unit.isThreatening() || Util::getTime() < Time(4, 30)))
                    trapTowards = Position(BWEB::Map::getMainChoke()->Center());
                else if (unit.getPosition().isValid() && Terrain::getEnemyStartingPosition().isValid()) {
                    auto path = mapBWEM.GetPath(unit.getPosition(), Terrain::getEnemyStartingPosition());
                    trapTowards = (path.empty() || !path.front()) ? Terrain::getEnemyStartingPosition() : Position(path.front()->Center());
                }

                // Create surround positions in a primitive fashion
                vector<pair<Position, double>> surroundPositions;
                auto width = unit.getType().isBuilding() ? unit.getType().tileWidth() * 16 : unit.getType().width();
                auto height = unit.getType().isBuilding() ? unit.getType().tileHeight() * 16 : unit.getType().height();
                for (double x = -1.0; x <= 1.0; x += 1.0 / double(unit.getType().tileWidth())) {
                    auto p = (unit.getPosition()) + Position(int(x * width), int(-1.0 * height));
                    auto q = (unit.getPosition()) + Position(int(x * width), int(1.0 * height));
                    surroundPositions.push_back(make_pair(p, p.getDistance(trapTowards)));
                    surroundPositions.push_back(make_pair(q, q.getDistance(trapTowards)));
                }
                for (double y = -1.0; y <= 1.0; y += 1.0 / double(unit.getType().tileHeight())) {
                    if (y <= -0.99 || y >= 0.99)
                        continue;
                    auto p = (unit.getPosition()) + Position(int(-1.0 * width), int(y * height));
                    auto q = (unit.getPosition()) + Position(int(1.0 * width), int(y * height));
                    surroundPositions.push_back(make_pair(p, p.getDistance(trapTowards)));
                    surroundPositions.push_back(make_pair(q, q.getDistance(trapTowards)));
                }

                // Sort positions by summed distances
                sort(surroundPositions.begin(), surroundPositions.end(), [&](auto &l, auto &r) {
                    return l.second < r.second;
                });

                // Assign closest targeter
                for (auto &[pos, dist] : surroundPositions) {
                    auto closestTargeter = Util::getClosestUnit(pos, PlayerState::Self, [&](auto &u) {
                        return u->hasTarget()
                            && find(allowedTypes.begin(), allowedTypes.end(), u->getType()) != allowedTypes.end()
                            && (!u->getSurroundPosition().isValid() || u->getSurroundPosition() == pos) && u->getRole() == Role::Combat
                            && *u->getTarget().lock() == unit;
                    });

                    // Get time to arrive to the surround position
                    if (closestTargeter) {
                        auto speedDiff = closestTargeter->getSpeed() - unit.getSpeed();
                        auto framesToCatchUp = (speedDiff * closestTargeter->getPosition().getDistance(pos) / closestTargeter->getSpeed()) + (closestTargeter->getPosition().getDistance(pos) / closestTargeter->getSpeed());
                        auto correctedPos = pos + Position(int(unit.unit()->getVelocityX() * framesToCatchUp), int(unit.unit()->getVelocityY() * framesToCatchUp));

                        if (Util::findWalkable(*closestTargeter, correctedPos))
                            closestTargeter->setSurroundPosition(correctedPos);
                    }
                }
            }
        }

        void updatePaths()
        {
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo& unit = *u;
                getEngagePosition(unit);
                getEngageDistance(unit);
            }

            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo& unit = *u;
                if (unit.hasTarget()) {
                    trySharedPaths(unit);
                    getTargetPath(unit);
                    getRetreatPath(unit);
                    getEngagePosition(unit);
                    getEngageDistance(unit);
                    getInterceptPosition(unit);
                }
            }
        }
    }

    void onFrame()
    {
        updateSurroundPositions();
        updatePaths();
    }
}