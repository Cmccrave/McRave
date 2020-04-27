#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Terrain {

    namespace {
        set <const Area*> allyTerritory;
        set <const Area*> enemyTerritory;
        Position enemyStartingPosition = Positions::Invalid;
        TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        Position mineralHold = Positions::Invalid;
        Position defendPosition = Positions::Invalid;
        Position attackPosition = Positions::Invalid;
        Position harassPosition = Positions::Invalid;
        TilePosition enemyNatural = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;
        set<const Base *> allBases;

        bool shitMap = false;
        bool islandMap = false;
        bool reverseRamp = false;
        bool flatRamp = false;
        bool narrowNatural = false;
        bool defendNatural = false;
        bool abandonNatural = false;

        void findEnemyStartingPosition()
        {
            if (enemyStartingPosition.isValid())
                return;

            // Find closest enemy building
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;
                auto distBest = 1280.0;
                auto tileBest = TilePositions::Invalid;
                if (!unit.getType().isBuilding() || !unit.getTilePosition().isValid())
                    continue;

                // Find closest starting location
                for (auto &start : Broodwar->getStartLocations()) {
                    double dist = start.getDistance(unit.getTilePosition());
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
            if (Players::vZ() && Util::getTime() < Time(3, 00)) {
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

            // Assume
            vector<TilePosition> unexploredStarts;
            if (!enemyStartingPosition.isValid()) {
                for (auto &topLeft : mapBWEM.StartingLocations()) {
                    auto botRight = topLeft + TilePosition(3, 2);
                    if (!Broodwar->isExplored(botRight) && !Broodwar->isExplored(topLeft))
                        unexploredStarts.push_back(botRight);
                }
                if (unexploredStarts.size() == 1) {
                    enemyStartingPosition = Position(unexploredStarts.front()) + Position(64, 48);
                    enemyStartingTilePosition = unexploredStarts.front();
                }
            }
        }

        void findEnemyNatural()
        {
            if (enemyNatural.isValid())
                return;

            // Find enemy natural area
            double distBest = DBL_MAX;
            for (auto &area : mapBWEM.Areas()) {
                for (auto &base : area.Bases()) {

                    if (base.Geysers().size() == 0
                        || area.AccessibleNeighbours().size() == 0
                        || base.Center().getDistance(enemyStartingPosition) < 128)
                        continue;

                    double dist = BWEB::Map::getGroundDistance(base.Center(), enemyStartingPosition);
                    if (dist < distBest) {
                        distBest = dist;
                        enemyNatural = base.Location();
                    }
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
                if (BWEB::Map::isUsed(station.getBWEMBase()->Location()) != None
                    || enemyStartingTilePosition == station.getBWEMBase()->Location()
                    || !station.getBWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea())
                    || station.getBWEMBase()->Location() == enemyNatural)
                    continue;

                // Get value of the expansion
                double value = 0.0;
                for (auto &mineral : station.getBWEMBase()->Minerals())
                    value += double(mineral->Amount());
                for (auto &gas : station.getBWEMBase()->Geysers())
                    value += double(gas->Amount());
                if (station.getBWEMBase()->Geysers().size() == 0)
                    value = value / 10.0;

                // Get distance of the expansion
                double distance;
                if (!station.getBWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()))
                    distance = log(station.getBWEMBase()->Center().getDistance(enemyStartingPosition));
                else
                    distance = BWEB::Map::getGroundDistance(enemyStartingPosition, station.getBWEMBase()->Center()) / (BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), station.getBWEMBase()->Center()));

                double score = value / distance;

                if (score > best) {
                    best = score;
                    enemyExpand = (TilePosition)station.getBWEMBase()->Center();
                }
            }
        }

        void findAttackPosition()
        {
            // Attack furthest enemy station
            if (Stations::getEnemyStations().size() > 0) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;

                for (auto &station : Stations::getEnemyStations()) {
                    auto &s = *station.second;
                    const auto dist = enemyStartingPosition.getDistance(s.getBWEMBase()->Center());
                    if (dist >= distBest) {
                        distBest = dist;
                        posBest = s.getResourceCentroid();
                    }
                }
                attackPosition = posBest;
            }

            // Attack enemy main
            else if (enemyStartingPosition.isValid() && !Broodwar->isExplored(enemyStartingTilePosition))
                attackPosition = enemyStartingPosition;
            else
                attackPosition = Positions::Invalid;

            Broodwar->drawCircleMap(attackPosition, 3, Colors::Red, true);
        }

        void findDefendPosition()
        {
            UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
            Position oldDefendPosition = defendPosition;
            narrowNatural = BWEB::Map::getNaturalChoke() ? int(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1).getDistance(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2)) / 4) <= 2 : false;
            defendNatural = BWEB::Map::getNaturalChoke() ? BuildOrder::isWallNat() || BuildOrder::buildCount(baseType) > 1 || vis(baseType) > 1 || defendPosition == Position(BWEB::Map::getNaturalChoke()->Center()) || (/*reverseRamp && */!Players::vZ() && Players::getSupply(PlayerState::Self) > 140) : false;

            if (islandMap) {
                defendPosition = BWEB::Map::getMainPosition();
                return;
            }

            // Set mineral holding positions
            double distBest = DBL_MAX;
            for (auto &station : Stations::getMyStations()) {
                auto &s = *station.second;
                double dist;
                if (enemyStartingPosition.isValid())
                    dist = BWEB::Map::getGroundDistance(enemyStartingPosition, Position(s.getResourceCentroid()));
                else
                    dist = mapBWEM.Center().getDistance(Position(s.getResourceCentroid()));

                if (dist < distBest) {
                    distBest = dist;
                    mineralHold = s.getResourceCentroid();
                }
            }

            // If we want to defend our mineral line until we can stabilize
            if (!Strategy::defendChoke()) {
                defendPosition = mineralHold;
                defendNatural = false;
            }

            // Natural defending
            else if (defendNatural) {
                defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());
                allyTerritory.insert(BWEB::Map::getNaturalArea());

                // If there are multiple chokepoints
                if (BWEB::Map::getNaturalArea()->ChokePoints().size() >= 3) {
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

                // Check to see if we have a wall
                else if (Walls::getNaturalWall() && BuildOrder::isWallNat()) {
                    Position opening(Walls::getNaturalWall()->getOpening());
                    defendPosition = opening.isValid() ? opening : Walls::getNaturalWall()->getCentroid();
                }

                // Move the defend position on maps like destination
                while (!Broodwar->isBuildable((TilePosition)defendPosition)) {
                    double distBest = DBL_MAX;
                    TilePosition test = (TilePosition)defendPosition;
                    for (int x = test.x - 1; x <= test.x + 1; x++) {
                        for (int y = test.y - 1; y <= test.y + 1; y++) {
                            TilePosition t(x, y);
                            const Position p = Position(t) + Position(16, 16);
                            if (!t.isValid())
                                continue;

                            double dist = p.getDistance(BWEB::Map::getNaturalPosition());

                            if (dist < distBest) {
                                distBest = dist;
                                defendPosition = p;
                            }
                        }
                    }
                }
            }

            // Main defending
            else if (Walls::getMainWall() && BuildOrder::isWallMain()) {
                Position opening(Walls::getMainWall()->getOpening());
                defendPosition = opening.isValid() ? opening : Walls::getMainWall()->getCentroid();
            }
            else {
                if (BWEB::Map::getMainArea() && shitMap) {
                    if (enemyStartingPosition.isValid()) {
                        for (auto &choke : mapBWEM.GetPath(BWEB::Map::getMainPosition(), enemyStartingPosition)) {
                            defendPosition = Position(choke->Center());
                            break;
                        }
                    }
                }
                else {
                    defendPosition = Position(BWEB::Map::getMainChoke()->Center());
                }
            }

            // If we aren't defending the natural, remove the area in case we added it
            if (!defendNatural)
                allyTerritory.erase(BWEB::Map::getNaturalArea());
        }

        void findHarassPosition()
        {
            auto best = INT_MAX;
            for (auto &station : Stations::getEnemyStations()) {
                auto tile = station.second->getBWEMBase()->Location();
                auto current = Grids::lastVisibleFrame(tile);
                if (current < best) {
                    best = current;
                    harassPosition = Position(tile);
                }
            }

            // Check enemy start and expand as well
            if (Grids::lastVisibleFrame(enemyStartingTilePosition) < best) {
                harassPosition = enemyStartingPosition;
                best = Grids::lastVisibleFrame(enemyStartingTilePosition);
            }
            if (Grids::lastVisibleFrame(enemyNatural) < best) {
                harassPosition = Position(enemyNatural);
                best = Grids::lastVisibleFrame(enemyNatural);
            }
        }

        void updateAreas()
        {
            // Squish areas
            if (BWEB::Map::getNaturalArea()) {

                UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
                if ((BuildOrder::buildCount(baseType) >= 2 || BuildOrder::isWallNat()) && BWEB::Map::getNaturalArea())
                    allyTerritory.insert(BWEB::Map::getNaturalArea());

                // HACK: Add to my territory if chokes are shared
                if (BWEB::Map::getMainChoke() == BWEB::Map::getNaturalChoke() || islandMap)
                    allyTerritory.insert(BWEB::Map::getNaturalArea());
            }

            // Abandon natural when not needed
            if (isInAllyTerritory(BWEB::Map::getNaturalArea()) && Strategy::enemyRush()) {
                abandonNatural = true;
                allyTerritory.erase(BWEB::Map::getNaturalArea());
            }
            if (abandonNatural && !Strategy::enemyRush()) {
                abandonNatural = false;
                allyTerritory.insert(BWEB::Map::getNaturalArea());
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
            if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(BWEB::Map::getMainTile())))
                islandMap = true;
        }

        // HACK: Play Plasma as an island map
        if (Broodwar->mapFileName().find("Plasma") != string::npos)
            islandMap = true;

        // HACK:: Alchemist is a shit map
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
    }

    void onFrame()
    {
        findEnemyStartingPosition();
        findEnemyNatural();
        findEnemyNextExpand();
        findAttackPosition();
        findDefendPosition();
        findHarassPosition();

        updateAreas();
    }

    bool isInAllyTerritory(TilePosition here)
    {
        if (here.isValid() && mapBWEM.GetArea(here))
            return isInAllyTerritory(mapBWEM.GetArea(here));
        return false;
    }

    bool isInAllyTerritory(const BWEM::Area * area)
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

    bool isInEnemyTerritory(const BWEM::Area * area)
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

    Position closestUnexploredStart() {
        double distBest = DBL_MAX;
        Position posBest = Positions::None;
        for (auto &tile : mapBWEM.StartingLocations()) {
            Position center = Position(tile) + Position(64, 48);
            double dist = center.getDistance(BWEB::Map::getMainPosition());

            if (!Broodwar->isExplored(tile) && dist < distBest) {
                distBest = dist;
                posBest = center;
            }
        }
        return posBest;
    }

    Position randomBasePosition()
    {
        int random = rand() % (allBases.size());
        int i = 0;
        for (auto &base : allBases) {
            if (i == random)
                return base->Center();
            else
                i++;
        }
        return BWEB::Map::getMainPosition();
    }

    bool isShitMap() { return shitMap; }
    bool isIslandMap() { return islandMap; }
    bool isReverseRamp() { return reverseRamp; }
    bool isFlatRamp() { return flatRamp; }
    bool isNarrowNatural() { return narrowNatural; }
    bool isDefendNatural() { return defendNatural; }
    bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(BWAPI::TilePosition(enemyStartingPosition)); }
    Position getAttackPosition() { return attackPosition; }
    Position getDefendPosition() { return defendPosition; }
    Position getHarassPosition() { return harassPosition; }
    Position getEnemyStartingPosition() { return enemyStartingPosition; }
    Position getMineralHoldPosition() { return mineralHold; }
    TilePosition getEnemyNatural() { return enemyNatural; }
    TilePosition getEnemyExpand() { return enemyExpand; }
    TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
    set <const Area*>& getAllyTerritory() { return allyTerritory; }
    set <const Area*>& getEnemyTerritory() { return enemyTerritory; }
    set <const Base*>& getAllBases() { return allBases; }
}