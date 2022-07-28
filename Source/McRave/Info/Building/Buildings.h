#pragma once
#include "Main/McRave.h"

namespace McRave::Buildings {

    void onFrame();

    std::set<BWAPI::TilePosition>& getUnpoweredPositions();
    std::set<BWAPI::Position>& getLarvaPositions();
    bool isValidDefense(BWAPI::TilePosition);
};