#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "WorkerInfo.h"

using namespace BWAPI;
using namespace std;

class WorkerTrackerClass
{
	map <Unit, WorkerInfo> myWorkers;
	map <WalkPosition, int> recentExplorations;
	Unit scouter;
	bool scouting = true;
	int deadScoutFrame = 0;
	int minWorkers, gasWorkers;
public:
	bool isScouting() { return scouting; }
	map <Unit, WorkerInfo>& getMyWorkers() { return myWorkers; }
	Unit getScout() { return scouter; }
	Unit getClosestWorker(Position);

	void update();
	void updateWorkers();
	void updateInformation(WorkerInfo&);
	void updateDecision(WorkerInfo&);

	//bool shouldMoveToBuild(WorkerInfo&);

	bool shouldAssign(WorkerInfo&);
	bool shouldBuild(WorkerInfo&);
	bool shouldFight(WorkerInfo&);
	bool shouldGather(WorkerInfo&);
	bool shouldRepair(WorkerInfo&);
	bool shouldReturnCargo(WorkerInfo&);
	bool shouldScout(WorkerInfo&);

	void assign(WorkerInfo&);
	void build(WorkerInfo&);
	void fight(WorkerInfo&);
	void gather(WorkerInfo&);
	void repair(WorkerInfo&);
	void returnCargo(WorkerInfo&);
	void scout(WorkerInfo&);

	void explore(WorkerInfo&);

	void storeWorker(Unit);
	void removeWorker(Unit);
};

typedef Singleton<WorkerTrackerClass> WorkerTracker;
