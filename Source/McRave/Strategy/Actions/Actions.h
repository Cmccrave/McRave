#pragma once
#include "Main/Common.h"

namespace McRave {
    struct Action {
        BWAPI::UnitType type = BWAPI::UnitTypes::None;
        BWAPI::TechType tech = BWAPI::TechTypes::None;
        BWAPI::Position pos  = BWAPI::Positions::None;
        BWAPI::Unit source   = nullptr;
        BWAPI::Unit target   = nullptr;
        int frame            = 0;
        int width            = 0;

        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::TechType t, int w) { source = u, pos = p, tech = t, frame = BWAPI::Broodwar->getFrameCount(), width = w; }
        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::UnitType t, int w) { source = u, pos = p, type = t, frame = BWAPI::Broodwar->getFrameCount(), width = w; }

        Action(BWAPI::Unit su, BWAPI::Unit tu, BWAPI::TechType t) { source = su, target = tu, tech = t, frame = BWAPI::Broodwar->getFrameCount(); }

        friend bool operator<(const Action &left, const Action &right) { return left.frame < right.frame; }

        Action(){};
    };
} // namespace McRave

namespace McRave::Actions {

    inline std::vector<Action> myActions;
    inline std::vector<Action> enemyActions;
    inline std::vector<Action> allyActions;
    inline std::vector<Action> neutralActions;

    // Adds an Action at a Position
    template <class T> void addAction(BWAPI::Unit unit, BWAPI::Position here, T type, PlayerState player = PlayerState::Neutral, int width = 0)
    {
        if (player == PlayerState::Enemy)
            enemyActions.push_back(Action(unit, here, type, width));
        if (player == PlayerState::Self)
            myActions.push_back(Action(unit, here, type, width));
        if (player == PlayerState::Ally)
            allyActions.push_back(Action(unit, here, type, width));
        if (player == PlayerState::Neutral)
            neutralActions.push_back(Action(unit, here, type, width));
    }

    // Adds an Action at a Unit
    template <class T> void addAction(BWAPI::Unit source, BWAPI::Unit target, T type, PlayerState player = PlayerState::Neutral)
    {
        if (player == PlayerState::Enemy)
            enemyActions.push_back(Action(source, target, type));
        if (player == PlayerState::Self)
            myActions.push_back(Action(source, target, type));
        if (player == PlayerState::Ally)
            allyActions.push_back(Action(source, target, type));
        if (player == PlayerState::Neutral)
            neutralActions.push_back(Action(source, target, type));
    }

    std::vector<Action> *getActions(PlayerState);

    // Position overlap of a UnitType
    bool overlapsActions(BWAPI::Unit, BWAPI::Position, BWAPI::UnitType, PlayerState, int distance = 0);

    // Position overlap of a TechType
    bool overlapsActions(BWAPI::Unit, BWAPI::Position, BWAPI::TechType, PlayerState, int distance = 0);

    // Unit overlap of a TechType
    bool overlapsActions(BWAPI::Unit, BWAPI::Unit, BWAPI::TechType);

    bool overlapsDetection(BWAPI::Unit, BWAPI::Position, PlayerState);
    bool isInDanger(UnitInfo &, BWAPI::Position here = BWAPI::Positions::Invalid);

    void onFrame();
} // namespace McRave::Actions