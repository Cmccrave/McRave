#include "Pathing.h"

#include "BWEB.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Terrain.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Pathing {

    namespace {

        void getEngagePosition(UnitInfo &unit, UnitInfo &target)
        {
            // Suicidal units engage on top of the target
            if (unit.isSuicidal()) {
                unit.setEngagePosition(target.getPosition());
                return;
            }

            auto distance = double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));
            auto range    = (target.isFlying() ? unit.getAirRange() : unit.getGroundRange());

            // No need to calculate for units that don't move or are in range
            if (unit.getRole() == Role::Defender || unit.getSpeed() <= 0.0) {
                unit.setEngagePosition(unit.getPosition());
                unit.setEngDist(0.0);
                return;
            }

            // Special case: hydras have a long initial animation with a short repeated animation. Get them much further in range before attacking
            if (unit.getType() == Zerg_Hydralisk) {
                if (!unit.hasAttackedRecently() && !target.isMelee() && unit.getSpeed() >= target.getSpeed() && !target.isSplasher())
                    range = 64.0;
            }

            // Create an air distance calculation for engage position
            auto engagePosition = Util::shiftTowards(target.getPosition(), unit.getPosition(), min(distance, range));
            unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
            unit.setEngagePosition(engagePosition);
        }

        void getInterceptPosition(UnitInfo &unit, UnitInfo &target)
        {
            // If we can't see the units speed, return its current position
            if (!target.unit()->exists() || unit.getSpeed() == 0.0 || target.getSpeed() == 0.0 || !target.getPosition().isValid())
                return;

            auto range          = target.isFlying() ? unit.getAirRange() : unit.getGroundRange();
            auto boxDistance    = Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition());
            auto framesToArrive = clamp((boxDistance - range) / unit.getSpeed(), 0.0, 24.0);
            auto intercept      = target.getPosition() + Position(int(target.unit()->getVelocityX() * framesToArrive), int(target.unit()->getVelocityY() * framesToArrive));
            unit.setInterceptPosition(intercept);
        }

        void updateSurroundPositions()
        {
            if (Players::ZvZ())
                return;

            // Allowed types to surround
            auto allowedTypes = {Zerg_Zergling, Protoss_Zealot};

            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;

                if (unit.isFlying() || unit.getType().isBuilding() || Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()))
                    continue;

                // Get the furthest unit targeting this to offset how many frames to estimate
                auto furthestTargeter = Util::getFurthestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u->hasTarget() && find(allowedTypes.begin(), allowedTypes.end(), u->getType()) != allowedTypes.end() && u->getRole() == Role::Combat && *u->getTarget().lock() == unit;
                });
                if (furthestTargeter) {
                    auto furthestFramesToArrive = (clamp(furthestTargeter->getPosition().getDistance(unit.getPosition()) / furthestTargeter->getSpeed(), 0.0, 24.0));

                    // Figure out how to trap the unit
                    auto trapTowards = unit.getPosition();// +Position(int(unit.unit()->getVelocityX() * furthestFramesToArrive), int(unit.unit()->getVelocityY() * furthestFramesToArrive));
                    if (unit.getType().isWorker() && Terrain::inTerritory(PlayerState::Self, unit.getPosition())) {
                        trapTowards += Position(int(unit.unit()->getVelocityX() * furthestFramesToArrive), int(unit.unit()->getVelocityY() * furthestFramesToArrive));
                        trapTowards /= 2.0;
                        // furthestFramesToArrive *= 1.15;
                    }
                    // Visuals::drawLine(unit.getPosition(), trapTowards, Colors::Purple);

                    // Create surround positions in a circle
                    vector<pair<Position, double>> surroundPositions;
                    double radius = max(unit.getType().width(), unit.getType().height()) * 0.8;
                    for (int i = 0; i < 8; i++) {
                        double angle = (2.0 * M_PI / 8) * i;
                        Position pos(unit.getPosition().x + int(cos(angle) * radius), unit.getPosition().y + int(sin(angle) * radius));

                        surroundPositions.emplace_back(pos, pos.getDistance(trapTowards));
                    }

                    // Sort positions by summed distances
                    sort(surroundPositions.begin(), surroundPositions.end(), [&](auto &l, auto &r) { return l.second < r.second; });

                    // Assign closest targeter
                    int nx = 0;
                    for (auto &[pos, dist] : surroundPositions) {
                        nx++;
                        // Visuals::drawCircle(pos, 4, Colors::Blue);

                        auto closestTargeter = Util::getClosestUnit(pos, PlayerState::Self, [&](auto &u) {
                            return u->hasTarget() && find(allowedTypes.begin(), allowedTypes.end(), u->getType()) != allowedTypes.end() &&
                                   (!u->getSurroundPosition().isValid() || u->getSurroundPosition() == pos) && u->getRole() == Role::Combat && *u->getTarget().lock() == unit;
                        });

                        // Get time to arrive to the surround position
                        if (closestTargeter) {
                            auto dirx = (trapTowards.x - unit.getPosition().x) / (1.0 + unit.getPosition().getDistance(trapTowards));
                            auto diry = (trapTowards.y - unit.getPosition().y) / (1.0 + unit.getPosition().getDistance(trapTowards));

                            auto expandx = (pos.x - unit.getPosition().x) / unit.getPosition().getDistance(pos);
                            auto expandy = (pos.y - unit.getPosition().y) / unit.getPosition().getDistance(pos);

                            auto closestFramesToArrive = (clamp(pow(closestTargeter->getPosition().getDistance(pos) / closestTargeter->getSpeed(), 1.5), 2.0, 64.0));

                            if (!unit.getType().isWorker()) {
                                auto ct_angle  = BWEB::Map::getAngle(closestTargeter->getPosition(), pos);
                                auto pos_angle = BWEB::Map::getAngle(pos, unit.getPosition());

                                auto angleDiff = abs(ct_angle - pos_angle);
                            }

                            auto correctedPos = pos + Position(int(dirx * closestFramesToArrive), int(diry * closestFramesToArrive)) +
                                                Position(int(expandx * closestFramesToArrive), int(expandy * closestFramesToArrive));

                            closestTargeter->setSurroundPosition(correctedPos);
                            Visuals::drawLine(closestTargeter->getPosition(), correctedPos, Colors::Green);
                            Visuals::drawCircle(correctedPos, 4, Colors::Green);
                        }
                    }
                }
            }
        }

        void updatePaths()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.hasTarget()) {
                    auto &target = *unit.getTarget().lock();
                    getEngagePosition(unit, target);
                    getInterceptPosition(unit, target);
                }
            }
        }
    } // namespace

    void getHarassPath(UnitInfo &unit, BWEB::Path &path) {}

    void getExplorePath(UnitInfo &unit, BWEB::Path &path) {}

    void getDefaultPath(UnitInfo &unit, BWEB::Path &path) {}

    void onFrame()
    {
        updateSurroundPositions();
        updatePaths();
    }
} // namespace McRave::Pathing