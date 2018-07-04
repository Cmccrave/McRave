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

		Position enemyStartingPosition = Positions::Invalid, playerStartingPosition;
		TilePosition enemyStartingTilePosition, playerStartingTilePosition;

		Position mineralHold, backMineralHold;
		Position attackPosition, defendPosition;
		TilePosition enemyNatural = TilePositions::Invalid;
		TilePosition enemyExpand = TilePositions::Invalid;

		set<Base const*> allBases;

		Wall* wall;
		void findEnemyStartingPosition(), findEnemyNatural(), findEnemyNextExpand(), findDefendPosition(), findAttackPosition();
		void updateTerrain();
		bool islandMap;
		bool enemyWall;
		
	public:
		void onStart(), onFrame();
		bool findNaturalWall(vector<UnitType>&, const vector<UnitType>& defenses ={});
		bool findMainWall(vector<UnitType>&, const vector<UnitType>& defenses ={});

		bool isIslandMap() { return islandMap; }
		bool isEnemyWalled() { return enemyWall; }

		const Wall* getWall() { return wall; }

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

		Position getAttackPosition() { return attackPosition; }
		Position getDefendPosition() { return defendPosition; }

		// Experimental
		bool overlapsBases(TilePosition);
		bool overlapsNeutrals(TilePosition);
	};
}

typedef Singleton<McRave::TerrainManager> TerrainSingleton;
