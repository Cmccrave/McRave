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

    template<typename T, typename E>
    constexpr auto contains(T const &cont, E &&e) {
        return std::find(cont.begin(), cont.end(), std::forward<E>(e)) != cont.end();
    }

    template<typename T>
    bool isWalkable(T here) {
        const auto start = BWAPI::WalkPosition(here);
        for (int x = start.x; x < start.x + 4; x++) {
            for (int y = start.y; y < start.y + 4; y++) {
                if (Grids::getMobility(BWAPI::WalkPosition(x, y)) < 1)
                    return false;
                if (BWEB::Map::isUsed(TilePosition(WalkPosition(x, y))) != UnitTypes::None)
                    return false;
            }
        }
        return true;
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
        auto &last = path.getSource();

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

    int chokeWidth(const BWEM::ChokePoint *);
    const BWEM::ChokePoint * getClosestChokepoint(BWAPI::Position);

    int getCastRadius(BWAPI::TechType);
    double getCastLimit(BWAPI::TechType);

    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, BWAPI::Position);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, int, int);
    bool isTightWalkable(UnitInfo&, BWAPI::Position);

    BWAPI::Position getConcavePosition(UnitInfo&, BWEM::Area const *, BWEM::ChokePoint const *);
    BWAPI::Position getInterceptPosition(UnitInfo&);
    BWAPI::Position clipLine(BWAPI::Position, BWAPI::Position);
    BWAPI::Position clipPosition(BWAPI::Position);
    BWAPI::Position vectorProjection(std::pair<BWAPI::Position, BWAPI::Position>, BWAPI::Position);

    Time getTime();
    void onStart();
    void onFrame();
}
