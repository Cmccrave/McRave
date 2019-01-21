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

    BWAPI::Position findViablePosition(UnitInfo&, std::function<double(BWAPI::WalkPosition)>);
    void onFrame();
    bool overlapsActions(BWAPI::Unit, BWAPI::TechType, BWAPI::Position, int);
    bool overlapsActions(BWAPI::Unit, BWAPI::UnitType, BWAPI::Position, int);
    bool overlapsAllyDetection(BWAPI::Position);
    bool overlapsEnemyDetection(BWAPI::Position);
    bool isInDanger(UnitInfo&, BWAPI::Position here = BWAPI::Positions::Invalid);

    bool misc(UnitInfo&);
    bool move(UnitInfo&);
    bool approach(UnitInfo&);
    bool surround(UnitInfo&);
    bool defend(UnitInfo&);
    bool kite(UnitInfo&);
    bool attack(UnitInfo&);
    bool hunt(UnitInfo&);
    bool escort(UnitInfo&);
    bool special(UnitInfo&);
    bool retreat(UnitInfo&);

    namespace {
        inline std::vector <Action> myActions;
        inline std::vector <Action> enemyActions;
    }

    // Adds a UnitType or TechType command at a Position
    template<class T>
    void addAction(BWAPI::Unit unit, BWAPI::Position here, T type, bool enemy = false) {
        auto &commands = enemy ? enemyActions : myActions;
        commands.push_back(Action(unit, here, type));
    }    
}