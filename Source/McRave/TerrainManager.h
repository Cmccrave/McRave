#pragma once
#include <BWAPI.h>
#include "Singleton.h"

namespace McRave
{
    class TerrainManager
    {
        std::set <const BWEM::Area*> allyTerritory;
        std::set <const BWEM::Area*> enemyTerritory;

        BWAPI::Position enemyStartingPosition = Positions::Invalid;
        BWAPI::Position playerStartingPosition;
        BWAPI::TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        BWAPI::TilePosition playerStartingTilePosition;

        BWAPI::Position mineralHold, backMineralHold;
        BWAPI::Position attackPosition, defendPosition;
        BWAPI::TilePosition enemyNatural = TilePositions::Invalid;
        BWAPI::TilePosition enemyExpand = TilePositions::Invalid;
        std::vector<BWAPI::Position> meleeChokePositions;
        std::vector<BWAPI::Position> rangedChokePositions;

        std::set<BWEM::Base const*> allBases;

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
        bool findNaturalWall(std::vector<BWAPI::UnitType>&, const std::vector<BWAPI::UnitType>& defenses ={});
        bool findMainWall(std::vector<BWAPI::UnitType>&, const std::vector<BWAPI::UnitType>& defenses ={});
        bool isIslandMap() { return islandMap; }

        // Main ramp information
        bool isReverseRamp() { return reverseRamp; }
        bool isFlatRamp() { return flatRamp; }

        // Natural ramp information
        bool isNarrowNatural() { return narrowNatural; }
        bool isDefendNatural() { return defendNatural; }

        bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(BWAPI::TilePosition(enemyStartingPosition)); }

        const BWEB::Walls::Wall* getMainWall() { return mainWall; }
        const BWEB::Walls::Wall* getNaturalWall() { return naturalWall; }

        BWAPI::Position getMineralHoldPosition() { return mineralHold; }
        BWAPI::Position getBackMineralHoldPosition() { return backMineralHold; }
        bool isInAllyTerritory(BWAPI::TilePosition);
        bool isInEnemyTerritory(BWAPI::TilePosition);
        bool isStartingBase(BWAPI::TilePosition);

        std::set <const BWEM::Area*>& getAllyTerritory() { return allyTerritory; }
        std::set <const BWEM::Area*>& getEnemyTerritory() { return enemyTerritory; }
        std::set <BWEM::Base const*>& getAllBases() { return allBases; }

        BWAPI::Position getEnemyStartingPosition() { return enemyStartingPosition; }
        BWAPI::Position getPlayerStartingPosition() { return playerStartingPosition; }
        BWAPI::TilePosition getEnemyNatural() { return enemyNatural; }
        BWAPI::TilePosition getEnemyExpand() { return enemyExpand; }
        BWAPI::TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
        BWAPI::TilePosition getPlayerStartingTilePosition() { return playerStartingTilePosition; }

        BWAPI::Position closestUnexploredStart();
        BWAPI::Position randomBasePosition();

        BWAPI::Position getAttackPosition() { return attackPosition; }
        BWAPI::Position getDefendPosition() { return defendPosition; }

        std::vector<BWAPI::Position> getMeleeChokePositions() { return meleeChokePositions; }
        std::vector<BWAPI::Position> getRangedChokePositions() { return rangedChokePositions; }
    };
}

typedef Singleton<McRave::TerrainManager> TerrainSingleton;
