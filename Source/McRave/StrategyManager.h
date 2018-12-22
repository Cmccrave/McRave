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

		bool enemyFE = false;
		bool invis = false;
		bool rush = false;
		bool holdChoke = false;

		bool proxy = false;
		bool gasSteal = false;
		bool enemyScout = false;
		bool pressure = false;
		string enemyBuild = "Unknown";
		int poolFrame, lingFrame;
		int enemyGas;
		int enemyFrame;

		int inboundScoutFrame;
		int inboundLingFrame;

		bool goonRange = false;			
		bool vultureSpeed = false;

		void checkNeedDetection();
		void checkEnemyPressure();
		void checkEnemyProxy();
		void checkEnemyRush();
		void checkHoldChoke();

		void updateScoring();
		void updateSituationalBehaviour();
		void updateEnemyBuild();
		void updateMadMixScore();
		void updateProtossUnitScore(UnitType, int);
		void updateTerranUnitScore(UnitType, int);
		//void updateZergUnitScore(UnitType, int);

	public:
		string getEnemyBuild() { return enemyBuild; }
		bool enemyFastExpand() { return enemyFE; }
		bool enemyRush() { return rush; }
		bool needDetection() { return invis; }
		bool defendChoke() { return holdChoke; }
		bool enemyProxy() { return proxy; }
		bool enemyGasSteal() { return gasSteal; }
		bool enemyScouted() { return enemyScout; }
		bool enemyBust() { return enemyBuild.find("Hydra") != string::npos; }
		bool enemyPressure() { return pressure; }
		int getPoolFrame() { return poolFrame; }
			
		void onFrame();
		double getUnitScore(UnitType);
		map <UnitType, double>& getUnitScores() { return unitScore; }
		UnitType getHighestUnitScore();		
	};
}

typedef Singleton<McRave::StrategyManager> StrategySingleton;
