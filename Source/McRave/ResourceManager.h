#pragma once
#include <BWAPI.h>
#include "ResourceInfo.h"

namespace McRave::Resources
{
    int getGasCount();
    int getIncomeMineral();
    int getIncomeGas();
    bool isMinSaturated();
    bool isGasSaturated();
    map <Unit, ResourceInfo>& getMyMinerals();
    map <Unit, ResourceInfo>& getMyGas();
    map <Unit, ResourceInfo>& getMyBoulders();

    void onFrame();
    void storeResource(Unit);
    void removeResource(Unit);
}
