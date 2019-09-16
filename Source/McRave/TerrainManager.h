#pragma once
#include <BWAPI.h>

namespace McRave::Terrain {

    void onStart();
    void onFrame();

    bool isInAllyTerritory(BWAPI::TilePosition);
    bool isInAllyTerritory(const BWEM::Area *);

    bool isInEnemyTerritory(BWAPI::TilePosition);
    bool isStartingBase(BWAPI::TilePosition);
    BWAPI::Position closestUnexploredStart();
    BWAPI::Position randomBasePosition();
       
    bool isShitMap();
    bool isIslandMap();
    bool isReverseRamp();
    bool isFlatRamp();
    bool isNarrowNatural();
    bool isDefendNatural();
    bool foundEnemy();

    BWAPI::Position getAttackPosition();
    BWAPI::Position getDefendPosition();
    BWAPI::Position getEnemyStartingPosition();
    BWAPI::Position getMineralHoldPosition();
    BWAPI::Position getBackMineralHoldPosition();
    BWAPI::TilePosition getEnemyNatural();
    BWAPI::TilePosition getEnemyExpand();
    BWAPI::TilePosition getEnemyStartingTilePosition();
    std::vector<BWAPI::Position> getMeleeChokePositions();
    std::vector<BWAPI::Position> getRangedChokePositions();
    std::set <const BWEM::Area*>& getAllyTerritory();
    std::set <const BWEM::Area*>& getEnemyTerritory();
    std::set <BWEM::Base const*>& getAllBases();
    BWEB::Wall* getMainWall();
    BWEB::Wall* getNaturalWall();


    inline bool isExplored(BWAPI::Position here) { return BWAPI::Broodwar->isExplored(BWAPI::TilePosition(here)); }
}
