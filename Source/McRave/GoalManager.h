#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitInfo;
	class GoalManager
	{
		void updateGoals();
		void assignClosestToGoal(Position, vector<UnitType>);
	public:
		void onFrame();
	};
}

typedef Singleton<McRave::GoalManager> GoalSingleton;
