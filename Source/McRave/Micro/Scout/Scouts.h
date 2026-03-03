#pragma once
#include "Main/Common.h"
#include "BWEB.h"

namespace McRave::Scouts
{
    void onFrame();
    void onStart();
    void removeUnit(UnitInfo&);
    bool gatheringInformation();
    bool gotFullScout();
    bool isSacrificeScout();
    bool enemyDeniedScout();
    std::vector<const BWEB::Station *> getScoutOrder(BWAPI::UnitType);
}