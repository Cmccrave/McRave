#include "BWEB.h"
#include <chrono>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace BWEB {

    namespace {
        map<const BWEM::ChokePoint * const, Wall> walls;
        bool logInfo = true;

        int existingDefenseGrid[256][256];

        map<TilePosition, BWAPI::Color> testTiles;
    }

    void Wall::initialize()
    {
        // Set important terrain features
        pylonWall               = count(rawBuildings.begin(), rawBuildings.end(), BWAPI::UnitTypes::Protoss_Pylon) > 1;
        base                    = !area->Bases().empty() ? &area->Bases().front() : nullptr;
        station                 = Stations::getClosestStation(TilePosition(area->Top()));
        chokeAngle              = station->getDefenseAngle();
    }

    bool Wall::tryLocations(vector<TilePosition>& tryOrder, set<TilePosition>& insertList, UnitType type)
    {
        auto debugColor = Colors::White;
        if (type.tileWidth() == 3)
            debugColor = Colors::Grey;
        if (type.tileWidth() == 4)
            debugColor = Colors::Black;

        // Defense types place every possible tile
        if (type.tileWidth() == 2 && type.tileHeight() == 2 && type != Protoss_Pylon && type != Zerg_Spire) {
            for (auto placement : tryOrder) {
                auto tile = station->getBase()->Location() + placement;
                auto stationDefense = type.tileHeight() == 2 && type.tileHeight() == 2 && station->getDefenses().find(tile) != station->getDefenses().end();
                if ((!Map::isReserved(tile, 2, 2) && BWEB::Map::isPlaceable(type, tile)) || stationDefense) {
                    insertList.insert(tile);
                    Map::addUsed(tile, type);
                    return true;
                }
            }
        }

        // Blocking space for movement
        else if (!type.isBuilding()) {
            if (!requireTight) {
                for (auto placement : tryOrder) {
                    auto tile = station->getBase()->Location() + placement;
                    if (BWEB::Map::isPlaceable(type, tile)) {
                        openings.insert(tile);
                        Map::addUsed(tile, type);
                        return true;
                    }
                }
            }
        }

        // Other types only need 1 unique spot
        else {
            for (auto placement : tryOrder) {
                auto tile = station->getBase()->Location() + placement;
                testTiles[tile] = debugColor;
                if (BWEB::Map::isPlaceable(type, tile) && !BWEB::Map::isReserved(tile, type.tileWidth(), type.tileHeight())) {
                    insertList.insert(tile);
                    Map::addUsed(tile, type);
                    return true;
                }
            }
        }

        // Pylon needs a fallback placement
        if (type == Protoss_Pylon && !pylonWall) {
            auto distBest = DBL_MAX;
            auto tileBest = TilePositions::Invalid;
            for (auto &tile : station->getDefenses()) {
                auto dist = tile.getDistance(TilePosition(choke->Center()));
                if (dist < distBest) {
                    distBest = dist;
                    tileBest = tile;
                }
            }

            if (BWEB::Map::isPlaceable(type, tileBest)) {
                insertList.insert(tileBest);
                Map::addUsed(tileBest, type);
                return true;
            }
        }
        return false;
    }

    void Wall::addPieces()
    {
        // For each piece, try to place it a known distance away depending on how the angles of chokes look     
        auto closestMain = Stations::getClosestMainStation(station->getBase()->Location());
        auto mainChokeCenter = Position(closestMain->getChokepoint()->Center());
        vector<TilePosition> smlOrder, medOrder, lrgOrder, opnOrder;
        auto flipOrder = false; // TODO: Right now we flip the opposite way so we always can get out            
        auto flipHorizontal = false;
        auto flipVertical = false;
        auto maintainShape = false;

        // Stations without chokepoints (or multiple) don't get determined on start
        if (!station->isMain() && !station->isNatural()) {
            auto bestGeo = Position(choke->Center()) + Position(4, 4);
            auto geoDist = DBL_MAX;
            for (auto &geo : choke->Geometry()) {
                auto p = Position(geo) + Position(4, 4);
                if (p.getDistance(station->getBase()->Center()) < geoDist)
                    bestGeo = Position(geo) + Position(4, 4);
            }
            chokeAngle = Map::getAngle(make_pair(station->getBase()->Center(), bestGeo)) + M_PI_D2;
        }

        defenseArrangement = int(round(chokeAngle / M_PI_D4)) % 4;

        const auto adjustOrder = [&](auto& order, auto diff) {
            for (auto &tile : order)
                tile = tile + diff;
        };

        // Iteration attempts move buildings closer
        auto iteration = 0;
        //if (station->isNatural()) {
        //    auto dist = Position(choke->Center()).getDistance(station->getBase()->Center());
        //    if (dist >= 288.0)
        //        iteration+=2;
        //}

        // If this isn't a natural or main, allow it to start further back
        auto maxIteration = 2;
        if (station && !station->isMain() && !station->isNatural()) {
            iteration -= 2;
            maxIteration = 0;
        }
        
        // This is ugly
        auto hatchOffset = requireTight ? 6 : 0;
        if (station->isMain()) {
            iteration = 5;
            maxIteration = 8;
        }

        for (; iteration < maxIteration; iteration+=2) {
            cleanup();
            smallTiles.clear();
            mediumTiles.clear();
            largeTiles.clear();

            // 0/8 - Horizontal
            if (defenseArrangement == 0) {
                lrgOrder        ={ {-4, -5}, {-3, -5}, {-2, -5}, {-1, -5}, {0, -5}, {1, -5}, {2, -5}, {3, -5}, {4, -5} };
                medOrder        ={ {-4, -4}, {-3, -4}, {-2, -4}, {-1, -4}, {0, -4}, {1, -4}, {2, -4}, {3, -4}, {4, -4} };
                smlOrder        ={ {-2, -4}, {-1, -4}, { 0, -4}, { 1, -4}, {2, -4}, {3, -4}, {4, -4}, {5, -4} };
                flipOrder       = station->isNatural() && mainChokeCenter.x > base->Center().x;
                flipVertical    = base->Center().y < Position(choke->Center()).y;

                // Shift positions based on chokepoint offset and iteration
                auto diffX = !maintainShape ? TilePosition(choke->Center()).x - base->Location().x : 0;
                auto diffY = -iteration;
                wallOffset = TilePosition(0, -iteration - hatchOffset);
                adjustOrder(lrgOrder, TilePosition(diffX, diffY));
                adjustOrder(medOrder, TilePosition(diffX, diffY));
                adjustOrder(smlOrder, TilePosition(diffX, diffY));
            }

            // pi/4 - Angled
            else if (defenseArrangement == 1 || defenseArrangement == 3) {
                lrgOrder        ={ {0, -7}, {-2, -5}, {-4, -3}, {-6, -1}, {-8,  0} };
                medOrder        ={ {1, -6}, {-1, -4}, {-3, -2}, {-5, 0} };
                smlOrder        ={ {0, -4}, {-2, -2}, {-4,  0} };

                // TODO: these flips don't really work as intended
                flipOrder       = (station->isNatural() && mainChokeCenter.x < base->Center().x && mainChokeCenter.y > base->Center().y)
                    || (station->isNatural() && mainChokeCenter.x > base->Center().x && mainChokeCenter.y < base->Center().y)
                    || (!station->isMain() && !station->isNatural() && station->getResourceCentroid().x < station->getBase()->Center().x);
                flipVertical    = base->Center().y < Position(choke->Center()).y;
                flipHorizontal  = base->Center().x < Position(choke->Center()).x;

                // Shift positions based on iteration
                auto diffX = -iteration;
                auto diffY = -iteration;
                wallOffset = TilePosition(-iteration - hatchOffset, -iteration - hatchOffset);
                adjustOrder(lrgOrder, TilePosition(diffX, diffY));
                adjustOrder(medOrder, TilePosition(diffX, diffY));
                adjustOrder(smlOrder, TilePosition(diffX, diffY));

                // If this isn't a natural or main, allow it to place more large blocks as zerg
                if (station && !station->isMain() && !station->isNatural() && Broodwar->self()->getRace() == Races::Zerg) {
                    lrgOrder.push_back(TilePosition(-4, 0));
                    lrgOrder.push_back(TilePosition(0, -3));
                    lrgOrder.push_back(TilePosition(-4, -3));
                    //medOrder.push_back(TilePosition(-1, -7));
                    //medOrder.push_back(TilePosition(-3, -5));
                    //medOrder.push_back(TilePosition(-5, -3));
                    //medOrder.push_back(TilePosition(-7, -1));
                }
            }

            // pi/2 - Vertical
            else if (defenseArrangement == 2) {
                lrgOrder        ={ {-6, -4}, {-6, -3}, {-6, -2}, {-6, -1}, {-6,  0}, {-6,  1}, {-6,  2}, {-6,  3}, {-6,  4} };
                medOrder        ={ {-5, -3}, {-5, -2}, {-5, -1}, {-5,  0}, {-5,  1}, {-5,  2}, {-5,  3}, {-5,  4} };
                smlOrder        ={ {-4, -1}, {-4,  0}, {-4,  1}, {-4,  2} };
                flipOrder       = station->isNatural() && mainChokeCenter.y > base->Center().y;
                flipHorizontal  = base->Center().x < Position(choke->Center()).x;

                // Shift positions based on chokepoint offset and iteration
                auto diffX = -iteration;
                auto diffY = !maintainShape ? TilePosition(choke->Center()).y - base->Location().y : 0;
                wallOffset = TilePosition(-iteration - hatchOffset, 0);
                adjustOrder(lrgOrder, TilePosition(diffX, diffY));
                adjustOrder(medOrder, TilePosition(diffX, diffY));
                adjustOrder(smlOrder, TilePosition(diffX, diffY));
            }

            // Flip them vertically / horizontally as needed
            const auto flipPositions = [&](auto& tryOrder, auto type) {
                if (flipVertical) {
                    wallOffset = TilePosition(wallOffset.x, iteration + hatchOffset);
                    for (auto &placement : tryOrder) {
                        auto diff = 3 - type.tileHeight();
                        placement.y = -(placement.y - diff);
                    }
                }
                if (flipHorizontal) {
                    wallOffset = TilePosition(iteration + hatchOffset, wallOffset.y);
                    for (auto &placement : tryOrder) {
                        auto diff = 4 - type.tileWidth();
                        placement.x = -(placement.x - diff);
                    }
                }
                if (!flipOrder)
                    std::reverse(tryOrder.begin(), tryOrder.end());
            };

            // Reverse order if these are "blocking" pieces to lengthen time to enter main
            opnOrder = smlOrder;
            std::reverse(opnOrder.begin(), opnOrder.end());

            // Check if positions need to be flipped vertically or horizontally
            flipPositions(smlOrder, Protoss_Pylon);
            flipPositions(medOrder, Protoss_Forge);
            flipPositions(lrgOrder, Protoss_Gateway);
            flipPositions(opnOrder, Protoss_Dragoon);

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                for (auto &building : rawBuildings) {
                    if (building.tileWidth() == 4)
                        tryLocations(lrgOrder, largeTiles, Zerg_Hatchery);
                    if (building.tileWidth() == 3)
                        tryLocations(medOrder, mediumTiles, Zerg_Evolution_Chamber);
                    if (building.tileWidth() == 2)
                        tryLocations(smlOrder, smallTiles, Zerg_Spire);
                }
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                std::reverse(lrgOrder.begin(), lrgOrder.end());
                tryLocations(medOrder, mediumTiles, Protoss_Forge);
                tryLocations(lrgOrder, largeTiles, Protoss_Gateway);
                tryLocations(opnOrder, smallTiles, Protoss_Dragoon); // Make sure we always have an opening before placing a Pylon
                tryLocations(smlOrder, smallTiles, Protoss_Pylon);
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                tryLocations(lrgOrder, largeTiles, Terran_Barracks);
                tryLocations(medOrder, mediumTiles, Terran_Bunker);
            }

            // If iteration was negative, we prevent moving the defenses backwards by resetting the walloffset here
            if (iteration < 0)
                wallOffset = TilePosition(0, 0);

            if ((getSmallTiles().size() + getMediumTiles().size() + getLargeTiles().size()) == getRawBuildings().size())
                break;
        }

        // Find remaining openings
        while (tryLocations(opnOrder, smallTiles, Protoss_Dragoon)) {}
    }

    void Wall::addDefenses()
    {
        auto left = Position(choke->Center()).x < base->Center().x;
        auto up = Position(choke->Center()).y < base->Center().y;
        auto baseDist = base->Center().getDistance(Position(choke->Center()));

        map<int, vector<TilePosition>> wallPlacements;
        auto flipHorizontal = false;
        auto flipVertical = false;
        if (defenseArrangement == 0) { // 0/8 - Horizontal
            wallPlacements[1] ={ {-4, -2}, {-2, -2}, {0, -2}, {2, -2}, {4, -2}, {6, -2} };
            wallPlacements[2] ={ {-4, 0}, {-3, 0} , {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0} };
            flipVertical = base->Center().y < Position(choke->Center()).y;
        }
        else if (defenseArrangement == 1 || defenseArrangement == 3) { // pi/4 - Angled
            wallPlacements[1] ={ {-4, 2}, {-2, 0}, {0, -2}, {2, -4}, {4, -6} };
            wallPlacements[2] ={ {-4, 4}, {-2, 2}, {0, 0}, {2, -2}, {4, -4} };
            flipVertical    = base->Center().y < Position(choke->Center()).y;
            flipHorizontal  = base->Center().x < Position(choke->Center()).x;
        }
        else if (defenseArrangement == 2) {  // pi/2 - Vertical
            wallPlacements[1] ={ {-2, 4}, {-2, 2}, {-2, 0}, {-2, -2}, {-2, -4} };
            wallPlacements[2] ={ {0, 5}, {0, 4}, {0, 3}, {0, 2}, {0, 1}, {0, 0}, {0, -1}, {0, -2}, {0, -3}, {0, -4} };
            flipHorizontal  = base->Center().x < Position(choke->Center()).x;
        }

        // Flip them vertically / horizontally as needed
        if (base->Center().y < Position(choke->Center()).y) {
            for (int i = 1; i <= 4; i++) {
                for (auto &placement : wallPlacements[i])
                    placement.y = -(placement.y - 1);
            }
        }
        if (base->Center().x < Position(choke->Center()).x) {
            for (int i = 1; i <= 4; i++) {
                for (auto &placement : wallPlacements[i])
                    placement.x = -(placement.x - 2);
            }
        }

        auto defenseType = UnitTypes::None;
        if (Broodwar->self()->getRace() == Races::Protoss)
            defenseType = UnitTypes::Protoss_Photon_Cannon;
        if (Broodwar->self()->getRace() == Races::Terran)
            defenseType = UnitTypes::Terran_Missile_Turret;
        if (Broodwar->self()->getRace() == Races::Zerg)
            defenseType = UnitTypes::Zerg_Creep_Colony;

        // Add station defenses to the set
        for (auto &tile : station->getDefenses()) {
            auto center = Position(tile) + Position(16, 16);
            auto idx = 3;
            if (center.getDistance(Position(choke->Center())) < baseDist)
                idx = 2;
            if (find(wallPlacements[1].begin(), wallPlacements[1].end(), tile - base->Location() - wallOffset) != wallPlacements[1].end())
                idx = 1;

            defenses[idx].insert(tile);
            defenses[0].insert(tile);
        }

        // Add wall defenses to the set
        for (auto &[i, placements] : wallPlacements) {
            for (auto &placement : placements) {
                auto tile = base->Location() + placement + wallOffset;

                if (Map::isPlaceable(defenseType, tile) && !Map::isReserved(tile, 2, 2)) {
                    defenses[i].insert(tile);
                    Map::addUsed(tile, defenseType);
                    defenses[0].insert(tile);
                }
            }
        }
    }

    void Wall::cleanup()
    {
        // Remove used from tiles
        for (auto &tile : smallTiles)
            Map::removeUsed(tile, 2, 2);
        for (auto &tile : mediumTiles)
            Map::removeUsed(tile, 3, 2);
        for (auto &tile : largeTiles)
            Map::removeUsed(tile, 4, 3);
        for (auto &tile : openings)
            Map::removeUsed(tile, 1, 1);
        for (auto &[_, tiles] : defenses) {
            for (auto &tile : tiles)
                Map::removeUsed(tile, 2, 2);
        }
    }

    const int Wall::getGroundDefenseCount() const
    {
        // Returns how many visible ground defensive structures exist in this Walls defense locations
        int count = 0;
        if (defenses.find(0) != defenses.end()) {
            for (auto &tile : defenses.at(0)) {
                auto type = Map::isUsed(tile);
                if (type == UnitTypes::Protoss_Photon_Cannon
                    || type == UnitTypes::Zerg_Sunken_Colony
                    || type == UnitTypes::Terran_Bunker)
                    count++;
            }
        }
        return count;
    }

    const int Wall::getAirDefenseCount() const
    {
        // Returns how many visible air defensive structures exist in this Walls defense locations
        int count = 0;
        if (defenses.find(0) != defenses.end()) {
            for (auto &tile : defenses.at(0)) {
                auto type = Map::isUsed(tile);
                if (type == UnitTypes::Protoss_Photon_Cannon
                    || type == UnitTypes::Zerg_Spore_Colony
                    || type == UnitTypes::Terran_Missile_Turret)
                    count++;
            }
        }
        return count;
    }

    const void Wall::draw()
    {
        set<Position> anglePositions;
        int color = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        if (station) {
            auto dist = Position(choke->Center()).getDistance(station->getBase()->Center());
            Broodwar->drawTextMap(station->getBase()->Center(), "%.2f", dist);
            //Broodwar << wallOffset << endl;
        }

        // Draw boxes around each feature
        auto drawBoxes = true;
        if (drawBoxes) {
            for (auto &tile : smallTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
            }
            for (auto &tile : mediumTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
            }
            for (auto &tile : largeTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
            }
            for (auto &tile : openings) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(33, 33), color, true);
            }
            for (int i = 1; i <= 4; i++) {
                for (auto &tile : defenses[i]) {
                    Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
                    Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW %d", textColor, i);
                }
            }
        }

        // Draw the line and angle of the ChokePoint
        auto p1 = choke->Pos(choke->end1);
        auto p2 = choke->Pos(choke->end2);
        Broodwar->drawTextMap(Position(choke->Center()), "%c%.2f", Text::Grey, chokeAngle);
        Broodwar->drawLineMap(Position(p1), Position(p2), Colors::Grey);
    }
}

namespace BWEB::Walls {

    const Wall * const createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, const UnitType tightType, const vector<UnitType>& defenses, const bool openWall, const bool requireTight)
    {
        ofstream writeFile;
        string buffer;
        auto timePointNow = chrono::system_clock::now();
        auto timeNow = chrono::system_clock::to_time_t(timePointNow);

        // Print the clock position of this Wall
        auto clock = (round((Map::getAngle(make_pair(Position(area->Top()), Map::mapBWEM.Center())) - M_PI_D2) / 0.52));
        if (clock < 0)
            clock+=12;

        // Open the log file if desired and write information
        if (logInfo) {
            writeFile.open("bwapi-data/write/BWEB_Wall.txt");
            writeFile << ctime(&timeNow);
            writeFile << Broodwar->mapFileName().c_str() << endl;
            writeFile << "At: " << clock << " o'clock." << endl;
            writeFile << endl;

            writeFile << "Buildings:" << endl;
            for (auto &building : buildings)
                writeFile << building.c_str() << endl;
            writeFile << endl;
        }

        // Verify inputs are correct
        if (!area) {
            writeFile << "BWEB: Can't create a wall without a valid BWEM::Area" << endl;
            return nullptr;
        }

        if (!choke) {
            writeFile << "BWEB: Can't create a wall without a valid BWEM::Chokepoint" << endl;
            return nullptr;
        }

        // Verify not attempting to create a Wall in the same Area/ChokePoint combination
        for (auto &[_, wall] : walls) {
            if (wall.getChokePoint() == choke) {
                writeFile << "BWEB: Can't create a Wall where one already exists." << endl;
                return &wall;
            }
        }

        // Create a Wall
        Wall wall(area, choke, buildings, defenses, tightType, requireTight, openWall);

        // Verify the Wall creation was successful
        auto wallFound = (wall.getSmallTiles().size() + wall.getMediumTiles().size() + wall.getLargeTiles().size()) == wall.getRawBuildings().size();
        if (wallFound) {
            for (auto &tile : wall.getSmallTiles())
                Map::addReserve(tile, 2, 2);
            for (auto &tile : wall.getMediumTiles())
                Map::addReserve(tile, 3, 2);
            for (auto &tile : wall.getLargeTiles())
                Map::addReserve(tile, 4, 3);
            for (auto &tile : wall.getOpenings())
                Map::addReserve(tile, 1, 1);
            for (auto &tile : wall.getDefenses())
                Map::addReserve(tile, 2, 2);
        }

        // Log information
        if (logInfo) {
            double dur = std::chrono::duration <double, std::milli>(chrono::system_clock::now() - timePointNow).count();
            writeFile << "Generation Time: " << dur << "ms and " << (wallFound ? "successful." : "failed.") << endl;
            writeFile << "--------------------" << endl;
        }

        // If we found a suitable Wall, push into container and return pointer to it
        if (wallFound) {
            walls.emplace(choke, wall);
            return &walls.at(choke);
        }
        return nullptr;
    }

    const Wall * const createProtossWall()
    {
        vector<UnitType> buildings ={ Protoss_Forge, Protoss_Gateway, Protoss_Pylon };
        return createWall(buildings,
            BWEB::Stations::getStartingNatural()->getBase()->GetArea(),
            BWEB::Stations::getStartingNatural()->getChokepoint(),
            UnitTypes::None,
            { Protoss_Photon_Cannon },
            true,
            false);
    }

    const Wall * const createTerranWall()
    {
        vector<UnitType> buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
        auto tightType = Broodwar->enemy()->getRace() == Races::Zerg ? Zerg_Zergling : Protoss_Zealot;
        return createWall(buildings,
            BWEB::Stations::getStartingNatural()->getBase()->GetArea(),
            BWEB::Stations::getStartingNatural()->getChokepoint(),
            tightType,
            { Terran_Missile_Turret },
            false,
            true);
    }

    const Wall * const createZergWall()
    {
        vector<UnitType> buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber };
        return createWall(buildings,
            BWEB::Stations::getStartingNatural()->getBase()->GetArea(),
            BWEB::Stations::getStartingNatural()->getChokepoint(),
            UnitTypes::None,
            { Zerg_Sunken_Colony },
            true,
            false);
    }

    const Wall * const getWall(const BWEM::ChokePoint * choke)
    {
        if (!choke)
            return nullptr;

        for (auto &[_, wall] : walls) {
            if (wall.getChokePoint() == choke)
                return &wall;
        }
        return nullptr;
    }

    map<const BWEM::ChokePoint * const, Wall>& getWalls() {
        return walls;
    }

    void draw()
    {
        for (auto &[tile, color] : testTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(32, 32), color);
        }
        for (auto &[_, wall] : walls)
            wall.draw();
    }
}
