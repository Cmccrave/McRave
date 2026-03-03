#pragma once
#include "Main/Common.h"
#include "BWEB.h"

namespace McRave::Expansion {

    void onFrame();
    void onStart();

    bool expansionBlockersExists();
    std::vector<const BWEB::Station*> getExpandOrder();
}
