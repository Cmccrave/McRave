#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

struct CommandType
{
	UnitType type = UnitTypes::None;
	TechType tech = TechTypes::None;
	Position pos = Positions::None;
	Unit unit = nullptr;
	int frame = 0;

	CommandType(Unit u, Position p, TechType t) { unit = u, pos = p, tech = t, frame = Broodwar->getFrameCount(); }
	CommandType(Unit u, Position p, UnitType t) { unit = u, pos = p, type = t, frame = Broodwar->getFrameCount(); }

	friend bool operator< (const CommandType &left, const CommandType &right)
	{
		return left.frame < right.frame;
	}
};

namespace McRave
{
	class UnitInfo;
	class CommandManager
	{
		vector <CommandType> myCommands;
		vector <CommandType> enemyCommands;

		void updateUnits(), updateDecision(UnitInfo&), updateEnemyCommands();
		
		typedef bool (CommandManager::*Command)(UnitInfo&);
		vector<Command> commands{ &CommandManager::misc, &CommandManager::special, &CommandManager::attack, &CommandManager::approach, &CommandManager::kite, &CommandManager::escort, &CommandManager::hunt, &CommandManager::defend, &CommandManager::move };
	public:
		void onFrame();
		vector <CommandType>& getMyCommands() { return myCommands; }
		vector <CommandType>& getEnemyCommands() { return enemyCommands; }
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
		bool safeMove(UnitInfo&);
		bool hunt(UnitInfo&);
		bool escort(UnitInfo&);
		bool special(UnitInfo&);

		// Adds a UnitType or TechType command at a Position
		template<class T>
		void addCommand(Unit unit, Position here, T type, bool enemy = false) {
			auto &commands = enemy ? enemyCommands : myCommands;
			commands.push_back(CommandType(unit, here, type));
		}
	};
}

typedef Singleton<McRave::CommandManager> CommandSingleton;