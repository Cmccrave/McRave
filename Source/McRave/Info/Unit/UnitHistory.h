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
        Position lastGoal      = Positions::Invalid;
        Position lastFormation = Positions::Invalid;
        WalkPosition lastWalk  = WalkPositions::Invalid;
        TilePosition lastTile  = TilePositions::Invalid;

        GlobalState lastGState = GlobalState::None;
        LocalState lastLState  = LocalState::None;

    public:
        std::map<int, Position> &getPositionHistory() { return positionHistory; }
        std::map<int, UnitCommandType> &getCommandHistory() { return commandHistory; }
        std::map<int, std::pair<Order, Position>> &getOrderHistory() { return orderHistory; }

        UnitType getLastType() { return lastType; }
        Role getLastRole() { return lastRole; }
        Position getLastPosition() { return lastPos; }
        Position getLastGoal() { return lastGoal; }
        Position getLastFormation() { return lastFormation; }
        WalkPosition getLastWalk() { return lastWalk; }
        TilePosition getLastTile() { return lastTile; }

        GlobalState getLastGlobalState() { return lastGState; }
        LocalState getLastLocalState() { return lastLState; }
    };
} // namespace McRave
