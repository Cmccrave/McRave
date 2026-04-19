#include "Pathing.h"

#include "BWEB.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
#include "Micro/Combat/Combat.h"

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

            // Special case: units that "swipe" attack will usually move a few pixels too close after their attack due to decel
            if (unit.getType() == Zerg_Mutalisk || unit.getType() == Terran_Wraith)
                range -= 48.0;

            // Create an air distance calculation for engage position
            //auto distance       = double(Util::boxDistance(unit, target));
            //auto engagePosition = Util::shiftTowards(target.getPosition(), unit.getPosition(), min(distance, range));
            //unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
            //unit.setEngagePosition(engagePosition);

            auto distance = double(Util::boxDistance(unit, target));
            auto moveDist = std::max(0.0, distance - range);
            auto engagePosition = Util::shiftTowards(unit.getPosition(), target.getPosition(), moveDist);
            unit.setEngagePosition(engagePosition);
            unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));

            if (unit.unit()->isSelected())
                Visuals::drawCircle(engagePosition, 10, Colors::Purple);
        }

        void getInterceptPosition(UnitInfo &unit, UnitInfo &target)
        {
            // If we can't see the units speed, return its current position
            if (!target.unit()->exists() || unit.getSpeed() == 0.0 || target.getSpeed() == 0.0 || !target.getPosition().isValid())
                return;

            // Only generate for light air for now
            if (!unit.isLightAir() || unit.isWithinRange(target))
                return;

            auto toTarget  = target.getPosition() - unit.getPosition();
            auto time      = max(target.getPosition().getDistance(unit.getPosition()) / unit.getSpeed(), 1.0);
            auto intercept = target.getPosition() + Position(int(round(target.unit()->getVelocityX() * time)), int(round(target.unit()->getVelocityY() * time)));

            // Only need to intercept if it's moving away
            if (intercept.getDistance(unit.getPosition()) > target.getPosition().getDistance(unit.getPosition()))
                unit.setInterceptPosition(intercept);
        }

        using PositionScore = std::pair<Position, double>;
        template <typename Generator> std::vector<PositionScore> generatePositions(UnitInfo &enemy, Position biasTowards, Generator gen)
        {
            std::vector<PositionScore> positions;
            for (auto &pos : gen(enemy, biasTowards)) {
                positions.emplace_back(pos, pos.getDistance(biasTowards));
            }

            std::sort(positions.begin(), positions.end(), [](auto &l, auto &r) { return l.second < r.second; });
            return positions;
        }

        auto lineGenerator = [&](UnitInfo &enemy, Position biasTowards) {
            vector<Position> result;
            Position dir = biasTowards - enemy.getPosition();

            double length = max(1.0, enemy.getPosition().getDistance(biasTowards));
            double dx     = dir.x / length;
            double dy     = dir.y / length;

            int fx = int(dx * 32.0);
            int fy = int(dy * 32.0);

            for (int i = -3; i < 3; i++) {
                auto x = int(-dy * i * (enemy.getType().width() - 1)) + fx;
                auto y = int(dx * i * (enemy.getType().height() - 1)) + fy;
                result.emplace_back(enemy.getPosition().x + x, enemy.getPosition().y + y);
            }

            return result;
        };

        auto circleGenerator = [&](UnitInfo &enemy, Position biasTowards) {
            std::vector<Position> result;

            double radius = std::max(enemy.getType().width(), enemy.getType().height()) * 0.8;

            for (int i = 0; i < 8; i++) {
                double angle = (2.0 * M_PI / 8) * i;
                result.emplace_back(enemy.getPosition().x + int(cos(angle) * radius), enemy.getPosition().y + int(sin(angle) * radius));
            }

            return result;
        };

        void updateInterceptPositions(UnitInfo &enemy) {}

        void updateTrapPositions(UnitInfo &enemy)
        {
            if (enemy.getType() != Terran_Vulture || enemy.getCurrentSpeed() == 0.0)
                return;

            // Figure out how to trap the unit
            auto biasTowards    = Terrain::inTerritory(PlayerState::Self, enemy.getPosition()) ? Combat::getDefendPosition() : Position(Util::getClosestChokepoint(enemy.getPosition())->Center());
            auto closestStation = Stations::getClosestStationAir(enemy.getPosition(), PlayerState::Self);
            if (closestStation && closestStation->isNatural()) {
                auto closestMain = BWEB::Stations::getClosestMainStation(enemy.getPosition());
                if (closestMain && Stations::ownedBy(closestMain) == PlayerState::Self)
                    biasTowards = Terrain::getMainRamp().entrance;
            }

            static set<UnitType> allowedTypes = {Zerg_Zergling, Protoss_Zealot};

            auto assignPosition = [&](UnitInfo &unit, Position pos) {
                double distToBias = enemy.getPosition().getDistance(biasTowards);
                double distToPos  = enemy.getPosition().getDistance(pos);

                auto dirx = (biasTowards.x - enemy.getPosition().x) / max(1.0, distToBias);
                auto diry = (biasTowards.y - enemy.getPosition().y) / max(1.0, distToBias);

                auto expandx = (pos.x - unit.getPosition().x) / max(1.0, distToPos);
                auto expandy = (pos.y - unit.getPosition().y) / max(1.0, distToPos);

                auto frames = clamp(unit.getPosition().getDistance(pos) / unit.getSpeed(), 2.0, 64.0);

                auto percentSpeed = enemy.getCurrentSpeed() / enemy.getSpeed();

                double factor      = distToBias / 64.0;
                auto weightTowards = (frames + 12) * factor;
                auto weightExpand  = (frames + 2);
                auto finalPosition = pos + Position(int(dirx * weightTowards), int(diry * weightTowards)) + Position(int(expandx * weightExpand), int(expandy * weightExpand));

                Visuals::drawLine(unit.getPosition(), finalPosition, Colors::Green);
                Visuals::drawCircle(finalPosition, 4, Colors::Green);

                if (Util::findWalkable(unit, finalPosition))
                    unit.setTrapPosition(finalPosition);
            };

            auto positions = generatePositions(enemy, biasTowards, lineGenerator);
            for (auto &[pos, dist] : positions) {
                auto closestTargeter = Util::getClosestUnit(pos, PlayerState::Self, [&](auto &u) {
                    return allowedTypes.find(u->getType()) != allowedTypes.end() && u->hasTarget() && u->getRole() == Role::Combat &&
                           (!u->getTrapPosition().isValid() || u->getTrapPosition() == pos) && *u->getTarget().lock() == enemy;
                });
                if (closestTargeter)
                    assignPosition(*closestTargeter, pos);
            }
        }

        void updateSurroundPositions(UnitInfo &enemy)
        {
            // List of viable surround targets
            static set<UnitType> surroundTypes = {Protoss_Zealot, Protoss_Dragoon, Terran_Goliath, Terran_Siege_Tank_Siege_Mode, Terran_Siege_Tank_Tank_Mode};
            if (surroundTypes.find(enemy.getType()) == surroundTypes.end())
                return;

            // List of viable surround targeters
            static set<UnitType> allowedTypes = {Zerg_Zergling, Protoss_Zealot};
            auto biasTowards                  = enemy.getPosition() + Position(int(enemy.unit()->getVelocityX() * 24.0), int(enemy.unit()->getVelocityY() * 24.0));

            // Need at least 5 units targeting to surround
            auto cnt = 0;
            for (auto &targeter : enemy.getUnitsTargetingThis()) {
                if (Util::contains(allowedTypes, targeter.lock()->getType()))
                    cnt++;            
            }
            if (cnt < 5)
                return;

            auto assignPosition = [&](UnitInfo &unit, Position pos) {
                double distToBias = enemy.getPosition().getDistance(biasTowards);
                double distToPos  = enemy.getPosition().getDistance(pos);

                auto dirx = (biasTowards.x - enemy.getPosition().x) / max(1.0, distToBias);
                auto diry = (biasTowards.y - enemy.getPosition().y) / max(1.0, distToBias);

                auto expandx = (pos.x - unit.getPosition().x) / max(1.0, distToPos);
                auto expandy = (pos.y - unit.getPosition().y) / max(1.0, distToPos);

                auto frames = clamp(pow(unit.getPosition().getDistance(pos) / unit.getSpeed(), 1.5), 2.0, 64.0);

                auto finalPosition = pos + Position(int(dirx * frames), int(diry * frames)) + Position(int(expandx * frames), int(expandy * frames));

                // Visuals::drawLine(unit.getPosition(), finalPosition, Colors::Green);
                // Visuals::drawCircle(finalPosition, 4, Colors::Green);
                unit.setSurroundPosition(finalPosition);
            };

            auto positions = generatePositions(enemy, biasTowards, circleGenerator);
            for (auto &[pos, dist] : positions) {
                auto closestTargeter = Util::getClosestUnit(pos, PlayerState::Self, [&](auto &u) {
                    return allowedTypes.find(u->getType()) != allowedTypes.end() && u->hasTarget() && u->getRole() == Role::Combat &&
                           (!u->getSurroundPosition().isValid() || u->getSurroundPosition() == pos) && *u->getTarget().lock() == enemy;
                });
                if (closestTargeter)
                    assignPosition(*closestTargeter, pos);
            }
        }

        void updatePositions()
        {
            for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &enemy = *e;

                if (enemy.isFlying() || enemy.getType().isBuilding() || Terrain::inTerritory(PlayerState::Enemy, enemy.getPosition()))
                    continue;

                updateInterceptPositions(enemy);
                updateTrapPositions(enemy);
                updateSurroundPositions(enemy);
            }

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

    void onFrame() { updatePositions(); }
} // namespace McRave::Pathing