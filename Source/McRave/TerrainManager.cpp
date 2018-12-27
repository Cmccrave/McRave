#include "McRave.h"

void TerrainManager::onStart()
{
    mapBWEM.Initialize();
    mapBWEM.EnableAutomaticPathAnalysis();
    bool startingLocationsOK = mapBWEM.FindBasesForStartingLocations();
    assert(startingLocationsOK);
    playerStartingTilePosition = Broodwar->self()->getStartLocation();
    playerStartingPosition = Position(playerStartingTilePosition) + Position(64, 48);

    // Check if the map is an island map
    islandMap = false;
    for (auto &start : mapBWEM.StartingLocations()) {
        if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(playerStartingTilePosition)))
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
}

void TerrainManager::onFrame()
{
    findEnemyStartingPosition();
    findEnemyNatural();
    findEnemyNextExpand();
    findAttackPosition();
    findDefendPosition();

    updateConcavePositions();
    updateAreas();
}

void TerrainManager::findEnemyStartingPosition()
{
    if (enemyStartingPosition.isValid())
        return;

    // Find closest enemy building
    for (auto &u : Units().getEnemyUnits()) {
        UnitInfo &unit = u.second;
        double distBest = 1280.0;
        TilePosition tileBest = TilePositions::Invalid;
        if (!unit.getType().isBuilding() || !unit.getTilePosition().isValid())
            continue;

        for (auto &start : Broodwar->getStartLocations()) {
            double dist = start.getDistance(unit.getTilePosition());
            if (dist < distBest)
                distBest = dist, tileBest = start;
        }
        if (tileBest.isValid() && tileBest != playerStartingTilePosition) {
            enemyStartingPosition = Position(tileBest) + Position(64, 48);
            enemyStartingTilePosition = tileBest;
        }
    }
}

void TerrainManager::findEnemyNatural()
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

void TerrainManager::findEnemyNextExpand()
{
    double best = 0.0;
    for (auto &station : BWEB::Stations::getStations()) {

        UnitType shuttle = Broodwar->self()->getRace().getTransport();

        // Shuttle check for island bases, check enemy owned bases
        if (!station.BWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()) && Units().getEnemyCount(shuttle) <= 0)
            continue;
        if (BWEB::Map::isUsed(station.BWEMBase()->Location()))
            continue;
        if (Terrain().getEnemyStartingTilePosition() == station.BWEMBase()->Location())
            continue;

        // Get value of the expansion
        double value = 0.0;
        for (auto &mineral : station.BWEMBase()->Minerals())
            value += double(mineral->Amount());
        for (auto &gas : station.BWEMBase()->Geysers())
            value += double(gas->Amount());
        if (station.BWEMBase()->Geysers().size() == 0)
            value = value / 10.0;

        // Get distance of the expansion
        double distance;
        if (!station.BWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()))
            distance = log(station.BWEMBase()->Center().getDistance(Terrain().getEnemyStartingPosition()));
        else if (!Players().vT())
            distance = BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), station.BWEMBase()->Center()) / (BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), station.BWEMBase()->Center()));
        else
            distance = BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), station.BWEMBase()->Center());

        double score = value / distance;

        if (score > best) {
            best = score;
            enemyExpand = (TilePosition)station.BWEMBase()->Center();
        }
    }
}

void TerrainManager::findAttackPosition()
{
    // HACK: Hitchhiker has issues pathing to enemy expansions, just attack the main for now
    if (Broodwar->mapFileName().find("Hitchhiker") != string::npos)
        attackPosition = enemyStartingPosition;

    // Attack possible enemy expansion location
    else if (enemyExpand.isValid() && !Broodwar->isExplored(enemyExpand) && !Broodwar->isExplored(enemyExpand + TilePosition(4, 3)))
        attackPosition = (Position)enemyExpand;

    // Attack furthest enemy station
    else if (MyStations().getEnemyStations().size() > 0) {
        double distBest = 0.0;
        Position posBest;

        for (auto &station : MyStations().getEnemyStations()) {
            auto &s = *station.second;
            double dist = Players().vP() ? 1.0 / enemyStartingPosition.getDistance(s.BWEMBase()->Center()) : enemyStartingPosition.getDistance(s.BWEMBase()->Center());
            if (dist >= distBest) {
                distBest = dist;
                posBest = s.BWEMBase()->Center();
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

void TerrainManager::findDefendPosition()
{
    UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
    Position oldDefendPosition = defendPosition;
    reverseRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) < Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
    flatRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) == Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
    narrowNatural = BWEB::Map::getNaturalChoke() ? int(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1).getDistance(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2)) / 4) <= 2 : false;
    defendNatural = BWEB::Map::getNaturalChoke() ? BuildOrder::buildCount(baseType) > 1 || Broodwar->self()->visibleUnitCount(baseType) > 1 || defendPosition == Position(BWEB::Map::getNaturalChoke()->Center()) || reverseRamp : false;

    if (islandMap) {
        defendPosition = BWEB::Map::getMainPosition();
        return;
    }

    // Defend our main choke if we're hiding tech
    if (BuildOrder::isHideTech() && BWEB::Map::getMainChoke() && Broodwar->self()->visibleUnitCount(baseType) == 1) {
        defendPosition = (Position)BWEB::Map::getMainChoke()->Center();
        return;
    }

    // Set mineral holding positions
    double distBest = DBL_MAX;
    for (auto &station : MyStations().getMyStations()) {
        auto &s = *station.second;
        double dist;
        if (Terrain().getEnemyStartingPosition().isValid())
            dist = BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), Position(s.ResourceCentroid()));
        else
            dist = mapBWEM.Center().getDistance(Position(s.ResourceCentroid()));

        if (dist < distBest) {
            distBest = dist;
            mineralHold = (Position(s.ResourceCentroid()) + s.BWEMBase()->Center()) / 2;
            backMineralHold = (Position(s.ResourceCentroid()) - Position(s.BWEMBase()->Center())) + Position(s.ResourceCentroid());
        }
    }

    // Natural defending
    if (naturalWall) {
        Position door(naturalWall->getDoor());
        defendPosition = door.isValid() ? door : naturalWall->getCentroid();
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
    else if (mainWall) {
        Position door(mainWall->getDoor());
        defendPosition = door.isValid() ? door : mainWall->getCentroid();
    }
    else
        defendPosition = Position(BWEB::Map::getMainChoke()->Center());

    // If enemy non 2 gate proxy, defend natural choke
    if (Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate" && BWEB::Map::getNaturalChoke()) {
        defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());
        allyTerritory.insert(BWEB::Map::getNaturalArea());
    }

    // If this isn't the same as the last position, make new concave positions
    if (defendPosition != oldDefendPosition) {
        meleeChokePositions.clear();
        rangedChokePositions.clear();
    }
}

void TerrainManager::updateConcavePositions()
{
    for (auto tile : meleeChokePositions) {
        Broodwar->drawCircleMap(Position(tile), 8, Colors::Blue);
    }
    for (auto tile : rangedChokePositions) {
        Broodwar->drawCircleMap(Position(tile), 8, Colors::Blue);
    }

    if (!meleeChokePositions.empty() || !rangedChokePositions.empty() || Broodwar->getFrameCount() < 100 || defendPosition == mineralHold)
        return;

    auto area = defendNatural ? BWEB::Map::getNaturalArea() : BWEB::Map::getMainArea();
    auto choke = defendNatural ? BWEB::Map::getNaturalChoke() : BWEB::Map::getMainChoke();
    auto width = Util::chokeWidth(choke);
    auto line = Util::lineOfBestFit(choke);
    auto center = (defendNatural && naturalWall) ? Position(naturalWall->getDoor()) : Position(choke->Center());
    auto perpSlope = -1.0 / line.slope;
    Position test1, test2;

    int testX1 = Position(choke->Center()).x - 32;
    int testY1 = int(line.y(testX1));

    int testX2 = Position(choke->Center()).x + 32;
    int testY2 = int(line.y(testX2));

    if (abs(perpSlope) == DBL_MAX) {
        test1.x = center.x + 64;
        test1.y = center.y;
        test2.x = center.x - 64;
        test2.y = center.y;
    }

    else if (perpSlope == 0.0) {
        test1.x = center.x;
        test1.y = center.y + 64;
        test2.x = center.x;
        test2.y = center.y - 64;
    }

    else {
        auto dx = (128.0 / sqrt(1.0 + pow(perpSlope, 2.0)));
        auto dy = perpSlope * dx;
        test1.x = center.x + int(dx);
        test1.y = center.y + int(dy);
        test2.x = center.x - int(dx);
        test2.y = center.y - int(dy);
    }

    Broodwar->drawLineMap(center, test1, Colors::Red);
    Broodwar->drawLineMap(center, test2, Colors::Green);

    auto melee1 = Util::parallelLine(line, center.x, 12.0);
    //auto melee2 = Util::parallelLine(line, center.x, 30.0);
    auto melee3 = Util::parallelLine(line, center.x, 48.0);
    auto ranged1 = Util::parallelLine(line, center.x, 128.0);
    auto ranged2 = Util::parallelLine(line, center.x, 192.0);
    auto ranged3 = Util::parallelLine(line, center.x, 256.0);

    auto melee4 = Util::parallelLine(line, center.x, -12.0);
    //auto melee5 = Util::parallelLine(line, center.x, -30.0);
    auto melee6 = Util::parallelLine(line, center.x, -48.0);
    auto ranged4 = Util::parallelLine(line, center.x, -128.0);
    auto ranged5 = Util::parallelLine(line, center.x, -192.0);
    auto ranged6 = Util::parallelLine(line, center.x, -256.0);

    const auto addPlacements =[&](Line line, double gap, vector<Position>& thisVector, UnitType t) {
        if (abs(line.slope) != DBL_MAX) {
            auto xStart = Position(choke->Center()).x - max(128, Util::chokeWidth(choke));
            int yStart = int(line.y(xStart));
            auto current = Position(xStart, yStart);
            Position last = current;
            gap += 8.0;

            while (current.isValid() && xStart < Position(choke->Center()).x + max(128, Util::chokeWidth(choke))) {
                WalkPosition w(current);

                Broodwar->drawCircleMap(current, 2, Colors::Red);

                if (last.getDistance(current) > gap && Util::isWalkable(w, w, t) && mapBWEM.GetArea(w) == area && current.getDistance(Position(choke->Center())) < SIM_RADIUS) {
                    thisVector.push_back(current);
                    last = current;
                }

                xStart += 1;
                yStart = int(line.y(xStart));
                current = Position(xStart, yStart);
            }
        }
        else {
            auto xStart = Position(choke->Center()).x;
            int yStart = int(line.y(xStart));
            auto current = Position(xStart, yStart);
            Position last = Positions::Invalid;

            // Broodwar->drawCircleMap(current, 2, Colors::Red);

            while (current.isValid() && xStart < Position(choke->Center()).x + max(128, Util::chokeWidth(choke))) {
                WalkPosition w(current);

                if (last.getDistance(current) > gap && Util::isWalkable(w, w, t) && mapBWEM.GetArea(w) == area && current.getDistance(Position(choke->Center())) < 320.0) {
                    thisVector.push_back(Position(w));
                    last = current;
                }

                yStart += 1;
                current = Position(xStart, yStart);
            }
        }
    };

    if (Terrain().isInAllyTerritory(TilePosition(test1))) {
        //addPlacements(line, 19.0, meleeChokePositions);
        addPlacements(melee1, 19.0, meleeChokePositions, UnitTypes::Protoss_Zealot);
        //addPlacements(melee2, 19.0, meleeChokePositions, UnitTypes::Protoss_Zealot);
        addPlacements(melee3, 19.0, meleeChokePositions, UnitTypes::Protoss_Zealot);
        addPlacements(ranged1, 32.0, rangedChokePositions, UnitTypes::Protoss_Zealot);
        addPlacements(ranged2, 32.0, rangedChokePositions, UnitTypes::Protoss_Zealot);
        addPlacements(ranged3, 32.0, rangedChokePositions, UnitTypes::Protoss_Zealot);
    }
    else {
        //addPlacements(line, 19.0, meleeChokePositions);
        addPlacements(melee4, 19.0, meleeChokePositions, UnitTypes::Protoss_Dragoon);
        //addPlacements(melee5, 19.0, meleeChokePositions, UnitTypes::Protoss_Dragoon);
        addPlacements(melee6, 19.0, meleeChokePositions, UnitTypes::Protoss_Dragoon);
        addPlacements(ranged4, 32.0, rangedChokePositions, UnitTypes::Protoss_Dragoon);
        addPlacements(ranged5, 32.0, rangedChokePositions, UnitTypes::Protoss_Dragoon);
        addPlacements(ranged6, 32.0, rangedChokePositions, UnitTypes::Protoss_Dragoon);
    }
}

void TerrainManager::updateAreas()
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



bool TerrainManager::findNaturalWall(vector<UnitType>& types, const vector<UnitType>& defenses)
{
    // Hack: Make a bunch of walls as Zerg for testing - disabled atm
    if (Broodwar->self()->getRace() == Races::Zerg) {
        for (auto &area : mapBWEM.Areas()) {

            // Only make walls at gas bases that aren't starting bases
            bool invalidBase = false;
            for (auto &base : area.Bases()) {
                if (base.Starting())
                    invalidBase = true;
            }
            if (invalidBase)
                continue;

            const ChokePoint * bestChoke = nullptr;
            double distBest = DBL_MAX;
            for (auto &choke : area.ChokePoints()) {
                auto dist = Position(choke->Center()).getDistance(mapBWEM.Center());
                if (dist < distBest) {
                    distBest = dist;
                    bestChoke = choke;
                }
            }
            BWEB::Walls::createWall(types, &area, bestChoke, UnitTypes::None, defenses);

            if (&area == BWEB::Map::getNaturalArea())
                naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalArea());
        }
        return true;
    }

    else {
        if (naturalWall)
            return true;

        UnitType wallTight;
        bool reservePath = Broodwar->self()->getRace() != Races::Terran;

        if (Broodwar->self()->getRace() == Races::Terran && Players().vP())
            wallTight = UnitTypes::Protoss_Zealot;
        else if (Players().vZ())
            wallTight = UnitTypes::Zerg_Zergling;
        else
            wallTight = UnitTypes::Terran_Vulture;

        // Create a wall
        BWEB::Walls::createWall(types, BWEB::Map::getNaturalArea(), BWEB::Map::getNaturalChoke(), wallTight, defenses, reservePath);
        naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalArea());

        if (naturalWall)
            return true;
    }
    return false;
}

bool TerrainManager::findMainWall(vector<UnitType>& types, const vector<UnitType>& defenses)
{
    if (BWEB::Walls::getWall(BWEB::Map::getMainArea(), BWEB::Map::getMainChoke()) || BWEB::Walls::getWall(BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke()))
        return true;
    UnitType wallTight = Broodwar->enemy()->getRace() == Races::Protoss ? UnitTypes::Protoss_Zealot : UnitTypes::Zerg_Zergling;

    BWEB::Walls::createWall(types, BWEB::Map::getMainArea(), BWEB::Map::getMainChoke(), wallTight, defenses, false, true);
    BWEB::Walls::Wall * wallB = BWEB::Walls::getWall(BWEB::Map::getMainArea(), BWEB::Map::getMainChoke());
    if (wallB) {
        mainWall = wallB;
        return true;
    }

    BWEB::Walls::createWall(types, BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke(), wallTight, defenses, false, true);
    BWEB::Walls::Wall * wallA = BWEB::Walls::getWall(BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke());
    if (wallA) {
        mainWall = wallA;
        return true;
    }
    return false;
}

bool TerrainManager::isInAllyTerritory(TilePosition here)
{
    // Find the area of this tile position and see if it exists in ally territory
    if (here.isValid() && mapBWEM.GetArea(here) && allyTerritory.find(mapBWEM.GetArea(here)) != allyTerritory.end())
        return true;
    return false;
}

bool TerrainManager::isInEnemyTerritory(TilePosition here)
{
    // Find the area of this tile position and see if it exists in enemy territory
    if (here.isValid() && mapBWEM.GetArea(here) && enemyTerritory.find(mapBWEM.GetArea(here)) != enemyTerritory.end())
        return true;
    return false;
}

bool TerrainManager::isStartingBase(TilePosition here)
{
    for (auto tile : Broodwar->getStartLocations()) {
        if (here.getDistance(tile) < 4)
            return true;
    }
    return false;
}

Position TerrainManager::closestUnexploredStart() {
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

Position TerrainManager::randomBasePosition()
{
    int random = rand() % (Terrain().getAllBases().size());
    int i = 0;
    for (auto &base : Terrain().getAllBases()) {
        if (i == random)
            return base->Center();
        else
            i++;
    }
    return BWEB::Map::getMainPosition();
}