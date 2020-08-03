#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;;

namespace McRave::Util {

    namespace {
        Time gameTime(0, 0);
    }

    const BWEM::ChokePoint * getClosestChokepoint(Position here)
    {
        double distBest = DBL_MAX;
        const BWEM::ChokePoint * closest = nullptr;

        for (auto &area : mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                double dist = Position(choke->Center()).getDistance(here);
                if (dist < distBest) {
                    distBest = dist;
                    closest = choke;
                }
            }
        }
        return closest;
    }

    double getCastLimit(TechType tech)
    {
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Maelstrom || tech == TechTypes::Plague || tech == TechTypes::Ensnare)
            return 1.5;
        if (tech == TechTypes::Stasis_Field)
            return 2.5;
        return 0.0;
    }

    double getCastRadius(TechType tech)
    {
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Stasis_Field || tech == TechTypes::Maelstrom || tech == TechTypes::Dark_Swarm || tech == TechTypes::Plague || tech == TechTypes::Ensnare)
            return 48.0;
        return 1.0;
    }

    bool rectangleIntersect(Position topLeft, Position botRight, Position target)
    {
        if (target.x >= topLeft.x
            && target.x < botRight.x
            && target.y >= topLeft.y
            && target.y < botRight.y)
            return true;
        return false;
    }

    bool rectangleIntersect(Position topLeft, Position botRight, int x, int y)
    {
        if (x >= topLeft.x
            && x < botRight.x
            && y >= topLeft.y
            && y < botRight.y)
            return true;
        return false;
    }

    bool isTightWalkable(UnitInfo& unit, Position here)
    {
        if (unit.getType().isFlyer() && unit.getRole() != Role::Transport)
            return true;

        const auto w = WalkPosition(here);
        const auto hw = unit.getWalkWidth() / 2;
        const auto hh = unit.getWalkHeight() / 2;

        const auto left = max(0, w.x - hw);
        const auto right = min(1024, w.x + hw + (1 - unit.getWalkWidth() % 2));
        const auto top = max(0, w.y - hh);
        const auto bottom = min(1024, w.y + hh + (1 - unit.getWalkWidth() % 2));

        // Rectangle of current unit position
        const auto topLeft = Position(unit.getWalkPosition());
        const auto botRight = topLeft + Position(unit.getWalkWidth() * 8, unit.getWalkHeight() * 8) + Position(8 * (1 - unit.getWalkWidth() % 2), 8 * (1 - unit.getWalkHeight() % 2));

        for (auto x = left; x < right; x++) {
            for (auto y = top; y < bottom; y++) {
                const WalkPosition w(x, y);
                const auto p = Position(w) + Position(4, 4);

                if (rectangleIntersect(topLeft, botRight, p))
                    continue;
                else if (Grids::getMobility(w) < 1 || Grids::getCollision(w) > 0)
                    return false;
            }
        }
        return true;
    }

    Position getInterceptPosition(UnitInfo& unit)
    {
        // If we can't see the units speed, return its current position
        if (!unit.getTarget().unit()->exists() || unit.getSpeed() == 0.0 || unit.getTarget().getSpeed() == 0.0)
            return unit.getTarget().getPosition();

        auto timeToEngage = clamp((unit.getEngDist() / unit.getSpeed()) * unit.getTarget().getSpeed() / unit.getSpeed(), 12.0, 96.0);
        auto targetDestination = unit.getTarget().unit()->getOrderTargetPosition();
        targetDestination = unit.getTarget().unit()->getOrder() != Orders::AttackUnit ? Util::clipPosition(targetDestination) : unit.getTarget().getPosition();
        return targetDestination;
    }

    Position clipLine(Position source, Position target)
    {
        if (target.isValid())
            return target;

        auto sqDist = source.getDistance(target);
        auto clip = clipPosition(target);
        auto dx = clip.x - target.x;
        auto dy = clip.y - target.y;

        if (abs(dx) < abs(dy)) {
            int y = (int)sqrt(sqDist - dx * dx);
            target.x = clip.x;
            if (source.y - y < 0)
                target.y = source.y + y;
            else if (source.y + y >= Broodwar->mapHeight() * 32)
                target.y = source.y - y;
            else
                target.y = (target.y >= source.y) ? source.y + y : source.y - y;
        }
        else {
            int x = (int)sqrt(sqDist - dy * dy);
            target.y = clip.y;

            if (source.x - x < 0)
                target.x = source.x + x;
            else if (source.x + x >= Broodwar->mapWidth() * 32)
                target.x = source.x - x;
            else
                target.x = (target.x >= source.x) ? source.x + x : source.x - x;
        }
        return target;
    }

    Position clipPosition(Position source)
    {
        source.x = clamp(source.x, 0, Broodwar->mapWidth() * 32);
        source.y = clamp(source.y, 0, Broodwar->mapHeight() * 32);
        return source;
    }

    Position vectorProjection(pair<Position, Position> line, Position here)
    {
        auto directionVector = line.second - line.first;
        auto currVector = here - line.first;
        auto projCalc = double((directionVector.x * currVector.x) + (directionVector.y * currVector.y)) / (pow(directionVector.x, 2.0) + pow(directionVector.y, 2.0));
        return (line.first + Position(int(projCalc * directionVector.x), int(projCalc * directionVector.y)));
    }

    Time getTime()
    {
        return gameTime;
    }

    void onStart()
    {

    }

    void onFrame()
    {
        if (Broodwar->getFrameCount() % 24 == 0 && Broodwar->getFrameCount() != 0) {
            gameTime.seconds++;
            if (gameTime.seconds >= 60) {
                gameTime.seconds = 0;
                gameTime.minutes++;
            }
        }
    }
}