#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "UnitInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerManager
	{
		set<Position> scoutAssignments;
		set<Unit> scouts;

		int deadScoutFrame = 0;
		int minWorkers = 0, gasWorkers = 0;
		int scoutCount;		

		bool needGas();
		bool proxyCheck = false;
	public:
		UnitInfo* getClosestScout(Position);

		void onFrame();
		void updateWorkers();
		void updateDecision(UnitInfo&);
		void updateScouts();
		
		bool shouldAssign(UnitInfo&);
		bool shouldBuild(UnitInfo&);
		bool shouldClearPath(UnitInfo&);
		bool shouldFight(UnitInfo&);
		bool shouldGather(UnitInfo&);
		bool shouldReturnCargo(UnitInfo&);
		bool shouldScout(UnitInfo&);

		void assign(UnitInfo&);
		void build(UnitInfo&);
		void clearPath(UnitInfo&);
		void gather(UnitInfo&);
		void returnCargo(UnitInfo&);
		void scout(UnitInfo&);
		void safeMove(UnitInfo&);
		void explore(UnitInfo&);
	};
}

typedef Singleton<McRave::WorkerManager> WorkerSingleton;
