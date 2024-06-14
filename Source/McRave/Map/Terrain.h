#pragma once
#include <BWAPI.h>

namespace McRave::Terrain {

    struct Ramp {
        BWAPI::Position entrance, exit, center;
        double angle;
    };

    Ramp& getMainRamp();

    BWAPI::Position getClosestMapCorner(BWAPI::Position);
    BWAPI::Position getClosestMapEdge(BWAPI::Position);
    BWAPI::Position getOldestPosition(const BWEM::Area *);
    BWAPI::Position getSafeSpot(const BWEB::Station *);
    void onStart();
    void onFrame();

    std::set <const BWEM::Base*>& getAllBases();

    BWAPI::Position getEnemyStartingPosition();
    const BWEB::Station * getEnemyMain();
    const BWEB::Station * getEnemyNatural();
    const BWEB::Station * const getMyMain();
    const BWEB::Station * const getMyNatural();
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
    void addTerritory(PlayerState, const BWEB::Station*);
    void removeTerritory(PlayerState, const BWEB::Station*);
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

    // Chokepoint information
    BWAPI::Position getChokepointCenter(const BWEM::ChokePoint * chokepoint);
    double getChokepointAngle(const BWEM::ChokePoint * chokepoint);

    inline bool isExplored(BWAPI::Position here) { return BWAPI::Broodwar->isExplored(BWAPI::TilePosition(here)); }
}
