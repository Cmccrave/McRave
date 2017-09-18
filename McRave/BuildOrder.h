#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class BuildOrderTrackerClass
{
	map <UnitType, int> buildingDesired;
	int opening;
	bool getOpening = true, getTech = false, learnedOpener = false;	
	UnitType techUnit;
	set <UnitType> techList;
	//const char json[];
	vector <int> configStuff;
public:
	map <UnitType, int>& getBuildingDesired() { return buildingDesired; }
	bool isOpener() { return getOpening; }
	int getOpener() { return opening; }

	void recordWinningBuild(bool);
	void loadConfig();

	void update();
	void updateDecision();
	void updateBuild();

	void protossOpener();
	void protossTech();
	void protossSituational();

	void terranOpener();
	void terranTech();
	void terranSituational();

	void zergOpener();
	void zergTech();
	void zergSituational();
};

typedef Singleton<BuildOrderTrackerClass> BuildOrderTracker;