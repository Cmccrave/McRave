#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class StrategyTrackerClass
{
	map <UnitType, double> unitScore;
	set <UnitType> lockedType;
	bool allyFastExpand = false;
	bool enemyFastExpand = false;
	bool invis = false;
	bool rush = false;
	bool holdRamp = false;
	bool bust = false;
	bool zealotsLocked = false;
	bool marinesLocked = false;
	bool playPassive = false;
	double recallFrame = 0.0;

	// Testing stuff
	set <Bullet> myBullets;
	map <UnitType, double> unitPerformance;

public:

	map <UnitType, double>& getUnitScore() { return unitScore; }
	bool isAllyFastExpand() { return allyFastExpand; }
	bool isEnemyFastExpand() { return enemyFastExpand; }
	bool needDetection() { return invis; }
	bool isRush() { return rush; }
	bool isHoldRamp() { return holdRamp; }
	bool isBust() { return bust; }
	bool isZealotsLocked() { return zealotsLocked; }
	bool isPlayPassive() { return playPassive; }

	bool isUnitLocked(UnitType unit) { return (lockedType.find(unit) != lockedType.end()); }

	// Updating
	void update();
	void updateBullets();
	void updateScoring();
	void protossStrategy();
	void terranStrategy();
	void zergStrategy();
	void updateSituationalBehaviour();
	void updateProtossUnitScore(UnitType, int);
	void updateTerranUnitScore(UnitType, int);
	void updateZergUnitScore(UnitType, int);

	// Check if we have locked a unit out of being allowed
	bool isLocked(UnitType);

	void recallEvent() { recallFrame = Broodwar->getFrameCount(); }
	double getRecallFrame() { return recallFrame; }
};

typedef Singleton<StrategyTrackerClass> StrategyTracker;