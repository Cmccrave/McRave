#pragma once
#include <BWAPI.h>

namespace McRave::Util {

    const BWEM::ChokePoint * getClosestChokepoint(BWAPI::Position);

    int getCastRadius(BWAPI::TechType);
    double getCastLimit(BWAPI::TechType);

    int boxDistance(BWAPI::UnitType, BWAPI::Position, BWAPI::UnitType, BWAPI::Position);

    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, BWAPI::Position);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, int, int);
    bool findWalkable(BWAPI::Position, BWAPI::UnitType, BWAPI::Position&, bool visual = false);
    bool findWalkable(UnitInfo&, BWAPI::Position&, bool visual = false);

    BWAPI::Position shiftTowards(BWAPI::Position here, BWAPI::Position target, double dist);
    BWAPI::Position projectLine(std::pair<BWAPI::Position, BWAPI::Position> line, BWAPI::Position here);
    BWAPI::Position clipLine(BWAPI::Position, BWAPI::Position);
    BWAPI::Position clipPosition(BWAPI::Position);

    BWAPI::Position getPathPoint(UnitInfo&, BWAPI::Position);

    std::vector<BWAPI::WalkPosition>& getWalkCircle(int);

    Time getTime();

    void onStart();
    void onFrame();

    double log10(int);

    /// Log a thing
    static void debug(std::string& stuff)
    {
        auto frameString = "[" + std::to_string(BWAPI::Broodwar->getFrameCount()) + "]";
        std::ofstream writeFile;
        writeFile.open("bwapi-data/write/McRave_Debug_Log.txt", std::ios::app);
        writeFile << Util::getTime().toString() << frameString << stuff << std::endl;
    }

    template<typename F>
    UnitInfo* getClosestUnit(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = DBL_MAX;
        auto &units = Units::getUnits(player);
        UnitInfo* best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = here.getDistance(u->getPosition());
            if (dist < distBest) {
                best = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    UnitInfo* getFurthestUnit(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = 0.0;
        auto &units = Units::getUnits(player);
        UnitInfo* best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = here.getDistance(u->getPosition());
            if (dist > distBest) {
                best = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    UnitInfo* getClosestUnitGround(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = DBL_MAX;
        auto &units = Units::getUnits(player);
        UnitInfo* best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = BWEB::Map::getGroundDistance(here, u->getPosition());
            if (dist < distBest) {
                best = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    UnitInfo* getFurthestUnitGround(BWAPI::Position here, PlayerState player, F &&pred) {
        auto distBest = 0.0;
        auto &units = Units::getUnits(player);
        UnitInfo* best = nullptr;

        for (auto &u : units) {
            if (!pred(u))
                continue;

            auto dist = BWEB::Map::getGroundDistance(here, u->getPosition());
            if (dist > distBest) {
                best = &*u;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    void testPointOnPath(BWEB::Path& path, F &&pred) {
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

    template<typename F>
    void testAllPointOnPath(BWEB::Path& path, F &&pred) {
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

    template<typename F>
    std::vector<BWAPI::Position> findAllPointOnPath(BWEB::Path& path, F &&pred) {
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
}
