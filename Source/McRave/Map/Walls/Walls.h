#pragma once
#include "BWEB.h"

namespace McRave::Walls {

    void onStart();
    void onFrame();

    bool isComplete(BWEB::Wall *const);
    int getColonyCount(BWEB::Wall *const);
    int needGroundDefenses(BWEB::Wall &);
    int needAirDefenses(BWEB::Wall &);
    BWEB::Wall *const getMainWall();
    BWEB::Wall *const getNaturalWall();
} // namespace McRave::Walls
