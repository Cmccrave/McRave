#pragma once
#include <BWAPI.h>

namespace McRave::Producing
{
    int getReservedMineral();
    int getReservedGas();
    double scoreUnit(BWAPI::UnitType);
    bool hasIdleProduction();
    void onFrame();
    bool producedThisFrame();

    bool larvaTrickRequired(UnitInfo& unit);
    bool larvaTrickOptional(UnitInfo& unit);
}