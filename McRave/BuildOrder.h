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
	bool scout = false;
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
	bool shouldScout() { return scout; }

	set <UnitType>& getTechList() { return techList; }
	bool isOneGateCore() { return oneGateCore; }
	bool isForgeExpand() { return forgeExpand; }
	bool isBioBuild() { return bioBuild; }
	bool isNexusFirst() { return nexusFirst; }

	void onEnd(bool);
	void onStart();
	void update();
	void updateBuild();

	void protossOpener(), protossTech(), protossSituational();
	void terranOpener(), terranTech(), terranSituational();
	void zergOpener(), zergTech(), zergSituational();

	// Protoss Builds																
	void PFFESafe(), PFFEStandard();												// PvZ Builds
	void P12Nexus(), P21Nexus(), PDTExpand(), P2GateDragoon();						// PvT Builds
	void P3GateObs(), PNZCore(), PZCore();											// PvP Builds
	void PZZCore(), P4Gate();														// Misc
	// Retired or not created	void PFFEGreedy(), P2GateDT(), P1GateReaver();									

	// Terran Builds
	void T2Fact();
	//void TShallowTwo();
	void TSparks();

	// Zerg Builds
	void ZOverpool();
};

typedef Singleton<BuildOrderTrackerClass> BuildOrderTracker;