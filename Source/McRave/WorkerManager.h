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
		map <Unit, WorkerInfo> myWorkers;
		set<Position> scoutAssignments;
		set<Unit> scouts;

		int deadScoutFrame = 0;
		int minWorkers = 0, gasWorkers = 0;
		int scoutCount;		

		bool needGas();
		bool proxyCheck = false;
	public:
		map <Unit, WorkerInfo>& getMyWorkers() { return myWorkers; }
		Unit getClosestWorker(Position, bool);
		WorkerInfo* getClosestScout(Position);

		void onFrame();
		void updateDecision(WorkerInfo&);
		void updateScouts();
		
		bool shouldAssign(WorkerInfo&);
		bool shouldBuild(WorkerInfo&);
		bool shouldClearPath(WorkerInfo&);
		bool shouldFight(WorkerInfo&);
		bool shouldGather(WorkerInfo&);
		bool shouldReturnCargo(WorkerInfo&);
		bool shouldScout(WorkerInfo&);

		void assign(WorkerInfo&);
		void build(WorkerInfo&);
		void clearPath(WorkerInfo&);
		void fight(WorkerInfo&);
		void gather(WorkerInfo&);
		void returnCargo(WorkerInfo&);
		void scout(WorkerInfo&);
		void safeMove(WorkerInfo&);
		void explore(WorkerInfo&);

		void storeWorker(Unit);
		void removeWorker(Unit);
	};
}

typedef Singleton<McRave::WorkerManager> WorkerSingleton;
