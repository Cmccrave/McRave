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
    std::map <BWAPI::Unit, ResourceInfo>& getMyMinerals();
    std::map <BWAPI::Unit, ResourceInfo>& getMyGas();
    std::map <BWAPI::Unit, ResourceInfo>& getMyBoulders();

    void onFrame();
    void storeResource(BWAPI::Unit);
    void removeResource(BWAPI::Unit);
}
