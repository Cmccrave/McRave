#pragma once
#include <BWAPI.h>

namespace McRave::Grids
{
    void onFrame();
    void onStart();

    float getAGroundCluster(WalkPosition here);
    float getAGroundCluster(Position here);

    float getAAirCluster(WalkPosition here);
    float getAAirCluster(Position here);

    float getEGroundThreat(WalkPosition here);
    float getEGroundThreat(Position here);

    float getEAirThreat(WalkPosition here);
    float getEAirThreat(Position here);

    float getEGroundCluster(WalkPosition here);
    float getEGroundCluster(Position here);
    float getEAirCluster(WalkPosition here);
    float getEAirCluster(Position here);

    int getCollision(WalkPosition here);
    int getESplash(WalkPosition here);

    int getMobility(WalkPosition here);
    double getDistanceHome(WalkPosition here);

    int lastVisibleFrame(TilePosition t);
    int lastVisitedFrame(WalkPosition w);
}