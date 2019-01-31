#pragma once
#include <BWAPI.h>

namespace McRave::Workers {

    void onFrame();
    void removeUnit(const std::shared_ptr<UnitInfo>&);
}
