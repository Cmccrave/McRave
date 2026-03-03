#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss {

    void defaultPvZ() {
        inOpening =                                     true;
        inBookSupply =                                  vis(Protoss_Pylon) < 2;
        wallNat =                                       vis(Protoss_Nexus) >= 2;
        wallMain =                                      false;
        scout =                                         vis(Protoss_Gateway) > 0;
        wantNatural =                                   false;
        wantThird =                                     false;
        proxy =                                         false;
        hideTech =                                      false;
        rush =                                          false;
        transitionReady =                               false;

        gasLimit =                                      INT_MAX;

        desiredDetection =                              Protoss_Observer;
        focusUnit =                                     UnitTypes::None;
    }

    void PvZ()
    {
        defaultPvZ();

        // Builds
        if (currentBuild == P_2Gate)
            PvZ_2G();
        if (currentBuild == P_1GateCore)
            PvZ_1GC();
        if (currentBuild == P_FFE)
            PvZ_FFE();
    }
}