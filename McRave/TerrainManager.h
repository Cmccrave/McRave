#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class TerrainTrackerClass
{
	set <int> allyTerritory;
	set <int> enemyTerritory;
	set<TilePosition> allBaseLocations;
	Position enemyStartingPosition = Positions::Invalid, playerStartingPosition;
	TilePosition enemyStartingTilePosition, playerStartingTilePosition, FFEPosition;
	TilePosition secondChoke, firstChoke;
	Position mineralHold, backMineralHold;
	Position attackPosition;
	TilePosition natural;
	TilePosition bMedium, bLarge, bSmall;

public:
	void onStart();
	void update();
	void updateAreas();
	void updateChokes();	
	void updateWalls();

	Position getClosestBaseCenter(Unit);
	Position getMineralHoldPosition() { return mineralHold; }
	Position getBackMineralHoldPosition() { return backMineralHold; }
	bool isInAllyTerritory(TilePosition);
	bool isInEnemyTerritory(TilePosition);

	set <int>& getAllyTerritory() { return allyTerritory; }
	set <int>& getEnemyTerritory() { return enemyTerritory; }
	set<TilePosition>& getAllBaseLocations() { return allBaseLocations; }

	Position getEnemyStartingPosition() { return enemyStartingPosition; }
	Position getPlayerStartingPosition() { return playerStartingPosition; }
	TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
	TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }
	TilePosition getFFEPosition() { return FFEPosition; }
	TilePosition getFirstChoke() { return firstChoke; }
	TilePosition getSecondChoke() { return secondChoke; }

	TilePosition getSmallWall() { return bSmall; }
	TilePosition getMediumWall() { return bMedium; }
	TilePosition getLargeWall() { return bLarge; }


	Position getAttackPosition() { return attackPosition; }
};

typedef Singleton<TerrainTrackerClass> TerrainTracker;