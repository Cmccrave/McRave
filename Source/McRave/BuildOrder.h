#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include <sstream>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class Item
	{
		int actualCount, reserveCount;
	public:
		Item() {};

		Item(int actual, int reserve = -1) {
			actualCount = actual, reserveCount = (reserve == -1 ? actual : reserve);
		}

		int const getReserveCount() { return reserveCount; }
		int const getActualCount() { return actualCount; }
	};

	struct Build {
		vector <string> transitions;
		vector <string> openers;
	};

	class BuildOrderManager
	{
		// Testing new build stuff
		map <string, Build> myBuilds;
		stringstream ss;
		string currentBuild = "";
		string currentOpener = "";
		string currentTransition = "";
		void getDefaultBuild();
		bool isBuildAllowed(Race, string);
		bool isTransitionAllowed(Race, string, string);
		bool isOpenerAllowed(Race, string, string);
		bool isBuildPossible(string);

		map <UnitType, Item> itemQueue;
		bool getOpening = true, getTech, bioBuild, wallNat, wallMain, scout, productionSat, techSat;
		bool fastExpand, proxy, hideTech, playPassive, rush;
		UpgradeType firstUpgrade;
		TechType firstTech;
		UnitType techUnit;
		UnitType productionUnit;
		set <UnitType> techList, unlockedType;
		vector <string> buildNames;


		int satVal, prodVal, techVal, baseVal;
		int gasLimit = INT_MAX;
		int zealotLimit = INT_MAX;
		int dragoonLimit = INT_MAX;
		int lingLimit = INT_MAX;
		int droneLimit = INT_MAX;


		void updateBuild();

		void getNewTech();
		void checkNewTech();
		void checkAllTech();
		void checkExoticTech();

	public:
		bool shouldAddProduction(), shouldAddGas(), techComplete();
		bool shouldExpand();
		map<UnitType, Item>& getItemQueue() { return itemQueue; }

		UnitType getTechUnit() { return techUnit; }
		string getCurrentBuild() { return currentBuild; }
		string getCurrentOpener() { return currentOpener; }
		string getCurrentTransition() { return currentTransition; }

		UpgradeType getFirstUpgrade() { return firstUpgrade; }
		TechType getFirstTech() { return firstTech; }
		set <UnitType>& getTechList() { return techList; }
		set <UnitType>& getUnlockedList() { return unlockedType; }

		int buildCount(UnitType);
		int gasWorkerLimit() { return gasLimit; }

		bool isUnitUnlocked(UnitType unit) { return unlockedType.find(unit) != unlockedType.end(); }
		bool isTechUnit(UnitType unit) { return techList.find(unit) != techList.end(); }


		bool isOpener() { return getOpening; }
		bool isBioBuild() { return bioBuild; }
		bool isFastExpand() { return fastExpand; }
		bool shouldScout() { return scout; }
		bool isWallNat() { return wallNat; }
		bool isWallMain() { return wallMain; }
		bool isProxy() { return proxy; }
		bool isHideTech() { return hideTech; }
		bool isPlayPassive() { return playPassive; }
		bool isRush() { return rush; }

		bool firstReady();

		void onEnd(bool), onStart(), onFrame();
		void protossOpener(), protossTech(), protossSituational(), protossUnlocks(), protossIslandPlay();
		void terranOpener(), terranTech(), terranSituational(), terranUnlocks(), terranIslandPlay();
		void zergOpener(), zergTech(), zergSituational(), zergUnlocks(), zergIslandPlay();

		void PScoutMemes(), PDWEBMemes(), PArbiterMemes(), PShuttleMemes();	// Gimmick builds	


		void PProxy99();								// 2Gate Proxy - "http://liquipedia.net/starcraft/2_Gateway_(vs._Zerg)"		
		void PProxy6();
		void P2GateExpand();							// 2Gate Expand			
		void P2GateDragoon();							// 2 Gate Dragoon - "http://liquipedia.net/starcraft/10/15_Gates_(vs._Terran)"		
		void P3Nexus();									// Triple Nexus
		void PZealotDrop();

		void P1GateCore();
		//void P2Gate();
		void PFFE();
		void P21Nexus();
		void P12Nexus();



		void Reaction2GateDefensive();
		void Reaction2GateAggresive();
		void Reaction2Gate();

		void T2Fact();
		void TSparks();
		void T2PortWraith();
		void T1RaxFE();
		void TNukeMemes();
		void TBCMemes();
		void T2RaxFE();
		void T1FactFE();

		void Z2HatchMuta();
		void Z3HatchLing();
		void Z4Pool();
		void Z9Pool();
		void Z2HatchHydra();
		void Z3HatchBeforePool();
		void ZLurkerTurtle();
		void Z9PoolSpire();
	};

}

typedef Singleton<McRave::BuildOrderManager> BuildOrderSingleton;