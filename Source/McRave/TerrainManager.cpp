#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;

namespace McRave::Terrain {

    namespace {
        set <const Area*> allyTerritory;
        set <const Area*> enemyTerritory;
        Position enemyStartingPosition = Positions::Invalid;
        TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        Position mineralHold, backMineralHold;
        Position attackPosition, defendPosition;
        TilePosition enemyNatural = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;
        vector<Position> meleeChokePositions;
        vector<Position> rangedChokePositions;
        set<const Base *> allBases;
        BWEB::Wall* mainWall = nullptr;
        BWEB::Wall* naturalWall = nullptr;
        UnitType tightType = UnitTypes::None;

        bool islandMap = false;
        bool reverseRamp = false;
        bool flatRamp = false;
        bool narrowNatural = false;
        bool defendNatural = false;

        void findEnemyStartingPosition()
        {
            if (enemyStartingPosition.isValid())
                return;

            // Find closest enemy building
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;
                double distBest = 1280.0;
                TilePosition tileBest = TilePositions::Invalid;
                if (!unit.getType().isBuilding() || !unit.getTilePosition().isValid())
                    continue;

                for (auto &start : Broodwar->getStartLocations()) {
                    double dist = start.getDistance(unit.getTilePosition());
                    if (dist < distBest)
                        distBest = dist, tileBest = start;
                }
                if (tileBest.isValid() && tileBest != BWEB::Map::getMainTile()) {
                    enemyStartingPosition = Position(tileBest) + Position(64, 48);
                    enemyStartingTilePosition = tileBest;
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
            double best = 0.0;
            for (auto &station : BWEB::Stations::getStations()) {

                UnitType shuttle = Broodwar->self()->getRace().getTransport();

                // Shuttle check for island bases, check enemy owned bases
                if (!station.getBWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()) && Units::getEnemyCount(shuttle) <= 0)
                    continue;
                if (BWEB::Map::isUsed(station.getBWEMBase()->Location()) != UnitTypes::None)
                    continue;
                if (enemyStartingTilePosition == station.getBWEMBase()->Location())
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
                else if (!Players::vT())
                    distance = BWEB::Map::getGroundDistance(enemyStartingPosition, station.getBWEMBase()->Center()) / (BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), station.getBWEMBase()->Center()));
                else
                    distance = BWEB::Map::getGroundDistance(enemyStartingPosition, station.getBWEMBase()->Center());

                double score = value / distance;

                if (score > best) {
                    best = score;
                    enemyExpand = (TilePosition)station.getBWEMBase()->Center();
                }
            }
        }

        void findAttackPosition()
        {
            // Attack possible enemy expansion location
            if (enemyExpand.isValid() && !Broodwar->isExplored(enemyExpand) && !Broodwar->isExplored(enemyExpand + TilePosition(4, 3)))
                attackPosition = (Position)enemyExpand;

            // Attack furthest enemy station
            else if (Stations::getEnemyStations().size() > 0) {
                double distBest = 0.0;
                Position posBest;

                for (auto &station : Stations::getEnemyStations()) {
                    auto &s = *station.second;
                    double dist = Players::vP() ? 1.0 / enemyStartingPosition.getDistance(s.getBWEMBase()->Center()) : enemyStartingPosition.getDistance(s.getBWEMBase()->Center());
                    if (dist >= distBest) {
                        distBest = dist;
                        posBest = s.getBWEMBase()->Center();
                    }
                }
                attackPosition = posBest;
            }

            // Attack enemy main
            else if (enemyStartingPosition.isValid() && !Broodwar->isExplored(enemyStartingTilePosition))
                attackPosition = enemyStartingPosition;
            else
                attackPosition = Positions::Invalid;
        }

        void findDefendPosition()
        {
            Broodwar->drawCircleMap(defendPosition, 10, Colors::Brown);

            UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
            Position oldDefendPosition = defendPosition;
            reverseRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) < Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
            flatRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) == Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
            narrowNatural = BWEB::Map::getNaturalChoke() ? int(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1).getDistance(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2)) / 4) <= 2 : false;
            defendNatural = BWEB::Map::getNaturalChoke() ? BuildOrder::buildCount(baseType) > 1 || vis(baseType) > 1 || defendPosition == Position(BWEB::Map::getNaturalChoke()->Center()) || (reverseRamp && !Players::vZ() && Players::getSupply(PlayerState::Self) > 100) : false;

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
                    mineralHold = (Position(s.getResourceCentroid()) + s.getBWEMBase()->Center()) / 2;
                    backMineralHold = (Position(s.getResourceCentroid()) - Position(s.getBWEMBase()->Center())) + Position(s.getResourceCentroid());
                }
            }

            // If enemy is rushing we want to defend our mineral line until we can stabilize
            if ((Strategy::enemyRush() || Strategy::enemyProxy() || Strategy::enemyPressure()) && !Strategy::defendChoke() && !BuildOrder::isWallNat()) {
                defendPosition = mineralHold;
                defendNatural = false;
            }

            // Defend our main choke if we're hiding tech
            else if (BuildOrder::isHideTech() && BWEB::Map::getMainChoke() && Broodwar->self()->visibleUnitCount(baseType) == 1) {
                defendPosition = (Position)BWEB::Map::getMainChoke()->Center();
                defendNatural = false;
            }

            // Natural defending
            else if (naturalWall && BuildOrder::isWallNat()) {
                Position opening(naturalWall->getOpening());
                defendPosition = opening.isValid() ? opening : naturalWall->getCentroid();
                allyTerritory.insert(BWEB::Map::getNaturalArea());
                defendNatural = true;
            }
            else if (defendNatural) {
                defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());
                allyTerritory.insert(BWEB::Map::getNaturalArea());

                // Move the defend position on maps like destination
                while (!Broodwar->isBuildable((TilePosition)defendPosition)) {
                    double distBest = DBL_MAX;
                    TilePosition test = (TilePosition)defendPosition;
                    for (int x = test.x - 1; x <= test.x + 1; x++) {
                        for (int y = test.y - 1; y <= test.y + 1; y++) {
                            TilePosition t(x, y);
                            if (!t.isValid())
                                continue;

                            double dist = Position(t).getDistance((Position)BWEB::Map::getNaturalArea()->Top());

                            if (dist < distBest) {
                                distBest = dist;
                                defendPosition = (Position)t;
                            }
                        }
                    }
                }
            }

            // Main defending
            else if (mainWall && BuildOrder::isWallMain()) {
                Position opening(mainWall->getOpening());
                defendPosition = opening.isValid() ? opening : mainWall->getCentroid();
            }
            else
                defendPosition = Position(BWEB::Map::getMainChoke()->Center());

            // If this isn't the same as the last position, make new concave positions
            if (defendPosition != oldDefendPosition) {
                meleeChokePositions.clear();
                rangedChokePositions.clear();
            }

            // If we aren't defending the natural, remove the area in case we added it
            if (!defendNatural)
                allyTerritory.erase(BWEB::Map::getNaturalArea());
        }

        void updateAreas()
        {
            // Squish areas
            if (BWEB::Map::getNaturalArea()) {

                // Add "empty" areas (ie. Andromeda areas around main)	
                for (auto &area : BWEB::Map::getNaturalArea()->AccessibleNeighbours()) {
                    for (auto &choke : area->ChokePoints()) {
                        if (choke->Center() == BWEB::Map::getMainChoke()->Center())
                            allyTerritory.insert(area);
                    }
                }

                UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
                if ((BuildOrder::buildCount(baseType) >= 2 || BuildOrder::isWallNat()) && BWEB::Map::getNaturalArea())
                    allyTerritory.insert(BWEB::Map::getNaturalArea());

                // HACK: Add to my territory if chokes are shared
                if (BWEB::Map::getMainChoke() == BWEB::Map::getNaturalChoke())
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

        // Check if the map is an island map
        for (auto &start : mapBWEM.StartingLocations()) {
            if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(BWEB::Map::getMainTile())))
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

        // Figure out what we need to be tight against
        if (Broodwar->self()->getRace() == Races::Terran && Players::vP())
            tightType = UnitTypes::Protoss_Zealot;
        else if (Players::vZ())
            tightType = UnitTypes::Zerg_Zergling;
        else
            tightType = UnitTypes::None;
    }

    void onFrame()
    {
        findEnemyStartingPosition();
        findEnemyNatural();
        findEnemyNextExpand();
        findAttackPosition();
        findDefendPosition();

        updateAreas();
    }

    bool inRangeOfWallPieces(UnitInfo& unit)
    {
        if (naturalWall) {
            for (auto &piece : naturalWall->getSmallTiles()) {
                if (BWEB::Map::isUsed(piece) != UnitTypes::None) {
                    auto center = Position(piece) + Position(32, 32);
                    if (unit.getPosition().getDistance(center) < unit.getGroundRange() + 32)
                        return true;
                }
            }
            for (auto &piece : naturalWall->getMediumTiles()) {
                if (BWEB::Map::isUsed(piece) != UnitTypes::None) {
                    auto center = Position(piece) + Position(48, 32);
                    if (unit.getPosition().getDistance(center) < unit.getGroundRange() + 48)
                        return true;
                }
            }
            for (auto &piece : naturalWall->getLargeTiles()) {
                if (BWEB::Map::isUsed(piece) != UnitTypes::None) {
                    auto center = Position(piece) + Position(64, 48);
                    if (unit.getPosition().getDistance(center) < unit.getGroundRange() + 64)
                        return true;
                }
            }
        }
        return false;
    }

    bool inRangeOfWallDefenses(UnitInfo& unit)
    {
        if (naturalWall) {
            for (auto &piece : naturalWall->getDefenses()) {
                if (BWEB::Map::isUsed(piece) != UnitTypes::None) {
                    auto center = Position(piece) + Position(32, 32);
                    if (unit.getPosition().getDistance(center) < unit.getGroundRange() + 32)
                        return true;
                }
            }
        }
        return false;
    }

    bool findNaturalWall(vector<UnitType>& types, const vector<UnitType>& defenses, bool tight)
    {
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings)
            return false;

        // Map specific criteria
        if (Broodwar->mapFileName().find("Destination") != string::npos || Broodwar->mapFileName().find("Python") != string::npos) {
            tight = false;
            tightType = UnitTypes::None;
        }

        // Make a bunch of walls as Zerg for testing
        if (false && Broodwar->self()->getRace() == Races::Zerg) {
            for (auto &area : mapBWEM.Areas()) {

                // Only make walls at gas bases that aren't starting bases
                bool invalidBase = false;
                for (auto &base : area.Bases()) {
                    if (base.Starting())
                        invalidBase = true;
                }
                if (invalidBase || area.Bases().empty())
                    continue;

                const ChokePoint * bestChoke = nullptr;
                double distBest = DBL_MAX;
                for (auto &choke : area.ChokePoints()) {
                    auto dist = BWEB::Map::getGroundDistance(Position(choke->Center()), mapBWEM.Center());
                    if (dist < distBest) {
                        distBest = dist;
                        bestChoke = choke;
                    }
                }

                BWEB::Walls::createWall(types, &area, bestChoke, UnitTypes::None, defenses, true, false);

                if (&area == BWEB::Map::getNaturalArea())
                    naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalArea());
            }
            return true;
        }

        else {
            auto openWall = Broodwar->self()->getRace() != Races::Terran;
            auto choke = BWEB::Map::getNaturalChoke();
            auto area = BWEB::Map::getNaturalArea();
            naturalWall = BWEB::Walls::createWall(types, area, choke, tightType, defenses, openWall, tight);
            return naturalWall != nullptr;
        }
        return false;
    }

    bool findMainWall(vector<UnitType>& types, const vector<UnitType>& defenses, bool tight)
    {
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings)
            return false;

        if (BWEB::Walls::getWall(BWEB::Map::getMainArea(), BWEB::Map::getMainChoke()) || BWEB::Walls::getWall(BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke()))
            return true;

        auto openWall = Broodwar->self()->getRace() != Races::Terran;
        auto choke = BWEB::Map::getNaturalChoke();
        auto area = BWEB::Map::getNaturalArea();

        mainWall = BWEB::Walls::createWall(types, area, choke, tightType, defenses, openWall, tight);
        return mainWall != nullptr;
    }

    bool isInAllyTerritory(TilePosition here)
    {
        // Find the area of this tile position and see if it exists in ally territory
        if (here.isValid() && mapBWEM.GetArea(here) && allyTerritory.find(mapBWEM.GetArea(here)) != allyTerritory.end())
            return true;
        return false;
    }

    bool isInEnemyTerritory(TilePosition here)
    {
        // Find the area of this tile position and see if it exists in enemy territory
        if (here.isValid() && mapBWEM.GetArea(here) && enemyTerritory.find(mapBWEM.GetArea(here)) != enemyTerritory.end())
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

    bool isIslandMap() { return islandMap; }
    bool isReverseRamp() { return reverseRamp; }
    bool isFlatRamp() { return flatRamp; }
    bool isNarrowNatural() { return narrowNatural; }
    bool isDefendNatural() { return defendNatural; }
    bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(BWAPI::TilePosition(enemyStartingPosition)); }
    Position getAttackPosition() { return attackPosition; }
    Position getDefendPosition() { return defendPosition; }
    Position getEnemyStartingPosition() { return enemyStartingPosition; }
    Position getMineralHoldPosition() { return mineralHold; }
    Position getBackMineralHoldPosition() { return backMineralHold; }
    TilePosition getEnemyNatural() { return enemyNatural; }
    TilePosition getEnemyExpand() { return enemyExpand; }
    TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }
    vector<Position> getMeleeChokePositions() { return meleeChokePositions; }
    vector<Position> getRangedChokePositions() { return rangedChokePositions; }
    set <const Area*>& getAllyTerritory() { return allyTerritory; }
    set <const Area*>& getEnemyTerritory() { return enemyTerritory; }
    set <const Base *>& getAllBases() { return allBases; }
    const BWEB::Wall* getMainWall() { return mainWall; }
    const BWEB::Wall* getNaturalWall() { return naturalWall; }
}