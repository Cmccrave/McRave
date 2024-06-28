#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Pathing {

    namespace {

        void getEngagePosition(UnitInfo& unit, UnitInfo& target)
        {
            // Suicidal units engage on top of the target
            if (unit.isSuicidal()) {
                unit.setEngagePosition(target.getPosition());
                return;
            }

            auto distance = Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition());
            auto range = target.isFlying() ? unit.getAirRange() : unit.getGroundRange();

            // No need to calculate for units that don't move or are in range
            if (unit.getRole() == Role::Defender || unit.getSpeed() <= 0.0 || distance <= range) {
                unit.setEngagePosition(unit.getPosition());
                unit.setEngDist(0.0);
                return;
            }

            // Create an air distance calculation for engage position for flyers
            if (unit.isFlying() || unit.hasTransport() || true) {                
                auto direction = ((distance - range) / distance);
                auto engageX = int((unit.getPosition().x - target.getPosition().x) * direction);
                auto engageY = int((unit.getPosition().y - target.getPosition().y) * direction);
                auto engagePosition = unit.getPosition() - Position(engageX, engageY);
                unit.setEngagePosition(engagePosition);
                unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
            }

            // Create a binary search tree in a circle around the target
            else {
                const auto calc = [&](auto p) {
                    //return p.getDistance(unit.getPosition());
                    return BWEB::Map::getGroundDistance(p, unit.getPosition()) + BWEB::Map::getGroundDistance(p, target.getPosition());
                };
                auto engage = Util::findPointOnCircle(unit.getPosition(), target.getPosition(), range, calc);
                unit.setEngagePosition(engage.second);
                unit.setEngDist(engage.first);
            }
        }

        void getInterceptPosition(UnitInfo& unit, UnitInfo& target)
        {
            // If we can't see the units speed, return its current position
            if (!target.unit()->exists()
                || unit.getSpeed() == 0.0
                || target.getSpeed() == 0.0
                || !target.getPosition().isValid())
                return;

            auto range = target.isFlying() ? unit.getAirRange() : unit.getGroundRange();
            auto boxDistance = Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition());
            auto framesToArrive = clamp((boxDistance - range) / unit.getSpeed(), 0.0, 24.0);
            auto intercept = target.getPosition() + Position(int(target.unit()->getVelocityX() * framesToArrive), int(target.unit()->getVelocityY() * framesToArrive));
            unit.setInterceptPosition(intercept);
        }

        void updateSurroundPositions()
        {
            return;
            if (Players::ZvZ())
                return;

            // Allowed types to surround
            auto allowedTypes ={ Zerg_Zergling, Protoss_Zealot };

            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo& unit = *u;

                if (unit.isFlying() || unit.getType().isBuilding())
                    continue;

                // Figure out how to trap the unit
                auto trapTowards = Terrain::getEnemyStartingPosition();
                if (unit.unit()->getOrder() == Orders::Move) {
                    if (unit.getCurrentSpeed() > 0.0)
                        trapTowards = unit.unit()->getOrderTargetPosition();
                    else
                        continue;
                }
                else if (unit.getPosition().isValid() && Terrain::getEnemyStartingPosition().isValid()) {
                    auto path = mapBWEM.GetPath(unit.getPosition(), Terrain::getEnemyStartingPosition());
                    trapTowards = (path.empty() || !path.front()) ? Terrain::getEnemyStartingPosition() : Position(path.front()->Center());
                }
                else
                    continue;

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
                        auto framesToArrive = clamp(closestTargeter->getPosition().getDistance(pos) / unit.getSpeed(), 0.0, 48.0);
                        auto dirx = (trapTowards.x - unit.getPosition().x) / unit.getPosition().getDistance(trapTowards);
                        auto diry = (trapTowards.y - unit.getPosition().y) / unit.getPosition().getDistance(trapTowards);

                        auto expandx = (pos.x - unit.getPosition().x) / unit.getPosition().getDistance(pos);
                        auto expandy = (pos.y - unit.getPosition().y) / unit.getPosition().getDistance(pos);

                        auto correctedPos = pos + Position(int(dirx * framesToArrive), int(diry * framesToArrive)) + Position(int(expandx * framesToArrive), int(expandy * framesToArrive));

                        if (Util::findWalkable(*closestTargeter, correctedPos)) {
                            closestTargeter->setSurroundPosition(correctedPos);
                            Visuals::drawLine(closestTargeter->getPosition(), correctedPos, Colors::Green);
                            Broodwar->drawCircleMap(correctedPos, 4, Colors::Green);
                        }
                    }
                }
            }
        }

        void updatePaths()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo& unit = *u;
                if (unit.hasTarget()) {
                    auto &target = *unit.getTarget().lock();
                    getEngagePosition(unit, target);
                    getInterceptPosition(unit, target);
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