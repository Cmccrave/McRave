#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Grids
{
    namespace {
        bool resetGrid[1024][1024] ={};
        int timeGrid[1024][1024] ={};
        int visibleGrid[256][256] ={};
        int visitedGrid[1024][1024] ={};
        vector<WalkPosition> resetVector;

        int currentFrame = 0;

        // Ally Grid
        float parentDistance[1024][1024];
        float aGroundCluster[1024][1024] ={};
        float aAirCluster[1024][1024] ={};

        // Enemy Grid
        float eGroundThreat[1024][1024] ={};
        float eAirThreat[1024][1024] ={};
        float eGroundCluster[1024][1024] ={};
        float eAirCluster[1024][1024] ={};
        int eSplash[1024][1024] ={};

        // Mobility Grid
        int mobility[1024][1024] ={};
        int collision[1024][1024] ={};
        double distanceHome[1024][1024] ={};


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

        void saveReset(int x, int y)
        {
            if (!std::exchange(resetGrid[x][y], 1)) {
                resetVector.emplace_back(x,y);
            }
        }

        void addSplash(UnitInfo& unit)
        {
            int walkWidth = (int)ceil(unit.getType().width() / 8.0);
            int walkHeight = (int)ceil(unit.getType().height() / 8.0);

            WalkPosition start(unit.getTarget().getWalkPosition());
            Position target = unit.getTarget().getPosition();

            for (int x = start.x - 12; x <= start.x + 12 + walkWidth; x++) {
                for (int y = start.y - 12; y <= start.y + 12 + walkHeight; y++) {

                    WalkPosition w(x, y);
                    Position p = Position(w) + Position(4, 4);
                    if (!w.isValid())
                        continue;

                    saveReset(x, y);
                    eSplash[x][y] += (target.getDistance(p) <= 96);
                }
            }
        }

        void addToGrids(UnitInfo& unit)
        {
            if ((unit.getType().isWorker() && unit.getPlayer() != Broodwar->self() && (!unit.unit()->exists() || Broodwar->getFrameCount() > 10000 || unit.unit()->isConstructing() || (Terrain::isInAllyTerritory(unit.getTilePosition()) && (Broodwar->getFrameCount() - unit.getLastAttackFrame() > 500))))
                || unit.getType() == UnitTypes::Protoss_Interceptor
                || unit.getType() == UnitTypes::Terran_Medic)
                return;

            // Pixel and walk sizes
            auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0);
            auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0);

            // Choose threat grid
            auto grdGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eGroundThreat;
            auto airGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eAirThreat;

            // Choose cluster grid
            auto clusterGrid = unit.getPlayer() == Broodwar->self() ?
                (unit.getType().isFlyer() ? aAirCluster : aGroundCluster) :
                (unit.getType().isFlyer() ? eAirCluster : eGroundCluster);


            // Limit checks so we don't have to check validity
            auto radius = 1 + int(max(unit.getGroundReach(), unit.getAirReach())) / 8;
            auto left = max(0, unit.getWalkPosition().x - radius);
            auto right = min(1024, unit.getWalkPosition().x + walkWidth + radius);
            auto top = max(0, unit.getWalkPosition().y - radius);
            auto bottom = min(1024, unit.getWalkPosition().y + walkHeight + radius);

            // Pixel rectangle
            auto topLeft = Position(unit.getWalkPosition());
            auto botRight = topLeft + Position(walkWidth * 8, walkHeight * 8);
            int x1 = unit.getPosition().x;
            int y1 = unit.getPosition().y;

            // Iterate tiles and add to grid
            for (int x = left; x < right; x++) {
                for (int y = top; y < bottom; y++) {

                    auto dist = fasterDistGrids(x1, y1, (x * 8) + 4, (y * 8) + 4);

                    // Collision
                    if (!unit.getType().isFlyer() && Util::rectangleIntersect(topLeft, botRight, x*8, y*8)) {
                        collision[x][y] += 1;
                        saveReset(x,y);
                    }

                    // Threat
                    if (grdGrid && dist <= unit.getGroundReach()) {
                        grdGrid[x][y] += float(unit.getVisibleGroundStrength());
                        saveReset(x, y);
                    }
                    if (airGrid && dist <= unit.getAirReach()) {
                        airGrid[x][y] += float(unit.getVisibleAirStrength());
                        saveReset(x, y);
                    }

                    // Cluster
                    if (clusterGrid && dist < 96.0 && (unit.getPlayer() != Broodwar->self() || unit.getRole() == Role::Combat)) {
                        clusterGrid[x][y] += float(unit.getPriority());
                        saveReset(x, y);
                    }
                }
            }
        }

        void reset()
        {
            Visuals::startPerfTest();
            for (auto &w : resetVector) {
                int x = w.x;
                int y = w.y;

                aGroundCluster[x][y] = 0;
                aAirCluster[x][y] = 0;

                eGroundThreat[x][y] = 0.0;
                eAirThreat[x][y] = 0.0;
                eGroundCluster[x][y] = 0;
                eAirCluster[x][y] = 0;
                eSplash[x][y] = 0;

                collision[x][y] = 0;
                resetGrid[x][y] = 0;
            }
            resetVector.clear();
            Visuals::endPerfTest("Grid:Reset");
        }

        void updateAlly()
        {
            Visuals::startPerfTest();
            for (auto &u : Units::getMyUnits()) {
                UnitInfo &unit = u.second;

                // Add a visited grid for rough guideline of what we've seen by this unit recently
                auto start = unit.getWalkPosition();
                for (int x = start.x - 4; x < start.x + 8; x++) {
                    for (int y = start.y - 4; y < start.y + 8; y++) {
                        auto t = WalkPosition(x, y);
                        if (t.isValid())
                            visitedGrid[x][y] = Broodwar->getFrameCount();
                    }
                }

                // Spider mines are added to the enemy splash grid so ally units avoid allied mines
                if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
                    if (!unit.isBurrowed() && unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
                        addSplash(unit);
                }

                else if (!unit.unit()->isLoaded())
                    addToGrids(unit);
            }
            Visuals::endPerfTest("Grid:Self");
        }

        void updateEnemy()
        {
            Visuals::startPerfTest();
            for (auto &u : Units::getEnemyUnits()) {
                UnitInfo &unit = u.second;
                if (unit.unit()->exists() && (unit.unit()->isStasised() || unit.unit()->isMaelstrommed()))
                    continue;

                auto longRangeUnit = unit.getGroundRange() >= 224.0;

                if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Protoss_Scarab) {
                    if (unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
                        addSplash(unit);
                }
                else {
                    addToGrids(unit);
                }
            }
            Visuals::endPerfTest("Grid:Enemy");
        }

        void updateNeutral()
        {
            // Collision Grid - TODO
            for (auto &u : Broodwar->neutral()->getUnits()) {

                WalkPosition start = WalkPosition(u->getTilePosition());
                int width = u->getType().tileWidth() * 4;
                int height = u->getType().tileHeight() * 4;
                if (u->getType().isFlyer())
                    continue;

                for (int x = start.x; x < start.x + width; x++) {
                    for (int y = start.y; y < start.y + height; y++) {

                        if (!WalkPosition(x, y).isValid())
                            continue;

                        collision[x][y] = 1;
                        saveReset(x, y);
                    }
                }

                if (u->getType() == UnitTypes::Spell_Disruption_Web)
                    Command::addCommand(u, u->getPosition(), TechTypes::Disruption_Web);
            }
        }

        void updateVisibility()
        {
            for (int x = 0; x <= Broodwar->mapWidth(); x++) {
                for (int y = 0; y <= Broodwar->mapHeight(); y++) {
                    TilePosition t(x, y);
                    if (Broodwar->isVisible(t))
                        visibleGrid[x][y] = Broodwar->getFrameCount();
                }
            }
        }

        void draw()
        {
            return;// Remove this to draw stuff
            for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
                    WalkPosition w(x, y);

                    if (collision[x][y] == 0)
                        Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
                }
            }
        }

        void updateMobility()
        {
            for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {

                    WalkPosition w(x, y);
                    if (!w.isValid()
                        || !mapBWEM.GetMiniTile(w).Walkable())
                        continue;

                    for (int i = -12; i < 12; i++) {
                        for (int j = -12; j < 12; j++) {

                            WalkPosition w2(x + i, y + j);
                            if (w2.isValid() && mapBWEM.GetMiniTile(w2).Walkable())
                                mobility[x][y] += 1;
                        }
                    }

                    mobility[x][y] = min(10, int(floor(mobility[x][y] / 56)));


                    // Island
                    if ((mapBWEM.GetArea(w) && mapBWEM.GetArea(w)->AccessibleNeighbours().size() == 0) || !BWEB::Map::isWalkable((TilePosition)w))
                        mobility[x][y] = -1;

                    // Setup what is possible to check ground distances on
                    if (mobility[x][y] <= 0)
                        distanceHome[x][y] = -1;
                    else if (mobility[x][y] > 0)
                        distanceHome[x][y] = 0;
                }
            }
        }

        void updateDistance()
        {
            WalkPosition source(BWEB::Map::getMainPosition());
            vector<WalkPosition> direction{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 },{ -1,-1 },{ -1, 1 },{ 1, -1 },{ 1, 1 } };
            float root2 = float(sqrt(2.0));

            std::queue<BWAPI::WalkPosition> nodeQueue;
            nodeQueue.emplace(source);

            for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
                    distanceHome[x][y] = DBL_MAX;
                }
            }

            // While not empty, pop off top
            while (!nodeQueue.empty()) {
                auto const tile = nodeQueue.front();
                nodeQueue.pop();

                int i = 0;
                for (auto const &d : direction) {
                    auto const next = tile + d;
                    i++;

                    if (next.isValid() && tile.isValid()) {
                        // If next has a distance assigned or isn't walkable
                        if (distanceHome[next.x][next.y] != DBL_MAX || !Broodwar->isWalkable(next))
                            continue;

                        // Add distance and add to queue
                        distanceHome[next.x][next.y] = (i > 4 ? root2 : 1.0) + parentDistance[tile.x][tile.y];
                        parentDistance[next.x][next.y] = distanceHome[next.x][next.y];
                        nodeQueue.emplace(next);
                    }
                }
            }
        }
    }

    void onFrame()
    {
        reset();
        updateAlly();
        updateEnemy();
        updateNeutral();
        updateVisibility();
        draw();
    }

    void onStart()
    {
        updateMobility();
        updateDistance();
    }

    float getAGroundCluster(WalkPosition here) { return aGroundCluster[here.x][here.y]; }
    float getAGroundCluster(Position here) { return getAGroundCluster(WalkPosition(here)); }

    float getAAirCluster(WalkPosition here) { return aAirCluster[here.x][here.y]; }
    float getAAirCluster(Position here) { return getAAirCluster(WalkPosition(here)); }

    float getEGroundThreat(WalkPosition here) { return eGroundThreat[here.x][here.y]; }
    float getEGroundThreat(Position here) { return getEGroundThreat(WalkPosition(here)); }

    float getEAirThreat(WalkPosition here) { return eAirThreat[here.x][here.y]; }
    float getEAirThreat(Position here) { return getEAirThreat(WalkPosition(here)); }

    float getEGroundCluster(WalkPosition here) { return eGroundCluster[here.x][here.y]; }
    float getEGroundCluster(Position here) { return getEGroundCluster(WalkPosition(here)); }
    float getEAirCluster(WalkPosition here) { return eAirCluster[here.x][here.y]; }
    float getEAirCluster(Position here) { return getEAirCluster(WalkPosition(here)); }

    int getCollision(WalkPosition here) { return collision[here.x][here.y]; }
    int getESplash(WalkPosition here) { return eSplash[here.x][here.y]; }

    int getMobility(WalkPosition here) { return mobility[here.x][here.y]; }
    int getMobility(Position here) { return getMobility(WalkPosition(here)); }

    double getDistanceHome(WalkPosition here) { return distanceHome[here.x][here.y]; }

    int lastVisibleFrame(TilePosition t) { return visibleGrid[t.x][t.y]; }
    int lastVisitedFrame(WalkPosition w) { return visitedGrid[w.x][w.y]; }
}