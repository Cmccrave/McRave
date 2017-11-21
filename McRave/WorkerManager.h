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
	Unit scout;
	bool scouting = true;
	int deadScoutFrame = 0;
	int minWorkers, gasWorkers;
public:

	bool isScouting() { return scouting; }
	map <Unit, WorkerInfo>& getMyWorkers() { return myWorkers; }
	Unit getScout() { return scout; }
	Unit getClosestWorker(Position);
	
	void update();
	void updateWorkers();
	
	void updateBuilding(WorkerInfo&);
	void updateScouting(WorkerInfo&);
	void updateGathering(WorkerInfo&);

	void updateScout(WorkerInfo&);
	void updateInformation(WorkerInfo&);
	void updateDecision(WorkerInfo&);
	void assignWorker(WorkerInfo&);
	void reAssignWorker(WorkerInfo&);
	void exploreArea(WorkerInfo&);

	void shouldMoveToBuild(WorkerInfo&);

	void storeWorker(Unit);
	void removeWorker(Unit);
};

typedef Singleton<WorkerTrackerClass> WorkerTracker;
