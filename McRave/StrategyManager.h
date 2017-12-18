#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class StrategyTrackerClass
{
	map <UnitType, double> unitScore;	
	bool allyFastExpand = false;
	bool enemyFastExpand = false;
	bool invis = false;
	bool rush = false;
	bool holdChoke = false;
	bool playPassive = false;
	bool hideTech = false;
	string enemyBuild = "Unknown";
	int poolFrame, lingFrame;
	int enemyGas;

	// Testing stuff
	set <Bullet> myBullets;
	map <UnitType, double> unitPerformance;

public:
	string getEnemyBuild() { return enemyBuild; }
	map <UnitType, double>& getUnitScore() { return unitScore; }
	bool isAllyFastExpand() { return allyFastExpand; }
	bool isEnemyFastExpand() { return enemyFastExpand; }
	bool needDetection() { return invis; }
	bool isRush() { return rush; }
	bool isHoldChoke() { return holdChoke; }
	bool isPlayPassive() { return playPassive; }
	int getPoolFrame() { return poolFrame; }

	// Updating
	void update();
	void updateBullets();
	void updateScoring();
	void protossStrategy();
	void terranStrategy();
	void zergStrategy();
	void updateSituationalBehaviour();
	void updateEnemyBuild();
	void updateProtossUnitScore(UnitType, int);
	void updateTerranUnitScore(UnitType, int);
	void updateZergUnitScore(UnitType, int);

	bool shouldPlayPassive();
	bool shouldDefendRush();
	bool shouldHoldChoke();
	bool shouldHideTech();
	bool shouldGetDetection();	

	// Check if we have locked a unit out of being allowed
	bool isLocked(UnitType);
};

typedef Singleton<StrategyTrackerClass> StrategyTracker;