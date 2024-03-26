#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Terrain {

    namespace {

        // Territory
        map<const Area*, PlayerState> territoryArea;
        map<WalkPosition, PlayerState> territoryChokeGeometry;

        // Enemy
        const BWEB::Station * enemyNatural = nullptr;
        const BWEB::Station * enemyMain = nullptr;
        Position enemyStartingPosition = Positions::Invalid;
        TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;

        // Map
        map<WalkPosition, pair<const BWEM::Area *, const BWEM::Area *>> areaChokeGeometry;
        set<const Base *> allBases;
        vector<Position> mapEdges, mapCorners;
        bool islandMap = false;
        bool reverseRamp = false;
        bool flatRamp = false;
        bool narrowNatural = false;

        // Cleanup
        vector<Position> groundCleanupPositions;
        vector<Position> airCleanupPositions;

        // Overlord scouting
        map<const BWEB::Station * const, Position> safeSpots;

        void findEnemy()
        {
            // If we think we found the enemy but we were wrong
            if (enemyStartingPosition.isValid()) {
                if (Broodwar->isExplored(enemyStartingTilePosition) && Util::getTime() < Time(5, 00) && BWEB::Map::isUsed(enemyStartingTilePosition) == UnitTypes::None) {
                    enemyStartingPosition = Positions::Invalid;
                    enemyStartingTilePosition = TilePositions::Invalid;
                    enemyNatural = nullptr;
                    enemyMain = nullptr;
                    Stations::getStations(PlayerState::Enemy).clear();
                }
                else
                    return;
            }

            // Find closest enemy building
            if (Util::getTime() < Time(5, 00)) {
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    UnitInfo &unit = *u;
                    if (!unit.getType().isBuilding()
                        || !unit.getTilePosition().isValid()
                        || unit.isFlying()
                        || unit.isProxy())
                        continue;

                    const auto closestMain = BWEB::Stations::getClosestMainStation(unit.getTilePosition());

                    // Set start if valid
                    if (closestMain && closestMain != getMyMain()) {
                        enemyStartingTilePosition = closestMain->getBase()->Location();
                        enemyStartingPosition = Position(enemyStartingTilePosition) + Position(64, 48);
                    }
                }
            }

            // Infer based on enemy Overlord
            if (Players::vZ() && Util::getTime() < Time(3, 15)) {
                auto inferedStart = TilePositions::Invalid;
                auto inferedCount = 0;
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    UnitInfo &unit = *u;

                    if (unit.getType() != Zerg_Overlord)
                        continue;

                    for (auto &start : Broodwar->getStartLocations()) {
                        if (start == Terrain::getMainTile())
                            continue;                        

                        auto startCenter = Position(start) + Position(64, 48);
                        auto frameDiff = abs(Broodwar->getFrameCount() - (unit.getPosition().getDistance(startCenter) / unit.getSpeed()));

                        if (frameDiff < 120) {
                            inferedStart = start;
                            inferedCount++;
                        }
                    }

                    // Set start if valid
                    if (inferedCount == 1 && inferedStart.isValid() && inferedStart != getMainTile()) {
                        enemyStartingPosition = Position(inferedStart) + Position(64, 48);
                        enemyStartingTilePosition = inferedStart;
                    }
                }
            }

            // Assume based on only 1 remaining location
            vector<TilePosition> unexploredStarts;
            if (!enemyStartingPosition.isValid()) {
                for (auto &topLeft : mapBWEM.StartingLocations()) {
                    auto botRight = topLeft + TilePosition(3, 2);
                    if (!Broodwar->isExplored(botRight) || !Broodwar->isExplored(topLeft))
                        unexploredStarts.push_back(topLeft);
                }

                if (int(unexploredStarts.size()) == 1) {
                    enemyStartingPosition = Position(unexploredStarts.front()) + Position(64, 48);
                    enemyStartingTilePosition = unexploredStarts.front();
                }
            }

            // Locate Stations
            if (enemyStartingPosition.isValid()) {
                enemyMain = BWEB::Stations::getClosestMainStation(enemyStartingTilePosition);
                enemyNatural = BWEB::Stations::getClosestNaturalStation(enemyStartingTilePosition);

                if (enemyMain) {
                    addTerritory(PlayerState::Enemy, enemyMain);
                    Stations::getStations(PlayerState::Enemy).push_back(enemyMain);
                }
            }
        }

        void findEnemyNextExpand()
        {
            if (!enemyStartingPosition.isValid())
                return;

            double best = 0.0;
            for (auto &station : BWEB::Stations::getStations()) {

                // If station is used
                if (BWEB::Map::isUsed(station.getBase()->Location()) != None
                    || enemyStartingTilePosition == station.getBase()->Location()
                    || !station.getBase()->GetArea()->AccessibleFrom(getMainArea())
                    || &station == enemyNatural)
                    continue;

                // Get value of the expansion
                double value = 0.0;
                for (auto &mineral : station.getBase()->Minerals())
                    value += double(mineral->Amount());
                for (auto &gas : station.getBase()->Geysers())
                    value += double(gas->Amount());
                if (station.getBase()->Geysers().size() == 0)
                    value = value / 10.0;

                // Get distance of the expansion
                double distance;
                if (!station.getBase()->GetArea()->AccessibleFrom(getMainArea()))
                    distance = log(station.getBase()->Center().getDistance(enemyStartingPosition));
                else
                    distance = BWEB::Map::getGroundDistance(enemyStartingPosition, station.getBase()->Center()) / (BWEB::Map::getGroundDistance(getMainPosition(), station.getBase()->Center()));

                double score = value / distance;

                if (score > best) {
                    best = score;
                    enemyExpand = (TilePosition)station.getBase()->Center();
                }
            }
        }

        void findCleanupPositions()
        {
            groundCleanupPositions.clear();
            airCleanupPositions.clear();

            // Early on it's just the starting locations
            if (Util::getTime() < Time(5, 00)) {
                for (auto &station : BWEB::Stations::getStations()) {
                    if (station.isMain() && !Stations::isBaseExplored(&station)) {
                        groundCleanupPositions.push_back(station.getBase()->Center());
                        airCleanupPositions.push_back(station.getBase()->Center());
                        Visuals::drawCircle(station.getBase()->Center(), 4, Colors::Teal, true);
                    }
                }
                return;
            }

            // If any base hasn't been visited in past 5 minutes
            for (auto &base : allBases) {
                auto frameDiff = (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(base->Center()));
                if (frameDiff > 7200 || !Broodwar->isExplored(TilePosition(base->Center()))) {
                    groundCleanupPositions.push_back(base->Center());
                    airCleanupPositions.push_back(base->Center());
                }
            }

            // If any reachable tile hasn't been visited in past 5 minutes
            if (groundCleanupPositions.empty()) {
                auto best = 0.0;
                for (int x = 0; x < Broodwar->mapWidth(); x++) {
                    for (int y = 0; y < Broodwar->mapHeight(); y++) {
                        auto t = TilePosition(x, y);
                        auto p = Position(t) + Position(16, 16);

                        if (!Broodwar->isBuildable(t) || !BWEB::Map::isWalkable(t, Protoss_Dragoon))
                            continue;

                        auto frameDiff = (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t));
                        if (frameDiff > 7200 || !Broodwar->isExplored(t)) {
                            groundCleanupPositions.push_back(p);
                            airCleanupPositions.push_back(p);
                        }
                    }
                }
            }
        }

        void updateAreas()
        {
            // Squish areas
            if (getNaturalArea()) {

                // Add to territory if chokes are shared
                if (getMainChoke() == getNaturalChoke() || islandMap)
                    addTerritory(PlayerState::Self, getMyNatural());

                // Add natural if we plan to take it
                if (BuildOrder::isOpener() && BuildOrder::takeNatural())
                    addTerritory(PlayerState::Self, getMyNatural());
            }

            narrowNatural = getNaturalChoke() && getNaturalChoke()->Width() <= 64;
        }
    }

    Position getClosestMapCorner(Position here)
    {
        auto closestCorner = Positions::Invalid;
        auto closestDist = DBL_MAX;
        for (auto &corner : mapCorners) {
            auto dist = corner.getDistance(here);
            if (dist < closestDist) {
                closestDist = dist;
                closestCorner = corner;
            }
        }
        return closestCorner;
    }

    Position getClosestMapEdge(Position here)
    {
        auto closestCorner = Positions::Invalid;
        auto closestDist = DBL_MAX;
        for (auto &e : mapEdges) {
            auto edge = (e.x == -1) ? Position(here.x, e.y) : Position(e.x, here.y);
            auto dist = edge.getDistance(here);
            if (dist < closestDist) {
                closestDist = dist;
                closestCorner = edge;
            }
        }
        return closestCorner;
    }

    Position getOldestPosition(const Area * area)
    {
        auto oldest = DBL_MAX;
        auto oldestTile = TilePositions::Invalid;
        auto start = getMainArea()->TopLeft();
        auto end = getMainArea()->BottomRight();
        auto closestStation = Stations::getClosestStationGround(Position(area->Top()), PlayerState::Self);

        for (int x = start.x; x < end.x; x++) {
            for (int y = start.y; y < end.y; y++) {
                auto t = TilePosition(x, y);
                auto p = Position(t) + Position(16, 16);
                if (!t.isValid()
                    || mapBWEM.GetArea(t) != area
                    || Broodwar->isVisible(t)
                    || (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t) < 480)
                    || !Broodwar->isBuildable(t + TilePosition(-1, 0))
                    || !Broodwar->isBuildable(t + TilePosition(1, 0))
                    || !Broodwar->isBuildable(t + TilePosition(0, -1))
                    || !Broodwar->isBuildable(t + TilePosition(0, 1)))
                    continue;

                auto visible = closestStation ? Grids::getLastVisibleFrame(t) / p.getDistance(closestStation->getBase()->Center()) : Grids::getLastVisibleFrame(t);
                if (visible < oldest) {
                    oldest = visible;
                    oldestTile = t;
                }
            }
        }
        return Position(oldestTile) + Position(16, 16);
    }

    Position getSafeSpot(const BWEB::Station * station) {
        if (station && safeSpots.find(station) != safeSpots.end())
            return safeSpots[station];
        return Positions::Invalid;
    }

    bool inArea(const BWEM::Area * area, Position here)
    {
        if (!here.isValid())
            return false;
        if (mapBWEM.GetArea(TilePosition(here)) == area)
            return true;
        auto geo = areaChokeGeometry.find(WalkPosition(here));
        if (geo != areaChokeGeometry.end()) {
            const auto &areas = geo->second;
            return areas.first == area || areas.second == area;
        }
        return false;
    }

    bool inTerritory(PlayerState playerState, Position here)
    {
        if (!here.isValid())
            return false;

        auto area = mapBWEM.GetArea(TilePosition(here));
        return (area && territoryArea[area] == playerState) || (!area && territoryChokeGeometry[WalkPosition(here)] == playerState);
    }

    bool inTerritory(PlayerState playerState, const BWEM::Area * area)
    {
        return (area && territoryArea[area] == playerState);
    }

    void addTerritory(PlayerState playerState, const BWEB::Station * station)
    {
        // Add the current station area to territory
        if (territoryArea[station->getBase()->GetArea()] == PlayerState::None) {
            territoryArea[station->getBase()->GetArea()] = playerState;

            // Add individual choke tiles to territory
            if (station->getChokepoint()) {
                for (auto &geo : station->getChokepoint()->Geometry()) {
                    if (territoryChokeGeometry[geo] == PlayerState::None)
                        territoryChokeGeometry[geo] = playerState;
                }
            }

            // Add empty areas between main/natural partners to territory
            if (station->isMain() || station->isNatural()) {
                auto closestPartner = station->isMain() ? BWEB::Stations::getClosestNaturalStation(station->getBase()->Location())
                    : BWEB::Stations::getClosestMainStation(station->getBase()->Location());

                if (closestPartner) {
                    for (auto &area : station->getBase()->GetArea()->AccessibleNeighbours()) {

                        if (area->ChokePoints().size() > 2
                            || area == closestPartner->getBase()->GetArea())
                            continue;

                        for (auto &choke : area->ChokePoints()) {
                            if (choke == closestPartner->getChokepoint())
                                territoryArea[area] = playerState;
                        }
                    }
                }
            }
        }
    }

    void removeTerritory(PlayerState playerState, const BWEB::Station * const station)
    {
        // Remove the current station area from territory
        if (territoryArea[station->getBase()->GetArea()] == playerState) {
            territoryArea[station->getBase()->GetArea()] = PlayerState::None;

            // Remove individual choke tiles from territory
            if (station->getChokepoint()) {
                for (auto &geo : station->getChokepoint()->Geometry()) {
                    if (territoryChokeGeometry[geo] == playerState)
                        territoryChokeGeometry[geo] = PlayerState::None;
                }
            }

            // Remove empty areas between main/natural partners from territory
            if (station->isMain() || station->isNatural()) {
                auto closestPartner = station->isMain() ? BWEB::Stations::getClosestNaturalStation(station->getBase()->Location())
                    : BWEB::Stations::getClosestMainStation(station->getBase()->Location());

                if (closestPartner) {
                    for (auto &area : station->getBase()->GetArea()->AccessibleNeighbours()) {

                        if (area->ChokePoints().size() > 2
                            || area == closestPartner->getBase()->GetArea())
                            continue;

                        for (auto &choke : area->ChokePoints()) {
                            if (choke == closestPartner->getChokepoint())
                                territoryArea[area] = PlayerState::None;
                        }
                    }
                }
            }
        }
    }

    void onStart()
    {
        // Initialize BWEM and BWEB
        mapBWEM.Initialize();
        mapBWEM.EnableAutomaticPathAnalysis();
        mapBWEM.FindBasesForStartingLocations();
        BWEB::Map::onStart();
        BWEB::Stations::findStations();

        // Check if the map is an island map
        for (auto &start : mapBWEM.StartingLocations()) {
            if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(getMainTile())))
                islandMap = true;
        }

        // HACK: Play Plasma as an island map
        if (Broodwar->mapFileName().find("Plasma") != string::npos)
            islandMap = true;

        // Store non island bases
        for (auto &area : mapBWEM.Areas()) {
            if (!islandMap && area.AccessibleNeighbours().size() == 0)
                continue;
            for (auto &base : area.Bases())
                allBases.insert(&base);
        }

        // For every area, store their choke geometry as belonging to both areas to prevent nullptr areas
        for (auto &area : mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                for (auto &walk : choke->Geometry())
                    areaChokeGeometry[walk] = choke->GetAreas();
            }
        }
        
        mapEdges ={ {-1, 0}, {-1, Broodwar->mapHeight() * 32}, {0, -1}, {Broodwar->mapWidth() * 32, -1} };
        mapCorners ={ {0, 0}, {0, Broodwar->mapHeight() * 32 }, {Broodwar->mapWidth() * 32, 0}, {Broodwar->mapWidth() * 32, Broodwar->mapHeight() * 32} };

        reverseRamp = Broodwar->getGroundHeight(getMainTile()) < Broodwar->getGroundHeight(getNaturalTile());
        flatRamp = Broodwar->getGroundHeight(getMainTile()) == Broodwar->getGroundHeight(getNaturalTile());
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        findEnemy();
        findEnemyNextExpand();
        findCleanupPositions();
        updateAreas();
        Visuals::endPerfTest("Terrain");
    }

    set<const Base*>& getAllBases() { return allBases; }

    Position getEnemyStartingPosition() { return enemyStartingPosition; }
    const BWEB::Station * getEnemyMain() { return enemyMain; }
    const BWEB::Station * getEnemyNatural() { return enemyNatural; }
    TilePosition getEnemyExpand() { return enemyExpand; }
    TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }

    bool isIslandMap() { return islandMap; }
    bool isReverseRamp() { return reverseRamp; }
    bool isFlatRamp() { return flatRamp; }
    bool isNarrowNatural() { return narrowNatural; }
    bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(TilePosition(enemyStartingPosition)); }
    vector<Position>& getGroundCleanupPositions() { return groundCleanupPositions; }
    vector<Position>& getAirCleanupPositions() { return airCleanupPositions; }

    // Main information
    const BWEB::Station * const getMyMain() { return  BWEB::Stations::getStartingMain(); }
    BWAPI::Position getMainPosition() { return BWEB::Stations::getStartingMain()->getBase()->Center(); }
    BWAPI::TilePosition getMainTile() { return BWEB::Stations::getStartingMain()->getBase()->Location(); }
    const BWEM::Area * getMainArea() { return BWEB::Stations::getStartingMain()->getBase()->GetArea(); }
    const BWEM::ChokePoint * getMainChoke() { return BWEB::Stations::getStartingMain()->getChokepoint(); }

    // Natural information
    const BWEB::Station * const getMyNatural() { return BWEB::Stations::getStartingNatural(); }
    BWAPI::Position getNaturalPosition() { return BWEB::Stations::getStartingNatural()->getBase()->Center(); }
    BWAPI::TilePosition getNaturalTile() { return BWEB::Stations::getStartingNatural()->getBase()->Location(); }
    const BWEM::Area * getNaturalArea() { return BWEB::Stations::getStartingNatural()->getBase()->GetArea(); }
    const BWEM::ChokePoint * getNaturalChoke() { return BWEB::Stations::getStartingNatural()->getChokepoint(); }
}