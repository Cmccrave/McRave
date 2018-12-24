#pragma once
#include "McRave.h"
#include <BWAPI.h>

struct CommandType
{
	UnitType type = UnitTypes::None;
	TechType tech = TechTypes::None;
	Position pos = Positions::None;
	Unit unit = nullptr;
	int frame = 0;

	CommandType(Unit u, Position p, TechType t) { unit = u, pos = p, tech = t, frame = Broodwar->getFrameCount(); }
	CommandType(Unit u, Position p, UnitType t) { unit = u, pos = p, type = t, frame = Broodwar->getFrameCount(); }

	friend bool operator< (const CommandType &left, const CommandType &right) {
		return left.frame < right.frame;
	}
};


namespace McRave::Command {
	Position findViablePosition(UnitInfo&, function<double(WalkPosition)>);
	void onFrame();
	bool overlapsCommands(Unit, TechType, Position, int);
	bool overlapsCommands(Unit, UnitType, Position, int);
	bool overlapsAllyDetection(Position);
	bool overlapsEnemyDetection(Position);
	bool isInDanger(UnitInfo&, Position here = Positions::Invalid);

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
		inline vector <CommandType> myCommands;
		inline vector <CommandType> enemyCommands;
		inline constexpr tuple commands{ misc, special, attack, approach, kite, defend, hunt, escort, retreat, move };
	}

	// Adds a UnitType or TechType command at a Position
	template<class T>
	void addCommand(Unit unit, Position here, T type, bool enemy = false) {
		auto &commands = enemy ? enemyCommands : myCommands;
		commands.push_back(CommandType(unit, here, type));
	}

	// Iterates all commands possible
	template<typename T, int idx = 0>
	int iterateCommands(T const &tpl, UnitInfo& unit) {
		if constexpr (idx < std::tuple_size<T>::value)
			if (!std::get<idx>(tpl)(unit))
				return iterateCommands<T, idx + 1>(tpl, unit);
		return idx;
	}
}