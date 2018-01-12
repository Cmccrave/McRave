#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include <sstream>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class BuildOrderTrackerClass
	{
		map <UnitType, int> buildingDesired;
		bool getOpening = true, getTech = false, learnedOpener = false;
		bool fastExpand = false;
		bool bioBuild = false;
		bool scout = false;
		UpgradeType firstUpgrade;
		TechType firstTech;
		UnitType techUnit;
		set <UnitType> techList;
		set <UnitType> unlockedType;
		vector <string> buildNames;
		string currentBuild = "Test";
		stringstream ss;
		int satVal, gateVal, techVal, baseVal;
		bool productionSat, techSat;
	public:
		// Build learning functions
		string getCurrentBuild() { return currentBuild; }
		void getDefaultBuild();
		bool isBuildAllowed(Race, string);
		bool isUnitUnlocked(UnitType unit) { return (unlockedType.find(unit) != unlockedType.end()); }

		map <UnitType, int>& getBuildingDesired() { return buildingDesired; }
		bool isOpener() { return getOpening; }

		UpgradeType getFirstUpgrade() { return firstUpgrade; }
		TechType getFirstTech() { return firstTech; }

		set <UnitType>& getTechList() { return techList; }
		bool isBioBuild() { return bioBuild; }
		bool isFastExpand() { return fastExpand; }

		bool shouldExpand();
		bool shouldTech();
		bool shouldAddProduction();
		bool shouldAddGas();
		bool shouldScout() { return scout; }

		bool techComplete();

		void onEnd(bool);
		void onStart();
		void update();
		void updateBuild();

		void protossOpener(), protossTech(), protossSituational();
		void terranOpener(), terranTech(), terranSituational();
		void zergOpener(), zergTech(), zergSituational();

		void PvPSituational(), PvTSituational(), PvZSituational();

		// Protoss Builds																
		void PFFEStandard();															// PvZ Builds
		void P12Nexus(), P21Nexus(), PDTExpand(), P2GateDragoon();						// PvT Builds
		void P3GateObs(), PNZCore(), PZCore();											// PvP Builds
		void PZZCore(), P4Gate();														// Misc
		// Retired or not created	void PFFEGreedy(), PFFESafe(), P2GateDT(), P1GateReaver();									

		// Terran Builds
		void T2Fact();
		//void TShallowTwo();
		void TSparks();

		// Zerg Builds
		void ZOverpool();
	};
}

typedef Singleton<McRave::BuildOrderTrackerClass> BuildOrderTracker;