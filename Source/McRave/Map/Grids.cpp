#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Grids
{
    namespace {

        vector<WalkPosition> resetVector;

        struct Grid {
            float airThreat, groundThreat;
            int airDensity, groundDensity;
            int fullCollision; // TODO: Can we remove this? Use vertical + horizontal instead
            int horizontalCollision, verticalCollision;
            int visible;
            int mobility;
            void wipe() {
                airThreat = 0.0;
                groundThreat = 0.0;
                airDensity = 0;
                groundDensity = 0;
                fullCollision = 0;
                horizontalCollision = 0;
                verticalCollision = 0;
            }
        };
        Grid enemyGrid[1048576];
        Grid selfGrid[1048576];
        Grid neutralGrid[1048576];
        bool resetGrid[1048576];

        constexpr int gridWalkScale = 1024;
        constexpr int gridTileScale = 256;

        void saveReset(int x, int y)
        {
            if (!std::exchange(resetGrid[gridWalkScale * y + x], 1))
                resetVector.emplace_back(x, y);
        }

        void addCollision(UnitInfo& unit)
        {
            // Pixel rectangle (make any even size units an extra WalkPosition)
            const auto topLeft = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y - unit.getType().dimensionUp());
            const auto topRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y - unit.getType().dimensionUp());
            const auto botLeft = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y + unit.getType().dimensionDown() + 1);
            const auto botRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y + unit.getType().dimensionDown() + 1);

            auto &grid = unit.getPlayer() == Broodwar->self() ? selfGrid : enemyGrid;

            // 
            WalkPosition TL = WalkPosition(topLeft);
            WalkPosition TR = WalkPosition(topRight);
            WalkPosition BL = WalkPosition(botLeft);
            WalkPosition BR = WalkPosition(botRight);

            if (topLeft.y % 8 > 0) {
                for (auto x = TL.x; x <= TR.x; x++) {
                    const auto i = gridWalkScale * TL.y + x;
                    grid[i].verticalCollision = topLeft.y % 8;
                    saveReset(x, TL.y);
                }
            }
            if (botRight.y % 8 > 0) {
                for (auto x = BL.x; x <= BR.x; x++) {
                    const auto i = gridWalkScale * BL.y + x;
                    grid[i].verticalCollision = 8 - botRight.y % 8;
                    saveReset(x, BL.y);
                }
            }

            if (topLeft.x % 8 > 0) {
                for (auto y = TL.y; y <= BL.y; y++) {
                    const auto i = gridWalkScale * y + TL.x;
                    grid[i].horizontalCollision = topLeft.x % 8;
                    saveReset(TL.x, y);
                }
            }
            if (botRight.x % 8 > 0) {
                for (auto y = TR.y; y <= BR.y; y++) {
                    const auto i = gridWalkScale * y + TR.x;
                    grid[i].horizontalCollision = 8 - botRight.x % 8;
                    saveReset(TR.x, y);
                }
            }
        }

        int fasterDistGrids(int x1, int y1, int x2, int y2) {
            unsigned int min = abs((int)(x1 - x2));
            unsigned int max = abs((int)(y1 - y2));
            if (max < min)
                std::swap(min, max);

            if (min < (max >> 2))
                return max;

            unsigned int minCalc = (3 * min) >> 3;
            return (minCalc >> 5) + minCalc + max - (max >> 4) - (max >> 6);
        }

        void addToGrids(UnitInfo& unit, Grid* grid)
        {
            // Limit checks so we don't have to check validity
            auto radius = unit.getPlayer() != Broodwar->self() ? 1 + int(max(unit.getGroundReach(), unit.getAirReach())) / 8 : 8;

            const auto left = max(0, unit.getWalkPosition().x - radius);
            const auto right = min(1024, unit.getWalkPosition().x + unit.getWalkWidth() + radius);
            const auto top = max(0, unit.getWalkPosition().y - radius);
            const auto bottom = min(1024, unit.getWalkPosition().y + unit.getWalkHeight() + radius);

            // Pixel rectangle (make any even size units an extra WalkPosition)
            const auto topLeft = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y - unit.getType().dimensionUp());
            const auto topRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y - unit.getType().dimensionUp());
            const auto botLeft = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y + unit.getType().dimensionDown() + 1);
            const auto botRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y + unit.getType().dimensionDown() + 1);
            const auto x1 = unit.getPosition().x;
            const auto y1 = unit.getPosition().y;

            const auto clusterTopLeft = topLeft - Position(88, 88);
            const auto clusterBotRight = botRight + Position(88, 88);
            const auto allowCluster = (unit.getPlayer() != Broodwar->self() || unit.getRole() == Role::Combat);
            const auto allowCollision = !unit.isFlying() && !unit.isBurrowed();
            const auto allowGround = unit.getPlayer() != Broodwar->self() && unit.canAttackGround();
            const auto allowAir = unit.getPlayer() != Broodwar->self() && unit.canAttackAir();

            if (!unit.isFlying())
                addCollision(unit);

            // Iterate tiles and add to grid
            for (int x = left; x < right; x++) {
                for (int y = top; y < bottom; y++) {
                    auto &index = grid[gridWalkScale * y + x];
                    auto savePlease = false;
                    const auto dist = fasterDistGrids(x1, y1, (x * 8) + 4, (y * 8) + 4) - 32;

                    // Cluster
                    if (allowCluster && x * 8 >= clusterTopLeft.x && y * 8 >= clusterTopLeft.y && x * 8 <= clusterBotRight.x && y * 8 <= clusterBotRight.y) {
                        unit.isFlying() ? index.airDensity++ : index.groundDensity++;
                        savePlease = true;
                    }

                    // Collision
                    if (allowCollision && Util::rectangleIntersect(topLeft, botRight, x * 8, y * 8) && Util::rectangleIntersect(topLeft, botRight, (x + 1) * 8 - 1, (y + 1) * 8 - 1)) {
                        index.fullCollision++;
                        savePlease = true;
                    }

                    // Threat
                    if (allowGround && dist <= unit.getGroundReach()) {
                        index.groundThreat += (dist <= unit.getGroundRange() ? float(unit.getVisibleGroundStrength()) : float(unit.getVisibleGroundStrength() / max(1.0, logLookup16[dist / 16])));
                        savePlease = true;
                    }
                    if (allowAir && dist <= unit.getAirReach()) {
                        index.airThreat += (dist <= unit.getAirRange() ? float(unit.getVisibleAirStrength()) : float(unit.getVisibleAirStrength() / max(1.0, logLookup16[dist / 16])));
                        savePlease = true;
                    }

                    if (savePlease)
                        saveReset(x, y);
                }
            }
        }

        void reset()
        {
            Visuals::startPerfTest();
            for (auto &pos : resetVector) {
                selfGrid[gridWalkScale * pos.y + pos.x].wipe();
                enemyGrid[gridWalkScale * pos.y + pos.x].wipe();
                neutralGrid[gridWalkScale * pos.y + pos.x].wipe();
            }
            Visuals::endPerfTest("Grid Reset");
        }

        void updateGrids()
        {
            // Don't add to any grid under these conditions
            const auto canAddToGrid = [&](auto &unit) {
                if ((unit.unit()->exists() && (unit.unit()->isStasised() || unit.unit()->isMaelstrommed() || unit.unit()->isLoaded()))
                    || !unit.getPosition().isValid()
                    || unit.getType() == Protoss_Interceptor
                    || unit.getType().isSpell()
                    || unit.getType() == Terran_Vulture_Spider_Mine
                    || unit.getType() == Protoss_Scarab
                    || (unit.getPlayer() == Broodwar->self() && !unit.getType().isBuilding() && !unit.unit()->isCompleted())
                    || (unit.getPlayer() != Broodwar->self() && unit.getType().isWorker() && !unit.hasAttackedRecently() && !Scouts::gatheringInformation())
                    || (unit.getPlayer() != Broodwar->self() && !unit.hasTarget() && !unit.unit()->exists() && !Scouts::gatheringInformation()))
                    return false;
                return true;
            };

            // Iterate player states and units, adding to grids if possible
            map<PlayerState, string> playerStates ={ {PlayerState::Self, "Self"}, {PlayerState::Enemy, "Enemy"}, {PlayerState::Neutral, "Neutral"} };
            for (auto &[player, name] : playerStates) {
                Visuals::startPerfTest();
                for (auto &u : Units::getUnits(player)) {
                    UnitInfo &unit = *u;
                    if (!canAddToGrid(unit))
                        continue;

                    auto &grid = player == PlayerState::Self ? selfGrid : enemyGrid; // TODO: Use neutral grids for collision, expect enemy for now
                    addToGrids(unit, grid);
                }
                Visuals::endPerfTest("Grid " + name);
            }
        }

        void updateVisibility(int frame)
        {
            // TODO: Enemy visibility
            for (int x = 0; x < 256; x++) {
                for (int y = 0; y < 256; y++) {
                    TilePosition t(x, y);
                    if (t.isValid() && Broodwar->isVisible(t))
                        selfGrid[gridTileScale * y + x].visible = frame;
                }
            }
        }

        void initializeMobility()
        {
            auto &grid = neutralGrid;
            for (auto &gas : Broodwar->getGeysers()) {
                auto t = WalkPosition(gas->getTilePosition());
                for (int x = t.x; x < t.x + gas->getType().tileWidth() * 4; x++) {
                    for (int y = t.y; y < t.y + gas->getType().tileHeight() * 4; y++) {
                        neutralGrid[gridWalkScale * y + x].mobility = -1;
                    }
                }
            }

            for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
                    auto &index = neutralGrid[gridWalkScale * y + x];

                    WalkPosition w(x, y);
                    if (!w.isValid() || index.mobility != 0)
                        continue;

                    if (!Broodwar->isWalkable(w)) {
                        index.mobility = -1;
                        continue;
                    }

                    for (int i = -12; i < 12; i++) {
                        for (int j = -12; j < 12; j++) {

                            WalkPosition w2(x + i, y + j);
                            if (w2.isValid() && mapBWEM.GetMiniTile(w2).Walkable() && index.mobility != -1)
                                index.mobility += 1;
                        }
                    }

                    index.mobility = clamp(int(floor(index.mobility / 56)), 1, 10);


                    // Island
                    if (mapBWEM.GetArea(w) && mapBWEM.GetArea(w)->AccessibleNeighbours().size() == 0)
                        index.mobility = -1;
                }
            }
        }

        void initializeLogTable()
        {
            for (int i = 0; i < 1024; i++) {
                logLookup16[i] = log(16 * i);
            }
        }

        void createChokeDirections()
        {
            const auto walkableTilesAround = [&](auto &t, auto d) {
                auto walkables = 0;
                auto aD = d;
                while (aD > 0) {
                    const auto aroundTiles ={ WalkPosition(-aD, -aD), WalkPosition(-aD, 0), WalkPosition(-aD, aD), WalkPosition(0, -aD), WalkPosition(0, aD), WalkPosition(aD, -aD), WalkPosition(aD, 0), WalkPosition(aD, aD) };
                    for (auto &tile : aroundTiles) {
                        if (WalkPosition(t + tile).isValid() && mapBWEM.GetMiniTile(t + tile).Walkable()) {
                            //Broodwar->drawLineMap(Position(t + tile), Position(t), Colors::Green);
                            walkables++;
                        }
                    }
                    aD--;
                }
                return walkables;
            };

            for (auto &area : mapBWEM.Areas()) {
                for (auto &choke : area.ChokePoints()) {
                    auto cD = int(round(double(choke->Width()) / 8.0));
                    const vector<WalkPosition> distTiles ={ WalkPosition(-cD, -cD), WalkPosition(-cD, 0), WalkPosition(-cD, cD), WalkPosition(0, -cD), WalkPosition(0, cD), WalkPosition(cD, -cD), WalkPosition(cD, 0), WalkPosition(cD, cD) };

                    auto largest = 0;
                    pair<WalkPosition, WalkPosition> bestPair;
                    for (auto itr = distTiles.begin(); itr != distTiles.end(); itr++) {
                        const auto tile = *itr + WalkPosition(choke->Center());
                        auto opposite = WalkPosition(choke->Center()) + distTiles.at(distTiles.size() - 1 - distance(distTiles.begin(), itr));
                        if (!tile.isValid() || !opposite.isValid() || !mapBWEM.GetMiniTile(tile).Walkable() || !mapBWEM.GetMiniTile(opposite).Walkable())
                            continue;

                        const auto walkables = walkableTilesAround(tile, cD);
                        Broodwar->drawTextMap(Position(tile), "%d", walkables);


                        // Get largest and opposite point
                        if (walkables > largest) {
                            largest = walkables;
                            bestPair = make_pair(tile, opposite);
                        }
                    }
                    
                    if (bestPair.first.isValid())
                        Visuals::drawLine(bestPair.first, bestPair.second, Colors::Red);
                }
            }
        }
    }

    void onFrame()
    {
        reset();
        updateGrids();
        updateVisibility(Broodwar->getFrameCount());
        //createChokeDirections();
    }

    void onStart()
    {
        initializeLogTable();
        initializeMobility();
        updateVisibility(-9999);
    }


    float getGroundThreat(Position here, PlayerState player) {
        return getGroundThreat(WalkPosition(here), player);
    }

    float getGroundThreat(WalkPosition here, PlayerState player) {
        return (player == PlayerState::Self ? selfGrid[gridWalkScale * here.y + here.x].groundThreat : enemyGrid[gridWalkScale * here.y + here.x].groundThreat);
    }

    float getGroundThreat(TilePosition here, PlayerState player) {
        return player == PlayerState::Self ? selfGrid[gridTileScale * here.y + here.x].groundThreat : enemyGrid[gridTileScale * here.y + here.x].groundThreat;
    }

    float getAirThreat(Position here, PlayerState player) {
        return getAirThreat(WalkPosition(here), player);
    }

    float getAirThreat(WalkPosition here, PlayerState player) {
        return player == PlayerState::Self ? selfGrid[gridWalkScale * here.y + here.x].airThreat : enemyGrid[gridWalkScale * here.y + here.x].airThreat;
    }

    float getAirThreat(TilePosition here, PlayerState player) {
        return player == PlayerState::Self ? selfGrid[gridTileScale * here.y + here.x].airThreat : enemyGrid[gridTileScale * here.y + here.x].airThreat;
    }

    float getGroundDensity(Position here, PlayerState player) {
        return getGroundDensity(WalkPosition(here), player);
    }

    float getGroundDensity(WalkPosition here, PlayerState player) {
        return (player == PlayerState::Self ? float(selfGrid[gridWalkScale * here.y + here.x].groundDensity) : float(enemyGrid[gridWalkScale * here.y + here.x].groundDensity));
    }

    float getAirDensity(Position here, PlayerState player) {
        return getAirDensity(WalkPosition(here), player);
    }

    float getAirDensity(WalkPosition here, PlayerState player) {
        return (player == PlayerState::Self ? float(selfGrid[gridWalkScale * here.y + here.x].airDensity) : float(enemyGrid[gridWalkScale * here.y + here.x].airDensity));
    }

    int getFCollision(WalkPosition here, PlayerState player) {
        const auto index = gridWalkScale * here.y + here.x;
        if (player == PlayerState::All)
            return neutralGrid[index].fullCollision + enemyGrid[index].fullCollision + selfGrid[index].fullCollision;
        if (player == PlayerState::Self)
            return selfGrid[index].fullCollision;
        if (player == PlayerState::Enemy)
            return enemyGrid[index].fullCollision;
        return 0;
    }

    int getVCollision(WalkPosition here, PlayerState player) {
        const auto index = gridWalkScale * here.y + here.x;
        if (player == PlayerState::All)
            return neutralGrid[index].verticalCollision + enemyGrid[index].verticalCollision + selfGrid[index].verticalCollision;
        if (player == PlayerState::Self)
            return selfGrid[index].verticalCollision;
        if (player == PlayerState::Enemy)
            return enemyGrid[index].verticalCollision;
        return 0;
    }

    int getHCollision(WalkPosition here, PlayerState player) {
        const auto index = gridWalkScale * here.y + here.x;
        if (player == PlayerState::All)
            return neutralGrid[index].horizontalCollision + enemyGrid[index].horizontalCollision + selfGrid[index].horizontalCollision;
        if (player == PlayerState::Self)
            return selfGrid[index].horizontalCollision;
        if (player == PlayerState::Enemy)
            return enemyGrid[index].horizontalCollision;
        return 0;
    }

    int getMobility(WalkPosition here) { return neutralGrid[gridWalkScale * here.y + here.x].mobility; }
    int getMobility(Position here) { return neutralGrid[gridWalkScale * (here.y / 8) + (here.x / 8)].mobility; }

    int getLastVisibleFrame(Position here) { return selfGrid[gridTileScale * (here.y / 32) + (here.x / 32)].visible; }
    int getLastVisibleFrame(TilePosition here) { return selfGrid[gridTileScale * here.y + here.x].visible; }
}