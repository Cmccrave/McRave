#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Pathing {

    namespace {

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

            if (unit.isWithinRange(*unitTarget)) {
                unit.setEngagePosition(unit.getPosition());
                unit.setEngDist(0.0);
                return;
            }


            //
            auto range = unitTarget->isFlying() ? unit.getAirRange() : unit.getGroundRange();
            if (unit.isFlying()) {
                auto engage = Util::getClosestPointToRadiusAir(unit.getPosition(), unitTarget->getPosition(), range);
                unit.setEngagePosition(engage.second);
                unit.setEngDist(engage.first - range);
            }

            // Create a binary search tree in a circle around the target
            else {
                auto engage = Util::getClosestPointToRadiusGround(unit.getPosition(), unitTarget->getPosition(), range);
                unit.setEngagePosition(engage.second);
                unit.setEngDist(engage.first - range);
            }
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

                if (unit.getUnitsTargetingThis().empty()
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