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
        Position mineralHold, backMineralHold;
        Position defendPosition = Positions::Invalid;
        Position attackPosition = Positions::Invalid;
        TilePosition enemyNatural = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;
        vector<Position> meleeChokePositions;
        vector<Position> rangedChokePositions;
        set<const Base *> allBases;
        BWEB::Wall* mainWall = nullptr;
        BWEB::Wall* naturalWall = nullptr;

        // Wall parameters
        vector<UnitType> buildings;
        vector<UnitType> defenses;
        bool tight = true;
        UnitType tightType = None;

        bool shitMap = false;
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
            UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
            Position oldDefendPosition = defendPosition;
            reverseRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) < Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
            flatRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) == Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
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
                    mineralHold = (Position(s.getResourceCentroid()) + s.getBWEMBase()->Center()) / 2;
                    backMineralHold = (Position(s.getResourceCentroid()) - Position(s.getBWEMBase()->Center())) + Position(s.getResourceCentroid());
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
                else if (naturalWall && BuildOrder::isWallNat()) {
                    Position opening(naturalWall->getOpening());
                    defendPosition = opening.isValid() ? opening : naturalWall->getCentroid();
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
            else if (mainWall && BuildOrder::isWallMain()) {
                Position opening(mainWall->getOpening());
                defendPosition = opening.isValid() ? opening : mainWall->getCentroid();
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

            // If this isn't the same as the last position, make new concave positions
            if (defendPosition != oldDefendPosition) {
                meleeChokePositions.clear();
                rangedChokePositions.clear();
            }

            // If we aren't defending the natural, remove the area in case we added it
            if (!defendNatural)
                allyTerritory.erase(BWEB::Map::getNaturalArea());
        }

        void findNaturalWall()
        {
            if (Broodwar->getGameType() == GameTypes::Use_Map_Settings || Broodwar->self()->getRace() == Races::Terran)
                return;

            auto openWall = Broodwar->self()->getRace() != Races::Terran;
            auto choke = BWEB::Map::getNaturalChoke();
            auto area = BWEB::Map::getNaturalArea();

            naturalWall = BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);

            if (!naturalWall && Broodwar->self()->getRace() == Races::Zerg) {
                buildings ={ Zerg_Hatchery };
                naturalWall = BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
            }
            if (!naturalWall && Broodwar->self()->getRace() == Races::Zerg) {
                buildings ={ Zerg_Evolution_Chamber };
                naturalWall = BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
            }
        }

        void findMainWall()
        {
            if (Broodwar->getGameType() == GameTypes::Use_Map_Settings || Broodwar->self()->getRace() != Races::Terran)
                return;

            auto openWall = Broodwar->self()->getRace() != Races::Terran;
            auto choke = BWEB::Map::getMainChoke();
            auto area = BWEB::Map::getMainArea();

            mainWall = BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
        }

        void findOtherWalls()
        {
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

                    BWEB::Walls::createWall(buildings, &area, bestChoke, None, defenses, true, false);

                    if (&area == BWEB::Map::getNaturalArea())
                        naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalArea());
                }
            }
        }

        void initializeWallParameters()
        {
            // Figure out what we need to be tight against
            if (Broodwar->self()->getRace() == Races::Terran && Players::vP())
                tightType = Protoss_Zealot;
            else if (Players::vZ())
                tightType = Zerg_Zergling;
            else
                tightType = None;

            // Protoss wall parameters
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::vZ()) {
                    tight = false;
                    buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                    defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
                }
                else {
                    int count = 2;
                    tight = false;
                    buildings.insert(buildings.end(), count, Protoss_Pylon);
                    defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
                }
            }

            // Terran wall parameters
            if (Broodwar->self()->getRace() == Races::Terran) {
                tight = true;

                /*if (Broodwar->mapFileName().find("Heartbreak") != string::npos)
                    buildings ={ Terran_Barracks, Terran_Supply_Depot };
                else*/
                buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
            }

            // Zerg wall parameters
            if (Broodwar->self()->getRace() == Races::Zerg) {
                tight = false;
                buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber };
                defenses.insert(defenses.end(), 8, Zerg_Creep_Colony);
            }

            // Map specific criteria
            if (Broodwar->self()->getRace() != Races::Terran && (Broodwar->mapFileName().find("Destination") != string::npos || Broodwar->mapFileName().find("Hitchhiker") != string::npos || Broodwar->mapFileName().find("Python") != string::npos || Broodwar->mapFileName().find("BlueStorm") != string::npos)) {
                tight = false;
                tightType = None;
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

        initializeWallParameters();
        findMainWall();
        findNaturalWall();
        findOtherWalls();
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

    int needGroundDefenses(BWEB::Wall& wall)
    {
        auto groundCount = wall.getGroundDefenseCount();

        // P
        if (Broodwar->self()->getRace() == Races::Protoss) {
            auto prepareExpansionDefenses = Util::getTime() < Time(10, 0) && vis(Protoss_Nexus) >= 2 && com(Protoss_Forge) > 0;

            if (Players::vP() && prepareExpansionDefenses && BuildOrder::isWallNat()) {
                auto cannonCount = 2 + int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                return cannonCount - groundCount;
            }

            if (Players::vZ() && BuildOrder::isWallNat()) {
                auto cannonCount = int(com(Protoss_Forge) > 0)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 6)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

                // TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons        
                if (Strategy::getEnemyBuild() == "2HatchHydra")
                    return 5 - groundCount;
                else if (Strategy::getEnemyBuild() == "3HatchHydra")
                    return 4 - groundCount;
                else if (Strategy::getEnemyBuild() == "2HatchMuta" && Util::getTime() > Time(4, 0))
                    return 3 - groundCount;
                else if (Strategy::getEnemyBuild() == "3HatchMuta" && Util::getTime() > Time(5, 0))
                    return 3 - groundCount;
                else if (Strategy::getEnemyBuild() == "4Pool")
                    return 2 + (Players::getSupply(PlayerState::Self) >= 24) - groundCount;

                return cannonCount - groundCount;
            }
        }

        // Z
        if (Broodwar->self()->getRace() == Races::Zerg) {
            auto visSunken = vis(Zerg_Creep_Colony) + vis(Zerg_Sunken_Colony);

            if (Players::vP())
                return (2 * Strategy::enemyRush()) + int((Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon)) / max(1, visSunken)) - groundCount;
            if (Players::vT())
                return (2 * (Util::getTime() > Time(3,45) && Strategy::enemyPressure())) + int((Players::getCurrentCount(PlayerState::Enemy, Terran_Marine) + Players::getCurrentCount(PlayerState::Enemy, Terran_Firebat) / 6) / max(1, visSunken)) - groundCount;
        }
        return 0;
    }

    int needAirDefenses(BWEB::Wall& wall)
    {
        auto airCount = wall.getAirDefenseCount();

        // P
        if (Broodwar->self()->getRace() == Races::Protoss) {

        }

        // Z
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players::vP() && Util::getTime() > Time(5, 0))
                return Strategy::enemyAir() - airCount;
        }
        return 0;
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
    set <const Base*>& getAllBases() { return allBases; }
    BWEB::Wall* getMainWall() { return mainWall; }
    BWEB::Wall* getNaturalWall() { return naturalWall; }
}