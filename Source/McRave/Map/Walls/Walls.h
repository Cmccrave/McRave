#pragma once
#include <BWAPI.h>

namespace McRave::Walls {

    void onStart();
    void onFrame();
    
    int needGroundDefenses(const BWEB::Wall&);
    int needAirDefenses(const BWEB::Wall&);
    const BWEB::Wall * const getMainWall();
    const BWEB::Wall * const getNaturalWall();
}
