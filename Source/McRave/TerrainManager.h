#pragma once
#include <BWAPI.h>

namespace McRave::Terrain {

    void onStart();
    void onFrame();

    bool inRangeOfWall(UnitInfo&);
    bool findNaturalWall(std::vector<BWAPI::UnitType>&, const std::vector<BWAPI::UnitType>& defenses ={});
    bool findMainWall(std::vector<BWAPI::UnitType>&, const std::vector<BWAPI::UnitType>& defenses ={});
    bool isInAllyTerritory(BWAPI::TilePosition);
    bool isInEnemyTerritory(BWAPI::TilePosition);
    bool isStartingBase(BWAPI::TilePosition);
    BWAPI::Position closestUnexploredStart();
    BWAPI::Position randomBasePosition();
       
    bool isIslandMap();
    bool isReverseRamp();
    bool isFlatRamp();
    bool isNarrowNatural();
    bool isDefendNatural();
    bool foundEnemy();
    BWAPI::Position getAttackPosition();
    BWAPI::Position getDefendPosition();
    BWAPI::Position getEnemyStartingPosition();
    BWAPI::Position getPlayerStartingPosition();
    BWAPI::Position getMineralHoldPosition();
    BWAPI::Position getBackMineralHoldPosition();
    BWAPI::TilePosition getEnemyNatural();
    BWAPI::TilePosition getEnemyExpand();
    BWAPI::TilePosition getEnemyStartingTilePosition();
    BWAPI::TilePosition getPlayerStartingTilePosition();
    std::vector<BWAPI::Position> getMeleeChokePositions();
    std::vector<BWAPI::Position> getRangedChokePositions();
    std::set <const BWEM::Area*>& getAllyTerritory();
    std::set <const BWEM::Area*>& getEnemyTerritory();
    std::set <BWEM::Base const*>& getAllBases();
    const BWEB::Walls::Wall* getMainWall();
    const BWEB::Walls::Wall* getNaturalWall();
}
