#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void defaultPvZ() {
        inOpening =                                 true;
        inBookSupply =                                  vis(Protoss_Pylon) < 2;
        wallNat =                                       vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
        wallMain =                                      false;
        scout =                                         vis(Protoss_Gateway) > 0;
        wantNatural =                                   false;
        wantThird =                                     false;
        proxy =                                         false;
        hideTech =                                      false;
        rush =                                          false;
        transitionReady =                               false;

        gasLimit =                                      INT_MAX;
        unitLimits[Protoss_Zealot] =                    INT_MAX;
        unitLimits[Protoss_Dragoon] =                   0;
        unitLimits[Protoss_Probe] =                     INT_MAX;

        desiredDetection =                              Protoss_Observer;
        focusUpgrade =                                  vis(Protoss_Dragoon) > 0 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        focusTech =                                     TechTypes::None;
        focusUnit =                                     None;

        armyComposition[Protoss_Probe] =                1.00;
        armyComposition[Protoss_Zealot] =               1.00;
    }

    void PvZ()
    {
        defaultPvZ();

        // Builds
        if (currentBuild == "2Gate")
            PvZ_2G();
        if (currentBuild == "1GateCore")
            PvZ_1GC();
        if (currentBuild == "FFE")
            PvZ_FFE();
    }
}