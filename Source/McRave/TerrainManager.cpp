#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Terrain {

    namespace {
        set<const Area*> allyTerritory;
        set<const Area*> enemyTerritory;
        set<const Base *> allBases;
        BWEB::Station * enemyNatural = nullptr;
        BWEB::Station * enemyMain = nullptr;
        BWEB::Station * myNatural = nullptr;
        BWEB::Station * myMain = nullptr;
        Position enemyStartingPosition = Positions::Invalid;
        Position defendPosition = Positions::Invalid;
        Position attackPosition = Positions::Invalid;
        Position harassPosition = Positions::Invalid;
        TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;
        const ChokePoint * defendChoke = nullptr;
        const Area * defendArea = nullptr;

        bool shitMap = false;
        bool islandMap = false;
        bool reverseRamp = false;
        bool flatRamp = false;
        bool narrowNatural = false;
        bool defendNatural = false;
        bool abandonNatural = false;
        int timeHarassingHere = 0;

        void findEnemy()
        {
            if (enemyStartingPosition.isValid()) {
                if (Util::getTime() < Time(10, 00) && Broodwar->isExplored(enemyStartingTilePosition) && BWEB::Map::isUsed(enemyStartingTilePosition) == UnitTypes::None) {
                    enemyStartingPosition = Positions::Invalid;
                    enemyStartingTilePosition = TilePositions::Invalid;
                    enemyNatural = nullptr;
                    enemyMain = nullptr;
                }
                else
                    return;                
            }

            // Find closest enemy building
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;
                auto distBest = 640.0;
                auto tileBest = TilePositions::Invalid;
                if (!unit.getType().isBuilding() || !unit.getTilePosition().isValid())
                    continue;

                // Find closest starting location
                for (auto &start : Broodwar->getStartLocations()) {
                    auto center = Position(start) + Position(64, 48);
                    auto dist = center.getDistance(unit.getPosition());
                    if (dist < distBest) {
                        distBest = dist;
                        tileBest = start;
                    }
                }

                // Set start if valid
                if (tileBest.isValid() && tileBest != BWEB::Map::getMainTile()) {
                    enemyStartingPosition = Position(tileBest) + Position(64, 48);
                    enemyStartingTilePosition = tileBest;
                }
            }

            // Infer based on enemy Overlord
            if (Players::vZ() && Util::getTime() < Time(2, 15)) {
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    UnitInfo &unit = *u;
                    auto frameDiffBest = DBL_MAX;
                    auto tileBest = TilePositions::Invalid;

                    if (unit.getType() != Zerg_Overlord)
                        continue;

                    for (auto &start : Broodwar->getStartLocations()) {
                        auto startCenter = Position(start) + Position(64, 48);
                        auto frameDiff = abs(Broodwar->getFrameCount() - (unit.getPosition().getDistance(startCenter) / unit.getSpeed()));

                        if (frameDiff < 240 && frameDiff < frameDiffBest) {
                            frameDiffBest = frameDiff;
                            tileBest = start;
                        }
                    }

                    // Set start if valid
                    if (tileBest.isValid() && tileBest != BWEB::Map::getMainTile()) {
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

                if (unexploredStarts.size() == 1) {
                    enemyStartingPosition = Position(unexploredStarts.front()) + Position(64, 48);
                    enemyStartingTilePosition = unexploredStarts.front();
                }
            }

            // Locate Stations
            if (enemyStartingPosition.isValid()) {
                enemyNatural = BWEB::Stations::getClosestNaturalStation(enemyStartingTilePosition);
                enemyMain = BWEB::Stations::getClosestMainStation(enemyStartingTilePosition);
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
                    || !station.getBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea())
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
                if (!station.getBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()))
                    distance = log(station.getBase()->Center().getDistance(enemyStartingPosition));
                else
                    distance = BWEB::Map::getGroundDistance(enemyStartingPosition, station.getBase()->Center()) / (BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), station.getBase()->Center()));

                double score = value / distance;

                if (score > best) {
                    best = score;
                    enemyExpand = (TilePosition)station.getBase()->Center();
                }
            }
        }

        void findAttackPosition()
        {
            // Attack furthest enemy station
            if (Stations::getEnemyStations().size() >= 3) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;

                for (auto &station : Stations::getEnemyStations()) {
                    auto &s = *station.second;
                    const auto dist = enemyStartingPosition.getDistance(s.getBase()->Center());
                    if (dist >= distBest) {
                        distBest = dist;
                        posBest = s.getResourceCentroid();
                    }
                }
                attackPosition = posBest;
            }

            // Attack enemy main
            else if (enemyStartingPosition.isValid())
                attackPosition = enemyStartingPosition;
            else
                attackPosition = Positions::Invalid;
        }

        void findDefendPosition()
        {
            const auto baseType = Broodwar->self()->getRace().getResourceDepot();
            narrowNatural = BWEB::Map::getNaturalChoke() && BWEB::Map::getNaturalChoke()->Width() <= 64;
            defendNatural = BWEB::Map::getNaturalChoke() && !abandonNatural && (BuildOrder::isWallNat() || BuildOrder::buildCount(baseType) > (1 + !BuildOrder::takeNatural()) || Stations::getMyStations().size() >= 2 || (!Players::PvZ() && Players::getSupply(PlayerState::Self) > 140));

            auto oldPos = defendPosition;

            auto mainChoke = BWEB::Map::getMainChoke();
            if (shitMap && enemyStartingPosition.isValid())
                mainChoke = mapBWEM.GetPath(BWEB::Map::getMainPosition(), enemyStartingPosition).front();

            // If we want to defend our mineral line
            if (!Strategy::defendChoke() || islandMap) {
                defendNatural = false;
                auto closestStation = BWEB::Stations::getClosestMainStation(BWEB::Map::getMainTile());
                auto closestUnit = Util::getClosestUnit(closestStation->getBase()->Center(), PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Combat;
                });

                if (closestUnit) {
                    auto distBest = DBL_MAX;
                    for (auto &defense : closestStation->getDefenseLocations()) {
                        auto center = Position(defense) + Position(32, 32);
                        if (Buildings::overlapsPlan(*closestUnit, center))
                            continue;
                        auto dist = center.getDistance(closestStation->getResourceCentroid());
                        if (dist < distBest) {
                            defendPosition = center;
                            distBest = dist;
                        }
                    }
                }
                else
                    defendPosition = closestStation ? (closestStation->getResourceCentroid() + closestStation->getBase()->Center()) / 2 : BWEB::Map::getMainPosition();
            }

            // If we want to prevent a runby
            else if (Strategy::defendChoke() && (Players::ZvT() || (Players::ZvP() && Util::getTime() < Time(8, 00)))) {
                defendPosition = Position(mainChoke->Center());
                defendNatural = false;
            }

            // Natural defending position
            else if (defendNatural) {
                allyTerritory.insert(BWEB::Map::getNaturalArea());
                defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());

                // Check to see if we have a wall
                if (Walls::getNaturalWall() && BuildOrder::isWallNat()) {
                    Position opening(Walls::getNaturalWall()->getOpening());
                    defendPosition = opening.isValid() ? opening : Walls::getNaturalWall()->getCentroid();
                }

                // If there are multiple chokepoints
                else if (BWEB::Map::getNaturalArea()->ChokePoints().size() >= 3) {
                    defendPosition = Position(0, 0);
                    int count = 0;
                    for (auto &choke : BWEB::Map::getNaturalArea()->ChokePoints()) {
                        if (BWEB::Map::getGroundDistance(Position(choke->Center()), mapBWEM.Center()) < BWEB::Map::getGroundDistance(BWEB::Map::getNaturalPosition(), mapBWEM.Center())) {
                            defendPosition += Position(choke->Center());
                            count++;
                        }
                    }
                    if (count > 0)
                        defendPosition /= count;
                    else
                        defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());
                }
            }

            // Main defending position
            else {
                allyTerritory.insert(BWEB::Map::getMainArea());
                allyTerritory.erase(BWEB::Map::getNaturalArea()); // Erase just in case we dropped natural defense
                defendPosition = Position(mainChoke->Center());

                // Check to see if we have a wall
                if (Walls::getMainWall() && BuildOrder::isWallMain()) {
                    Position opening(Walls::getMainWall()->getOpening());
                    defendPosition = opening.isValid() ? opening : Walls::getMainWall()->getCentroid();
                }
            }

            // Natural defending area and choke
            if (defendNatural) {

                // Decide which area is within my territory, useful for maps with small adjoining areas like Andromeda
                auto &[a1, a2] = defendChoke->GetAreas();
                if (a1 && Terrain::isInAllyTerritory(a1))
                    defendArea = a1;
                if (a2 && Terrain::isInAllyTerritory(a2))
                    defendArea = a2;
                defendChoke = BWEB::Map::getNaturalChoke();
            }

            // Main defend area and choke
            else {
                defendChoke = mainChoke;
                defendArea = BWEB::Map::getMainArea();
            }

            if (defendPosition != oldPos)
                Combat::resetDefendPositionCache();
        }

        void findHarassPosition()
        {
            auto best = 0.0;

            if (Players::ZvT() && enemyMain) {
                harassPosition = enemyMain->getBase()->Center();
                return;
            }

            // Harass all stations by last visited
            if (Stations::getEnemyStations().size() >= 3) {
                for (auto &[_, station] : Stations::getEnemyStations()) {
                    auto score = Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(station->getBase()->Center()));
                    if (score > best) {
                        score = best;
                        harassPosition = station->getBase()->Center();
                    }
                }
            }

            // Check enemy main and natural
            else if (enemyMain && enemyNatural) {
                auto mainScore = Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(enemyMain->getBase()->Center()));
                auto natScore = Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(enemyNatural->getBase()->Center()));

                auto switchPosition = timeHarassingHere > 500
                    || (mainScore == 0 && harassPosition == enemyMain->getBase()->Center() && BWEB::Map::isUsed(enemyNatural->getBase()->Location()) != UnitTypes::None)
                    || (natScore == 0 && harassPosition == enemyNatural->getBase()->Center() && BWEB::Map::isUsed(enemyMain->getBase()->Location()) != UnitTypes::None);

                if (switchPosition && harassPosition != enemyMain->getBase()->Center()) {
                    harassPosition = enemyMain->getBase()->Center();
                    best = mainScore;
                }
                else if (switchPosition && harassPosition != enemyNatural->getBase()->Center()) {
                    harassPosition = enemyNatural->getBase()->Center();
                    best = natScore;
                }

                if (switchPosition)
                    timeHarassingHere = 0;
                else
                    timeHarassingHere++;
            }
        }

        void updateAreas()
        {
            // Squish areas
            if (BWEB::Map::getNaturalArea()) {

                // Add to territory if chokes are shared
                if (BWEB::Map::getMainChoke() == BWEB::Map::getNaturalChoke() || islandMap)
                    allyTerritory.insert(BWEB::Map::getNaturalArea());

                // Abandon natural when not needed - DISABLED: not sure what strategy will ever need this
                if (Strategy::getEnemyBuild() == "2Gate" && Strategy::enemyProxy()) {
                    if (isInAllyTerritory(BWEB::Map::getNaturalArea()) && Strategy::enemyProxy())
                        abandonNatural = true;
                    if (abandonNatural && !Strategy::enemyProxy())
                        abandonNatural = false;
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

    bool isInAllyTerritory(TilePosition here)
    {
        if (here.isValid() && mapBWEM.GetArea(here))
            return isInAllyTerritory(mapBWEM.GetArea(here));
        return false;
    }

    bool isInAllyTerritory(const Area * area)
    {
        if (allyTerritory.find(area) != allyTerritory.end())
            return true;
        return false;
    }

    bool isInEnemyTerritory(TilePosition here)
    {
        if (here.isValid() && mapBWEM.GetArea(here) && enemyTerritory.find(mapBWEM.GetArea(here)) != enemyTerritory.end())
            return true;
        return false;
    }

    bool isInEnemyTerritory(const Area * area)
    {
        if (enemyTerritory.find(area) != enemyTerritory.end())
            return true;
        return false;
    }

    bool isStartingBase(TilePosition here)
    {
        for (auto tile : Broodwar->getStartLocations()) {
            if (here.getDistance(tile) < 4)
                return true;
        }
        return false;
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
            if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(BWEB::Map::getMainTile())))
                islandMap = true;
        }

        // HACK: Play Plasma as an island map
        if (Broodwar->mapFileName().find("Plasma") != string::npos)
            islandMap = true;

        // HACK: Alchemist is a shit map (no seriously, if you're reading this you have no idea)
        if (Broodwar->mapFileName().find("Alchemist") != string::npos)
            shitMap = true;

        // Store non island bases
        for (auto &area : mapBWEM.Areas()) {
            if (!islandMap && area.AccessibleNeighbours().size() == 0)
                continue;
            for (auto &base : area.Bases())
                allBases.insert(&base);
        }

        // Add "empty" areas (ie. Andromeda areas around main)
        for (auto &area : BWEB::Map::getNaturalArea()->AccessibleNeighbours()) {

            if (area->ChokePoints().size() > 2 || shitMap)
                continue;

            for (auto &choke : area->ChokePoints()) {
                if (choke->Center() == BWEB::Map::getMainChoke()->Center()) {
                    allyTerritory.insert(area);
                }
            }
        }

        reverseRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) < Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
        flatRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) == Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
        myMain = BWEB::Stations::getClosestMainStation(BWEB::Map::getMainTile());
        myNatural = BWEB::Stations::getClosestNaturalStation(BWEB::Map::getNaturalTile());
    }

    void onFrame()
    {
        findEnemy();

        findEnemyNextExpand();
        findAttackPosition();
        findDefendPosition();
        findHarassPosition();

        updateAreas();
    }

    set<const Area*>& getAllyTerritory() { return allyTerritory; }
    set<const Area*>& getEnemyTerritory() { return enemyTerritory; }
    set<const Base*>& getAllBases() { return allBases; }
    Position getAttackPosition() { return attackPosition; }
    Position getDefendPosition() { return defendPosition; }
    Position getHarassPosition() { return harassPosition; }
    Position getEnemyStartingPosition() { return enemyStartingPosition; }
    BWEB::Station * getEnemyMain() { return enemyMain; }
    BWEB::Station * getEnemyNatural() { return enemyNatural; }
    BWEB::Station * getMyMain() { return myMain; }
    BWEB::Station * getMyNatural() { return myNatural; }
    TilePosition getEnemyExpand() { return enemyExpand; }
    TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
    const ChokePoint * getDefendChoke() { return defendChoke; }
    const Area * getDefendArea() { return defendArea; }
    bool isShitMap() { return shitMap; }
    bool isIslandMap() { return islandMap; }
    bool isReverseRamp() { return reverseRamp; }
    bool isFlatRamp() { return flatRamp; }
    bool isNarrowNatural() { return narrowNatural; }
    bool isDefendNatural() { return defendNatural; }
    bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(TilePosition(enemyStartingPosition)); }
}