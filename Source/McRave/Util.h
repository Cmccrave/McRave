#pragma once
#include <BWAPI.h>

namespace McRave::Util {

    template<typename F>
    std::shared_ptr<UnitInfo> getClosestUnit(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = DBL_MAX;
        auto &units = Units::getUnits(player);
        std::shared_ptr<UnitInfo> best = nullptr;

        for (auto &u : units) {
            auto &unit = *u;

            if (!unit.unit() || !pred(unit))
                continue;

            auto dist = here.getDistance(unit.getPosition());
            if (dist < distBest) {
                best = u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    std::shared_ptr<UnitInfo> getFurthestUnit(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = 0.0;
        auto &units = Units::getUnits(player);
        std::shared_ptr<UnitInfo> best = nullptr;

        for (auto &u : units) {
            auto &unit = *u;

            if (!unit.unit() || !pred(unit))
                continue;

            auto dist = here.getDistance(unit.getPosition());
            if (dist > distBest) {
                best = u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    std::shared_ptr<UnitInfo> getClosestUnitGround(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = DBL_MAX;
        auto &units = Units::getUnits(player);
        std::shared_ptr<UnitInfo> best = nullptr;

        for (auto &u : units) {
            auto &unit = *u;

            if (!unit.unit() || !pred(unit))
                continue;

            auto dist = BWEB::Map::getGroundDistance(here, unit.getPosition());
            if (dist < distBest) {
                best = u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename T, int idx = 0>
    int iterateCommands(T const &tpl, UnitInfo& unit) {
        if constexpr (idx < std::tuple_size<T>::value)
            if (!std::get<idx>(tpl)(unit))
                return iterateCommands<T, idx + 1>(tpl, unit);
        return idx;
    }

    template<typename F>
    BWAPI::Position findPointOnPath(BWEB::Path& path, F &&pred) {
        BWAPI::TilePosition last = BWAPI::TilePositions::Invalid;

        // For each TilePosition on the path
        for (auto &pos : path.getTiles()) {

            // If last wasn't valid, this is likely the first TilePosition
            if (!last.isValid()) {
                last = pos;
                continue;
            }

            // As long as last doesn't equal pos
            while (last != pos) {
                if (pred(Position(last) + Position(16, 16)))
                    return Position(last) + Position(16, 16);

                // Increment or decrement based on where we need to go
                last.x != pos.x ? (last.x > pos.x ? last.x-- : last.x++) : 0;
                last.y != pos.y ? (last.y > pos.y ? last.y-- : last.y++) : 0;
            }
            last = pos;
        }
        return Positions::Invalid;
    }

    const BWEM::ChokePoint * getClosestChokepoint(BWAPI::Position);

    double getCastRadius(BWAPI::TechType);
    double getCastLimit(BWAPI::TechType);

    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, BWAPI::Position);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, int, int);
    bool isTightWalkable(UnitInfo&, BWAPI::Position);

    BWAPI::Position getInterceptPosition(UnitInfo&);
    BWAPI::Position clipLine(BWAPI::Position, BWAPI::Position);
    BWAPI::Position clipPosition(BWAPI::Position);
    BWAPI::Position vectorProjection(std::pair<BWAPI::Position, BWAPI::Position>, BWAPI::Position);

    Time getTime();
    void onStart();
    void onFrame();

    /// Approximation of Euclidian distance
    /// This is the same approximation that StarCraft's engine uses
    /// and thus should be more accurate than true Euclidian distance
    inline unsigned int disthelper(unsigned int dx, unsigned int dy) {
        // Helper takes and returns pixels
        if (dx < dy) {
            std::swap(dx, dy);
        }
        if (dx / 4u < dy) {
            dx = dx - dx / 16u + dy * 3u / 8u - dx / 64u + dy * 3u / 256u;
        }
        return dx;
    }

    /// Pixel distance
    inline unsigned int pxdistance(int px1, int py1, int px2, int py2) {
        unsigned int dx = std::abs(px1 - px2);
        unsigned int dy = std::abs(py1 - py2);
        return disthelper(dx, dy);
    }

    /// Distance between two bounding boxes, in pixels.
/// Brood War uses bounding boxes for both collisions and range checks
    inline int pxDistanceBB(
        int xminA,
        int yminA,
        int xmaxA,
        int ymaxA,
        int xminB,
        int yminB,
        int xmaxB,
        int ymaxB) {
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

    inline int boxDistance(BWAPI::UnitType typeA, BWAPI::Position posA, BWAPI::UnitType typeB, BWAPI::Position posB) {
        return pxDistanceBB(
            posA.x - typeA.dimensionLeft(),
            posA.y - typeA.dimensionUp(),
            posA.x + typeA.dimensionRight(),
            posA.y + typeA.dimensionDown(),
            posB.x - typeB.dimensionLeft(),
            posB.y - typeB.dimensionUp(),
            posB.x + typeB.dimensionRight(),
            posB.y + typeB.dimensionDown());
    }
}
