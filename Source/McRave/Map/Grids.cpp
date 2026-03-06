#include "Grids.h"

#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Micro/Scout/Scouts.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Grids {
    namespace {

        vector<WalkPosition> resetVector;

        struct Grid {
            float airThreat, groundThreat;
            int airDensity, groundDensity;
            int fullCollision; // TODO: Can we remove this? Use vertical + horizontal instead
            int horizontalCollision, verticalCollision;
            int visible = -100;
            int mobility;
            int lastUpdateFrame = 0;
            void wipe()
            {
                airThreat           = 0.0f;
                groundThreat        = 0.0f;
                airDensity          = 0.0f;
                groundDensity       = 0.0f;
                fullCollision       = 0;
                horizontalCollision = 0;
                verticalCollision   = 0;
            }
        };
        Grid enemyGrid[1048576];
        Grid selfGrid[1048576];
        Grid neutralGrid[1048576];
        bool resetGrid[1048576];
        int currentGridFrame = 0;

        constexpr int gridWalkScale = 1024;
        constexpr int gridTileScale = 256;

        int mapWalkWidth = 0;
        int mapWalkHeight = 0;

        void addCollision(UnitInfo &unit)
        {
            // Pixel rectangle (make any even size units an extra WalkPosition)
            const auto topLeft  = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y - unit.getType().dimensionUp());
            const auto topRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y - unit.getType().dimensionUp());
            const auto botLeft  = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y + unit.getType().dimensionDown() + 1);
            const auto botRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y + unit.getType().dimensionDown() + 1);

            // Just store these in a position, it's a viable struct here
            const auto topLeftMod  = Position(topLeft.x % 8, topLeft.y % 8);
            const auto botRightMod = Position(botRight.x % 8, botRight.y % 8);

            auto &grid = unit.getPlayer() == Broodwar->self() ? selfGrid : enemyGrid;

            //
            WalkPosition TL = WalkPosition(topLeft);
            WalkPosition TR = WalkPosition(topRight);
            WalkPosition BL = WalkPosition(botLeft);
            WalkPosition BR = WalkPosition(botRight);

            if (topLeft.isValid() && topLeftMod.y > 0) {
                for (auto x = TL.x; x <= TR.x; x++) {
                    auto &index             = grid[gridWalkScale * TL.y + x];
                    index.verticalCollision = topLeftMod.y;
                    index.lastUpdateFrame   = currentGridFrame;
                }
            }
            if (botRight.isValid() && botRightMod.y > 0) {
                for (auto x = BL.x; x <= BR.x; x++) {
                    auto &index             = grid[gridWalkScale * BL.y + x];
                    index.verticalCollision = 8 - botRightMod.y;
                    index.lastUpdateFrame   = currentGridFrame;
                }
            }

            if (topLeft.isValid() && topLeftMod.x > 0) {
                for (auto y = TL.y; y <= BL.y; y++) {
                    auto &index               = grid[gridWalkScale * y + TL.x];
                    index.horizontalCollision = topLeftMod.x;
                    index.lastUpdateFrame     = currentGridFrame;
                }
            }
            if (botRight.isValid() && botRightMod.x > 0) {
                for (auto y = TR.y; y <= BR.y; y++) {
                    auto &index               = grid[gridWalkScale * y + TR.x];
                    index.horizontalCollision = 8 - botRightMod.x;
                    index.lastUpdateFrame     = currentGridFrame;
                }
            }
            for (int x = TL.x; x <= BR.x; x++) {
                for (int y = TL.y; y <= BR.y; y++) {
                    auto &index = grid[gridWalkScale * y + x];
                    index.fullCollision++;
                    index.lastUpdateFrame = currentGridFrame;
                }
            }
        }

        void addToGrids(UnitInfo &unit, Grid *grid)
        {
            // Limit checks so we don't have to check validity
            auto radius = unit.getPlayer() != Broodwar->self() ? int(max(unit.getGroundReach(), unit.getAirReach())) / 8 : 10;

            // Pixel rectangle (make any even size units an extra WalkPosition)
            const auto topLeft  = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y - unit.getType().dimensionUp());
            const auto topRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y - unit.getType().dimensionUp());
            const auto botLeft  = Position(unit.getPosition().x - unit.getType().dimensionLeft(), unit.getPosition().y + unit.getType().dimensionDown() + 1);
            const auto botRight = Position(unit.getPosition().x + unit.getType().dimensionRight() + 1, unit.getPosition().y + unit.getType().dimensionDown() + 1);
            const auto x1       = unit.getPosition().x;
            const auto y1       = unit.getPosition().y;
            const auto center   = WalkPosition(unit.getPosition());

            const auto clusterTopLeft  = topLeft - Position(48, 48);
            const auto clusterBotRight = botRight + Position(48, 48);
            const auto allowCluster    = (unit.getPlayer() != Broodwar->self() || unit.getRole() == Role::Combat);
            const auto allowCollision  = !unit.isFlying() && !unit.isBurrowed();
            const auto allowGround     = unit.getPlayer() != Broodwar->self() && unit.getPlayer() != Broodwar->neutral() && unit.canAttackGround();
            const auto allowAir        = unit.getPlayer() != Broodwar->self() && unit.getPlayer() != Broodwar->neutral() && unit.canAttackAir();

            if (radius > 0 && (!unit.getType().isBuilding() || unit.canAttackAir() || unit.canAttackGround())) {
                radius += 2;
                for (auto &w : Util::getWalkCircle(radius)) {
                    auto walk = center + w;
                    if (!walk.isValid())
                        continue;

                    auto &index     = grid[gridWalkScale * walk.y + walk.x];
                    const auto dist = Util::fastDistance(x1, y1, (walk.x * 8) + 4, (walk.y * 8) + 4) + 8;
                    if (index.lastUpdateFrame != currentGridFrame) {
                        index.wipe();
                        index.lastUpdateFrame = currentGridFrame;
                    }

                    // Cluster
                    if (allowCluster && walk.x * 8 >= clusterTopLeft.x && walk.y * 8 >= clusterTopLeft.y && walk.x * 8 <= clusterBotRight.x && walk.y * 8 <= clusterBotRight.y) {
                        unit.isFlying() ? index.airDensity++ : index.groundDensity++;
                    }

                    // Threat
                    if (allowGround) {
                        const auto rangeDiff = 0.015625 * max(1.0, dist - unit.getGroundRange()); // This is just 1/64 so it decays over 2 tiles
                        index.groundThreat += (dist <= unit.getGroundRange() ? float(unit.getVisibleGroundStrength()) : float(unit.getVisibleGroundStrength() * rangeDiff));
                    }
                    if (allowAir) {
                        const auto rangeDiff = 0.015625 * max(1.0, dist - unit.getAirRange()); // This is just 1/64 so it decays over 2 tiles
                        index.airThreat += (dist <= unit.getAirRange() ? float(unit.getVisibleAirStrength()) : float(unit.getVisibleAirStrength() * rangeDiff));
                    }
                }
            }

            if (allowCollision)
                addCollision(unit);
        }

        void updateGrids()
        {
            // Don't add to any grid under these conditions
            const auto canAddToGrid = [&](auto &unit) {
                if ((unit.unit()->exists() && (unit.unit()->isStasised() || unit.unit()->isMaelstrommed() || unit.unit()->isLoaded())) || !unit.getPosition().isValid() ||
                    unit.getType() == Protoss_Interceptor || unit.getType().isSpell() || unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Protoss_Scarab ||
                    (unit.getPlayer() == Broodwar->self() && !unit.getType().isBuilding() && !unit.unit()->isCompleted()) ||
                    (unit.getPlayer() != Broodwar->self() && unit.getType().isWorker() && !unit.hasAttackedRecently() && !Scouts::gatheringInformation()) ||
                    (unit.getPlayer() != Broodwar->self() && !unit.hasTarget() && !unit.unit()->exists() && !Scouts::gatheringInformation()))
                    return false;
                return true;
            };

            // Iterate player states and units, adding to grids if possible
            static map<PlayerState, string> playerStates = {{PlayerState::Self, "Self"}, {PlayerState::Enemy, "Enemy"}, {PlayerState::Neutral, "Neutral"}};
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

        void updateVisibility()
        {
            // TODO: Enemy visibility
            for (int x = 0; x < 256; x++) {
                for (int y = 0; y < 256; y++) {
                    TilePosition t(x, y);
                    if (t.isValid() && Broodwar->isVisible(t))
                        selfGrid[gridTileScale * y + x].visible = currentGridFrame;
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
            mapWalkWidth  = Broodwar->mapWidth() * 4;
            mapWalkHeight = Broodwar->mapHeight() * 4;

            for (int i = 1; i < 2048; i++) {
                log8Lookup[i]    = log(8 * i + 8);
                invLog8Lookup[i] = 1.0f / log8Lookup[i];
            }
        }

        void createChokeDirections()
        {
            const auto walkableTilesAround = [&](auto &t, auto d) {
                auto walkables = 0;
                auto aD        = d;
                while (aD > 0) {
                    const auto aroundTiles = {WalkPosition(-aD, -aD), WalkPosition(-aD, 0),  WalkPosition(-aD, aD), WalkPosition(0, -aD),
                                              WalkPosition(0, aD),    WalkPosition(aD, -aD), WalkPosition(aD, 0),   WalkPosition(aD, aD)};
                    for (auto &tile : aroundTiles) {
                        if (WalkPosition(t + tile).isValid() && mapBWEM.GetMiniTile(t + tile).Walkable()) {
                            // Broodwar->drawLineMap(Position(t + tile), Position(t), Colors::Green);
                            walkables++;
                        }
                    }
                    aD--;
                }
                return walkables;
            };

            for (auto &area : mapBWEM.Areas()) {
                for (auto &choke : area.ChokePoints()) {
                    auto cD                              = int(round(double(choke->Width()) / 8.0));
                    const vector<WalkPosition> distTiles = {WalkPosition(-cD, -cD), WalkPosition(-cD, 0),  WalkPosition(-cD, cD), WalkPosition(0, -cD),
                                                            WalkPosition(0, cD),    WalkPosition(cD, -cD), WalkPosition(cD, 0),   WalkPosition(cD, cD)};

                    auto largest = 0;
                    pair<WalkPosition, WalkPosition> bestPair;
                    for (auto itr = distTiles.begin(); itr != distTiles.end(); itr++) {
                        const auto tile = *itr + WalkPosition(choke->Center());
                        auto opposite   = WalkPosition(choke->Center()) + distTiles.at(distTiles.size() - 1 - distance(distTiles.begin(), itr));
                        if (!tile.isValid() || !opposite.isValid() || !mapBWEM.GetMiniTile(tile).Walkable() || !mapBWEM.GetMiniTile(opposite).Walkable())
                            continue;

                        const auto walkables = walkableTilesAround(tile, cD);
                        Broodwar->drawTextMap(Position(tile), "%d", walkables);

                        // Get largest and opposite point
                        if (walkables > largest) {
                            largest  = walkables;
                            bestPair = make_pair(tile, opposite);
                        }
                    }

                    if (bestPair.first.isValid())
                        Visuals::drawLine(bestPair.first, bestPair.second, Colors::Red);
                }
            }
        }
    } // namespace

    void onFrame()
    {
        currentGridFrame = Broodwar->getFrameCount();
        updateGrids();
        updateVisibility();
        // createChokeDirections();

        //auto mousePos = WalkPosition(Broodwar->getScreenPosition() + Broodwar->getMousePosition());
        //auto grid     = Grids::getGroundThreat(mousePos, PlayerState::Enemy);
        //Broodwar << grid << endl;
    }

    void onStart()
    {
        initializeLogTable();
        initializeMobility();
        updateVisibility();
    }

    template <typename T> T getGridValue(const Grid *gridArray, int x, int y, T Grid::*member)
    {
        if (x < 0 || y < 0 || x > mapWalkWidth || y > mapWalkHeight) {
            return {};
        }
        const auto &index = gridArray[gridWalkScale * y + x];

        // If the data is from a previous frame, it's effectively 0
        if (index.lastUpdateFrame != currentGridFrame) {
            return {};
        }
        return index.*member;
    }

    float getGroundThreat(Position here, PlayerState player) { return getGroundThreat(WalkPosition(here), player); }

    float getGroundThreat(WalkPosition here, PlayerState player)
    {
        return (player == PlayerState::Self ? getGridValue(selfGrid, here.x, here.y, &Grid::groundThreat) : getGridValue(enemyGrid, here.x, here.y, &Grid::groundThreat));
    }

    float getGroundThreat(TilePosition here, PlayerState player) { return getGroundThreat(WalkPosition(here), player); }

    float getAirThreat(Position here, PlayerState player) { return getAirThreat(WalkPosition(here), player); }

    float getAirThreat(WalkPosition here, PlayerState player)
    {
        return (player == PlayerState::Self ? getGridValue(selfGrid, here.x, here.y, &Grid::airThreat) : getGridValue(enemyGrid, here.x, here.y, &Grid::airThreat));
    }

    float getAirThreat(TilePosition here, PlayerState player) { return getAirThreat(WalkPosition(here), player); }

    float getGroundDensity(Position here, PlayerState player) { return getGroundDensity(WalkPosition(here), player); }

    float getGroundDensity(WalkPosition here, PlayerState player)
    {
        return (player == PlayerState::Self ? getGridValue(selfGrid, here.x, here.y, &Grid::groundDensity) : getGridValue(enemyGrid, here.x, here.y, &Grid::groundDensity));
    }

    float getAirDensity(Position here, PlayerState player) { return getAirDensity(WalkPosition(here), player); }

    float getAirDensity(WalkPosition here, PlayerState player)
    {
        return (player == PlayerState::Self ? getGridValue(selfGrid, here.x, here.y, &Grid::airDensity) : getGridValue(enemyGrid, here.x, here.y, &Grid::airDensity));
    }

    int getFCollision(WalkPosition here, PlayerState player)
    {
        const auto index = gridWalkScale * here.y + here.x;
        if (player == PlayerState::All)
            return getGridValue(neutralGrid, here.x, here.y, &Grid::fullCollision) + getGridValue(enemyGrid, here.x, here.y, &Grid::fullCollision) +
                   getGridValue(selfGrid, here.x, here.y, &Grid::fullCollision);
        if (player == PlayerState::Self)
            return getGridValue(selfGrid, here.x, here.y, &Grid::fullCollision);
        if (player == PlayerState::Enemy)
            return getGridValue(enemyGrid, here.x, here.y, &Grid::fullCollision);
        return 0;
    }

    int getVCollision(WalkPosition here, PlayerState player)
    {
        const auto index = gridWalkScale * here.y + here.x;
        if (player == PlayerState::All)
            return getGridValue(neutralGrid, here.x, here.y, &Grid::verticalCollision) + getGridValue(enemyGrid, here.x, here.y, &Grid::verticalCollision) +
                   getGridValue(selfGrid, here.x, here.y, &Grid::verticalCollision);
        if (player == PlayerState::Self)
            return getGridValue(selfGrid, here.x, here.y, &Grid::verticalCollision);
        if (player == PlayerState::Enemy)
            return getGridValue(enemyGrid, here.x, here.y, &Grid::verticalCollision);
        return 0;
    }

    int getHCollision(WalkPosition here, PlayerState player)
    {
        const auto index = gridWalkScale * here.y + here.x;
        if (player == PlayerState::All)
            return getGridValue(neutralGrid, here.x, here.y, &Grid::horizontalCollision) + getGridValue(enemyGrid, here.x, here.y, &Grid::horizontalCollision) +
                   getGridValue(selfGrid, here.x, here.y, &Grid::horizontalCollision);
        if (player == PlayerState::Self)
            return getGridValue(selfGrid, here.x, here.y, &Grid::horizontalCollision);
        if (player == PlayerState::Enemy)
            return getGridValue(enemyGrid, here.x, here.y, &Grid::horizontalCollision);
        return 0;
    }

    int getMobility(WalkPosition here) { return neutralGrid[gridWalkScale * here.y + here.x].mobility; }
    int getMobility(Position here) { return neutralGrid[gridWalkScale * (here.y / 8) + (here.x / 8)].mobility; }

    int getLastVisibleFrame(Position here) { return selfGrid[gridTileScale * (here.y / 32) + (here.x / 32)].visible; }
    int getLastVisibleFrame(TilePosition here) { return selfGrid[gridTileScale * here.y + here.x].visible; }
} // namespace McRave::Grids