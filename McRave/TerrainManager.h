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
	Position attackPosition, defendPosition;
	TilePosition natural;
	TilePosition bMedium = TilePositions::None, bLarge = TilePositions::None, bSmall;
	Area const * naturalArea;
	TilePosition enemyNatural = TilePositions::Invalid;

	// Experimental
	TilePosition best;
	set<TilePosition> smallPosition, mediumPosition, largePosition;
	set<TilePosition> smallBlocks, mediumBlocks, largeBlocks;

public:
	void onStart();
	void update();
	void updateAreas();
	void updateChokes();	
	void updateWalls();

	void findFirstChoke();
	void findSecondChoke();
	void findNatural();
	void findEnemyNatural();

	void updateBlocks();

	int getGroundDistance(Position, Position);
	Position getClosestBaseCenter(Position);
	Position getMineralHoldPosition() { return mineralHold; }
	Position getBackMineralHoldPosition() { return backMineralHold; }
	bool isInAllyTerritory(TilePosition);
	bool isInEnemyTerritory(TilePosition);
	bool overlapsWall(TilePosition);
	Area const * getNaturalArea() { return naturalArea; }

	set <int>& getAllyTerritory() { return allyTerritory; }
	set <int>& getEnemyTerritory() { return enemyTerritory; }
	set<TilePosition>& getAllBaseLocations() { return allBaseLocations; }

	Position getEnemyStartingPosition() { return enemyStartingPosition; }
	Position getPlayerStartingPosition() { return playerStartingPosition; }
	TilePosition getEnemyNatural() { return enemyNatural; }
	TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
	TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }
	TilePosition getFFEPosition() { return FFEPosition; }
	TilePosition getFirstChoke() { return firstChoke; }
	TilePosition getSecondChoke() { return secondChoke; }
	TilePosition getNatural() { return natural; }

	TilePosition getSmallWall() { return bSmall; }
	TilePosition getMediumWall() { return bMedium; }
	TilePosition getLargeWall() { return bLarge; }

	Position getAttackPosition() { return attackPosition; }
	Position getDefendPosition() { return defendPosition; }

	// Experimental
	set<TilePosition> getSmallPosition() { return smallPosition; }
	set<TilePosition> getMediumPosition() { return mediumPosition; }
	set<TilePosition> getLargePosition() { return largePosition; }

	void insertSmallBlock(TilePosition, bool);
	void insertMediumBlock(TilePosition, bool);
	void insertLargeBlock(TilePosition, bool);
	bool overlapsBlocks(TilePosition);
	bool overlapsBases(TilePosition);
};

typedef Singleton<TerrainTrackerClass> TerrainTracker;