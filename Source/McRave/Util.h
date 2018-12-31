#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Util
{
    UnitInfo * getClosestUnit(Position, Player, UnitType t = UnitTypes::None);
    UnitInfo * getClosestUnit(UnitInfo&, Player, UnitType t = UnitTypes::None);
    UnitInfo * getClosestThreat(UnitInfo&);
    UnitInfo * getClosestBuilder(Position);

    int chokeWidth(const BWEM::ChokePoint *);
    const BWEM::ChokePoint * getClosestChokepoint(Position);

    double getHighestThreat(WalkPosition, UnitInfo&);

    bool unitInRange(UnitInfo& unit);
    bool reactivePullWorker(UnitInfo& unit);
    bool proactivePullWorker(UnitInfo& unit);
    bool pullRepairWorker(UnitInfo& unit);
    bool accurateThreatOnPath(UnitInfo&, BWEB::PathFinding::Path&);
    bool rectangleIntersect(Position, Position, Position);
    bool rectangleIntersect(Position, Position, int, int);

    // Walkability checks
    template<class T>
    bool isWalkable(T here)
    {
        auto start = WalkPosition(here);
        for (int x = start.x; x < start.x + 4; x++) {
            for (int y = start.y; y < start.y + 4; y++) {
                if (Grids::getMobility(WalkPosition(x, y)) == -1)
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

    bool isWalkable(WalkPosition start, WalkPosition finish, UnitType);

    // Create a line of best fit for a chokepoint
    Line lineOfBestFit(const BWEM::ChokePoint *);
    Line parallelLine(Line, int, double);

    Position getConcavePosition(UnitInfo&, BWEM::Area const * area = nullptr, Position here = Positions::Invalid);

    Position clipPosition(Position, Position);
    Position clipToMap(Position);
}
