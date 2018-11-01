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
		
		int deadScoutFrame = 0;
		int minWorkers = 0, gasWorkers = 0;

		bool closeToResource(UnitInfo&);
		bool needGas();		

		typedef bool (WorkerManager::*Command)(UnitInfo&);
		vector<Command> commands { &WorkerManager::ride, &WorkerManager::returnCargo, &WorkerManager::clearPath, &WorkerManager::build, &WorkerManager::gather };
	public:		

		void onFrame();
		void updateWorkers();
		void updateDecision(UnitInfo&);	
		void updateAssignment(UnitInfo&);
		
		bool shouldAssign(UnitInfo&);
		bool shouldBuild(UnitInfo&);
		bool shouldClearPath(UnitInfo&);
		bool shouldGather(UnitInfo&);
		bool shouldReturnCargo(UnitInfo&);

		bool ride(UnitInfo&);
		bool returnCargo(UnitInfo&);
		bool build(UnitInfo&);
		bool clearPath(UnitInfo&);
		bool gather(UnitInfo&);

		void removeUnit(Unit);
	};
}

typedef Singleton<McRave::WorkerManager> WorkerSingleton;
