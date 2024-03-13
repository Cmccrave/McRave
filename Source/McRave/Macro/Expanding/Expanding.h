#pragma once
#include "Main/McRave.h"

namespace McRave::Expansion {

    void onFrame();
    void onStart();

    bool expansionBlockersExists();
    std::vector<const BWEB::Station*> getExpandOrder();
}
