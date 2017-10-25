#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include <sstream>

using namespace BWAPI;
using namespace std;

class BuildOrderTrackerClass
{
	map <UnitType, int> buildingDesired;
	bool getOpening = true, getTech = false, learnedOpener = false;
	bool oneGateCore = false, forgeExpand = false;
	bool bioBuild = false;
	UnitType techUnit;
	set <UnitType> techList;	
	vector <string> buildNames;
	string currentBuild = "Test";
	stringstream ss;
public:
	// Build learning functions
	string getCurrentBuild() { return currentBuild; }
	void getDefaultBuild();
	bool isBuildAllowed(Race, string);

	map <UnitType, int>& getBuildingDesired() { return buildingDesired; }
	bool isOpener() { return getOpening; }	

	set <UnitType>& getTechList() { return techList; }
	bool isOneGateCore() { return oneGateCore; }
	bool isForgeExpand() { return forgeExpand; }
	bool isBioBuild() { return bioBuild; }

	void onEnd(bool);
	void onStart();
	void update();
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

	void ZZCore(), ZCore(), NZCore();
	void FFECannon(), FFEGateway(), FFENexus();
	void TwelveNexus();
	void RoboExpand(), DTExpand();
	void FourGate(), ZealotRush(), TenTwelveGate(); //ReaverRush();

	void TwoFactVult();
	//void ShallowTwo();
	void Sparks();
};

typedef Singleton<BuildOrderTrackerClass> BuildOrderTracker;