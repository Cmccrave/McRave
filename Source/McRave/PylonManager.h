#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

// Reference: https://docs.google.com/spreadsheets/d/188VI8uHoV8xzoQusGg7hDO5JeFLGO1i2b_5P3jc_-88/
// Note that small and medium buildings have identical requirements

namespace McRave::Pylons
{
    void storePylon(Unit);
    bool hasPower(TilePosition, UnitType);
}
