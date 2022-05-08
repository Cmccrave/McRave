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

        void getEngagePosition(UnitInfo& unit)
        {
            // Without a target, we cannot assume an engage position
            if (!unit.hasTarget() || unit.getPlayer() != Broodwar->self()) {
                unit.setEngagePosition(Positions::Invalid);
                return;
            }

            auto unitTarget = unit.getTarget().lock();
            if (unit.getRole() == Role::Defender || unit.isSuicidal()) {
                unit.setEngagePosition(unitTarget->getPosition());
                return;
            }

            // Create a binary search tree in a circle around the target
            auto testPosition = Positions::None;
            auto testDist = 0.0;
            auto up = unitTarget->getPosition();
            pair<double, double> radrange ={ 0.00, 3.14 };
            auto range = unitTarget->isFlying() ? unit.getAirRange() : unit.getGroundRange();
            for (int i = 1; i <= 5; i++) {
                auto diff = (radrange.second - radrange.first) / 4.0;
                auto p1 = up + Position(range*cos(radrange.first), range*sin(radrange.first));
                auto p2 = up + Position(range*cos(radrange.second), range*sin(radrange.second));

                if (!p1.isValid())
                    radrange = make_pair(radrange.second - diff, radrange.second + diff);
                else if (!p2.isValid())
                    radrange = make_pair(radrange.first - diff, radrange.first + diff);
                else {
                    auto dist1 = BWEB::Map::getGroundDistance(p1, unit.getPosition()) + BWEB::Map::getGroundDistance(p1, unitTarget->getPosition());
                    auto dist2 = BWEB::Map::getGroundDistance(p2, unit.getPosition()) + BWEB::Map::getGroundDistance(p2, unitTarget->getPosition());

                    if (i < 5)
                        radrange = dist1 < dist2 ? make_pair(radrange.first - diff, radrange.first + diff) : make_pair(radrange.second - diff, radrange.second + diff);
                    else {
                        testPosition = (dist1 < dist2 ? p1 : p2);
                        testDist = (dist1 < dist2 ? dist1 : dist2);
                    }

                    Broodwar->drawTextMap(p1, "%d", i);
                    Broodwar->drawTextMap(p2, "%d", i);
                }
            }
            unit.setEngagePosition(testPosition);
            unit.setEngDist(testDist);
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
            }

            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo& unit = *u;
                if (unit.hasTarget()) {
                    getEngagePosition(unit);
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