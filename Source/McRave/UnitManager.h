#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "UnitInfo.h"
#include <set>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitInfo;
	class UnitManager
	{
		map <Unit, UnitInfo> enemyUnits;
		map <Unit, UnitInfo> myUnits;
		map <Unit, UnitInfo> allyUnits;
		map <Unit, UnitInfo> neutrals;

		map <UnitSizeType, int> allySizes;
		map <UnitSizeType, int> enemySizes;
		map <UnitType, int> enemyComposition;

		map <UnitType, int> myTypes;
		map <Role, int> myRoles;

		set<Unit> enemyThreats;
		set<Unit> armedMines;
		set<Unit> splashTargets;

		double immThreat, proxThreat;
		bool ignoreSim;

		double globalAllyGroundStrength, globalEnemyGroundStrength;
		double globalAllyAirStrength, globalEnemyAirStrength;
		double allyDefense;
		double minThreshold, maxThreshold;
		int supply;
		int repWorkers;
		int scoutDeadFrame = 0;
		Position armyCenter;

		void updateUnitSizes();
		void updateUnits();
		void updateLocalSimulation(UnitInfo&);
		void updateLocalState(UnitInfo&);
		void updateGlobalState(UnitInfo&);
		void updateRole(UnitInfo&);
	public:
		int getRepairWorkers() { return repWorkers; }
		Position getArmyCenter() { return armyCenter; }

		set<Unit>& getSplashTargets() { return splashTargets; }

		map<Unit, UnitInfo>& getMyUnits() { return myUnits; }
		map<Unit, UnitInfo>& getEnemyUnits() { return enemyUnits; }
		map<Unit, UnitInfo>& getNeutralUnits() { return neutrals; }

		map<UnitSizeType, int>& getAllySizes() { return allySizes; }
		map<UnitSizeType, int>& getEnemySizes() { return enemySizes; }
		map<UnitType, int>& getEnemyComposition() { return enemyComposition; }

		double getImmThreat() { return immThreat; }
		double getProxThreat() { return proxThreat; }

		double getGlobalAllyGroundStrength() { return globalAllyGroundStrength; }
		double getGlobalEnemyGroundStrength() { return globalEnemyGroundStrength; }
		double getGlobalAllyAirStrength() { return globalAllyAirStrength; }
		double getGlobalEnemyAirStrength() { return globalEnemyAirStrength; }
		double getAllyDefense() { return allyDefense; }

		int getSupply() { return supply; }
		int getMyRoleCount(Role role) { return myRoles[role]; }
		int getMyTypeCount(UnitType type) { return myTypes[type]; }

		bool isThreatening(UnitInfo&);
		int getEnemyCount(UnitType);

		// Updating
		void onFrame();


		// Storage
		void onUnitDiscover(Unit);
		void onUnitCreate(Unit);
		void onUnitDestroy(Unit);
		void onUnitMorph(Unit);
		void onUnitRenegade(Unit);
		void onUnitComplete(Unit);

		void storeUnit(Unit);
		void removeUnit(Unit);
	};
}

typedef Singleton<McRave::UnitManager> UnitSingleton;