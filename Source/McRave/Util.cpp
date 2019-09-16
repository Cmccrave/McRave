#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;;

namespace McRave::Util {

    namespace {
        multimap<WalkPosition, double> badPosition;
        map<TilePosition, TilePosition> closestGeoCache;
        Time gameTime(0, 0);
    }

    bool isWalkable(UnitInfo& unit, WalkPosition here)
    {
        if (unit.getType().isFlyer() && unit.getRole() != Role::Transport)
            return true;

        auto walkWidth = (int)ceil(unit.getType().width() / 8.0);
        auto walkHeight = (int)ceil(unit.getType().height() / 8.0);

        auto halfW = walkWidth / 2;
        auto halfH = walkHeight / 2;
        auto wOffset = unit.getType().width() % 8 != 0 ? 1 : 0;
        auto hOffset = unit.getType().height() % 8 != 0 ? 1 : 0;

        auto left = max(0, here.x - halfW);
        auto right = min(1024, here.x + halfW + wOffset);
        auto top = max(0, here.y - halfH);
        auto bottom = min(1024, here.y + halfH + hOffset);

        // Pixel rectangle
        auto ff = unit.getType().isBuilding() ? Position(0, 0) : Position(8, 8);
        auto topLeft = Position(unit.getWalkPosition());
        auto botRight = topLeft + Position(walkWidth * 8, walkHeight * 8);

        for (int x = left; x < right; x++) {
            for (int y = top; y < bottom; y++) {
                const WalkPosition w(x, y);
                const auto p = Position(w) + Position(4, 4);

                if (!unit.getType().isFlyer()) {
                    if (rectangleIntersect(topLeft, botRight, p))
                        continue;
                    else if (Grids::getMobility(w) < 1 || Grids::getCollision(w) > 0)
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
            return 2.5;
        return 0.0;
    }

    int getCastRadius(TechType tech)
    {
        if (tech == TechTypes::Psionic_Storm || tech == TechTypes::Stasis_Field || tech == TechTypes::Maelstrom || tech == TechTypes::Plague || tech == TechTypes::Ensnare)
            return 48;
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

    Position getClosestChokeGeo(const BWEM::ChokePoint * choke, Position here)
    {
        auto distBest = DBL_MAX;
        auto walkBest = WalkPositions::Invalid;
        for (auto &w : choke->Geometry()) {
            const auto p = Position(w) + Position(4, 4);
            const auto dist = p.getDistance(here);

            if (dist < distBest) {
                walkBest = w;
                distBest = dist;
            }
        }
        return Position(walkBest) + Position(4, 4);
    }

    Position getConcavePosition(UnitInfo& unit, BWEM::Area const * area, Position here)
    {
        const auto enemyRangeExists = Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Dragoon) > 0
            || Players::getTotalCount(PlayerState::Enemy, UnitTypes::Zerg_Hydralisk) > 0
            || Players::vT();

        if (Broodwar->getFrameCount() % 50 == 0) {
            //closestGeoCache.clear();
            badPosition.clear();
        }

        auto meleeCount = com(Protoss_Zealot) + com(Zerg_Zergling) + com(Terran_Firebat);
        auto rangedCount = com(Protoss_Dragoon) + com(Protoss_Reaver) + com(Protoss_High_Templar) + com(Zerg_Hydralisk) + com(Terran_Marine) + com(Terran_Medic) + com(Terran_Siege_Tank_Siege_Mode) + com(Terran_Siege_Tank_Tank_Mode) + com(Terran_Vulture);

        auto base = area->Bases().empty() ? nullptr : &area->Bases().front();

        auto center = WalkPositions::None;
        auto scoreBest = DBL_MAX;
        auto posBest = BWEB::Map::getMainPosition();
        auto choke = Util::getClosestChokepoint(here);
        auto spacing = min(unit.getType().width(), unit.getType().height()) + 4;

        auto meleeRadius = Terrain::isDefendNatural() && Terrain::getNaturalWall() && Players::getSupply(PlayerState::Self) >= 40 && Players::vZ() ? 32.0 : min(32.0, meleeCount * 16.0);
        auto rangedRadius = max(meleeRadius + 32.0, Util::chokeWidth(choke) + (rangedCount * 8.0));
        auto radius = enemyRangeExists && Players::getSupply(PlayerState::Self) >= 80 ? max(meleeRadius, rangedRadius) : unit.getGroundRange() > 32.0 ? rangedRadius : meleeRadius;
        auto initialRadius = radius;

        radius = !enemyRangeExists && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > vis(Protoss_Zealot) * 2 ? 64.0 : radius;

        // Offset the concave based on where we are creating concave and how far it is from the nearest choke
        auto hereOffset = here.getDistance(BWEB::Map::getClosestChokeTile(choke, here));
        radius += hereOffset;

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
                    choke = c;
                }
            }
        }

        // Grab the closest choke geo as the center
        if (choke) {
            if (closestGeoCache[unit.getTilePosition()] != TilePositions::None) {
                center = WalkPosition(getClosestChokeGeo(choke, unit.getPosition()));
                closestGeoCache[unit.getTilePosition()] = TilePosition(center);
            }
            center = WalkPosition(closestGeoCache[unit.getTilePosition()]) + WalkPosition(2, 2);
            radius = min(unit.getPosition().getDistance(Position(center)) + 64.0, radius);
        }

        // If we are trying to expand, try to get out of the way
        auto resourceDepot = Broodwar->self()->getRace().getResourceDepot();
        if (BuildOrder::buildCount(resourceDepot) > vis(resourceDepot) && vis(resourceDepot) == 1) {
            radius = min(BWEB::Map::getNaturalPosition().getDistance(Position(BWEB::Map::getNaturalChoke()->Center())),
                Position(BWEB::Map::getMainChoke()->Center()).getDistance(Position(BWEB::Map::getNaturalChoke()->Center())));
            radius = clamp(radius, 0.0, radius - 128.0);
        }

        const auto isSuitable = [&](WalkPosition w) {
            const auto t = TilePosition(w);
            const auto pTest = Position(w) + Position(4, 4);

            if (!pTest.isValid()
                || pTest.getDistance(Position(center)) < radius /*- 64.0*/
                || pTest.getDistance(Position(center)) > radius + 64.0
                || (area && mapBWEM.GetArea(w) != area)
                || Buildings::overlapsQueue(unit, TilePosition(w))
                || !Util::isWalkable(unit, w))
                return false;

            auto cachedResult = badPosition.equal_range(w);

            // Check if too close or too far to a cache result
            for (auto itr = cachedResult.first; itr != cachedResult.second; itr++) {
                if (itr->second == radius)
                    return false;
            }

            // Make sure we are the closest unit to it
            auto closest = Util::getClosestUnit(pTest, PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Combat && u.getPosition().getDistance(pTest) < 48.0;
            });
            if (closest && closest != unit.shared_from_this())
                return false;
            return true;
        };

        const auto scorePosition = [&](Position pTest) {
            const auto distCenter = pTest.getDistance(Position(center));
            const auto distUnit = log(pTest.getDistance(unit.getPosition()) + 32.0);
            const auto distAreaBase = base ? base->Center().getDistance(pTest) : 1.0;
            return distCenter * distAreaBase * distUnit;
        };

        /*if (isSuitable(WalkPosition(unit.unit()->getLastCommand().getTargetPosition())))
            return unit.unit()->getLastCommand().getTargetPosition();
        if (isSuitable(WalkPosition(unit.getPosition())))
            return unit.getPosition();*/

            // Find a position around the center that is suitable        
        for (int x = center.x - 40; x <= center.x + 40; x++) {
            for (int y = center.y - 40; y <= center.y + 40; y++) {
                WalkPosition w(x, y);
                const auto p = Position(w) + Position(4, 4);
                const auto score = scorePosition(p);

                if (score < scoreBest && isSuitable(w)) {
                    posBest = p;
                    scoreBest = score;
                }
            }
        }
        return posBest;
    }

    Position getInterceptPosition(UnitInfo& unit)
    {
        // If we can't see the units speed, return its current position
        if (!unit.getTarget().unit()->exists() || unit.getSpeed() == 0.0 || unit.getTarget().getSpeed() == 0.0)
            return unit.getTarget().getPosition();

        auto timeToEngage = max(0.0, 2 * (unit.getEngDist() / unit.getSpeed()));
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

    Time getTime()
    {
        return gameTime;
    }

    void onFrame()
    {
        if (Broodwar->getFrameCount() % 24 == 0) {
            gameTime.seconds++;
            if (gameTime.seconds >= 60) {
                gameTime.seconds = 0;
                gameTime.minutes++;
            }
        }
    }
}