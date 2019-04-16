#pragma once
#include <BWAPI.h>

namespace McRave::Command {

    struct Action
    {
        BWAPI::UnitType type =  BWAPI::UnitTypes::None;
        BWAPI::TechType tech =  BWAPI::TechTypes::None;
        BWAPI::Position pos =  BWAPI::Positions::None;
        BWAPI::Unit unit = nullptr;
        int frame = 0;
        std::weak_ptr<UnitInfo> source;
        std::weak_ptr<UnitInfo> target;

        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::TechType t) { unit = u, pos = p, tech = t, frame = BWAPI::Broodwar->getFrameCount(); }
        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::UnitType t) { unit = u, pos = p, type = t, frame = BWAPI::Broodwar->getFrameCount(); }

        friend bool operator< (const Action &left, const Action &right) {
            return left.frame < right.frame;
        }
    };

    void onFrame();
    bool overlapsDetection(BWAPI::Unit, BWAPI::Position, PlayerState);
    bool isInDanger(UnitInfo&, BWAPI::Position here = BWAPI::Positions::Invalid);

    bool misc(UnitInfo& unit);
    bool move(UnitInfo& unit);
    bool approach(UnitInfo& unit);
    bool defend(UnitInfo& unit);
    bool kite(UnitInfo& unit);
    bool attack(UnitInfo& unit);
    bool hunt(UnitInfo& unit);
    bool escort(UnitInfo& unit);
    bool special(UnitInfo& unit);
    bool retreat(UnitInfo& unit);
    bool transport(UnitInfo& unit);

    inline std::vector <Action> myActions;
    inline std::vector <Action> enemyActions;
    inline std::vector <Action> allyActions;
    inline std::vector <Action> neutralActions;

    // Adds an Action at a Position
    template<class T>
    void addAction(BWAPI::Unit unit, BWAPI::Position here, T type, PlayerState player) {
        if (player == PlayerState::Enemy)
            enemyActions.push_back(Action(unit, here, type));
        if (player == PlayerState::Self)
            myActions.push_back(Action(unit, here, type));
        if (player == PlayerState::Ally)
            allyActions.push_back(Action(unit, here, type));
        if (player == PlayerState::Neutral)
            neutralActions.push_back(Action(unit, here, type));
    }

    // Check if a new Action would overlap an existing Action
    template<class T>
    bool overlapsActions(BWAPI::Unit unit, BWAPI::Position here, T type, PlayerState player, int distance = 0) {
        std::vector<Action>* actions;

        if (player == PlayerState::Enemy)
            actions = &enemyActions;
        if (player == PlayerState::Self)
            actions = &myActions;
        if (player == PlayerState::Ally)
            actions = &allyActions;
        if (player == PlayerState::Neutral)
            actions = &neutralActions;

        if (!actions)
            return false;

        if (std::is_same<BWAPI::UnitType, T>::value) {

            // Check overlap in a circle with UnitType Actions
            const auto circleOverlap = [&](Action& action) {
                if (action.unit != unit && action.type == type && action.pos.getDistance(here) <= distance * 2)
                    return true;
                return false;
            };

            // Check outgoing UnitType Actions for this PlayerState
            for (auto &action : *actions) {
                if (circleOverlap(action))
                    return true;
            }
        }
        else {

            // Bounding box of new Action
            auto checkTopLeft = here + BWAPI::Position(-distance / 2, -distance / 2);
            auto checkTopRight = here + BWAPI::Position(distance / 2, -distance / 2);
            auto checkBotLeft = here + BWAPI::Position(-distance / 2, distance / 2);
            auto checkBotRight = here + BWAPI::Position(distance / 2, distance / 2);

            // Check overlap in a box with TechType actions
            const auto boxOverlap = [&](Action& action) {

                // Bounding box of current Action
                auto topLeft = action.pos - BWAPI::Position(distance / 2, distance / 2);
                auto botRight = action.pos + BWAPI::Position(distance / 2, distance / 2);

                if (action.unit != unit && action.tech == type &&
                    (Util::rectangleIntersect(topLeft, botRight, checkTopLeft)
                        || Util::rectangleIntersect(topLeft, botRight, checkTopRight)
                        || Util::rectangleIntersect(topLeft, botRight, checkBotLeft)
                        || Util::rectangleIntersect(topLeft, botRight, checkBotRight)))
                    return true;
                return false;
            };

            // Check outgoing TechType Actions for this PlayerState
            for (auto &action : *actions) {
                if (boxOverlap(action))
                    return true;
            }

            // Check outgoing TechType Actions for neutral PlayerState
            for (auto &action : neutralActions) {
                if (boxOverlap(action))
                    return true;
            }
        }
        return false;
    }
}