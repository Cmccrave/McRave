#pragma once
#include <BWAPI.h>

namespace McRave::Util {

    template<typename F>
    const std::shared_ptr<UnitInfo> getClosestUnit(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = DBL_MAX;
        auto &units = Units::getUnits(player);
        std::shared_ptr<UnitInfo> best = nullptr;

        for (auto &u : units) {
            auto &unit = *u;

            if (!unit.unit() || !pred(unit))
                continue;

            double dist = here.getDistance(unit.getPosition());
            if (dist < distBest) {
                best = u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    const std::shared_ptr<UnitInfo> getFurthestUnit(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = 0.0;
        auto &units = Units::getUnits(player);
        std::shared_ptr<UnitInfo> best = nullptr;

        for (auto &u : units) {
            auto &unit = *u;

            if (!unit.unit() || !pred(unit))
                continue;

            double dist = here.getDistance(unit.getPosition());
            if (dist > distBest) {
                best = u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    const std::shared_ptr<UnitInfo> getClosestUnitGround(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = DBL_MAX;
        auto &units = Units::getUnits(player);
        std::shared_ptr<UnitInfo> best = nullptr;

        for (auto &u : units) {
            auto &unit = *u;

            if (!unit.unit() || !pred(unit))
                continue;

            double dist = BWEB::Map::getGroundDistance(here, unit.getPosition());
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
        auto start = BWAPI::WalkPosition(here);
        for (int x = start.x; x < start.x + 4; x++) {
            for (int y = start.y; y < start.y + 4; y++) {
                if (Grids::getMobility(BWAPI::WalkPosition(x, y)) == -1)
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

    int chokeWidth(const BWEM::ChokePoint *);
    const BWEM::ChokePoint * getClosestChokepoint(BWAPI::Position);

    double getCastLimit(BWAPI::TechType);
    double getCastRadius(BWAPI::TechType);

    bool unitInRange(UnitInfo&);
    bool reactivePullWorker(UnitInfo&);
    bool proactivePullWorker(UnitInfo&);
    bool pullRepairWorker(UnitInfo&);
    bool hasThreatOnPath(UnitInfo&, BWEB::Path&);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, BWAPI::Position);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, int, int);
    bool isWalkable(UnitInfo&, BWAPI::WalkPosition);

    Line lineOfBestFit(const BWEM::ChokePoint *);
    Line parallelLine(Line, int, double);

    BWAPI::Position getConcavePosition(UnitInfo&, double, BWEM::Area const * area = nullptr, BWAPI::Position here = BWAPI::Positions::Invalid);
    BWAPI::Position getInterceptPosition(UnitInfo&);
    BWAPI::Position clipLine(BWAPI::Position, BWAPI::Position);
    BWAPI::Position clipPosition(BWAPI::Position);
}
