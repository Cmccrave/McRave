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

            auto distance = double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));
            auto range = (target.isFlying() ? unit.getAirRange() : unit.getGroundRange());

            // No need to calculate for units that don't move or are in range
            if (unit.getRole() == Role::Defender || unit.getSpeed() <= 0.0) {
                unit.setEngagePosition(unit.getPosition());
                unit.setEngDist(0.0);
                return;
            }

            // Create an air distance calculation for engage position
            auto engagePosition = Util::shiftTowards(target.getPosition(), unit.getPosition(), min(distance, range));
            unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));

            // Move engage position closer than distance, so we always move forward in some way
            engagePosition = Util::shiftTowards(engagePosition, target.getPosition(), 32.0);
            unit.setEngagePosition(engagePosition);
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
            if (Players::ZvZ())
                return;

            // Allowed types to surround
            auto allowedTypes ={ Zerg_Zergling, Protoss_Zealot };

            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo& unit = *u;

                if (unit.isFlying()
                    || unit.getType().isBuilding()
                    || (unit.hasAttackedRecently() && unit.getType() != Protoss_Dragoon)
                    || Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()))
                    continue;

                // Get the furthest unit targeting this to offset how many frames to estimate
                auto furthestTargeter = Util::getFurthestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u->hasTarget()
                        && find(allowedTypes.begin(), allowedTypes.end(), u->getType()) != allowedTypes.end()
                        && u->getRole() == Role::Combat
                        && *u->getTarget().lock() == unit;
                });
                if (furthestTargeter) {
                    auto furthestFramesToArrive = (clamp(furthestTargeter->getPosition().getDistance(unit.getPosition()) / furthestTargeter->getSpeed(), 0.0, 48.0));


                    // Figure out how to trap the unit
                    auto trapTowards = unit.getPosition();
                    if (unit.getType().isWorker() && Terrain::inTerritory(PlayerState::Self, unit.getPosition())) {
                        trapTowards = unit.getPosition() + Position(int(unit.unit()->getVelocityX() * furthestFramesToArrive), int(unit.unit()->getVelocityY() * furthestFramesToArrive));
                        trapTowards += Terrain::getMainPosition();
                        trapTowards /= 2.0;
                        furthestFramesToArrive *= 1.5;
                    }
                    else {
                        trapTowards = unit.unit()->getOrderTargetPosition();
                    }
                    //Visuals::drawLine(unit.getPosition(), trapTowards, Colors::Purple);

                    // Create surround positions in a primitive fashion
                    vector<pair<Position, double>> surroundPositions;
                    auto width = (unit.getType().width()) / 2;
                    auto height = (unit.getType().height()) / 2;
                    for (int x = -1; x <= 1; x++) {
                        for (int y = -1; y <= 1; y++) {
                            if (x == 0 && y == 0)
                                continue;
                            auto pos = unit.getPosition() + Position(x * width, y * height);
                            surroundPositions.push_back(make_pair(pos, pos.getDistance(trapTowards)));
                        }
                    }

                    // Sort positions by summed distances
                    sort(surroundPositions.begin(), surroundPositions.end(), [&](auto &l, auto &r) {
                        return l.second < r.second;
                    });

                    // Assign closest targeter
                    int nx = 0;
                    for (auto &[pos, dist] : surroundPositions) {
                        nx++;
                        //Visuals::drawCircle(pos, 4, Colors::Blue);

                        auto closestTargeter = Util::getClosestUnit(pos, PlayerState::Self, [&](auto &u) {
                            return u->hasTarget()
                                && find(allowedTypes.begin(), allowedTypes.end(), u->getType()) != allowedTypes.end()
                                && (!u->getSurroundPosition().isValid() || u->getSurroundPosition() == pos) && u->getRole() == Role::Combat
                                && *u->getTarget().lock() == unit;
                        });

                        // Get time to arrive to the surround position
                        if (closestTargeter) {
                            auto dirx = (trapTowards.x - unit.getPosition().x) / (1.0 + unit.getPosition().getDistance(trapTowards));
                            auto diry = (trapTowards.y - unit.getPosition().y) / (1.0 + unit.getPosition().getDistance(trapTowards));

                            auto expandx = (pos.x - unit.getPosition().x) / unit.getPosition().getDistance(pos);
                            auto expandy = (pos.y - unit.getPosition().y) / unit.getPosition().getDistance(pos);

                            auto closestFramesToArrive = (clamp(pow(closestTargeter->getPosition().getDistance(pos) / closestTargeter->getSpeed(), 1.5), 2.0, 64.0));

                            auto ct_angle = BWEB::Map::getAngle(closestTargeter->getPosition(), pos);
                            auto pos_angle = BWEB::Map::getAngle(pos, unit.getPosition());

                            auto angleDiff = abs(ct_angle - pos_angle);
                            if (angleDiff > 0.25)
                                closestFramesToArrive *= min(2.5, angleDiff);

                            auto correctedPos = pos + Position(int(dirx * closestFramesToArrive), int(diry * closestFramesToArrive)) + Position(int(expandx * closestFramesToArrive), int(expandy * closestFramesToArrive));

                            if (Util::findWalkable(*closestTargeter, correctedPos)) {
                                closestTargeter->setSurroundPosition(correctedPos);
                                //Visuals::drawLine(closestTargeter->getPosition(), correctedPos, Colors::Green);
                                //Visuals::drawCircle(correctedPos, 4, Colors::Green);
                            }
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

    void getHarassPath(UnitInfo& unit, BWEB::Path& path)
    {

    }

    void getExplorePath(UnitInfo& unit, BWEB::Path& path)
    {

    }

    void getDefaultPath(UnitInfo& unit, BWEB::Path& path)
    {

    }

    void onFrame()
    {
        updateSurroundPositions();
        updatePaths();
    }
}