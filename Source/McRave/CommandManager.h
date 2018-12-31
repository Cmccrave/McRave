#pragma once
#include <BWAPI.h>

namespace McRave::Command {
    struct CommandType
    {
        BWAPI::UnitType type =  BWAPI::UnitTypes::None;
        BWAPI::TechType tech =  BWAPI::TechTypes::None;
        BWAPI::Position pos =  BWAPI::Positions::None;
        BWAPI::Unit unit = nullptr;
        int frame = 0;

        CommandType(BWAPI::Unit u, BWAPI::Position p, BWAPI::TechType t) { unit = u, pos = p, tech = t, frame = BWAPI::Broodwar->getFrameCount(); }
        CommandType(BWAPI::Unit u, BWAPI::Position p, BWAPI::UnitType t) { unit = u, pos = p, type = t, frame = BWAPI::Broodwar->getFrameCount(); }

        friend bool operator< (const CommandType &left, const CommandType &right) {
            return left.frame < right.frame;
        }
    };

    BWAPI::Position findViablePosition(UnitInfo&, function<double(BWAPI::WalkPosition)>);
    void onFrame();
    bool overlapsCommands(BWAPI::Unit, BWAPI::TechType, BWAPI::Position, int);
    bool overlapsCommands(BWAPI::Unit, BWAPI::UnitType, BWAPI::Position, int);
    bool overlapsAllyDetection(Position);
    bool overlapsEnemyDetection(Position);
    bool isInDanger(UnitInfo&, BWAPI::Position here = Positions::Invalid);

    bool misc(UnitInfo&);
    bool move(UnitInfo&);
    bool approach(UnitInfo&);
    bool defend(UnitInfo&);
    bool kite(UnitInfo&);
    bool attack(UnitInfo&);
    bool hunt(UnitInfo&);
    bool escort(UnitInfo&);
    bool special(UnitInfo&);
    bool retreat(UnitInfo&);

    namespace {
        inline std::vector <CommandType> myCommands;
        inline std::vector <CommandType> enemyCommands;
    }

    // Adds a UnitType or TechType command at a Position
    template<class T>
    void addCommand(BWAPI::Unit unit, BWAPI::Position here, T type, bool enemy = false) {
        auto &commands = enemy ? enemyCommands : myCommands;
        commands.push_back(CommandType(unit, here, type));
    }    
}