#pragma once
#include "BWEB.h"
#include "Main/Common.h"
#include "bwem.h"

namespace McRave::Terrain {

    struct Ramp {
        BWAPI::Position entrance, exit, center;
        double angle;
    };

    Ramp &getMainRamp();

    BWAPI::Position getClosestMapCorner(BWAPI::Position);
    BWAPI::Position getClosestMapEdge(BWAPI::Position);
    void onStart();
    void onFrame();

    std::set<const BWEM::Base *> &getAllBases();

    BWAPI::Position getEnemyStartingPosition();
    const BWEB::Station *getEnemyMain();
    const BWEB::Station *getEnemyNatural();
    const BWEB::Station *const getMyMain();
    const BWEB::Station *const getMyNatural();
    BWAPI::TilePosition getEnemyExpand();
    BWAPI::TilePosition getEnemyStartingTilePosition();

    bool isIslandMap();
    bool isReverseRamp();
    bool isFlatRamp();
    bool isNarrowNatural();
    bool isPocketNatural();
    bool foundEnemy();
    void addTerritory(PlayerState, const BWEB::Station *);
    void removeTerritory(PlayerState, const BWEB::Station *);
    std::vector<BWAPI::Position> &getGroundCleanupPositions();
    std::vector<BWAPI::Position> &getAirCleanupPositions();

    // Strategic checks
    bool isAtHome(BWAPI::Position);
    bool isAtEnemy(BWAPI::Position);
    bool isCloserHome(BWAPI::Position);
    bool isCloserEnemy(BWAPI::Position);

    // Main information
    BWAPI::Position getMainPosition();
    BWAPI::TilePosition getMainTile();
    const BWEM::Area *getMainArea();
    const BWEM::ChokePoint *getMainChoke();

    // Natural information
    BWAPI::Position getNaturalPosition();
    BWAPI::TilePosition getNaturalTile();
    const BWEM::Area *getNaturalArea();
    const BWEM::ChokePoint *getNaturalChoke();

    // Chokepoint information
    BWAPI::Position getChokepointCenter(const BWEM::ChokePoint *chokepoint);
    double getChokepointAngle(const BWEM::ChokePoint *chokepoint);

    template <typename T>          //
    inline bool isExplored(T here) //
    {
        return BWAPI::Broodwar->isExplored(BWAPI::TilePosition(here));
    }

    bool isChokepointGeo(BWAPI::Position);
    bool isChokepointGeo(BWAPI::WalkPosition);
    bool isChokepointGeo(BWAPI::TilePosition);

    // Area information
    struct AreaGeometry {
        std::vector<BWAPI::TilePosition> tiles;
        std::vector<BWAPI::WalkPosition> walks;
    };
    AreaGeometry* getAreaGeometry(const BWEM::Area *);
    inline AreaGeometry* getAreaGeometry(const BWEB::Station *station) { return getAreaGeometry(station->getBase()->GetArea()); };
    std::vector<BWAPI::WalkPosition> getAreaOutline(const BWEM::Area *);

    // Checks if "here" is in area
    bool inArea(const BWEM::Area *area, BWAPI::Position here);
    bool inArea(const BWEM::Area *area, BWAPI::WalkPosition here);
    bool inArea(const BWEM::Area *area, BWAPI::TilePosition here);

    // Checks if "here" is in the station area
    template <typename T>                                    //
    inline bool inArea(const BWEB::Station *station, T here) //
    {
        if (!here.isValid() || !station)
            return false;
        auto area = station->getBase()->GetArea();
        return inArea(area, here);
    }

    // Checks if "here" is in the area of "there"
    template <typename T>               //
    inline bool inArea(T there, T here) //
    {
        if (!here.isValid() || !there.isValid())
            return false;
        auto area = mapBWEM.GetArea(BWAPI::TilePosition(there));
        return inArea(area, here);
    }

    bool inTerritory(PlayerState, BWAPI::Position);
    bool inTerritory(PlayerState, const BWEM::Area *);

    // Checks if the source to the target is all within the given PlayerState territory
    template <typename T> //
    inline bool inTerritoryPath(PlayerState state, T source, T target)
    {
        if (!inTerritory(state, source) || !inTerritory(state, target))
            return false;

        for (auto path : mapBWEM.GetPath(source, target)) {
            if (!inTerritory(state, path->GetAreas().first) || !inTerritory(state, path->GetAreas().second)) {
                return false;
            }
        }
        return true;
    }

} // namespace McRave::Terrain
