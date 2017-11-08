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
	bool oneGateCore = false, forgeExpand = false, nexusFirst = false;
	bool bioBuild = false;
	UnitType techUnit;
	set <UnitType> techList;
	set <UnitType> unlockedType;
	vector <string> buildNames;
	string currentBuild = "Test";
	stringstream ss;
public:
	// Build learning functions
	string getCurrentBuild() { return currentBuild; }
	void getDefaultBuild();
	bool isBuildAllowed(Race, string);
	bool isUnitUnlocked(UnitType unit) { return (unlockedType.find(unit) != unlockedType.end()); }

	map <UnitType, int>& getBuildingDesired() { return buildingDesired; }
	bool isOpener() { return getOpening; }

	set <UnitType>& getTechList() { return techList; }
	bool isOneGateCore() { return oneGateCore; }
	bool isForgeExpand() { return forgeExpand; }
	bool isBioBuild() { return bioBuild; }
	bool isNexusFirst() { return nexusFirst; }

	void onEnd(bool);
	void onStart();
	void update();
	void updateBuild();

	// Toss Builds
	void protossOpener();
	void protossTech();
	void protossSituational();

	void terranOpener();
	void terranTech();
	void terranSituational();

	void zergOpener();
	void zergTech();
	void zergSituational();

	void PZZCore(), PZCore(), PNZCore();			// 1 Gate Core
	void PFFESafe(), PFFEStandard(), PFFEGreedy();	// FFE
	void P12Nexus(), P21Nexus(), PDTExpand();		// Early Nexus
	void P4Gate(), P2GateZealot(), P2GateDragoon(); // 1 Base Aggresion
	void PDTRush(), PReaverRush();					// 1 Base Tech

	// Terran Builds
	void T2Fact();
	//void TShallowTwo();
	void TSparks();

	// Zerg Builds
	void ZOverpool();
};

typedef Singleton<BuildOrderTrackerClass> BuildOrderTracker;