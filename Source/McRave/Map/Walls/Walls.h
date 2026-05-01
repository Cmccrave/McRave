#pragma once
#include "BWEB.h"

namespace McRave::Walls {

    void onStart();
    void onFrame();

    bool isDefenseFilled(BWEB::Wall *const);
    bool isComplete(BWEB::Wall *const);
    int getColonyCount(BWEB::Wall *const);
    int needGroundDefenses(BWEB::Wall * const);
    int needAirDefenses(BWEB::Wall * const);
    BWEB::Wall *const getMainWall();
    BWEB::Wall *const getNaturalWall();
} // namespace McRave::Walls
