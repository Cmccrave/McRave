#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss {

    void defaultPvFFA()
    {
        inOpening       = true;
        inBookSupply    = vis(Protoss_Pylon) < 2;
        wallNat         = vis(Protoss_Nexus) >= 2;
        wallMain        = false;
        scout           = vis(Protoss_Gateway) > 0;
        wantNatural     = false;
        wantThird       = false;
        proxy           = false;
        hideTech        = false;
        rush            = false;
        transitionReady = false;

        gasLimit = INT_MAX;
    }

    int zealotsNeeded_PvFFA()
    {
        if (currentOpener == P_NZCore)
            return 0;
        if (currentOpener == P_ZCore)
            return 1;
        if (currentOpener == P_ZZCore)
            return 1 + (vis(Protoss_Cybernetics_Core) > 0);
        return 1;
    }

    void PvFFA()
    {
        defaultPvFFA();

        // Builds
        if (currentBuild == P_1GateCore)
            PvFFA_1GC();
    }
} // namespace McRave::BuildOrder::Protoss