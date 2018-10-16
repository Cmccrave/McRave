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

		bool needGas();		
	public:		

		void onFrame();
		void updateWorkers();
		void updateDecision(UnitInfo&);		
		
		bool shouldAssign(UnitInfo&);
		bool shouldBuild(UnitInfo&);
		bool shouldClearPath(UnitInfo&);
		bool shouldFight(UnitInfo&);
		bool shouldGather(UnitInfo&);
		bool shouldReturnCargo(UnitInfo&);

		void assign(UnitInfo&);
		void build(UnitInfo&);
		void clearPath(UnitInfo&);
		void gather(UnitInfo&);
		void returnCargo(UnitInfo&);

	};
}

typedef Singleton<McRave::WorkerManager> WorkerSingleton;
