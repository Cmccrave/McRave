#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Terrain {

    namespace {
        map<const Area*, PlayerState> territoryArea;
        map<WalkPosition, PlayerState> territoryChokeGeometry;
        map<WalkPosition, pair<const BWEM::Area *, const BWEM::Area *>> areaChokeGeometry;
        set<const Base *> allBases;
        BWEB::Station * enemyNatural = nullptr;
        BWEB::Station * enemyMain = nullptr;
        Position enemyStartingPosition = Positions::Invalid;
        Position defendPosition = Positions::Invalid;
        Position attackPosition = Positions::Invalid;
        Position harassPosition = Positions::Invalid;
        TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;
        const ChokePoint * defendChoke = nullptr;
        const Area * defendArea = nullptr;
        map<BWEB::Station *, Position> safeSpots;
        vector<Position> mapEdges, mapCorners;
        int frame6MutasDone = 0;
        vector<Position> groundCleanupPositions;
        vector<Position> airCleanupPositions;

        bool islandMap = false;
        bool reverseRamp = false;
        bool flatRamp = false;
        bool narrowNatural = false;
        bool defendNatural = false;
        int timeHarassingHere = 1500;

        // TODO: This is unused, is it useful?
        bool checkDefendRunby()
        {
            if (Players::ZvP() && com(Zerg_Zergling) >= 16) {
                defendPosition = (Position(getMainChoke()->Center()) + Position(getNaturalChoke()->Center()) + Position(4, 4)) / 2;
                defendNatural = true;
                return true;
            }
            if (Players::ZvT()) {
                defendPosition = Position(getMainChoke()->Center()) + Position(4, 4);
                defendNatural = true;
                return true;
            }
            return false;
        }

        void findEnemy()
        {
            if (enemyStartingPosition.isValid()) {
                if (Broodwar->isExplored(enemyStartingTilePosition) && Util::getTime() < Time(8, 00) && BWEB::Map::isUsed(enemyStartingTilePosition) == UnitTypes::None) {
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

            // Infer based on enemy Overlord
            if (Players::vZ() && Util::getTime() < Time(3, 15)) {
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    UnitInfo &unit = *u;
                    auto frameDiffBest = DBL_MAX;
                    auto tileBest = TilePositions::Invalid;

                    if (unit.getType() != Zerg_Overlord)
                        continue;

                    for (auto &start : Broodwar->getStartLocations()) {
                        auto startCenter = Position(start) + Position(64, 48);
                        auto frameDiff = abs(Broodwar->getFrameCount() - (unit.getPosition().getDistance(startCenter) / unit.getSpeed()));

                        if ((frameDiff < 240 && frameDiff < frameDiffBest) || (frameDiff > 1800 && frameDiff < 2200 && frameDiff < frameDiffBest)) {
                            frameDiffBest = frameDiff;
                            tileBest = start;
                        }
                    }

                    // Set start if valid
                    if (tileBest.isValid() && tileBest != getMainTile()) {
                        enemyStartingPosition = Position(tileBest) + Position(64, 48);
                        enemyStartingTilePosition = tileBest;
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
                    || station == enemyNatural)
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

        void findAttackPosition()
        {
            // Choose a default attack position, in FFA we choose enemy station to us that isn't ours
            auto distBest = Players::vFFA() ? DBL_MAX : 0.0;
            auto posBest = Positions::Invalid;
            if (Players::vFFA()) {
                for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                    auto dist = station->getBase()->Center().getDistance(getMainPosition());
                    if (dist < distBest) {
                        distBest = dist;
                        posBest = station->getBase()->Center();
                    }
                }
            }
            else if (enemyStartingPosition.isValid() && Util::getTime() < Time(6, 00)) {
                attackPosition = enemyStartingPosition;
                return;
            }

            // Attack furthest enemy station from enemy main, closest enemy station to us in FFA
            attackPosition = enemyStartingPosition;
            distBest = Players::vFFA() ? DBL_MAX : 0.0;
            posBest = Positions::Invalid;

            for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                auto dist = enemyStartingPosition.getDistance(station->getBase()->Center());

                if ((!Players::vFFA() && dist < distBest)
                    || (Players::vFFA() && dist > distBest)
                    || BWEB::Map::isUsed(station->getBase()->Location()) == UnitTypes::None)
                    continue;

                const auto stationWalkable = [&](const TilePosition &t) {
                    return Util::rectangleIntersect(Position(station->getBase()->Location()), Position(station->getBase()->Location()) + Position(128, 96), Position(t));
                };

                auto pathFrom = getNaturalChoke() ? Position(getNaturalChoke()->Center()) : getMainPosition();
                BWEB::Path path(pathFrom, station->getBase()->Center(), Zerg_Ultralisk, true, true);
                path.generateJPS([&](auto &t) { return path.unitWalkable(t) || stationWalkable(t); });

                if (path.isReachable()) {
                    distBest = dist;
                    posBest = station->getResourceCentroid();
                }
            }
            attackPosition = posBest;
        }

        void findDefendPosition()
        {
            const auto baseType = Broodwar->self()->getRace().getResourceDepot();
            narrowNatural = getNaturalChoke() && getNaturalChoke()->Width() <= 64;
            defendNatural = getNaturalChoke() &&
                (BuildOrder::isWallNat()
                    || BuildOrder::takeNatural()
                    || BuildOrder::buildCount(baseType) > (1 + !BuildOrder::takeNatural())
                    || Stations::getStations(PlayerState::Self).size() >= 2
                    || (!Players::PvZ() && Players::getSupply(PlayerState::Self, Races::Protoss) > 140)
                    || (Broodwar->self()->getRace() != Races::Zerg && reverseRamp));

            auto oldPos = defendPosition;
            auto mainChoke = getMainChoke();

            // Natural defending position
            if (defendNatural && getMyNatural()) {
                defendChoke = getNaturalChoke();
                defendPosition = Stations::getDefendPosition(getMyNatural());
                addTerritory(PlayerState::Self, getMyNatural());
            }

            // Main defending position
            else if (mainChoke) {
                defendNatural = false;
                addTerritory(PlayerState::Self, getMyMain());
                removeTerritory(PlayerState::Self, getMyNatural()); // Erase just in case we dropped natural defense
                defendPosition = Position(mainChoke->Center()) + Position(4, 4);

                // Check to see if we have a wall
                if (Walls::getMainWall() && BuildOrder::isWallMain()) {
                    Position opening(Walls::getMainWall()->getOpening());
                    defendPosition = opening.isValid() ? opening : Walls::getMainWall()->getCentroid();
                }
            }

            // Natural defending area and choke
            if (defendNatural) {
                defendArea = getNaturalArea();
                defendChoke = getNaturalChoke();
                Zones::addZone(Position(getMainChoke()->Center()), ZoneType::Defend, 1, 160);
            }

            // Main defend area and choke
            else if (mainChoke) {
                defendChoke = mainChoke;
                defendArea = getMainArea();
            }

            Broodwar->drawTriangleMap(defendPosition + Position(0, -20), defendPosition + Position(-20, 10), defendPosition + Position(20, 10), Colors::Green);
        }

        void findHarassPosition()
        {
            auto oldHarass = harassPosition;
            if (!enemyMain)
                return;
            harassPosition = Positions::Invalid;

            if (com(Zerg_Mutalisk) >= 6 && frame6MutasDone == 0)
                frame6MutasDone = Broodwar->getFrameCount();

            const auto commanderInRange = [&](Position here) {
                auto commander = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                    return u->getType() == Zerg_Mutalisk;
                });
                return commander && commander->getPosition().getDistance(here) < 160.0;
            };
            const auto stationNotVisitedRecently = [&](auto &station) {
                return max(frame6MutasDone, Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(station->getResourceCentroid()))) > 2880 && !commanderInRange(station->getResourceCentroid());
            };

            // In FFA just hit closest base to us
            if (Players::vFFA() && attackPosition.isValid()) {
                harassPosition = attackPosition;
                return;
            }

            // Check if enemy lost all bases
            auto lostAll = !Stations::getStations(PlayerState::Enemy).empty();
            for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                if (!Stations::isBaseExplored(station) || BWEB::Map::isUsed(station->getBase()->Location()) != UnitTypes::None)
                    lostAll = false;
            }
            if (lostAll) {
                harassPosition = Positions::Invalid;
                return;
            }

            // Some hardcoded ZvT stuff
            if (Players::ZvT() && Util::getTime() < Time(8, 00) && enemyMain) {
                harassPosition = enemyMain->getResourceCentroid();
                return;
            }

            // Create a list of valid positions to harass/check
            vector<BWEB::Station*> stations = Stations::getStations(PlayerState::Enemy);
            if (Util::getTime() < Time(10, 00)) {
                stations.push_back(Terrain::getEnemyMain());
                stations.push_back(Terrain::getEnemyNatural());
            }

            // At a certain point we need to ensure they aren't mass expanding - check closest 2 if not visible in last 2 minutes
            auto checkNeeded = (Stations::getStations(PlayerState::Enemy).size() <= 2 && Util::getTime() > Time(10, 00)) || (Stations::getStations(PlayerState::Enemy).size() <= 3 && Util::getTime() > Time(15, 00));
            if (checkNeeded && Terrain::getEnemyNatural()) {
                auto station1 = Stations::getClosestStationGround(Terrain::getEnemyNatural()->getBase()->Center(), PlayerState::None, [&](auto &s) {
                    return s != Terrain::getEnemyMain() && s != Terrain::getEnemyNatural() && !s->getBase()->Geysers().empty();
                });
                if (station1) {
                    stations.push_back(station1);

                    auto station2 = Stations::getClosestStationGround(Terrain::getEnemyNatural()->getBase()->Center(), PlayerState::None, [&](auto &s) {
                        return s != station1 && s != Terrain::getEnemyMain() && s != Terrain::getEnemyNatural() && !s->getBase()->Geysers().empty();
                    });

                    if (station2)
                        stations.push_back(station2);
                }
            }

            // Harass all stations by last visited
            auto best = -1.0;
            for (auto &station : stations) {
                auto defCount = double(Stations::getAirDefenseCount(station));
                auto score = double(Broodwar->getFrameCount() - Grids::getLastVisibleFrame(TilePosition(station->getResourceCentroid()))) / exp(1.0 + defCount);
                if (score > best) {
                    best = score;
                    harassPosition = station->getResourceCentroid();
                }
            }
        }

        void findCleanupPositions()
        {
            groundCleanupPositions.clear();
            airCleanupPositions.clear();

            // Look at every TilePosition and sort by furthest oldest
            auto best = 0.0;
            for (int x = 0; x < Broodwar->mapWidth(); x++) {
                for (int y = 0; y < Broodwar->mapHeight(); y++) {
                    auto t = TilePosition(x, y);
                    auto p = Position(t) + Position(16, 16);

                    if (!Broodwar->isBuildable(t))
                        continue;

                    auto frameDiff = (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t));
                    if (frameDiff > 2880) {
                        groundCleanupPositions.push_back(p);
                        airCleanupPositions.push_back(p);
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

            // For every area, store their choke geometry as belonging to both areas to prevent nullptr areas
            for (auto &area : mapBWEM.Areas()) {
                for (auto &choke : area.ChokePoints()) {
                    for (auto &walk : choke->Geometry())
                        areaChokeGeometry[walk] = choke->GetAreas();                    
                }
            }
        }
    }

    Position getClosestMapCorner(Position here)
    {
        vector<Position> mapCorners ={
    {0, 0},
    {0, Broodwar->mapHeight() * 32 },
    {Broodwar->mapWidth() * 32, 0},
    {Broodwar->mapWidth() * 32, Broodwar->mapHeight() * 32}
        };

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
        vector<Position> mapEdges ={
    {here.x, 0},
    {here.x, Broodwar->mapHeight() * 32 },
    {0, here.y},
    {Broodwar->mapWidth() * 32, here.y}
        };

        auto closestCorner = Positions::Invalid;
        auto closestDist = DBL_MAX;
        for (auto &corner : mapEdges) {
            auto dist = corner.getDistance(here);
            if (dist < closestDist) {
                closestDist = dist;
                closestCorner = corner;
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

    Position getSafeSpot(BWEB::Station * station) {
        if (station && safeSpots.find(station) != safeSpots.end())
            return safeSpots[station];
        return Positions::Invalid;
    }

    bool inArea(const BWEM::Area * area, Position here)
    {
        if (!here.isValid())
            return false;
        return mapBWEM.GetArea(TilePosition(here)) == area || areaChokeGeometry.find(WalkPosition(here)) != areaChokeGeometry.end();
    }

    bool inTerritory(PlayerState playerState, Position here)
    {
        if (!here.isValid())
            return false;

        auto area = mapBWEM.GetArea(TilePosition(here));
        return (area && territoryArea[area] == playerState) || (!area && territoryChokeGeometry[WalkPosition(here)] == playerState);
    }

    bool inTerritory(PlayerState playerState, const BWEM::Area* area)
    {
        return (area && territoryArea[area] == playerState);
    }

    void addTerritory(PlayerState playerState, BWEB::Station* station)
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

    void removeTerritory(PlayerState playerState, BWEB::Station* station)
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

        reverseRamp = Broodwar->getGroundHeight(getMainTile()) < Broodwar->getGroundHeight(getNaturalTile());
        flatRamp = Broodwar->getGroundHeight(getMainTile()) == Broodwar->getGroundHeight(getNaturalTile());
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        findEnemy();
        findEnemyNextExpand();
        findAttackPosition();
        findDefendPosition();
        findHarassPosition();
        findCleanupPositions();
        updateAreas();
        Visuals::endPerfTest("Terrain");
    }

    set<const Base*>& getAllBases() { return allBases; }
    Position getAttackPosition() { return attackPosition; }
    Position getDefendPosition() { return defendPosition; }
    Position getHarassPosition() { return harassPosition; }
    Position getEnemyStartingPosition() { return enemyStartingPosition; }
    BWEB::Station * getEnemyMain() { return enemyMain; }
    BWEB::Station * getEnemyNatural() { return enemyNatural; }
    TilePosition getEnemyExpand() { return enemyExpand; }
    TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
    const ChokePoint * getDefendChoke() { return defendChoke; }
    const Area * getDefendArea() { return defendArea; }
    BWEB::Station * getDefendStation() { return defendNatural ? getMyNatural() : getMyMain(); }
    bool isIslandMap() { return islandMap; }
    bool isReverseRamp() { return reverseRamp; }
    bool isFlatRamp() { return flatRamp; }
    bool isNarrowNatural() { return narrowNatural; }
    bool isDefendNatural() { return defendNatural; }
    bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(TilePosition(enemyStartingPosition)); }
    vector<Position>& getGroundCleanupPositions() { return groundCleanupPositions; }
    vector<Position>& getAirCleanupPositions() { return airCleanupPositions; }

    // Main information
    BWEB::Station * getMyMain() { return  BWEB::Stations::getStartingMain(); }
    BWAPI::Position getMainPosition() { return BWEB::Stations::getStartingMain()->getBase()->Center(); }
    BWAPI::TilePosition getMainTile() { return BWEB::Stations::getStartingMain()->getBase()->Location(); }
    const BWEM::Area * getMainArea() { return BWEB::Stations::getStartingMain()->getBase()->GetArea(); }
    const BWEM::ChokePoint * getMainChoke() { return BWEB::Stations::getStartingMain()->getChokepoint(); }

    // Natural information
    BWEB::Station * getMyNatural() { return BWEB::Stations::getStartingNatural(); }
    BWAPI::Position getNaturalPosition() { return BWEB::Stations::getStartingNatural()->getBase()->Center(); }
    BWAPI::TilePosition getNaturalTile() { return BWEB::Stations::getStartingNatural()->getBase()->Location(); }
    const BWEM::Area * getNaturalArea() { return BWEB::Stations::getStartingNatural()->getBase()->GetArea(); }
    const BWEM::ChokePoint * getNaturalChoke() { return BWEB::Stations::getStartingNatural()->getChokepoint(); }
}