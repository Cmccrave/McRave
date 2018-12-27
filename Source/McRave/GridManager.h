#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

#pragma warning(disable : 4351)

namespace McRave
{
    class BaseInfo;
    class GridManager
    {
        bool resetGrid[1024][1024] ={};
        int timeGrid[1024][1024] ={};
        int visibleGrid[256][256] ={};
        int visitedGrid[1024][1024] ={};
        vector<WalkPosition> resetVector;

        int currentFrame = 0;

        // Ally Grid
        double parentDistance[1024][1024];
        double aGroundCluster[1024][1024] ={};
        double aAirCluster[1024][1024] ={};

        // Enemy Grid
        double eGroundThreat[1024][1024] ={};
        double eAirThreat[1024][1024] ={};
        double eGroundCluster[1024][1024] ={};
        double eAirCluster[1024][1024] ={};
        int eSplash[1024][1024] ={};

        // Mobility Grid
        int mobility[1024][1024] ={};
        int collision[1024][1024] ={};
        double distanceHome[1024][1024] ={};

        void saveReset(WalkPosition);
        void updateDistance(), updateMobility(), updateAlly(), updateEnemy(), updateNeutral(), updateVisibility(), reset(), draw();

        void addToGrids(UnitInfo&);
        void addSplash(UnitInfo&);
    public:

        // Update functions
        void onFrame(), onStart();

        void updateAllyMovement(Unit, WalkPosition);

        double getAGroundCluster(WalkPosition here) { return aGroundCluster[here.x][here.y]; }
        double getAGroundCluster(Position here) { return getAGroundCluster(WalkPosition(here)); }

        double getAAirCluster(WalkPosition here) { return aAirCluster[here.x][here.y]; }
        double getAAirCluster(Position here) { return getAAirCluster(WalkPosition(here)); }

        double getEGroundThreat(WalkPosition here) { return eGroundThreat[here.x][here.y]; }
        double getEGroundThreat(Position here) { return getEGroundThreat(WalkPosition(here)); }

        double getEAirThreat(WalkPosition here) { return eAirThreat[here.x][here.y]; }
        double getEAirThreat(Position here) { return getEAirThreat(WalkPosition(here)); }

        double getEGroundCluster(WalkPosition here) { return eGroundCluster[here.x][here.y]; }
        double getEGroundCluster(Position here) { return getEGroundCluster(WalkPosition(here)); }
        double getEAirCluster(WalkPosition here) { return eAirCluster[here.x][here.y]; }
        double getEAirCluster(Position here) { return getEAirCluster(WalkPosition(here)); }

        int getCollision(WalkPosition here) { return collision[here.x][here.y]; }
        int getESplash(WalkPosition here) { return eSplash[here.x][here.y]; }

        int getMobility(WalkPosition here) { return mobility[here.x][here.y]; }
        double getDistanceHome(WalkPosition here) { return distanceHome[here.x][here.y]; }

        int lastVisibleFrame(TilePosition t) { return visibleGrid[t.x][t.y]; }
        int lastVisitedFrame(WalkPosition w) { return visitedGrid[w.x][w.y]; }
    };

}

typedef Singleton<McRave::GridManager> GridSingleton;