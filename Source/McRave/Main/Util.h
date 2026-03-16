#pragma once
#include "Common.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"

namespace McRave::Util {

    const BWEM::ChokePoint *getClosestChokepoint(BWAPI::Position);

    int getCastRadius(BWAPI::TechType);
    double getCastLimit(BWAPI::TechType);

    int boxDistance(BWAPI::UnitType, BWAPI::Position, BWAPI::UnitType, BWAPI::Position);

    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, BWAPI::Position);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, int, int);

    std::pair<BWAPI::Position, BWAPI::Position> typeBoundingBox(BWAPI::Position here, BWAPI::UnitType type);

    bool findTerrainWalkable(BWAPI::Position here, BWAPI::UnitType type);

    bool findWalkable(BWAPI::Position, BWAPI::UnitType, BWAPI::Position &, bool visual = false);
    bool findWalkable(UnitInfo &, BWAPI::Position &, bool visual = false);

    BWAPI::Position shiftTowards(BWAPI::Position here, BWAPI::Position target, double dist);
    BWAPI::Position projectLine(std::pair<BWAPI::Position, BWAPI::Position> line, BWAPI::Position here);
    BWAPI::Position clipLine(BWAPI::Position, BWAPI::Position);
    BWAPI::Position clipPosition(BWAPI::Position);

    BWAPI::Position getPathPoint(UnitInfo &, BWAPI::Position);

    std::vector<BWAPI::WalkPosition> &getWalkCircle(int);
    std::vector<BWAPI::TilePosition> &getTileCircle(int);

    Time getTime();

    void onStart();
    void onFrame();

    double log10(int);

    void writeToLoggerImpl(const std::string &msg);

    template <typename... Args> void writeToLogger(const char *file, int line, Args &&... args)
    {
        std::ostringstream ss;

        auto bracketWrap = [](const auto &x) -> std::string {
            std::ostringstream temp;
            temp << "[" << x << "]";
            return temp.str();
        };

        ss << Util::getTime().toString() << bracketWrap(BWAPI::Broodwar->getFrameCount());
        ss << bracketWrap(std::string(file) + ":" + std::to_string(line)) << " ";
        (ss << ... << args);

        writeToLoggerImpl(ss.str());
    }

    inline float fastReciprocal(float x)
    {
        union {
            float f;
            uint32_t i;
        } u;
        u.f     = x;
        u.i     = 0x7EF311C3 - u.i;
        float r = u.f;
        r       = r * (2.0f - x * r);
        return r;
    }

    // Similar to BWAPI getApproxDistance
    inline int fastDistance(int x1, int y1, int x2, int y2)
    {
        unsigned int min = abs((int)(x1 - x2));
        unsigned int max = abs((int)(y1 - y2));
        if (max < min)
            std::swap(min, max);

        if (min < (max >> 2))
            return max;

        unsigned int minCalc = (3 * min) >> 3;
        return (minCalc >> 5) + minCalc + max - (max >> 4) - (max >> 6);
    }

    template <typename F> UnitInfo *getClosestUnit(BWAPI::Position here, PlayerState player, F &&pred)
    {
        auto distBest  = DBL_MAX;
        auto &units    = Units::getUnits(player);
        UnitInfo *best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = here.getDistance(u->getPosition());
            if (dist < distBest) {
                best     = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template <typename F> UnitInfo *getFurthestUnit(BWAPI::Position here, PlayerState player, F &&pred)
    {
        auto distBest  = 0.0;
        auto &units    = Units::getUnits(player);
        UnitInfo *best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = here.getDistance(u->getPosition());
            if (dist > distBest) {
                best     = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template <typename F> UnitInfo *getClosestUnitGround(BWAPI::Position here, PlayerState player, F &&pred)
    {
        auto distBest  = DBL_MAX;
        auto &units    = Units::getUnits(player);
        UnitInfo *best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = BWEB::Map::getGroundDistance(here, u->getPosition());
            if (dist < distBest) {
                best     = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template <typename F> UnitInfo *getFurthestUnitGround(BWAPI::Position here, PlayerState player, F &&pred)
    {
        auto distBest  = 0.0;
        auto &units    = Units::getUnits(player);
        UnitInfo *best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = BWEB::Map::getGroundDistance(here, u->getPosition());
            if (dist > distBest) {
                best     = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template <typename F> void testPointOnPath(BWEB::Path &path, F &&pred)
    {
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
                    return;

                // Increment or decrement based on where we need to go
                last.x != pos.x ? (last.x > pos.x ? last.x-- : last.x++) : 0;
                last.y != pos.y ? (last.y > pos.y ? last.y-- : last.y++) : 0;
            }
            last = pos;
        }
    }

    template <typename F> void testAllPointOnPath(BWEB::Path &path, F &&pred)
    {
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
                pred(Position(last) + Position(16, 16));

                // Increment or decrement based on where we need to go
                last.x != pos.x ? (last.x > pos.x ? last.x-- : last.x++) : 0;
                last.y != pos.y ? (last.y > pos.y ? last.y-- : last.y++) : 0;
            }
            last = pos;
        }
    }

    template <typename F> BWAPI::Position findPointOnPath(BWEB::Path &path, F &&pred)
    {
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

    template <typename F> std::vector<BWAPI::Position> findAllPointOnPath(BWEB::Path &path, F &&pred)
    {
        BWAPI::TilePosition last = BWAPI::TilePositions::Invalid;
        std::vector<BWAPI::Position> returnVector;

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
                    returnVector.push_back(Position(last) + Position(16, 16));

                // Increment or decrement based on where we need to go
                last.x != pos.x ? (last.x > pos.x ? last.x-- : last.x++) : 0;
                last.y != pos.y ? (last.y > pos.y ? last.y-- : last.y++) : 0;
            }
            last = pos;
        }
        return returnVector;
    }

    std::pair<double, BWAPI::Position> findPointOnCircle(BWAPI::Position source, BWAPI::Position target, double radius, std::function<double(BWAPI::Position)> calc);
} // namespace McRave::Util

constexpr const char *baseName(const char *path)
{
    const char *file = path;
    while (*path) {
        if (*path == '/' || *path == '\\')
            file = path + 1;
        ++path;
    }
    return file;
}

#define LOG(...)                                                                                                                                                                                       \
    do {                                                                                                                                                                                               \
        Util::writeToLogger(baseName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                                                \
    } while (0)

#define LOG_ONCE(...)                                                                                                                                                                                  \
    do {                                                                                                                                                                                               \
        static bool _logged = false;                                                                                                                                                                   \
        if (!_logged) {                                                                                                                                                                                \
            Util::writeToLogger(baseName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                                            \
            _logged = true;                                                                                                                                                                            \
        }                                                                                                                                                                                              \
    } while (0)

#define LOG_SLOW(...)                                                                                                                                                                                  \
    do {                                                                                                                                                                                               \
        static Time lastLogTime = Util::getTime() - Time(0, 05);                                                                                                                                       \
        if (Util::getTime() - lastLogTime >= Time(0, 05)) {                                                                                                                                            \
            Util::writeToLogger(baseName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                                            \
            lastLogTime = Util::getTime();                                                                                                                                                             \
        }                                                                                                                                                                                              \
    } while (0)