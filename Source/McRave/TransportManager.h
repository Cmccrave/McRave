#pragma once
#include <BWAPI.h>

namespace McRave::Transports {

    void onFrame();
    void removeUnit(const std::shared_ptr<UnitInfo>&);
}
