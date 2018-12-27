#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Production
{
    int getReservedMineral();
    int getReservedGas();
    bool hasIdleProduction();
    void onFrame();
}