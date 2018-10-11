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
		double allyRange, enemyRange, widths;
		vector <CommandType> myCommands;
		vector <CommandType> enemyCommands;
		map<Position, double> myGoals;

		void updateArbiter(UnitInfo&), updateDetector(UnitInfo&);
		void updateUnits(), updateDecision(UnitInfo&), updateEnemyCommands();
		void updateGoals();
		void assignClosestToGoal(Position, vector<UnitType>);

		void move(UnitInfo&), approach(UnitInfo&), defend(UnitInfo&), kite(UnitInfo&), attack(UnitInfo&), retreat(UnitInfo&);
		bool shouldAttack(UnitInfo&), shouldKite(UnitInfo&), shouldApproach(UnitInfo&), shouldUseSpecial(UnitInfo&), shouldDefend(UnitInfo&);
		bool isLastCommand(UnitInfo&, UnitCommandType, Position);
	public:
		void onFrame();
		vector <CommandType>& getMyCommands() { return myCommands; }
		vector <CommandType>& getEnemyCommands() { return enemyCommands; }
		bool overlapsCommands(Unit, TechType, Position, int);
		bool overlapsCommands(Unit, UnitType, Position, int);
		bool overlapsAllyDetection(Position);
		bool overlapsEnemyDetection(Position);

		bool isInDanger(UnitInfo&);
		bool isInDanger(Position);

		/// Adds a command
		template<class T>
		void addCommand(Unit unit, Position here, T type, bool enemy = false) {
			if (enemy)
				enemyCommands.push_back(CommandType(unit, here, type));
			else
				myCommands.push_back(CommandType(unit, here, type));
		}
	};
}

typedef Singleton<McRave::CommandManager> CommandSingleton;