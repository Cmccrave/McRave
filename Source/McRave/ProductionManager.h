#pragma once
#include <BWAPI.h>

namespace McRave::Production
{
    int getReservedMineral();
    int getReservedGas();
    bool hasIdleProduction();
    void onFrame();
}