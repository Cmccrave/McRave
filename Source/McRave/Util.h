#pragma once
#include <BWAPI.h>

namespace McRave::Util {

    UnitInfo * getClosestUnit(BWAPI::Position, BWAPI::Player, BWAPI::UnitType t = BWAPI::UnitTypes::None);
    UnitInfo * getClosestUnit(UnitInfo&, BWAPI::Player, BWAPI::UnitType t = BWAPI::UnitTypes::None);
    UnitInfo * getClosestThreat(UnitInfo&);
    UnitInfo * getClosestBuilder(BWAPI::Position);

    int chokeWidth(const BWEM::ChokePoint *);
    const BWEM::ChokePoint * getClosestChokepoint(BWAPI::Position);

    double getHighestThreat(BWAPI::WalkPosition, UnitInfo&);

    bool unitInRange(UnitInfo& unit);
    bool reactivePullWorker(UnitInfo& unit);
    bool proactivePullWorker(UnitInfo& unit);
    bool pullRepairWorker(UnitInfo& unit);
    bool accurateThreatOnPath(UnitInfo&, BWEB::PathFinding::Path&);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, BWAPI::Position);
    bool rectangleIntersect(BWAPI::Position, BWAPI::Position, int, int);

    // Walkability checks
    template<class T>
    bool isWalkable(T here)
    {
        auto start = BWAPI::WalkPosition(here);
        for (int x = start.x; x < start.x + 4; x++) {
            for (int y = start.y; y < start.y + 4; y++) {
                if (Grids::getMobility(BWAPI::WalkPosition(x, y)) == -1)
                    return false;
            }
        }
        return true;
    }

    // Iterates all commands possible
    template<typename T, int idx = 0>
    int iterateCommands(T const &tpl, UnitInfo& unit) {
        if constexpr (idx < std::tuple_size<T>::value)
            if (!std::get<idx>(tpl)(unit))
                return iterateCommands<T, idx + 1>(tpl, unit);
        return idx;
    }

    bool isWalkable(BWAPI::WalkPosition start, BWAPI::WalkPosition finish, BWAPI::UnitType);

    // Create a line of best fit for a chokepoint
    Line lineOfBestFit(const BWEM::ChokePoint *);
    Line parallelLine(Line, int, double);

    BWAPI::Position getConcavePosition(UnitInfo&, BWEM::Area const * area = nullptr, BWAPI::Position here = BWAPI::Positions::Invalid);

    BWAPI::Position clipPosition(BWAPI::Position, BWAPI::Position);
    BWAPI::Position clipToMap(BWAPI::Position);
}
