#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class TerrainTrackerClass
{
	CPPath path;
	set <int> allyTerritory;
	vector<Position> defendHere;
	set<Position> enemyBasePositions;
	set<TilePosition> allBaseLocations;
	Position enemyStartingPosition, playerStartingPosition;
	TilePosition enemyStartingTilePosition, playerStartingTilePosition, FFEPosition;
	TilePosition secondChoke, firstChoke;
	Position mineralHold;

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
	
	CPPath getPath() { return path; }
	set <int>& getAllyTerritory() { return allyTerritory; }	
	vector<Position>& getDefendHere() { return defendHere; }
	set<Position>& getEnemyBasePositions() { return enemyBasePositions; }
	set<TilePosition>& getAllBaseLocations() { return allBaseLocations; }

	Position getEnemyStartingPosition() { return enemyStartingPosition; }
	Position getPlayerStartingPosition() { return playerStartingPosition; }
	TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
	TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }	
	TilePosition getFFEPosition() { return FFEPosition; }
	TilePosition getFirstChoke() { return firstChoke; }
	TilePosition getSecondChoke() { return secondChoke; }
};

typedef Singleton<TerrainTrackerClass> TerrainTracker;