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

        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::TechType t) { unit = u, pos = p, tech = t, frame = BWAPI::Broodwar->getFrameCount(); }
        Action(BWAPI::Unit u, BWAPI::Position p, BWAPI::UnitType t) { unit = u, pos = p, type = t, frame = BWAPI::Broodwar->getFrameCount(); }

        friend bool operator< (const Action &left, const Action &right) {
            return left.frame < right.frame;
        }
    };

    //BWAPI::Position findViablePosition(UnitInfo&, std::function<double(BWAPI::WalkPosition)>);
    void onFrame();
    bool overlapsActions(BWAPI::Unit, BWAPI::TechType, BWAPI::Position, int);
    bool overlapsActions(BWAPI::Unit, BWAPI::UnitType, BWAPI::Position, int);
    bool overlapsAllyDetection(BWAPI::Position);
    bool overlapsEnemyDetection(BWAPI::Position);
    bool isInDanger(UnitInfo&, BWAPI::Position here = BWAPI::Positions::Invalid);

    bool misc(UnitInfo& unit);
    bool move(UnitInfo& unit);
    bool approach(UnitInfo& unit);
    bool surround(UnitInfo& unit);
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

    // Adds a UnitType or TechType command at a Position
    template<class T>
    void addAction(BWAPI::Unit unit, BWAPI::Position here, T type, bool enemy = false) {
        auto &commands = enemy ? enemyActions : myActions;
        auto test = Action(unit, here, type);
        commands.push_back(test);
    }
}