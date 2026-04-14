#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UpgradeTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss {

    void PvP_2G_DT()
    {
        inTransition = total(Protoss_Citadel_of_Adun) > 0;
        inOpening    = s < 80;
        inBookSupply = vis(Protoss_Pylon) < 3;

        focusUnit = Protoss_Dark_Templar;
        wallNat   = (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
        hideTech  = com(Protoss_Dark_Templar) <= 0;

        // Buildings
        buildQueue[Protoss_Pylon]            = (s >= 16) + (s >= 32) + (vis(Protoss_Cybernetics_Core) > 0);
        buildQueue[Protoss_Gateway]          = 2 + (com(Protoss_Cybernetics_Core) > 0);
        buildQueue[Protoss_Nexus]            = 1;
        buildQueue[Protoss_Assimilator]      = total(Protoss_Zealot) >= 1;
        buildQueue[Protoss_Cybernetics_Core] = total(Protoss_Zealot) >= 3 && vis(Protoss_Assimilator) >= 1 && s >= 46;
        buildQueue[Protoss_Citadel_of_Adun]  = atPercent(Protoss_Cybernetics_Core, 1.00);
        buildQueue[Protoss_Templar_Archives] = atPercent(Protoss_Citadel_of_Adun, 1.00);
        buildQueue[Protoss_Forge]            = s >= 70;

        // Pumping
        protossUnitPump[Protoss_Probe]        = true;
        protossUnitPump[Protoss_Zealot]       = total(Protoss_Zealot) < zealotsNeeded_PvP();
        protossUnitPump[Protoss_Dragoon]      = true;
        protossUnitPump[Protoss_Dark_Templar] = com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 2;
    }

    void PvP_2G_Robo()
    {
        // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"
        inTransition = total(Protoss_Robotics_Facility) > 0;
        inOpening    = com(Protoss_Robotics_Facility) == 0;
        inBookSupply = vis(Protoss_Pylon) < 3;

        focusUnit = enemyMaybeDT() ? Protoss_Observer : Protoss_Reaver;

        // Buildings
        buildQueue[Protoss_Pylon]             = (s >= 16) + (s >= 32) + (vis(Protoss_Cybernetics_Core) > 0);
        buildQueue[Protoss_Gateway]           = 2 + (com(Protoss_Robotics_Facility) > 0);
        buildQueue[Protoss_Nexus]             = 1;
        buildQueue[Protoss_Assimilator]       = total(Protoss_Zealot) >= 1;
        buildQueue[Protoss_Cybernetics_Core]  = total(Protoss_Zealot) >= 3 && vis(Protoss_Assimilator) >= 1 && s >= 46;
        buildQueue[Protoss_Robotics_Facility] = com(Protoss_Dragoon) >= 2;

        // Upgrades
        upgradeQueue[Singularity_Charge] = vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe]   = true;
        protossUnitPump[Protoss_Zealot]  = total(Protoss_Zealot) < zealotsNeeded_PvP();
        protossUnitPump[Protoss_Dragoon] = true;
    }

    void PvP_2G_Main()
    {
        // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)" - 10/12
        scout           = Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        transitionReady = vis(Protoss_Gateway) >= 2;
        gasLimit        = total(Protoss_Zealot) >= 3 ? INT_MAX : 0;

        // Buildings
        buildQueue[Protoss_Pylon]          = (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway]        = (s >= 20) + (s >= 24);
        buildQueue[Protoss_Shield_Battery] = Spy::enemyRush() && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;

        // Pumping
        protossUnitPump[Protoss_Probe]  = true;
        protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 5;
    }

    void PvP_2G()
    {
        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyBuild() == P_FFE || Spy::getEnemyTransition() == P_DT)
                currentTransition = P_Robo;
            else if (Spy::getEnemyBuild() == P_CannonRush)
                currentTransition = P_Robo;
            else if (Spy::enemyPressure())
                currentTransition = P_DT;
        }

        // Openers
        if (currentOpener == P_10_12)
            PvP_2G_Main();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_Robo)
                PvP_2G_Robo();
            if (currentTransition == P_DT)
                PvP_2G_DT();
        }
    }
} // namespace McRave::BuildOrder::Protoss