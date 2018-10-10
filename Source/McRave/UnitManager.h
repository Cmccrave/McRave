#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "UnitInfo.h"
#include <set>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitManager
	{
		map <Unit, UnitInfo> enemyUnits;
		map <Unit, UnitInfo> myUnits;
		map <Unit, UnitInfo> neutrals;

		map <Unit, UnitInfo> allyDefenses;
		map <UnitSizeType, int> allySizes;
		map <UnitSizeType, int> enemySizes;
		map <UnitType, int> enemyComposition;

		set<Unit> enemyThreats;
		set<Unit> armedMines;
		set<Unit> splashTargets;

		double immThreat, proxThreat;
		double avgGrdSim = 1.0, avgAirSim = 1.0;
		bool ignoreSim;

		double globalAllyGroundStrength, globalEnemyGroundStrength;
		double globalAllyAirStrength, globalEnemyAirStrength;
		double allyDefense;
		double minThreshold, maxThreshold;
		int supply;
		int repWorkers;

		//MCRSimOutput sim;
	public:
		int getRepairWorkers() { return repWorkers; }

		set<Unit>& getSplashTargets() { return splashTargets; }

		map<Unit, UnitInfo>& getMyUnits() { return myUnits; }
		map<Unit, UnitInfo>& getEnemyUnits() { return enemyUnits; }
		map<Unit, UnitInfo>& getNeutralUnits() { return neutrals; }

		map<UnitSizeType, int>& getAllySizes() { return allySizes; }
		map<UnitSizeType, int>& getEnemySizes() { return enemySizes; }
		map<UnitType, int>& getEnemyComposition() { return enemyComposition; }

		UnitInfo& getUnitInfo(Unit);
		UnitInfo* getClosestInvisEnemy(BuildingInfo&);	// TODO: just for scanners for now

		double getImmThreat() { return immThreat; }
		double getProxThreat() { return proxThreat; }

		double getGlobalAllyGroundStrength() { return globalAllyGroundStrength; }
		double getGlobalEnemyGroundStrength() { return globalEnemyGroundStrength; }
		double getGlobalAllyAirStrength() { return globalAllyAirStrength; }
		double getGlobalEnemyAirStrength() { return globalEnemyAirStrength; }
		double getAllyDefense() { return allyDefense; }
		int getSupply() { return supply; }

		bool isThreatening(UnitInfo&);
		int getEnemyCount(UnitType);

		// Updating
		void onFrame();
		void updateUnits();
		void updateEnemy(UnitInfo&);
		void updateAlly(UnitInfo&);
		void updateNeutral(UnitInfo&);

		void updateLocalSimulation(UnitInfo&);
		void updateStrategy(UnitInfo&);

		// Storage
		void onUnitDiscover(Unit);
		void onUnitCreate(Unit);
		void onUnitDestroy(Unit);
		void onUnitMorph(Unit);
		void onUnitRenegade(Unit);
		void onUnitComplete(Unit);

		void storeMyUnit(Unit);
		void storeEnemyUnit(Unit);
		void storeNeutralUnit(Unit);
	};
}

typedef Singleton<McRave::UnitManager> UnitSingleton;