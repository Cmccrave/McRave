#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TerrainManager
	{
		set <const BWEM::Area*> allyTerritory;
		set <const BWEM::Area*> enemyTerritory;

		Position enemyStartingPosition = Positions::Invalid;
		Position playerStartingPosition;
		TilePosition enemyStartingTilePosition = TilePositions::Invalid;
		TilePosition playerStartingTilePosition;

		Position mineralHold, backMineralHold;
		Position attackPosition, defendPosition;
		TilePosition enemyNatural = TilePositions::Invalid;
		TilePosition enemyExpand = TilePositions::Invalid;
		vector<Position> meleeChokePositions;
		vector<Position> rangedChokePositions;

		set<Base const*> allBases;

		BWEB::Walls::Wall* mainWall = nullptr;
		BWEB::Walls::Wall* naturalWall = nullptr;
		void findEnemyStartingPosition(), findEnemyNatural(), findEnemyNextExpand(), findDefendPosition(), findAttackPosition();
		void updateConcavePositions(), updateAreas();

		bool islandMap;
		bool reverseRamp;
		bool flatRamp;

		bool narrowNatural;
		bool defendNatural;
	public:
		void onStart(), onFrame();
		bool findNaturalWall(vector<UnitType>&, const vector<UnitType>& defenses ={});
		bool findMainWall(vector<UnitType>&, const vector<UnitType>& defenses ={});
		bool isIslandMap() { return islandMap; }

		// Main ramp information
		bool isReverseRamp() { return reverseRamp; }
		bool isFlatRamp() { return flatRamp; }

		// Natural ramp information
		bool isNarrowNatural() { return narrowNatural; }
		bool isDefendNatural() { return defendNatural; }

		bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(TilePosition(enemyStartingPosition)); }

		const BWEB::Walls::Wall* getMainWall() { return mainWall; }
		const BWEB::Walls::Wall* getNaturalWall() { return naturalWall; }

		Position getMineralHoldPosition() { return mineralHold; }
		Position getBackMineralHoldPosition() { return backMineralHold; }
		bool isInAllyTerritory(TilePosition);
		bool isInEnemyTerritory(TilePosition);
		bool isStartingBase(TilePosition);

		set <const BWEM::Area*>& getAllyTerritory() { return allyTerritory; }
		set <const BWEM::Area*>& getEnemyTerritory() { return enemyTerritory; }
		set <Base const*>& getAllBases() { return allBases; }

		Position getEnemyStartingPosition() { return enemyStartingPosition; }
		Position getPlayerStartingPosition() { return playerStartingPosition; }
		TilePosition getEnemyNatural() { return enemyNatural; }
		TilePosition getEnemyExpand() { return enemyExpand; }
		TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
		TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }

		Position closestUnexploredStart();
		Position randomBasePosition();

		Position getAttackPosition() { return attackPosition; }
		Position getDefendPosition() { return defendPosition; }

		vector<Position> getMeleeChokePositions() { return meleeChokePositions; }
		vector<Position> getRangedChokePositions() { return rangedChokePositions; }
	};
}

typedef Singleton<McRave::TerrainManager> TerrainSingleton;
