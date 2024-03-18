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

        vector<TilePosition> testTiles;
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
        // Defense types place every possible tile
        if (type.tileWidth() == 2 && type.tileHeight() == 2 && type != Protoss_Pylon) {
            for (auto placement : tryOrder) {
                auto tile = station->getBase()->Location() + placement;
                auto stationDefense = type.tileHeight() == 2 && type.tileHeight() == 2 && station->getDefenses().find(tile) != station->getDefenses().end();
                if ((!Map::isReserved(tile, 2, 2) && BWEB::Map::isPlaceable(type, tile)) || stationDefense) {
                    insertList.insert(tile);
                    Map::addReserve(tile, type.tileWidth(), type.tileHeight());
                    Map::addUsed(tile, type);
                }
            }
        }

        // Blocking space for movement
        else if (!type.isBuilding()) {
            for (auto placement : tryOrder) {
                auto tile = station->getBase()->Location() + placement;
                if (BWEB::Map::isPlaceable(type, tile)) {
                    openings.insert(tile);
                    Map::addReserve(tile, type.tileWidth(), type.tileHeight());
                    Map::addUsed(tile, type);
                    return true;
                }
            }
        }

        // Other types only need 1 unique spot
        else {
            for (auto placement : tryOrder) {
                auto tile = station->getBase()->Location() + placement;
                testTiles.push_back(tile);
                if (BWEB::Map::isPlaceable(type, tile)) {
                    insertList.insert(tile);
                    Map::addReserve(tile, type.tileWidth(), type.tileHeight());
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
                Map::addReserve(tileBest, type.tileWidth(), type.tileHeight());
                Map::addUsed(tileBest, type);
                return true;
            }
        }
        return false;
    }

    void Wall::addPieces()
    {
        // If not adding any buildings to the wall
        if (rawBuildings.empty()) {
            Broodwar << "Cant generate a wall without buildings" << endl;
            return;
        }

        // For each piece, try to place it a known distance away depending on how the angles of chokes look     
        auto closestMain = Stations::getClosestMainStation(station->getBase()->Location());
        auto mainChokeCenter = Position(closestMain->getChokepoint()->Center());
        vector<TilePosition> smlOrder, medOrder, lrgOrder, opnOrder;
        auto flipOrder = false; // TODO: Right now we flip the opposite way so we always can get out            
        auto flipHorizontal = false;
        auto flipVertical = false;
        defenseArrangement = int(round(chokeAngle / 0.785)) % 4;

        const auto adjustOrder = [&](auto& order, auto diff) {
            for (auto &tile : order)
                tile = tile + diff;
        };

        // 0/8 - Horizontal
        if (defenseArrangement == 0) {
            lrgOrder        ={ {-4, -6}, {-3, -6}, {-2, -6}, {-1, -6}, {0, -6}, {1, -6}, {2, -6}, {3, -6}, {4, -6} };
            medOrder        ={ {-4, -5}, {-3, -5}, {-2, -5}, {-1, -5}, {0, -5}, {1, -5}, {2, -5}, {3, -5}, {4, -5} };
            smlOrder        ={ {-2, -5}, {-1, -5}, { 0, -5}, { 1, -5}, {2, -5}, {3, -5}, {4, -5}, {5, -5} };
            flipOrder       = mainChokeCenter.x > base->Center().x;
            flipVertical    = base->Center().y < Position(choke->Center()).y;

            // Shift positions based on chokepoint offset
            auto diffX = TilePosition(choke->Center()).x - base->Location().x - 2;
            adjustOrder(lrgOrder, TilePosition(diffX, 0));
            adjustOrder(medOrder, TilePosition(diffX, 0));
            adjustOrder(smlOrder, TilePosition(diffX, 0));
        }

        // pi/4 - Angled
        else if (defenseArrangement == 1 || defenseArrangement == 3) {
            lrgOrder        ={ {-1, -8}, {-3, -6}, {-5, -4}, {-7, -2}, {-9,  0} };
            medOrder        ={ { 2, -9}, { 0, -7}, {-2, -5}, {-4, -3}, {-6, -1}, {-8, 1} };

            // TODO: these flips don't really work as intended
            flipOrder       = (mainChokeCenter.x < base->Center().x && mainChokeCenter.y > base->Center().y) || (mainChokeCenter.x > base->Center().x && mainChokeCenter.y < base->Center().y);
            flipVertical    = base->Center().y < Position(choke->Center()).y;
            flipHorizontal  = base->Center().x < Position(choke->Center()).x;
        }

        // pi/2 - Vertical
        else if (defenseArrangement == 2) {
            lrgOrder        ={ {-7, -4}, {-7, -3}, {-7, -2}, {-7, -1}, {-7,  0}, {-7,  1}, {-7,  2}, {-7,  3}, {-7,  4} };
            medOrder        ={ {-6, -3}, {-6, -2}, {-6, -1}, {-6,  0}, {-6,  1}, {-6,  2}, {-6,  3}, {-6,  4} };
            smlOrder        ={ {-5, -1}, {-5,  0}, {-5,  1}, {-5,  2} };
            flipOrder       = mainChokeCenter.y > base->Center().y;
            flipHorizontal  = base->Center().x < Position(choke->Center()).x;

            // Shift positions based on chokepoint offset
            auto diffY = TilePosition(choke->Center()).y - base->Location().y - 1;
            adjustOrder(lrgOrder, TilePosition(0, diffY));
            adjustOrder(medOrder, TilePosition(0, diffY));
            adjustOrder(smlOrder, TilePosition(0, diffY));
        }

        // Flip them vertically / horizontally as needed
        const auto flipPositions = [&](auto& tryOrder, auto type) {
            if (flipVertical) {
                for (auto &placement : tryOrder) {
                    auto diff = 3 - type.tileHeight();
                    placement.y = -(placement.y - diff);
                }
            }
            if (flipHorizontal) {
                for (auto &placement : tryOrder) {
                    auto diff = 4 - type.tileWidth();
                    placement.x = -(placement.x - diff);
                }
            }
            if (flipOrder)
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
            tryLocations(medOrder, mediumTiles, Zerg_Evolution_Chamber);
            tryLocations(lrgOrder, largeTiles, Zerg_Hatchery);
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

        // Find remaining openings
        while (tryLocations(opnOrder, smallTiles, Protoss_Dragoon)) {}
    }

    void Wall::addDefenses()
    {
        if (rawDefenses.empty() || !valid)
            return;

        auto left = Position(choke->Center()).x < base->Center().x;
        auto up = Position(choke->Center()).y < base->Center().y;
        auto type = (rawDefenses.front());
        auto baseDist = base->Center().getDistance(Position(choke->Center()));

        map<int, vector<TilePosition>> wallPlacements;
        auto flipHorizontal = false;
        auto flipVertical = false;
        if (defenseArrangement == 0) { // 0/8 - Horizontal
            wallPlacements[1] ={ {-4, -2}, {-2, -2}, {0, -2}, {2, -2}, {4, -2}, {6, -2} };
            wallPlacements[2] ={ {-4, 0}, {-2, 0}, {4, 0}, {6, 0} };
            flipVertical = base->Center().y < Position(choke->Center()).y;
        }
        else if (defenseArrangement == 1 || defenseArrangement == 3) { // pi/4 - Angled
            wallPlacements[1] ={ {-4, 2}, {-2, 0}, {0, -2}, {2, -4}, {4, -6} };
            wallPlacements[2] ={ {-4, 4}, {-2, 2}, {2, -2}, {4, -4} };
            flipVertical    = base->Center().y < Position(choke->Center()).y;
            flipHorizontal  = base->Center().x < Position(choke->Center()).x;
        }
        else if (defenseArrangement == 2) {  // pi/2 - Vertical
            wallPlacements[1] ={ {-2, 4}, {-2, 2}, {-2, 0}, {-2, -2}, {-2, -4} };
            wallPlacements[2] ={ {0, 5}, {0, 3}, {0, -2}, {0, -4} };
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

        // Add wall defenses to the set
        for (auto &[i, placements] : wallPlacements) {
            for (auto &placement : placements) {
                auto tile = base->Location() + placement;
                auto stationDefense = station->getDefenses().find(tile) != station->getDefenses().end();

                // TODO: Add defenses other than station defenses, right now adding extra will block paths
                if (/*(!Map::isReserved(tile, 2, 2) && Map::isPlaceable(defenseType, tile)) || */stationDefense) {
                    defenses[i].insert(tile);
                    Map::addReserve(tile, 2, 2);
                    Map::addUsed(tile, defenseType);
                    defenses[0].insert(tile);
                }
            }
        }

        // Add station defenses to the set
        for (auto &defense : station->getDefenses())
            defenses[0].insert(defense);
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

        // Draw boxes around each feature
        auto drawBoxes = true;
        if (drawBoxes) {
            for (auto &tile : smallTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
                anglePositions.insert(Position(tile) + Position(32, 32));
            }
            for (auto &tile : mediumTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
                anglePositions.insert(Position(tile) + Position(48, 32));
                //tightCheck(UnitTypes::Terran_Supply_Depot, tile, true);
            }
            for (auto &tile : largeTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
                anglePositions.insert(Position(tile) + Position(64, 48));
                //tightCheck(UnitTypes::Terran_Barracks, tile, true);
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

        // Draw angles of each piece
        auto drawAngles = false;
        if (drawAngles) {
            for (auto &pos1 : anglePositions) {
                for (auto &pos2 : anglePositions) {
                    if (pos1 == pos2)
                        continue;
                    const auto angle = Map::getAngle(make_pair(pos1, pos2));

                    Broodwar->drawLineMap(pos1, pos2, color);
                    Broodwar->drawTextMap((pos1 + pos2) / 2, "%c%.2f", textColor, angle);
                }
            }
        }

        // Draw the line and angle of the ChokePoint
        auto p1 = choke->Pos(choke->end1);
        auto p2 = choke->Pos(choke->end2);
        auto angle = Map::getAngle(make_pair(p1, p2));
        Broodwar->drawTextMap(Position(choke->Center()), "%c%.2f", Text::Grey, angle);
        Broodwar->drawLineMap(Position(p1), Position(p2), Colors::Grey);
        Broodwar->drawTextMap(Position(choke->Center()), "%d", defenseArrangement);
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
        auto clock = (round((Map::getAngle(make_pair(Position(area->Top()), Map::mapBWEM.Center())) - 1.57) / 0.52));
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
            if (wall.getArea() == area && wall.getChokePoint() == choke) {
                writeFile << "BWEB: Can't create a Wall where one already exists." << endl;
                return &wall;
            }
        }

        // Create a Wall
        Wall wall(area, choke, buildings, defenses, tightType, requireTight, openWall);

        // Verify the Wall creation was successful
        auto wallFound = (wall.getSmallTiles().size() + wall.getMediumTiles().size() + wall.getLargeTiles().size()) == wall.getRawBuildings().size();

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
        for (auto tile : testTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(32, 32), Colors::White);
        }
        for (auto &[_, wall] : walls)
            wall.draw();
    }
}
