#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

namespace McRave::BuildOrder::Protoss {

    void defaultPvT() {
        inOpening =                                     true;
        inBookSupply =                                  vis(Protoss_Pylon) < 2;
        wallNat =                                       com(Protoss_Pylon) >= 6;
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
        if (Spy::getEnemyTransition() == U_WorkerRush)
            return 3;

        if (currentOpener == P_NZCore)
            return 0;
        if (currentOpener == P_ZCore)
            return 1;
        return 0;
    }


    void PvT()
    {
        defaultPvT();

        // Builds
        if (currentBuild == P_1GateCore)
            PvT_1GC();
        if (currentBuild == P_2Gate)
            PvT_2G();
        if (currentBuild == P_2Base)
            PvT_2B();
    }
}