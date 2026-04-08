#pragma once
#include "Main/Common.h"

namespace McRave {
    class UnitHistory {
    protected:
        std::map<int, Position> positionHistory;
        std::map<int, UnitCommandType> commandHistory;
        std::map<int, std::pair<Order, Position>> orderHistory;

        UnitType lastType      = UnitTypes::None;
        Role lastRole          = Role::None;
        Position lastPos       = Positions::Invalid;
        Position lastFormation = Positions::Invalid;
        WalkPosition lastWalk  = WalkPositions::Invalid;
        TilePosition lastTile  = TilePositions::Invalid;

        GlobalState lastGState = GlobalState::None;
        LocalState lastLState  = LocalState::None;

    public:
        UnitType getLastType() { return lastType; }
        Role getLastRole() { return lastRole; }
        Position getLastPosition() { return lastPos; }
        Position getLastFormation() { return lastFormation; }
        WalkPosition getLastWalk() { return lastWalk; }
        TilePosition getLastTile() { return lastTile; }

        GlobalState getLastGlobalState() { return lastGState; }
        LocalState getLastLocalState() { return lastLState; }
    };
} // namespace McRave
