#pragma once
#include <BWAPI.h>

namespace McRave::Researching
{
    bool haveOrResearching(BWAPI::TechType);
    bool researchedThisFrame();
    void onFrame();

    int getReservedMineral();
    int getReservedGas();
    bool hasIdleResearch();
}