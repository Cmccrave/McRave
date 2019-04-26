#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB::Walls
{
    namespace {
        vector<Wall> walls;
        TilePosition initialStart, initialEnd, startTile, endTile;

        double bestWallScore = 0.0;
        vector<UnitType>::iterator typeIterator;
        map<TilePosition, UnitType> bestWall;
        map<TilePosition, UnitType> currentWall;

        UnitType tight;
        bool reservePath;
        bool requireTight;
        set<TilePosition> chokeTiles;
        Position test;

        int failedPlacement = 0;
        int failedAngle = 0;
        int failedPath = 0;
        int failedTight = 0;
        int failedSpawn = 0;

        void checkPathPoints(Wall& wall)
        {
            auto distBest = DBL_MAX;
            startTile = initialStart;
            endTile = initialEnd;

            if (initialStart.isValid() && (!Map::isWalkable(initialStart) || overlapsCurrentWall(initialStart) != UnitTypes::None || Map::isOverlapping(endTile) != 0)) {
                for (auto x = initialStart.x - 2; x < initialStart.x + 2; x++) {
                    for (auto y = initialStart.y - 2; y < initialStart.y + 2; y++) {
                        TilePosition t(x, y);
                        const auto dist = t.getDistance(endTile);
                        if (overlapsCurrentWall(t) != UnitTypes::None || !Map::isWalkable(t))
                            continue;

                        if (Map::mapBWEM.GetArea(t) == wall.getArea() && dist < distBest) {
                            startTile = t;
                            distBest = dist;
                        }
                    }
                }
            }

            distBest = 0.0;
            if (initialEnd.isValid() && (!Map::isWalkable(initialEnd) || overlapsCurrentWall(initialEnd) != UnitTypes::None || Map::isOverlapping(endTile) != 0)) {
                for (auto x = initialEnd.x - 4; x < initialEnd.x + 4; x++) {
                    for (auto y = initialEnd.y - 4; y < initialEnd.y + 4; y++) {
                        TilePosition t(x, y);
                        const auto dist = t.getDistance(startTile);
                        if (overlapsCurrentWall(t) != UnitTypes::None || !Map::isWalkable(t))
                            continue;

                        if (Map::mapBWEM.GetArea(t) && dist > distBest) {
                            endTile = t;
                            distBest = dist;
                        }
                    }
                }
            }
        }

        void initializePathPoints(Wall& wall)
        {
            auto choke = wall.getChokePoint();
            auto line = Map::lineOfBestFit(choke);
            auto n1 = line.first;
            auto n2 = line.second;
            auto dist = n1.getDistance(n2);
            auto dx1 = int((n2.x - n1.x) * 96.0 / dist);
            auto dy1 = int((n2.y - n1.y) * 96.0 / dist);
            auto dx2 = int((n1.x - n2.x) * 96.0 / dist);
            auto dy2 = int((n1.y - n2.y) * 96.0 / dist);
            auto direction1 = Position(-dy1, dx1) + Map::pConvert(choke->Center());
            auto direction2 = Position(-dy2, dx2) + Map::pConvert(choke->Center());
            auto trueDirection = !direction1.isValid() || Map::mapBWEM.GetArea(Map::tConvert(direction1)) == wall.getArea() ? direction2 : direction1;

            if (choke == Map::getNaturalChoke()) {
                initialStart = Map::tConvert(Map::getMainChoke()->Center());
                initialEnd = Map::tConvert(trueDirection);
            }
            else if (choke == Map::getMainChoke()) {
                initialStart = (Map::getMainTile() + Map::tConvert(Map::getMainChoke()->Center())) / 2;
                initialEnd = Map::tConvert(Map::getNaturalChoke()->Center());
            }
            else {
                initialStart = Map::tConvert(wall.getArea()->Top());
                initialEnd = Map::tConvert(trueDirection);
            }

            startTile = initialStart;
            endTile = initialEnd;
        }

        Path findOpeningInWall(Wall& wall, bool ignoreOverlap)
        {
            // Check that the path points are possible to reach
            checkPathPoints(wall);
            auto startCenter = Map::pConvert(startTile) + Position(16, 16);
            auto endCenter = Map::pConvert(endTile) + Position(16, 16);

            // Get a new path
            BWEB::Path newPath;
            newPath.createWallPath(currentWall, startCenter, endCenter, ignoreOverlap, false);
            return newPath;
        }

        bool isPoweringWall(Wall& wall, const TilePosition here)
        {
            for (auto &piece : currentWall) {
                const auto tile = piece.first;
                const auto type = piece.second;

                if (type == UnitTypes::Protoss_Pylon)
                    continue;

                if (type.tileWidth() == 4) {
                    auto powersThis = false;
                    if (tile.y - here.y == -5 || tile.y - here.y == 4) {
                        if (tile.x - here.x >= -4 && tile.x - here.x <= 1) powersThis = true;
                    }
                    if (tile.y - here.y == -4 || tile.y - here.y == 3) {
                        if (tile.x - here.x >= -7 && tile.x - here.x <= 4) powersThis = true;
                    }
                    if (tile.y - here.y == -3 || tile.y - here.y == 2) {
                        if (tile.x - here.x >= -8 && tile.x - here.x <= 5) powersThis = true;
                    }
                    if (tile.y - here.y >= -2 && tile.y - here.y <= 1) {
                        if (tile.x - here.x >= -8 && tile.x - here.x <= 6) powersThis = true;
                    }
                    if (!powersThis) return false;
                }
                else {
                    auto powersThis = false;
                    if (tile.y - here.y == 4) {
                        if (tile.x - here.x >= -3 && tile.x - here.x <= 2) powersThis = true;
                    }
                    if (tile.y - here.y == -4 || tile.y - here.y == 3) {
                        if (tile.x - here.x >= -6 && tile.x - here.x <= 5) powersThis = true;
                    }
                    if (tile.y - here.y >= -3 && tile.y - here.y <= 2) {
                        if (tile.x - here.x >= -7 && tile.x - here.x <= 6) powersThis = true;
                    }
                    if (!powersThis) return false;
                }
            }
            return true;
        }

        bool isWallTight(Wall& wall, UnitType building, const TilePosition here)
        {
            // Some constants
            const auto height = building.tileHeight() * 4;
            const auto width = building.tileWidth() * 4;
            const auto vertTight = (tight == UnitTypes::None) ? 32 : tight.height();
            const auto horizTight = (tight == UnitTypes::None) ? 32 : tight.width();

            // Checks each side of the building to see if it is valid for walling purposes
            const auto checkLeft = (building.tileWidth() * 16) - building.dimensionLeft() < horizTight;
            const auto checkRight = (building.tileWidth() * 16) - building.dimensionRight() - 1 < horizTight;
            const auto checkUp =  (building.tileHeight() * 16) - building.dimensionUp() < vertTight;
            const auto checkDown =  (building.tileHeight() * 16) - building.dimensionDown() - 1 < vertTight;

            // HACK: I don't have a great method for buildings that can check multiple tiles for Terrain tight, hardcoded a few as shown below
            const auto right = Map::wConvert(here) + WalkPosition(width, 0) + WalkPosition(building == UnitTypes::Terran_Barracks, 0);
            const auto left =  Map::wConvert(here) - WalkPosition(1, 0);
            const auto up =  Map::wConvert(here) - WalkPosition(0, 1);
            const auto down =  Map::wConvert(here) + WalkPosition(0, height) + WalkPosition(0, building == UnitTypes::Protoss_Gateway || building == UnitTypes::Terran_Supply_Depot);

            // Some testing parameters
            auto firstBuilding = currentWall.size() == 0;
            auto lastBuilding = currentWall.size() == (wall.getRawBuildings().size() - 1);
            auto terrainTight = false;
            auto parentTight = false;

            // Functions for each dimension check
            const auto fRight = [&](UnitType parent) {
                return (parent.tileWidth() * 16 - parent.dimensionLeft()) + (building.tileWidth() * 16 - building.dimensionRight() - 1);
            };
            const auto fLeft = [&](UnitType parent) {
                return (parent.tileWidth() * 16 - parent.dimensionRight() - 1) + (building.tileWidth() * 16 - building.dimensionLeft());
            };
            const auto fUp = [&](UnitType parent) {
                return (parent.tileHeight() * 16 - parent.dimensionDown() - 1) + (building.tileHeight() * 16 - building.dimensionUp());
            };
            const auto fDown = [&](UnitType parent) {
                return (parent.tileHeight() * 16 - parent.dimensionUp()) + (building.tileHeight() * 16 - building.dimensionDown() - 1);
            };

            // Function to check if it's terrain tight here
            const auto terrainTightCheck = [&](WalkPosition w, bool check) {
                auto t = Map::tConvert(w);

                // If the walkposition is invalid or unwalkable
                if (tight != UnitTypes::None && check && (!w.isValid() || !Broodwar->isWalkable(w)))
                    return true;

                // If the tile is touching some resources
                if (Map::isOverlapping(t))
                    return true;

                // If we don't care about walling tight and the tile isn't walkable
                if (!requireTight && !Map::isWalkable(t))
                    return true;
                return false;
            };

            // Functions to iterate each WalkPosition of a side
            const auto checkVerticalSide = [&](WalkPosition start, int length, bool check, const auto fDiff, int tightnessFactor) {
                for (auto x = start.x; x < start.x + length; x++) {
                    WalkPosition w(x, start.y);
                    auto t = Map::tConvert(w);
                    auto parent = overlapsCurrentWall(t);
                    auto parentTightCheck = parent != UnitTypes::None ? fDiff(parent) < tightnessFactor : false;

                    // Check if it's tight with the terrain
                    if (!terrainTight && terrainTightCheck(w, check))
                        terrainTight = true;

                    // Check if it's tight with a parent
                    if (!parentTight && parentTightCheck)
                        parentTight = true;
                }
            };

            const auto checkHorizontalSide = [&](WalkPosition start, int length, bool check, const auto fDiff, int tightnessFactor) {
                for (auto y = start.y; y < start.y + length; y++) {
                    WalkPosition w(start.x, y);
                    auto t = Map::tConvert(w);
                    auto parent = overlapsCurrentWall(t);
                    auto parentTightCheck = parent != UnitTypes::None ? fDiff(parent) < tightnessFactor : false;

                    // Check if it's tight with the terrain
                    if (!terrainTight && terrainTightCheck(w, check))
                        terrainTight = true;

                    // Check if it's tight with a parent
                    if (!parentTight && parentTightCheck)
                        parentTight = true;
                }
            };

            /* What this does:
            1) For each side, check if it's terrain tight
            2) For each side, check if it's tight with any parent buildings beside it
            3) If we want a tight wall, check that the final piece is terrain tight and parent tight*/
            checkVerticalSide(up, width, checkUp, fUp, vertTight);
            checkVerticalSide(down, width, checkDown, fDown, vertTight);
            checkHorizontalSide(left, height, checkLeft, fLeft, horizTight);
            checkHorizontalSide(right, height, checkRight, fRight, horizTight);

            // If we don't want a reserve path, we need all buildings to be tight at the tightness resolution...
            if (!reservePath) {
                if (!lastBuilding && !firstBuilding)	// ...to the parent
                    return parentTight;
                if (firstBuilding)						// ...to the terrain
                    return terrainTight;
                if (lastBuilding)						// ...to the parent and terrain
                    return (terrainTight && parentTight);
            }

            // If we do want a reserve path, we need this building to be tight at tile resolution to a parent or terrain
            else if (reservePath)
                return (terrainTight || parentTight);
            return false;
        }

        Position closestChokeTile(Position here) {
            auto best = DBL_MAX;
            Position posBest;
            for (auto &tile : chokeTiles) {
                auto p = Map::pConvert(tile) + Position(16, 16);
                if (p.getDistance(here) < best) {
                    posBest = p;
                    best = p.getDistance(here);
                }
            }
            return posBest;
        }

        bool iteratePieces(Wall& wall)
        {
            auto start = Map::tConvert(wall.getChokePoint()->Center());
            auto movedStart = false;
            auto multiplePylons = false;

            // Choke angle
            auto line = Map::lineOfBestFit(wall.getChokePoint());
            auto p1 = line.first;
            auto p2 = line.second;
            auto dy = abs(double(p1.y - p2.y));
            auto dx = abs(double(p1.x - p2.x));
            auto angle1 = dx > 0.0 ? atan(dy / dx) * 180.0 / 3.14 : 90.0;

            // Check if we are placing more Pylons to figure out how to calculate placement
            auto count = 0;
            for (auto type : wall.getRawBuildings()) {
                if (type == UnitTypes::Protoss_Pylon)
                    count++;
            }
            multiplePylons = count > 1;

            // Sort all the pieces and iterate over them to find the best wall - by Hannes
            if (find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon) != wall.getRawBuildings().end()) {
                sort(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), [](UnitType l, UnitType r) { return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
                sort(wall.getRawBuildings().begin(), find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons
            }
            else
                sort(wall.getRawBuildings().begin(), wall.getRawBuildings().end());

            // If the start position isn't buildable, move closer to the middle of the area until it is
            while (!Broodwar->isBuildable(start)) {
                auto distBest = DBL_MAX;
                auto test = start;
                for (int x = test.x - 1; x <= test.x + 1; x++) {
                    for (int y = test.y - 1; y <= test.y + 1; y++) {
                        TilePosition t(x, y);
                        if (!t.isValid())
                            continue;

                        auto p = Map::pConvert(t);
                        auto top = Map::pConvert(wall.getArea()->Top());
                        auto dist = p.getDistance(top);

                        if (dist < distBest) {
                            distBest = dist;
                            start = t;
                            movedStart = true;
                        }
                    }
                }
            }

            const auto scoreWall = [&] {

                // Create a path searching for an opening
                auto newPath = findOpeningInWall(wall, !reservePath);
                auto currentHole = TilePositions::None;

                // If we need an open wall and it's not reachable, or we require a closed wall and it is reachable
                if ((reservePath && !newPath.isReachable()) || (!reservePath && newPath.isReachable())) {
                    failedPath++;
                    return;
                }

                // If we need an opening
                for (auto &tile : newPath.getTiles()) {
                    double closestGeo = DBL_MAX;
                    for (auto &geo : wall.getChokePoint()->Geometry()) {
                        if (Map::tConvert(geo) == tile && tile.getDistance(startTile) < closestGeo && overlapsCurrentWall(tile) == UnitTypes::None) {
                            currentHole = tile;
                            closestGeo = tile.getDistance(startTile);
                        }
                    }
                }

                auto dist = 1.0;
                for (auto &piece : currentWall) {
                    auto tile = piece.first;
                    auto type = piece.second;
                    auto center = Map::pConvert(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);
                    auto chokeDist = closestChokeTile(center).getDistance(center);

                    dist += (!multiplePylons && type == UnitTypes::Protoss_Pylon) ? 1.0 / exp(chokeDist) : exp(chokeDist);
                }

                // Score wall based on path sizes and distances
                const auto score = !reservePath ? dist : currentHole.getDistance(startTile) * newPath.getDistance() / (dist);

                if (score > bestWallScore) {
                    bestWall = currentWall;
                    bestWallScore = score;
                }
            };

            const auto goodPlacement = [&](TilePosition t) {
                auto type = *typeIterator;
                auto p = Map::pConvert(t);

                if ((type == UnitTypes::Protoss_Pylon && !isPoweringWall(wall, t))
                    || overlapsCurrentWall(t, type.tileWidth(), type.tileHeight()) != UnitTypes::None
                    || Map::isOverlapping(t, type.tileWidth(), type.tileHeight(), true)
                    || !Map::isPlaceable(type, t)
                    || !Map::tilesWithinArea(wall.getArea(), t, type.tileWidth(), type.tileHeight()))
                    return false;
                return true;
            };

            const auto goodAngle = [&](Position center) {
                // We want to ensure the buildings are being placed at the correct angle compared to the chokepoint, within some tolerance
                auto angle2 = 0.0;
                auto badAngle = false;
                auto type = *typeIterator;

                if (!reservePath)
                    return true;

                if ((multiplePylons || type != UnitTypes::Protoss_Pylon) && !movedStart) {
                    for (auto piece : currentWall) {
                        auto tileB = piece.first;
                        auto typeB = piece.second;
                        auto centerB = Map::pConvert(tileB) + Position(typeB.tileWidth() * 16, typeB.tileHeight() * 16);
                        auto dy = abs(double(centerB.y - center.y));
                        auto dx = abs(double(centerB.x - center.x));

                        angle2 = dx > 0.0 ? atan(dy / dx) * 180.0 / 3.14 : 90.0;
                        if (abs(abs(angle1) - abs(angle2)) > 30.0)
                            badAngle = true;
                    }
                }
                return true;
            };

            const auto goodSpawn = [&]() {
                auto startCenter = Map::pConvert(startTile) + Position(16, 16);
                auto endCenter = Map::pConvert(endTile) + Position(16, 16);
                Position p;
                BWEB::Path pathHome, pathOut;

                // Check if we can lift the Barracks to get out
                if (Broodwar->self()->getRace() == Races::Terran) {
                    pathOut.createWallPath(currentWall, startCenter, endCenter, true, true);
                    if (!pathOut.isReachable())
                        return false;
                }

                for (auto[t, type] : currentWall) {

                    // If this type can't produce anything
                    if (!type.canProduce())
                        continue;

                    const auto checkHomeReachable = [&](TilePosition testTile) {
                        auto parent = overlapsCurrentWall(testTile);

                        if (Map::isWalkable(testTile) && overlapsCurrentWall(testTile) == UnitTypes::None)
                            return true;

                        // Look at moving the functions that check tightness so we can use them here
                        /*if (parent.dimensionUp() + type.dimensionDown() > tight)
                            return true;*/
                        return false;
                    };

                    // Check the bottom row for now
                    for (int x = 0; x < type.tileWidth(); x++) {
                        auto testTile = t + TilePosition(x, type.tileHeight());

                        if (checkHomeReachable(t)) {
                            p = Map::pConvert(t);
                            break;
                        }
                    }

                    //// Check if our units would spawn correctly
                    //pathHome.createWallPath(currentWall, p, startCenter, false, false);
                    //if (!pathHome.isReachable())
                    //    return false;
                }
                return true;
            };

            function<void(TilePosition)> recursiveCheck = [&](TilePosition start) -> void {
                auto radius = 10;
                auto type = *typeIterator;

                for (auto x = start.x - radius; x < start.x + radius; x++) {
                    for (auto y = start.y - radius; y < start.y + radius; y++) {
                        const TilePosition tile(x, y);
                        auto center = Map::pConvert(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);

                        if (!tile.isValid() || (multiplePylons && center.getDistance(closestChokeTile(center)) > 128.0))
                            continue;

                        if (!goodAngle(center)) {
                            failedAngle++;
                            continue;
                        }

                        if (!goodPlacement(tile)) {
                            failedPlacement++;
                            continue;
                        }

                        if (!isWallTight(wall, type, tile) && (multiplePylons || type != UnitTypes::Protoss_Pylon)) {
                            failedTight++;
                            continue;
                        }

                        // 1) Store the current type, increase the iterator
                        currentWall[tile] = type;
                        typeIterator++;

                        // 2) If at the end
                        if (typeIterator == wall.getRawBuildings().end()) {

                            // Check that units will spawn properly
                            if (!goodSpawn())
                                failedSpawn++;
                            else
                                scoreWall();
                        }
                        else
                            recursiveCheck(start);

                        // 3) Erase this current placement and repeat
                        if (typeIterator != wall.getRawBuildings().begin())
                            typeIterator--;
                        currentWall.erase(tile);

                    }
                }
            };

            // For each permutation, try to make a wall combination that is better than the current best
            do {
                currentWall.clear();
                typeIterator = wall.getRawBuildings().begin();
                recursiveCheck(start);
            } while (next_permutation(wall.getRawBuildings().begin(), find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon)));

            Broodwar << "Angle: " << failedAngle << endl;
            Broodwar << "Placement: " << failedPlacement << endl;
            Broodwar << "Tight: " << failedTight << endl;
            Broodwar << "Path: " << failedPath << endl;
            Broodwar << "Spawn: " << failedSpawn << endl;

            return !bestWall.empty();
        }
    }

    UnitType overlapsCurrentWall(const TilePosition here, const int width, const int height)
    {
        for (auto x = here.x; x < here.x + width; x++) {
            for (auto y = here.y; y < here.y + height; y++) {
                for (auto &placement : currentWall) {
                    const auto tile = placement.first;
                    if (x >= tile.x && x < tile.x + placement.second.tileWidth() && y >= tile.y && y < tile.y + placement.second.tileHeight())
                        return placement.second;
                }
            }
        }
        return UnitTypes::None;
    }

    void addToWall(UnitType building, Wall& wall, UnitType tight)
    {
        auto distance = DBL_MAX;
        auto tileBest = TilePositions::Invalid;
        auto start = Map::tConvert(wall.getCentroid());
        auto centroidDist = wall.getCentroid().getDistance(Map::pConvert(endTile));
        auto end = Map::pConvert(endTile);
        auto doorCenter = Map::pConvert(wall.getDoor()) + Position(16, 16);
        auto isDefense = building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Terran_Missile_Turret || building == UnitTypes::Terran_Bunker || building == UnitTypes::Zerg_Creep_Colony || building == UnitTypes::Zerg_Sunken_Colony || building == UnitTypes::Zerg_Creep_Colony;

        auto furthest = 0.0;
        // Find the furthest non pylon building to the chokepoint
        for (auto &tile : wall.getLargeTiles()) {
            Position p = Map::pConvert(tile) + Position(64, 48);
            auto dist = p.getDistance(Map::pConvert(wall.getChokePoint()->Center()));
            if (dist > furthest)
                furthest = dist;
        }
        for (auto &tile : wall.getMediumTiles()) {
            Position p = Map::pConvert(tile) + Position(48, 32);
            auto dist = p.getDistance(Map::pConvert(wall.getChokePoint()->Center()));
            if (dist > furthest)
                furthest = dist;
        }

        // Iterate around wall centroid to find a suitable position
        for (auto x = start.x - 8; x <= start.x + 8; x++) {
            for (auto y = start.y - 8; y <= start.y + 8; y++) {
                const TilePosition t(x, y);
                const auto center = Map::pConvert(t) + Position(32, 32);

                if (!t.isValid()
                    || Map::isOverlapping(t, building.tileWidth(), building.tileHeight())
                    || !Map::isPlaceable(building, t)
                    || Map::tilesWithinArea(wall.getArea(), t, 2, 2) == 0
                    || (isDefense && (center.getDistance(Map::pConvert(wall.getChokePoint()->Center())) < furthest || center.getDistance(doorCenter) < 96.0)))
                    continue;

                const auto dist = center.getDistance(doorCenter);

                if (dist < distance) {
                    tileBest = TilePosition(x, y);
                    distance = dist;
                }
            }
        }

        // If tile is valid, add to wall
        if (tileBest.isValid()) {
            currentWall[tileBest] = building;
            wall.insertDefense(tileBest);
            Map::addOverlap(tileBest, 2, 2);
        }
    }

    Wall * createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, const UnitType t, const vector<UnitType>& defenses, const bool rp, const bool rt)
    {
        // Reset our last calculations
        bestWallScore = 0.0;
        startTile = TilePositions::None;
        endTile = TilePositions::None;
        bestWall.clear();
        currentWall.clear();

        if (!area) {
            Broodwar << "BWEB: Can't create a wall without a valid BWEM::Area" << endl;
            return nullptr;
        }

        if (!choke) {
            Broodwar << "BWEB: Can't create a wall without a valid BWEM::Chokepoint" << endl;
            return nullptr;
        }

        if (buildings.empty()) {
            Broodwar << "BWEB: Can't create a wall with an empty vector of UnitTypes." << endl;
            return nullptr;
        }

        for (auto &wall : walls) {
            if (wall.getArea() == area && wall.getChokePoint() == choke) {
                Broodwar << "BWEB: Can't create a Wall where one already exists." << endl;
                return &wall;
            }
        }

        // Create a new wall object
        Wall wall(area, choke, buildings, defenses);

        // I got sick of passing the parameters everywhere, sue me
        tight = t;
        reservePath = rp;
        requireTight = rt;

        // Create a line of tiles that make up the geometry of the choke		
        for (auto &walk : wall.getChokePoint()->Geometry()) {
            auto t = Map::tConvert(walk);
            if (Broodwar->isBuildable(t))
                chokeTiles.insert(t);
        }

        const auto addDoor = [&](Path& currentPath) {
            // Check which tile is closest to each part on the path, set as door
            auto distBest = DBL_MAX;
            for (auto chokeTile : chokeTiles) {
                for (auto pathTile : currentPath.getTiles()) {
                    double dist = chokeTile.getDistance(pathTile);
                    if (dist < distBest) {
                        distBest = dist;
                        wall.setWallDoor(chokeTile);
                    }
                }
            }

            // If the line of tiles we made is empty, assign closest path tile to closest to wall centroid
            if (chokeTiles.empty()) {
                for (auto pathTile : currentPath.getTiles()) {
                    auto p = Map::pConvert(pathTile);
                    auto dist = wall.getCentroid().getDistance(p);
                    if (dist < distBest) {
                        distBest = dist;
                        wall.setWallDoor(pathTile);
                    }
                }
            }
        };

        // Set the walls reserve path if needed
        const auto addReservePath = [&](Path& currentPath) {
            if (reservePath) {
                for (auto &tile : currentPath.getTiles())
                    Map::addReserve(tile, 1, 1);
            }
        };

        // Set the walls centroid
        const auto addCentroid = [&] {
            auto centroid = Position(0, 0);
            auto sizeWall = int(currentWall.size());
            for (auto &piece : currentWall) {
                if (piece.second != UnitTypes::Protoss_Pylon)
                    centroid += Map::pConvert(piece.first) + Map::pConvert(piece.second.tileSize()) / 2;
                else
                    sizeWall--;
            }

            // Set a centroid if it's only a pylon wall
            if (sizeWall == 0) {
                sizeWall = currentWall.size();
                for (auto &piece : currentWall)
                    centroid += Map::pConvert(piece.first) + Map::pConvert(piece.second.tileSize()) / 2;
            }
            wall.setCentroid(centroid / sizeWall);
        };

        // Add wall defenses if requested
        const auto addDefenses = [&] {
            for (auto &building : wall.getRawDefenses())
                addToWall(building, wall, tight);
        };

        // Setup pathing parameters
        initializePathPoints(wall);

        // Iterate pieces, try to find best location
        if (iteratePieces(wall)) {
            for (auto &placement : bestWall) {
                wall.insertSegment(placement.first, placement.second);
                Map::addOverlap(placement.first, placement.second.tileWidth(), placement.second.tileHeight());
            }

            currentWall = bestWall;
            auto currentPath = findOpeningInWall(wall, false);

            addReservePath(currentPath);
            addCentroid();
            addDoor(currentPath);
            addDefenses();

            // Push wall into the vector
            walls.push_back(wall);
            return &walls.back();
        }
        return nullptr;
    }

    void createFFE()
    {
        vector<UnitType> buildings ={ UnitTypes::Protoss_Forge, UnitTypes::Protoss_Gateway, UnitTypes::Protoss_Pylon };
        vector<UnitType> defenses(6, UnitTypes::Protoss_Photon_Cannon);

        createWall(buildings, Map::getNaturalArea(), Map::getNaturalChoke(), UnitTypes::Zerg_Zergling, defenses, true, false);
    }

    void createZSimCity()
    {
        vector<UnitType> buildings ={ UnitTypes::Zerg_Hatchery, UnitTypes::Zerg_Evolution_Chamber, UnitTypes::Zerg_Evolution_Chamber };
        vector<UnitType> defenses(6, UnitTypes::Zerg_Sunken_Colony);

        createWall(buildings, Map::getNaturalArea(), Map::getNaturalChoke(), UnitTypes::None, defenses, true, false);
    }

    void createTWall()
    {
        vector<UnitType> buildings ={ UnitTypes::Terran_Supply_Depot, UnitTypes::Terran_Supply_Depot, UnitTypes::Terran_Barracks };
        vector<UnitType> defenses;
        auto type = UnitTypes::None;

        if (Broodwar->enemy())
            type = Broodwar->enemy()->getRace() == Races::Zerg ? UnitTypes::Zerg_Zergling : UnitTypes::Protoss_Zealot;

        createWall(buildings, Map::getMainArea(), Map::getMainChoke(), UnitTypes::Zerg_Zergling, defenses, false, true);
    }

    Wall * getClosestWall(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Wall * bestWall = nullptr;
        for (auto &wall : walls) {
            const auto dist = here.getDistance(TilePosition(wall.getChokePoint()->Center()));

            if (dist < distBest) {
                distBest = dist;
                bestWall = &wall;
            }
        }
        return bestWall;
    }

    vector<Wall>& getWalls() {
        return walls;
    }

    Wall* getWall(const BWEM::Area * area, const BWEM::ChokePoint * choke)
    {
        if (!area && !choke)
            return nullptr;

        for (auto &wall : walls) {
            if ((!area || wall.getArea() == area) && (!choke || wall.getChokePoint() == choke))
                return &wall;
        }
        return nullptr;
    }

    void draw()
    {
        for (auto &wall : walls) {
            for (auto &tile : wall.getSmallTiles())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
            for (auto &tile : wall.getMediumTiles())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), Broodwar->self()->getColor());
            for (auto &tile : wall.getLargeTiles())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), Broodwar->self()->getColor());
            for (auto &tile : wall.getDefenses())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
            Broodwar->drawBoxMap(Position(wall.getDoor()), Position(wall.getDoor()) + Position(33, 33), Broodwar->self()->getColor(), true);
            Broodwar->drawCircleMap(Position(wall.getCentroid()) + Position(16, 16), 8, Broodwar->self()->getColor(), true);

            auto p1 = wall.getChokePoint()->Pos(wall.getChokePoint()->end1);
            auto p2 = wall.getChokePoint()->Pos(wall.getChokePoint()->end2);

            Broodwar->drawLineMap(Position(p1), Position(p2), Colors::Green);
        }
        Broodwar->drawCircleMap(Position(startTile), 6, Colors::Red, true);
        Broodwar->drawCircleMap(Position(endTile), 6, Colors::Red, false);
    }

}
