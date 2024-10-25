#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void defaultPvT() {
        inOpening =                                     true;
        inBookSupply =                                  vis(Protoss_Pylon) < 2;
        wallNat =                                       com(Protoss_Pylon) >= 6 || currentOpener == "Natural";
        wallMain =                                      false;
        scout =                                         vis(Protoss_Cybernetics_Core) > 0;
        wantNatural =                                   true;
        wantThird =                                     true;
        proxy =                                         false;
        hideTech =                                      false;
        rush =                                          false;
        transitionReady =                               false;

        gasLimit =                                      gasMax();

        desiredDetection =                              Protoss_Observer;
    }

    int zealotsNeeded_PvT() {
        if (currentOpener == "NZCore")
            return 0;
        if (currentOpener == "ZCore")
            return 1;
        return 0;
    }


    void PvT()
    {
        defaultPvT();

        // Builds
        if (currentBuild == "1GateCore")
            PvT_1GC();
        if (currentBuild == "2Gate")
            PvT_2G();
        if (currentBuild == "2Base")
            PvT_2B();
    }
}