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

    void PvZ_2G_4Gate()
    {
        inOpening =                                 s < 110;
        inTransition =                              true;
        wallNat =                                   vis(Protoss_Nexus) >= 2;

        // Buildings
        buildQueue[Protoss_Gateway] =               2 + (s >= 62) + (s >= 70);
        buildQueue[Protoss_Assimilator] =           s >= 52;
        buildQueue[Protoss_Cybernetics_Core] =      total(Protoss_Zealot) >= 5;

        // Upgrades
        upgradeQueue[Singularity_Charge] =          vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] = true;
        protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 5;
        protossUnitPump[Protoss_Dragoon] = com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
    }

    void PvZ_2G_10_12()
    {
        // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)" - 10/12
        wallNat =                                       vis(Protoss_Nexus) >= 2;
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        transitionReady =                               vis(Protoss_Gateway) >= 2;

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Buildings
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 24);
        buildQueue[Protoss_Shield_Battery] =            Spy::enemyRush() && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;

        // Pumping
        protossUnitPump[Protoss_Probe] = true;
        protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
    }

    void PvZ_2G()
    {
        // Openers
        if (currentOpener == P_10_12)
            PvZ_2G_10_12();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_4Gate)
                PvZ_2G_4Gate();
        }
    }
}