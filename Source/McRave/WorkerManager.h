#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "WorkerInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class WorkerManager
	{
		map <Unit, WorkerUnit> myWorkers;
		set<Position> scoutAssignments;
		set<Unit> scouts;

		int deadScoutFrame = 0;
		int minWorkers = 0, gasWorkers = 0;
		int scoutCount;		

		bool needGas();
		bool proxyCheck = false;
	public:
		map <Unit, WorkerUnit>& getMyWorkers() { return myWorkers; }
		Unit getClosestWorker(Position, bool);
		WorkerUnit* getClosestScout(Position);

		void onFrame();
		void updateDecision(WorkerUnit&);
		void updateScouts();
		
		bool shouldAssign(WorkerUnit&);
		bool shouldBuild(WorkerUnit&);
		bool shouldClearPath(WorkerUnit&);
		bool shouldFight(WorkerUnit&);
		bool shouldGather(WorkerUnit&);
		bool shouldReturnCargo(WorkerUnit&);
		bool shouldScout(WorkerUnit&);

		void assign(WorkerUnit&);
		void build(WorkerUnit&);
		void clearPath(WorkerUnit&);
		void fight(WorkerUnit&);
		void gather(WorkerUnit&);
		void returnCargo(WorkerUnit&);
		void scout(WorkerUnit&);
		void safeMove(WorkerUnit&);
		void explore(WorkerUnit&);

		void storeWorker(Unit);
		void removeWorker(Unit);
	};
}

typedef Singleton<McRave::WorkerManager> WorkerSingleton;
