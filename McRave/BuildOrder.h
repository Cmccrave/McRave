#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include <sstream>

using namespace BWAPI;
using namespace std;

class BuildOrderTrackerClass
{
	map <UnitType, int> buildingDesired;
	int opening;
	bool getOpening = true, getTech = false, learnedOpener = false;	
	UnitType techUnit;
	set <UnitType> techList;	
	vector <string> buildNames;
	string currentBuild = "Test";
	stringstream ss;
public:
	map <UnitType, int>& getBuildingDesired() { return buildingDesired; }
	bool isOpener() { return getOpening; }
	int getOpener() { return opening; }
	string getCurrentBuild() { return currentBuild; }

	void onEnd(bool);
	void onStart();

	void getDefaultBuild();
	bool isBuildAllowed(Race, string);

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

	void ZZCore();
	void ZCore();
	void NZCore();
	void FFECannon();
	void FFEGateway();
	void FFENexus();
	void TwelveNexus();
	void DTExpand();
	void RoboExpand();
	void FourGate();
	void ZealotRush();
};

typedef Singleton<BuildOrderTrackerClass> BuildOrderTracker;