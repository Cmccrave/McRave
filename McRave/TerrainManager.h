#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TerrainTrackerClass
	{
		set <int> allyTerritory;
		set <int> enemyTerritory;
		Position enemyStartingPosition = Positions::Invalid, playerStartingPosition;
		TilePosition enemyStartingTilePosition, playerStartingTilePosition, FFEPosition;

		Position mineralHold, backMineralHold;
		Position attackPosition, defendPosition;
		TilePosition enemyNatural = TilePositions::Invalid;

		set<const Base*> allBases;
		Area const * naturalArea;
		Area const * mainArea;
		TilePosition firstChoke, natural, secondChoke;
		// Experimental


	public:
		void onStart();
		void update();
		void updateAreas();
		void updateChokes();
		void findEnemyNatural();

		double getGroundDistance(Position, Position);
		Position getClosestBaseCenter(Position);
		Position getMineralHoldPosition() { return mineralHold; }
		Position getBackMineralHoldPosition() { return backMineralHold; }
		bool isInAllyTerritory(TilePosition);
		bool isInEnemyTerritory(TilePosition);
		Area const * getNaturalArea() { return naturalArea; }

		set <int>& getAllyTerritory() { return allyTerritory; }
		set <int>& getEnemyTerritory() { return enemyTerritory; }
		const set <const Base*>& getAllBases() { return allBases; }

		Position getEnemyStartingPosition() { return enemyStartingPosition; }
		Position getPlayerStartingPosition() { return playerStartingPosition; }
		TilePosition getEnemyNatural() { return enemyNatural; }
		TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
		TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }
		TilePosition getFFEPosition() { return FFEPosition; }

		Position getAttackPosition() { return attackPosition; }
		Position getDefendPosition() { return defendPosition; }

		// Experimental
		bool overlapsBases(TilePosition);
		bool overlapsNeutrals(TilePosition);
	};
}

typedef Singleton<McRave::TerrainTrackerClass> TerrainTracker;