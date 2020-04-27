#pragma once
#include <BWAPI.h>

namespace McRave::Scouts
{
    void onFrame();
    void removeUnit(UnitInfo&);
    bool gotFullScout();
}