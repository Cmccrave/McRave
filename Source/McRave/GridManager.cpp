#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Grids
{
    namespace {
        bool resetGrid[1024][1024] ={};
        int visibleGrid[256][256] ={};
        int visitedGrid[1024][1024] ={};
        vector<WalkPosition> resetVector;

        // Ally Grid
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

        // Ultralisk Grid
        bool ultraWalkable[256][256]={};

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
                resetVector.emplace_back(x, y);
            }
        }

        void addSplash(UnitInfo& unit)
        {
            WalkPosition start(unit.getTarget().getWalkPosition());
            Position target = unit.getTarget().getPosition();

            for (int x = start.x - 12; x <= start.x + 12 + unit.getWalkWidth(); x++) {
                for (int y = start.y - 12; y <= start.y + 12 + unit.getWalkHeight(); y++) {

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
            // If it's an enemy worker, add to grids only if it's near our scout
            if (Util::getTime() < Time(8,0) && BuildOrder::shouldScout() && unit.getType().isWorker() && unit.getPlayer() != Broodwar->self()) {
                const auto closestScout = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Scout && !u.isFlying();
                });

                if (closestScout && unit.unit()->getOrder() != Orders::AttackUnit && unit.unit()->getOrderTargetPosition().getDistance(closestScout->getPosition()) > 64.0)
                    return;
            }

            if (unit.getType() == Protoss_Interceptor
                || unit.getType() == Terran_Medic
                || unit.getType().isWorker())
                return;

            // Pixel and walk sizes
            const auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : unit.getWalkWidth();
            const auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : unit.getWalkHeight();

            // Choose threat grid
            auto grdGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eGroundThreat;
            auto airGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eAirThreat;

            // Choose cluster grid
            auto clusterGrid = unit.getPlayer() == Broodwar->self() ?
                (unit.getType().isFlyer() ? aAirCluster : aGroundCluster) :
                (unit.getType().isFlyer() ? eAirCluster : eGroundCluster);

            // Limit checks so we don't have to check validity
            auto radius = (unit.getPlayer() != Broodwar->self() || unit.getRole() == Role::Combat) ? 1 + int(max(unit.getGroundReach(), unit.getAirReach())) / 8 : 0;

            if (unit.getType().isWorker() &&
                (unit.unit()->isConstructing() || unit.unit()->isGatheringGas() || unit.unit()->isGatheringMinerals()))
                radius = radius / 3;

            const auto left = max(0, unit.getWalkPosition().x - radius);
            const auto right = min(1024, unit.getWalkPosition().x + walkWidth + radius);
            const auto top = max(0, unit.getWalkPosition().y - radius);
            const auto bottom = min(1024, unit.getWalkPosition().y + walkHeight + radius);

            // Pixel rectangle (make any even size units an extra WalkPosition)
            const auto ff = unit.getType().isBuilding() ? Position(0, 0) : Position(8, 8);
            const auto topLeft = Position(unit.getWalkPosition());
            const auto botRight = topLeft + Position(walkWidth * 8, walkHeight * 8) + Position(8 * (1 - unit.getWalkWidth() % 2), 8 * (1 - unit.getWalkHeight() % 2));
            const auto x1 = unit.getPosition().x;
            const auto y1 = unit.getPosition().y;

            auto inRange = true;
            auto closest = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto&u) { return true; });

            // If no nearby unit owned by self, ignore threat grids
            if (!closest)
                inRange = false;
            else {
                const auto dist = closest->getPosition().getDistance(unit.getPosition()) - 640.0;
                const auto vision = closest->getType().sightRange();
                const auto range = max({ closest->getGroundRange(), closest->getAirRange(), unit.getGroundRange(), unit.getAirRange() });

                // If out of vision and range
                if (dist > vision && dist > range)
                    inRange = false;
            }

            // Iterate tiles and add to grid
            for (int x = left; x < right; x++) {
                for (int y = top; y < bottom; y++) {

                    //const auto dist = fasterDistGrids(x1, y1, (x * 8) + 4, (y * 8) + 4);
                    const auto dist = float(unit.getPosition().getDistance(Position((x * 8) + 4, (y * 8) + 4)));

                    // Cluster
                    if (clusterGrid && dist < 64.0 && (unit.getPlayer() != Broodwar->self() || unit.getRole() == Role::Combat)) {
                        clusterGrid[x][y] += (65.0f - dist) / 64.0f;
                        saveReset(x, y);
                    }

                    // Collision
                    if (!unit.isFlying() && !unit.isBurrowed() && Util::rectangleIntersect(topLeft, botRight, x * 8, y * 8)) {
                        collision[x][y] += 1;
                        saveReset(x, y);
                    }

                    // Threat
                    if (inRange && grdGrid && dist <= unit.getGroundReach()) {
                        grdGrid[x][y] += float(unit.getVisibleGroundStrength());
                        saveReset(x, y);
                    }
                    if (/*inRange && */airGrid && dist <= unit.getAirReach()) {
                        airGrid[x][y] += float(unit.getVisibleAirStrength());
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
            Visuals::endPerfTest("Grid Reset");
        }

        void updateAlly()
        {
            Visuals::startPerfTest();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;

                // Pixel and walk sizes
                auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0);
                auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0);

                // Add a visited grid for rough guideline of what we've seen by this unit recently
                auto start = unit.getWalkPosition();
                for (int x = start.x - 2; x < start.x + walkWidth + 2; x++) {
                    for (int y = start.y - 2; y < start.y + walkHeight + 2; y++) {
                        auto t = WalkPosition(x, y);
                        if (t.isValid()) {
                            visitedGrid[x][y] = Broodwar->getFrameCount();
                            //Broodwar->drawBoxMap(Position(t), Position(t) + Position(9, 9), Colors::Green);
                        }
                    }
                }

                // Spider mines are added to the enemy splash grid so ally units avoid allied mines
                if (unit.getType() == Terran_Vulture_Spider_Mine) {
                    if (!unit.isBurrowed() && unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
                        addSplash(unit);
                }

                else if (!unit.unit()->isLoaded())
                    addToGrids(unit);
            }
            Visuals::endPerfTest("Grid Self");
        }

        void updateEnemy()
        {
            Visuals::startPerfTest();
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;
                if (unit.unit()->exists() && (unit.unit()->isStasised() || unit.unit()->isMaelstrommed()))
                    continue;

                auto longRangeUnit = unit.getGroundRange() >= 224.0;

                if (unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Protoss_Scarab) {
                    if (unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
                        addSplash(unit);
                }
                else {
                    addToGrids(unit);
                }
            }
            Visuals::endPerfTest("Grid Enemy");
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

                        collision[x][y] += 1;
                        saveReset(x, y);
                    }
                }

                if (u->getType() == Spell_Disruption_Web)
                    Actions::addAction(u, u->getPosition(), TechTypes::Disruption_Web, PlayerState::Neutral);
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

        void updateMobility()
        {
            for (auto &gas : Broodwar->getGeysers()) {
                auto t = WalkPosition(gas->getTilePosition());
                for (int x = t.x; x < t.x + gas->getType().tileWidth() * 4; x++) {
                    for (int y = t.y; y < t.y + gas->getType().tileHeight() * 4; y++) {
                        mobility[x][y] = -1;
                    }
                }
            }

            for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {

                    WalkPosition w(x, y);
                    if (!w.isValid() || mobility[x][y] != 0)
                        continue;

                    if (!Broodwar->isWalkable(w)) {
                        mobility[x][y] = -1;
                        continue;
                    }

                    for (int i = -12; i < 12; i++) {
                        for (int j = -12; j < 12; j++) {

                            WalkPosition w2(x + i, y + j);
                            if (w2.isValid() && mapBWEM.GetMiniTile(w2).Walkable() && mobility[x + i][y + j] != -1)
                                mobility[x][y] += 1;
                        }
                    }

                    mobility[x][y] = clamp(int(floor(mobility[x][y] / 56)), 1, 10);


                    // Island
                    if (mapBWEM.GetArea(w) && mapBWEM.GetArea(w)->AccessibleNeighbours().size() == 0)
                        mobility[x][y] = -1;
                }
            }
        }

        void updateUltraGrid()
        {
            
        }
    }

    void onFrame()
    {
        reset();
        updateVisibility();
        updateAlly();
        updateEnemy();
        updateNeutral();
    }

    void onStart()
    {
        updateMobility();
        updateUltraGrid();

        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings) {
            for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
                    visibleGrid[x][y] = -9999;
                }
            }
        }
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

    int lastVisibleFrame(TilePosition t) { return visibleGrid[t.x][t.y]; }
    int lastVisitedFrame(WalkPosition w) { return visitedGrid[w.x][w.y]; }

    void addMovement(Position here, UnitInfo& unit)
    {
        if (unit.isLightAir()) {
            const auto walkWidth = unit.getWalkWidth();
            const auto walkHeight = unit.getWalkHeight();

            const auto left = max(0, WalkPosition(here).x - walkWidth - 4);
            const auto right = min(1024, WalkPosition(here).x + walkWidth + 4);
            const auto top = max(0, WalkPosition(here).y - walkHeight - 4);
            const auto bottom = min(1024, WalkPosition(here).y + walkHeight + 4);

            for (int x = left; x < right; x++) {
                for (int y = top; y < bottom; y++) {

                    //const auto dist = fasterDistGrids(x1, y1, (x * 8) + 4, (y * 8) + 4);
                    const auto dist = float(here.getDistance(Position((x * 8) + 4, (y * 8) + 4)));

                    // Cluster
                    if (dist < 64.0) {
                        aAirCluster[x][y] += (65.0f - dist) / 64.0f;
                        saveReset(x, y);
                    }
                }
            }
        }

        // Not using collision movement
        return;

        /*const auto w = WalkPosition(here);
        const auto hw = unit.getWalkWidth() / 2;
        const auto hh = unit.getWalkHeight() / 2;

        auto left = max(0, here.x - hw);
        auto right = min(1024, here.x + hw + wOffset);
        auto top = max(0, here.y - hh);
        auto bottom = min(1024, here.y + hh + hOffset);

        for (int x = left; x < right; x++) {
            for (int y = top; y < bottom; y++) {
                collision[x][y] += 1;
                saveReset(x, y);
            }
        }*/
    }

    bool isUltraWalkable(TilePosition t) { return ultraWalkable[t.x][t.y]; }
}