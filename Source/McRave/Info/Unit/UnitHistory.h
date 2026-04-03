#pragma once
#include "Main/Common.h"

namespace McRave {
    class UnitHistory {
    protected:
        std::map<int, Position> positionHistory;
        std::map<int, UnitCommandType> commandHistory;
        std::map<int, std::pair<Order, Position>> orderHistory;

        UnitType lastType     = UnitTypes::None;
        Role lastRole         = Role::None;
        Position lastPos      = Positions::Invalid;
        WalkPosition lastWalk = WalkPositions::Invalid;
        TilePosition lastTile = TilePositions::Invalid;

    public:
        UnitType getLastType() { return lastType; }
        Role getLastRole() { return lastRole; }
        TilePosition getLastTile() { return lastTile; }
        WalkPosition getLastWalk() { return lastWalk; }
        Position getLastPosition() { return lastPos; }
    };
} // namespace McRave
