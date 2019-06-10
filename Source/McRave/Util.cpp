#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Util {

    bool isWalkable(UnitInfo& unit, WalkPosition here)
    {
        if (unit.getType().isFlyer())
            return true;

        // Pixel rectangle
        auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0);
        auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0);
        auto topLeft = Position(unit.getWalkPosition());
        auto botRight = topLeft + Position(walkWidth * 8, walkHeight * 8);

        // Round up
        int halfW = (walkWidth + 1) / 2;
        int halfH = (walkHeight + 1) / 2;

        auto collision = Grids::getCollision(here);
        if (collision > 0)
            return false;

        for (int x = here.x - halfW; x < here.x + halfW; x++) {
            for (int y = here.y - halfH; y < here.y + halfH; y++) {
                WalkPosition w(x, y);
                Position p = Position(w) + Position(4, 4);
                if (!w.isValid())
                    return false;

                if (!unit.getType().isFlyer()) {
                    if (rectangleIntersect(topLeft, botRight, p))
                        continue;                    
                    else if (!Broodwar->isWalkable(w) || Grids::getCollision(w) > 0)
                        return false;
                }
            }
        }
        return true;
    }

    double getCastLimit(TechType tech)
    {
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Maelstrom || tech == TechTypes::Plague || tech == TechTypes::Ensnare)
            return 1.5;
        if (tech == TechTypes::Stasis_Field)
            return 3.0;
        return 0.0;
    }

    int getCastRadius(TechType tech)
    {
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Stasis_Field || tech == TechTypes::Maelstrom || tech == TechTypes::Plague || tech == TechTypes::Ensnare)
            return 96;
        return 0;
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

    int chokeWidth(const BWEM::ChokePoint * choke)
    {
        if (!choke)
            return 0;
        return int(choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2))) * 8;
    }

    Position getConcavePosition(UnitInfo& unit, double radius, BWEM::Area const * area, Position here)
    {
        auto center = WalkPositions::None;
        auto distBest = DBL_MAX;
        auto posBest = Positions::None;

        // Finds which position we are forming the concave at
        if (here.isValid())
            center = (WalkPosition)here;
        else if (area == BWEB::Map::getNaturalArea() && BWEB::Map::getNaturalChoke())
            center = BWEB::Map::getNaturalChoke()->Center();
        else if (area == BWEB::Map::getMainArea() && BWEB::Map::getMainChoke())
            center = BWEB::Map::getMainChoke()->Center();
        else if (area) {
            auto distEnemyBest = DBL_MAX;
            for (auto &c : area->ChokePoints()) {
                double dist = BWEB::Map::getGroundDistance(Position(c->Center()), Terrain::getEnemyStartingPosition());
                if (dist < distEnemyBest) {
                    distEnemyBest = dist;
                    center = c->Center();
                }
            }
        }

        const auto checkbest = [&](WalkPosition w) {
            auto t = TilePosition(w);
            auto p = Position(w) + Position(4, 4);
            auto dist = p.getDistance(Position(center));
            auto score = dist + log(p.getDistance(unit.getPosition()));
            
            auto correctArea = (here == Terrain::getDefendPosition() && Terrain::isInAllyTerritory(t)) || (t.isValid() && mapBWEM.GetArea(t) == area);

            if (!w.isValid() || !t.isValid() || !p.isValid()
                || (!Terrain::isInAllyTerritory(t) && area && mapBWEM.GetArea(w) != area)
                || dist < radius
                || score > distBest
                || !correctArea
                || Command::overlapsActions(unit.unit(), p, unit.getType(), PlayerState::Self, 24)
                || Command::isInDanger(unit, p)
                || Grids::getMobility(p) <= 6
                || Buildings::overlapsQueue(unit, TilePosition(w)))
                return;

            posBest = p;
            distBest = score;
        };

        // Find a position around the center that is suitable
        for (int x = center.x - 40; x <= center.x + 40; x++) {
            for (int y = center.y - 40; y <= center.y + 40; y++) {
                WalkPosition w(x, y);
                checkbest(w);
            }
        }
        return posBest;
    }

    Position getInterceptPosition(UnitInfo& unit)
    {
        // If we can't see the units speed, return its current position
        if (!unit.unit()->exists())
            return unit.getPosition();

        auto timeToEngage = max(0.0, (unit.getEngDist() / unit.getSpeed()));
        auto targetDestination = unit.getTarget().getPosition() + Position(int(unit.getTarget().unit()->getVelocityX() * timeToEngage), int(unit.getTarget().unit()->getVelocityY() * timeToEngage));
        targetDestination = Util::clipPosition(targetDestination);
        return targetDestination;
    }

    Position clipLine(Position source, Position target)
    {
        if (target.isValid())
            return target;

        auto sqDist = source.getApproxDistance(target);
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
}