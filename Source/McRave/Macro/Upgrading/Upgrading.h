#pragma once
#include "Main/Common.h"

namespace McRave::Upgrading {
    bool haveOrUpgrading(BWAPI::UpgradeType, int level = 1);
    bool haveUpgrade(BWAPI::UpgradeType, int level = 1);
    bool upgradedThisFrame();
    void onFrame();

    int getReservedMineral();
    int getReservedGas();
    bool hasIdleUpgrades();
} // namespace McRave::Upgrading