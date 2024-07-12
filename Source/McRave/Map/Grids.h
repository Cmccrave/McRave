#pragma once
#include <BWAPI.h>

namespace McRave::Grids
{
    void onFrame();
    void onStart();

    float getGroundThreat(BWAPI::Position, PlayerState);
    float getGroundThreat(BWAPI::WalkPosition, PlayerState);
    float getGroundThreat(BWAPI::TilePosition, PlayerState);
    float getAirThreat(BWAPI::Position, PlayerState);
    float getAirThreat(BWAPI::WalkPosition, PlayerState);
    float getAirThreat(BWAPI::TilePosition, PlayerState);
    float getGroundDensity(BWAPI::Position, PlayerState);
    float getGroundDensity(BWAPI::WalkPosition, PlayerState);
    float getAirDensity(BWAPI::Position, PlayerState);
    float getAirDensity(BWAPI::WalkPosition, PlayerState);    
    int getFCollision(BWAPI::WalkPosition, PlayerState);
    int getVCollision(BWAPI::WalkPosition, PlayerState);
    int getHCollision(BWAPI::WalkPosition, PlayerState);
    int getMobility(BWAPI::WalkPosition);
    int getMobility(BWAPI::Position);
    int getLastVisibleFrame(BWAPI::Position);
    int getLastVisibleFrame(BWAPI::TilePosition);
}