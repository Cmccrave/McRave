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
        void updateProtossGoals();
        void updateTerranGoals();
        void updateZergGoals();
        void assignNumberToGoal(Position, UnitType, int);
        void assignPercentToGoal(Position, UnitType, double);
    public:
        void onFrame();
    };
}

typedef Singleton<McRave::GoalManager> GoalSingleton;
