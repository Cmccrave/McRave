#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB::PathFinding
{
    class Path {
        std::vector<BWAPI::TilePosition> tiles;
        double dist;
        bool reachable;
    public:
        Path::Path()
        {
            tiles ={};
            dist = 0.0;
            reachable = false;
        }

        std::vector<BWAPI::TilePosition>& getTiles() { return tiles; }
        double getDistance() { return dist; }
        bool isReachable() { return reachable; }
        void createUnitPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position);
        void createWallPath(BWEM::Map&, std::map<BWAPI::TilePosition, BWAPI::UnitType>&, const BWAPI::Position, const BWAPI::Position, bool);

        void createPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position, std::function <bool(const BWAPI::TilePosition)>, std::vector<BWAPI::TilePosition>);
    };
}
