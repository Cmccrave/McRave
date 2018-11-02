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
		int minWorkers = 0, gasWorkers = 0;
		bool closeToResource(UnitInfo&);
		typedef bool (WorkerManager::*Command)(UnitInfo&);
		vector<Command> commands { &WorkerManager::misc, &WorkerManager::transport, &WorkerManager::returnCargo, &WorkerManager::clearPath, &WorkerManager::build, &WorkerManager::gather };
	public:		

		void onFrame();
		void updateWorkers();
		void updateDecision(UnitInfo&);	
		void updateAssignment(UnitInfo&);

		bool misc(UnitInfo&);
		bool transport(UnitInfo&);		
		bool returnCargo(UnitInfo&);
		bool build(UnitInfo&);
		bool clearPath(UnitInfo&);
		bool gather(UnitInfo&);

		void removeUnit(Unit);
	};
}

typedef Singleton<McRave::WorkerManager> WorkerSingleton;
