#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void PvFFA_1GC_3Gate()
    {
        // -nolink-
        inTransition =                                  total(Protoss_Gateway) >= 3;
        inOpening =                                     s < 82;
        gasLimit =                                      vis(Protoss_Gateway) >= 2 && com(Protoss_Gateway) < 3 ? 2 : INT_MAX;
        wallNat =                                       Util::getTime() > Time(4, 30);

        // Build
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 38) + (s >= 40);
    }

    void PvFFA_1GC_ZCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)"
        unitLimits[Protoss_Zealot] =                    s >= 60 ? 0 : 1;
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1;
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway] =                   s >= 20;
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 34;

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;
    }

    void PvFFA_1GC()
    {
        // Openers
        if (currentOpener == "1Zealot")
            PvFFA_1GC_ZCore();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "3Gate")
                PvFFA_1GC_3Gate();
        }
    }
}