#pragma once
#include <BWAPI.h>

namespace McRave::Terrain {
    BWAPI::Position getClosestMapCorner(BWAPI::Position);
    BWAPI::Position getClosestMapEdge(BWAPI::Position);
    BWAPI::Position getOldestPosition(const BWEM::Area *);
    BWAPI::Position getSafeSpot(BWEB::Station *);
    void onStart();
    void onFrame();

    std::set <const BWEM::Base*>& getAllBases();

    BWAPI::Position getEnemyStartingPosition();
    BWEB::Station * getEnemyMain();
    BWEB::Station * getEnemyNatural();
    BWEB::Station * getMyMain();
    BWEB::Station * getMyNatural();
    BWAPI::TilePosition getEnemyExpand();
    BWAPI::TilePosition getEnemyStartingTilePosition();

    bool isIslandMap();
    bool isReverseRamp();
    bool isFlatRamp();
    bool isNarrowNatural();
    bool foundEnemy();
    bool inArea(const BWEM::Area *, BWAPI::Position);
    bool inTerritory(PlayerState, BWAPI::Position);
    bool inTerritory(PlayerState, const BWEM::Area*);
    void addTerritory(PlayerState, BWEB::Station*);
    void removeTerritory(PlayerState, BWEB::Station*);
    std::vector<BWAPI::Position>& getGroundCleanupPositions();
    std::vector<BWAPI::Position>& getAirCleanupPositions();

    // Main information
    BWAPI::Position getMainPosition();
    BWAPI::TilePosition getMainTile();
    const BWEM::Area * getMainArea();
    const BWEM::ChokePoint * getMainChoke();

    // Natural information
    BWAPI::Position getNaturalPosition();
    BWAPI::TilePosition getNaturalTile();
    const BWEM::Area * getNaturalArea();
    const BWEM::ChokePoint * getNaturalChoke();

    inline bool isExplored(BWAPI::Position here) { return BWAPI::Broodwar->isExplored(BWAPI::TilePosition(here)); }
}
