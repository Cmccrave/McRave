#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

struct CommandType
{
	UnitType unit = UnitTypes::None;
	TechType tech = TechTypes::None;
	Position pos = Positions::None;
	int frame = 0;

	CommandType(Position p, TechType t) { pos = p, tech = t, frame = Broodwar->getFrameCount(); }
	CommandType(Position p, UnitType u) { pos = p, unit = u, frame = Broodwar->getFrameCount(); }

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
		double allyRange, enemyRange, widths;
		vector <CommandType> myCommands;
		vector <CommandType> enemyCommands;
		map<Position, double> myGoals;

		void updateArbiter(UnitInfo&), updateDarchon(UnitInfo&), updateDefiler(UnitInfo&), updateDetector(UnitInfo&), updateQueen(UnitInfo&);
		void updateAlliedUnits(), updateEnemyCommands();
		void updateGoals();

		void move(UnitInfo&), approach(UnitInfo&), defend(UnitInfo&), kite(UnitInfo&), attack(UnitInfo&), retreat(UnitInfo&);
		bool shouldAttack(UnitInfo&), shouldKite(UnitInfo&), shouldApproach(UnitInfo&), shouldUseSpecial(UnitInfo&), shouldDefend(UnitInfo&);
		bool isLastCommand(UnitInfo&, UnitCommandType, Position);
	public:
		void onFrame();
		vector <CommandType>& getMyCommands() { return myCommands; }
		vector <CommandType>& getEnemyCommands() { return enemyCommands; }
		bool overlapsCommands(TechType, Position, int);
		bool overlapsCommands(UnitType, Position, int);
		bool overlapsAllyDetection(Position);
		bool overlapsEnemyDetection(Position);

		bool isInDanger(UnitInfo&);
		bool isInDanger(Position);

		/// Adds a command
		template<class T>
		void addCommand(Position here, T type, bool enemy = false) {
			if (enemy)
				enemyCommands.push_back(CommandType(here, type));
			else
				myCommands.push_back(CommandType(here, type));
		}
	};
}

typedef Singleton<McRave::CommandManager> CommandSingleton;