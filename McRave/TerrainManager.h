#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class TerrainTrackerClass
{
	set <int> allyTerritory;
	set<Position> enemyBasePositions;
	set<TilePosition> allBaseLocations;
	Position enemyStartingPosition, playerStartingPosition;
	TilePosition enemyStartingTilePosition, playerStartingTilePosition, FFEPosition;
	TilePosition secondChoke, firstChoke;
	Position mineralHold, backMineralHold;
	Position attackPosition;

public:
	void onStart();
	void update();
	void updateAreas();
	void updateChokes();
	void removeTerritory(Unit);
	bool isInAllyTerritory(Unit);
	
	Position getClosestBaseCenter(Unit);
	Position getClosestEnemyBase(Position);
	Position getClosestAllyBase(Position);
	Position getMineralHoldPosition() { return mineralHold; }
	Position getBackMineralHoldPosition() { return backMineralHold; }
		
	set <int>& getAllyTerritory() { return allyTerritory; }
	set<Position>& getEnemyBasePositions() { return enemyBasePositions; }
	set<TilePosition>& getAllBaseLocations() { return allBaseLocations; }

	Position getEnemyStartingPosition() { return enemyStartingPosition; }
	Position getPlayerStartingPosition() { return playerStartingPosition; }
	TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
	TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }	
	TilePosition getFFEPosition() { return FFEPosition; }
	TilePosition getFirstChoke() { return firstChoke; }
	TilePosition getSecondChoke() { return secondChoke; }

	Position getAttackPosition() { return attackPosition; }
};

typedef Singleton<TerrainTrackerClass> TerrainTracker;