#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Pathing {

    namespace {

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
                return;
            }

            // Don't generate a target path in these cases
            if (unit.getTarget().isFlying() || unit.isFlying()) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                unit.setTargetPath(newPath);
                return;
            }

            // See if we can borrow a path from a targeter around us
            auto maxDist = Players::getSupply(PlayerState::Self, Races::None) / 2;
            for (auto &t : unit.getTarget().getTargetedBy()) {
                if (auto targeter = t.lock()) {
                    if (unit.getTarget() == targeter->getTarget() && targeter->getTargetPath().isReachable() && targeter->getPosition().getDistance(targeter->getTarget().getPosition()) < unit.getPosition().getDistance(unit.getTarget().getPosition()) && targeter->getPosition().getDistance(unit.getPosition()) < maxDist) {
                        unit.setTargetPath(targeter->getTargetPath());
                        return;
                    }
                }
            }

            // TODO: Need a custom target walkable for a building
            auto targetType = unit.getTarget().getType();
            auto targetPos = unit.getTarget().getPosition();
            const auto targetWalkable = [&](const TilePosition &t) {
                return targetType.isBuilding() && Util::rectangleIntersect(targetPos, targetPos + Position(targetType.tileSize()), Position(t));
            };

            // If should create path, grab one from BWEB
            if (unit.getTargetPath().getTarget() != TilePosition(unit.getTarget().getPosition()) && (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(unit.getTarget().getPosition())) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(unit.getTarget().getPosition()))))) {
                BWEB::Path newPath(unit.getPosition(), unit.getTarget().getPosition(), unit.getType());
                newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t) || targetWalkable(t); });
                unit.setTargetPath(newPath);

                if (!newPath.isReachable() && newPath.unitWalkable(unit.getTilePosition()))
                    unit.getTarget().lastUnreachableFrame = Broodwar->getFrameCount();
            }
        }

        void getEngagePosition(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setEngagePosition(Positions::Invalid);
                return;
            }

            if (unit.getRole() == Role::Defender || unit.isSuicidal()) {
                unit.setEngagePosition(unit.getTarget().getPosition());
                return;
            }

            // Have to set it to at least 64 or units can't path sometimes to engage position
            auto range = max(64.0, unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            if (unit.getTargetPath().isReachable() && (!unit.isWithinRange(unit.getTarget()) || unit.unit()->isLoaded())) {
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
            if (unit.getRole() == Role::Defender && unit.hasTarget()) {
                auto range = unit.getTarget().isFlying() ? unit.getAirRange() : unit.getGroundRange();
                unit.setEngDist(Util::boxDistance(unit.getType(), unit.getPosition(), unit.getTarget().getType(), unit.getTarget().getPosition()) - range);
                return;
            }

            if (!unit.hasTarget() || (unit.getPlayer() == Broodwar->self() && unit.getRole() != Role::Combat)) {
                unit.setEngDist(0.0);
                return;
            }

            if (unit.getPlayer() == Broodwar->self() && !Terrain::isInAllyTerritory(unit.getTilePosition()) && !unit.getTargetPath().isReachable() && !unit.getTarget().getType().isBuilding() && !unit.isFlying() && !unit.getTarget().isFlying() && Grids::getMobility(unit.getTarget().getPosition()) >= 4) {
                unit.setEngDist(DBL_MAX);
                return;
            }

            auto dist = (unit.isFlying() || unit.getTarget().isFlying()) ? unit.getPosition().getDistance(unit.getEngagePosition()) : BWEB::Map::getGroundDistance(unit.getPosition(), unit.getEngagePosition()) - 32.0;
            if (dist == DBL_MAX)
                dist = 2.0 * unit.getPosition().getDistance(unit.getEngagePosition());

            unit.setEngDist(dist);
        }

        void getInterceptPosition(UnitInfo& unit) {

            // If we can't see the units speed, return its current position
            if (!unit.hasTarget()
                || !unit.getTarget().unit()->exists()
                || unit.getSpeed() == 0.0
                || unit.getTarget().getSpeed() == 0.0
                || !unit.getTarget().getPosition().isValid()
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

                if (unit.getTargetedBy().empty())
                    continue;

                // Figure out how to trap the unit
                auto trapTowards = Position(BWEB::Map::getNaturalChoke()->Center());
                if (unit.isThreatening() || Util::getTime() < Time(4, 30))
                    trapTowards = Position(BWEB::Map::getMainChoke()->Center());
                else if (unit.getPosition().isValid() && Terrain::getEnemyStartingPosition().isValid()) {
                    auto path = mapBWEM.GetPath(unit.getPosition(), Terrain::getEnemyStartingPosition());
                    trapTowards = (path.empty() || !path.front()) ? Terrain::getEnemyStartingPosition() : Position(path.front()->Center());
                }

                // Create surround positions in a primitive fashion
                vector<pair<Position, double>> surroundPositions;
                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        if (x == 0 && y == 0)
                            continue;
                        auto p = (unit.getPosition()) + Position(x * unit.getType().width(), y * unit.getType().height());
                        surroundPositions.push_back(make_pair(p, p.getDistance(trapTowards)));
                    }
                }

                // Sort positions by summed distances
                sort(surroundPositions.begin(), surroundPositions.end(), [&](auto &l, auto &r) {
                    return l.second < r.second;
                });

                // Assign closest targeter
                int i = 0;
                for (auto &[pos, dist] : surroundPositions) {
                    Broodwar->drawTextMap(pos, "%d", i);
                    i++;
                    auto closestTargeter = Util::getClosestUnit(pos, PlayerState::Self, [&](auto &u) {
                        return u.hasTarget() && u.getTarget() == unit.weak_from_this()
                            && find(allowedTypes.begin(), allowedTypes.end(), u.getType()) != allowedTypes.end()
                            && (!u.getSurroundPosition().isValid() || u.getSurroundPosition() == pos) && u.getRole() == Role::Combat;
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
                    getPathToTarget(unit);
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