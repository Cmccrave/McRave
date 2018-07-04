#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class StrategyManager
	{
		map <UnitType, double> unitScore;
		bool allyFastExpand = false;
		bool enemyFE = false;
		bool invis = false;
		bool rush = false;
		bool holdChoke = false;
		bool hideTech = false;
		bool proxy = false;
		string enemyBuild = "Unknown";
		int poolFrame, lingFrame;
		int enemyGas;
		int enemyFrame;

		// Testing stuff
		set <Bullet> myBullets;
		map <UnitType, double> unitPerformance;
		bool goonRange = false;			
		bool shouldHoldChoke();
		bool shouldHideTech();
		bool shouldGetDetection();

		bool checkEnemyRush();
		bool checkEnemyProxy();

		set<Position> scoutTargets;

	public:
		string getEnemyBuild() { return enemyBuild; }
		bool enemyFastExpand() { return enemyFE; }
		bool enemyRush() { return rush; }
		bool needDetection() { return invis; }
		bool defendChoke() { return holdChoke; }
		bool enemyProxy() { return proxy; }
		int getPoolFrame() { return poolFrame; }
			

		// Updating
		void onFrame();
		void updateBullets();
		void updateScoring();
		void protossStrategy();
		void terranStrategy();
		void zergStrategy();
		void updateSituationalBehaviour();
		void updateEnemyBuild();

		void updateMadMixScore(UnitType, int);

		void updateProtossUnitScore(UnitType, int);
		void updateTerranUnitScore(UnitType, int);
		void updateZergUnitScore(UnitType, int);

		double getUnitScore(UnitType);
		map <UnitType, double>& getUnitScores() { return unitScore; }
		UnitType getHighestUnitScore();

		set<Position> getScoutTargets() { return scoutTargets; }
	};
}

typedef Singleton<McRave::StrategyManager> StrategySingleton;
