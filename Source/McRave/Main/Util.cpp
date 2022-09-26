#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;;

namespace McRave::Util {

    namespace {
        Time gameTime(0, 0);

        /// Approximation of Euclidian distance
        /// This is the same approximation that StarCraft's engine uses
        /// and thus should be more accurate than true Euclidian distance
        unsigned int disthelper(unsigned int dx, unsigned int dy) {
            if (dx < dy) {
                std::swap(dx, dy);
            }
            if (dx / 4u < dy) {
                dx = dx - dx / 16u + dy * 3u / 8u - dx / 64u + dy * 3u / 256u;
            }
            return dx;
        }

        /// Pixel distance
        unsigned int pxdistance(int px1, int py1, int px2, int py2) {
            unsigned int dx = std::abs(px1 - px2);
            unsigned int dy = std::abs(py1 - py2);
            return disthelper(dx, dy);
        }

        /// Distance between two bounding boxes, in pixels.
        /// Brood War uses bounding boxes for both collisions and range checks
        int pxDistanceBB(int xminA, int yminA, int xmaxA, int ymaxA, int xminB, int yminB, int xmaxB, int ymaxB) {
            if (xmaxB < xminA) { // To the left
                if (ymaxB < yminA) { // Fully above
                    return pxdistance(xmaxB, ymaxB, xminA, yminA);
                }
                else if (yminB > ymaxA) { // Fully below
                    return pxdistance(xmaxB, yminB, xminA, ymaxA);
                }
                else { // Adjecent
                    return xminA - xmaxB;
                }
            }
            else if (xminB > xmaxA) { // To the right
                if (ymaxB < yminA) { // Fully above
                    return pxdistance(xminB, ymaxB, xmaxA, yminA);
                }
                else if (yminB > ymaxA) { // Fully below
                    return pxdistance(xminB, yminB, xmaxA, ymaxA);
                }
                else { // Adjecent
                    return xminB - xmaxA;
                }
            }
            else if (ymaxB < yminA) { // Above
                return yminA - ymaxB;
            }
            else if (yminB > ymaxA) { // Below
                return yminB - ymaxA;
            }
            return 0;
        }
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
        if (tech == TechTypes::Plague)
            return 6.0;
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Maelstrom || tech == TechTypes::Ensnare)
            return 1.5;
        if (tech == TechTypes::Stasis_Field)
            return 1.5;
        return 0.0;
    }

    double getCastRadius(TechType tech)
    {
        if (tech == TechTypes::Dark_Swarm)
            return 96.0;
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Stasis_Field || tech == TechTypes::Maelstrom || tech == TechTypes::Plague || tech == TechTypes::Ensnare)
            return 48.0;
        return 1.0;
    }

    int boxDistance(BWAPI::UnitType typeA, BWAPI::Position posA, BWAPI::UnitType typeB, BWAPI::Position posB) {
        return pxDistanceBB(
            posA.x - typeA.dimensionLeft(),
            posA.y - typeA.dimensionUp(),
            posA.x + typeA.dimensionRight() + 1,
            posA.y + typeA.dimensionDown() + 1,
            posB.x - typeB.dimensionLeft(),
            posB.y - typeB.dimensionUp(),
            posB.x + typeB.dimensionRight() + 1,
            posB.y + typeB.dimensionDown() + 1);
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

    bool findWalkable(Position current, UnitType type, Position& here, bool visual)
    {
        // Take in a position by reference, create rectangles
        const auto currTopLeft = Position(current.x - type.dimensionLeft(), current.y - type.dimensionUp());
        const auto currBotRight = Position(current.x + type.dimensionRight() + 1, current.y + type.dimensionDown() + 1);
        auto hereTopLeft = Position(here.x - type.dimensionLeft(), here.y - type.dimensionUp());
        auto hereBotRight = Position(here.x + type.dimensionRight() + 1, here.y + type.dimensionDown() + 1);

        auto ovLeft = (8 - hereTopLeft.x % 8);
        auto ovRight = hereBotRight.x % 8;
        auto ovUp = (8 - hereTopLeft.y % 8);
        auto ovDown = hereBotRight.y % 8;

        // Checks if this walkposition touches the current bounding box of the unit
        const auto rectanglesTouch = [&](WalkPosition w) {
            return rectangleIntersect(currTopLeft, currBotRight, Position(w))
                || rectangleIntersect(currTopLeft, currBotRight, Position(w) + Position(0, 8))
                || rectangleIntersect(currTopLeft, currBotRight, Position(w) + Position(8, 0))
                || rectangleIntersect(currTopLeft, currBotRight, Position(w) + Position(8, 8));
        };

        const auto pixelSpace = [&](WalkPosition w, int curr, function<int(WalkPosition, PlayerState)> collision) {
            if (!w.isValid())
                return 0;
            if (!rectanglesTouch(w)) {
                if (Grids::getFCollision(w, PlayerState::All) > 0 || !Broodwar->isWalkable(w))
                    return 0;
                curr = min(curr, 8 - collision(w, PlayerState::All));
            }
            return curr;
        };

        // Check if inside the new rectangle there is collision
        for (auto x = hereTopLeft.x / 8 + 1; x <= hereBotRight.x / 8 - 1; x++) {
            for (auto y = hereTopLeft.y / 8 + 1; y <= hereBotRight.y / 8 - 1; y++) {
                WalkPosition w(x, y);
                if (!w.isValid() || rectanglesTouch(w))
                    continue;
                //if (visual)
                //    Visuals::walkBox(w, Colors::Yellow, true);
                if (Grids::getFCollision(w, PlayerState::All) > 0 || !Broodwar->isWalkable(w) || Grids::getVCollision(w, PlayerState::All) || Grids::getHCollision(w, PlayerState::All))
                    return false;
            }
        }

        // Create a box of WalkPosition along the bounding box of this UnitType, with center placed at "here"
        // Count the number of pixels available (+) on each side and the number of overlap pixels (-)
        auto availableUp = 8;
        auto availableDown = 8;
        for (auto x = hereTopLeft.x / 8; x <= hereBotRight.x / 8; x++) {
            const WalkPosition wUp(x, hereTopLeft.y / 8);
            const WalkPosition wDown(x, hereBotRight.y / 8);

            if (visual) {
                //Visuals::walkBox(wUp, Colors::Red);
                //Visuals::walkBox(wDown, Colors::Red);
            }

            availableUp = pixelSpace(wUp, availableUp, Grids::getVCollision);
            availableDown = pixelSpace(wDown, availableDown, Grids::getVCollision);
        }

        // If the y difference cannot fit the unit
        auto allowedVerticalSpace = hereBotRight.y - hereTopLeft.y + (availableUp - ovUp) + (availableDown - ovDown);
        if (allowedVerticalSpace < type.height())
            return false;

        auto availableLeft = 8;
        auto availableRight = 8;
        for (auto y = hereTopLeft.y / 8; y <= hereBotRight.y / 8; y++) {
            const WalkPosition wLeft(hereTopLeft.x / 8, y);
            const WalkPosition wRight(hereBotRight.x / 8, y);

            if (visual) {
                //Visuals::walkBox(wLeft, Colors::Blue);
                //Visuals::walkBox(wRight, Colors::Blue);
            }

            availableLeft = pixelSpace(wLeft, availableLeft, Grids::getHCollision);
            availableRight = pixelSpace(wRight, availableRight, Grids::getHCollision);
        }

        // If the x difference cannot fit the unit
        auto allowedHorizontalSpace = hereBotRight.x - hereTopLeft.x + (availableLeft - ovLeft) + (availableRight - ovRight);
        if (allowedHorizontalSpace < type.width())
            return false;

        // Check if we need to nudge the position vertically
        if (availableUp < ovUp)
            here.y += max(availableUp - ovUp, availableDown - ovDown);
        if (availableDown < ovDown)
            here.y -= max(availableUp - ovUp, availableDown - ovDown);

        // Check if we need to nudge the position horizontally
        if (availableLeft < ovLeft)
            here.x += max(availableLeft - ovLeft, availableRight - ovRight);
        if (availableRight < ovRight)
            here.x -= max(availableLeft - ovLeft, availableRight - ovRight);
        return true;
    }

    bool findWalkable(UnitInfo& unit, Position& here, bool visual) {
        return findWalkable(unit.getPosition(), unit.getType(), here, visual);
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
        source.x = clamp(source.x, 0, (Broodwar->mapWidth() * 32) - 1);
        source.y = clamp(source.y, 0, (Broodwar->mapHeight() * 32) - 1);
        return source;
    }

    Position projectLine(pair<Position, Position> line, Position here)
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
        auto seconds = int(double(Broodwar->getFrameCount()) / 23.81) % 60;
        auto minutes = int(double(Broodwar->getFrameCount()) / 23.81) / 60;
        gameTime.seconds = seconds;
        gameTime.minutes = minutes;
    }

    pair<double, Position> getClosestPointToRadiusAir(Position source, Position target, double radius)
    {
        auto diff = source - target;
        auto dist = source.getDistance(target);
        return { dist, source - (diff * ((dist - radius) / dist)) };
    }

    pair<double, Position> getClosestPointToRadiusGround(Position source, Position target, double radius)
    {
        auto tileSource = TilePosition(source);
        auto newSource = tileSource;
        auto tries = 0;
        while (!BWEB::Map::isWalkable(tileSource, Protoss_Dragoon) && tries < 10) {
            auto closestDist = DBL_MAX;
            tileSource = newSource;
            for (auto x = -1; x <= 1; x++) {
                for (auto y = -1; y <= 1; y++) {
                    auto newTile = tileSource + TilePosition(x, y);
                    auto dist = (Position(newTile) + Position(16, 16)).getDistance(source);
                    if (dist < closestDist && BWEB::Map::isWalkable(tileSource, Protoss_Dragoon)) {
                        newSource = newTile;
                        closestDist = dist;
                    }
                }
            }
            tries++;
        }
        source = Position(newSource) + Position(16, 16);

        // Create a search tree in a circle around the target
        auto position = target;
        auto dist = source.getDistance(target);
        pair<double, double> radrange ={ 0.00, 3.14 };
        for (int i = 1; i <= 10; i++) {
            auto diff = (radrange.second - radrange.first) / double(i + 1); // Allows for correction in the event that the first few points are unwalkable
            auto p1 = target + Position(radius*cos(radrange.first), radius*sin(radrange.first));
            auto p2 = target + Position(radius*cos(radrange.second), radius*sin(radrange.second));

            if (!p1.isValid()) {
                radrange ={ radrange.second - diff, radrange.second + diff };
            }
            else if (!p2.isValid()) {
                radrange ={ radrange.first - diff, radrange.first + diff };
            }
            else {
                auto dist1 = BWEB::Map::getGroundDistance(p1, source) + BWEB::Map::getGroundDistance(p1, target);
                auto dist2 = BWEB::Map::getGroundDistance(p2, source) + BWEB::Map::getGroundDistance(p2, target);

                if (i < 10) {
                    dist1 < dist2 ? radrange ={ radrange.first - diff, radrange.first + diff } : radrange ={ radrange.second - diff, radrange.second + diff };
                }
                else {
                    position = (dist1 < dist2 ? p1 : p2);
                    dist = (dist1 < dist2 ? dist1 : dist2);
                }
            }
        }
        return { dist, position };
    }
}