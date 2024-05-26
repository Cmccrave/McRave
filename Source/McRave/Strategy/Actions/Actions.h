#pragma once
#include <BWAPI.h>

namespace McRave {
    struct Action
    {
        BWAPI::UnitType type =  BWAPI::UnitTypes::None;
        BWAPI::TechType tech =  BWAPI::TechTypes::None;
        BWAPI::Position pos =  BWAPI::Positions::None;
        BWAPI::Unit unit = nullptr;
        int frame = 0;
        int width = 0;
        std::weak_ptr<UnitInfo> source;
        std::weak_ptr<UnitInfo> target;

        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::TechType t, int w) {
            unit = u, pos = p, tech = t, frame = BWAPI::Broodwar->getFrameCount(), width = w;
        }
        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::UnitType t, int w) {
            unit = u, pos = p, type = t, frame = BWAPI::Broodwar->getFrameCount(), width = w;
        }

        friend bool operator< (const Action &left, const Action &right) {
            return left.frame < right.frame;
        }

        Action() {};
    };
}

namespace McRave::Actions {

    inline std::vector <Action> myActions;
    inline std::vector <Action> enemyActions;
    inline std::vector <Action> allyActions;
    inline std::vector <Action> neutralActions;

    // Adds an Action at a Position
    template<class T>
    void addAction(BWAPI::Unit unit, BWAPI::Position here, T type, PlayerState player, int width = 0) {
        if (player == PlayerState::Enemy)
            enemyActions.push_back(Action(unit, here, type, width));
        if (player == PlayerState::Self)
            myActions.push_back(Action(unit, here, type, width));
        if (player == PlayerState::Ally)
            allyActions.push_back(Action(unit, here, type, width));
        if (player == PlayerState::Neutral)
            neutralActions.push_back(Action(unit, here, type, width));
    }

    std::vector<Action>* getActions(PlayerState);

    bool overlapsActions(BWAPI::Unit, BWAPI::Position, BWAPI::UnitType, PlayerState, int distance = 0);
    bool overlapsActions(BWAPI::Unit, BWAPI::Position, BWAPI::TechType, PlayerState, int distance = 0);
    bool overlapsDetection(BWAPI::Unit, BWAPI::Position, PlayerState);
    bool isInDanger(UnitInfo&, BWAPI::Position here = BWAPI::Positions::Invalid);

    void onFrame();
}