#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UpgradeTypes;
using namespace McRave::BuildOrder::All;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void PvP_2G_DT()
    {
        inTransition =                              total(Protoss_Citadel_of_Adun) > 0;
        inOpening =                                 s < 80;
        focusUnit =                                 Protoss_Dark_Templar;
        wallNat =                                   (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
        hideTech =                                  com(Protoss_Dark_Templar) <= 0;

        // Buildings
        buildQueue[Protoss_Gateway] =               2 + (com(Protoss_Cybernetics_Core) > 0);
        buildQueue[Protoss_Nexus] =                 1;
        buildQueue[Protoss_Assimilator] =           total(Protoss_Zealot) >= 3;
        buildQueue[Protoss_Cybernetics_Core] =      (total(Protoss_Zealot) >= 5 && vis(Protoss_Assimilator) >= 1);
        buildQueue[Protoss_Citadel_of_Adun] =       atPercent(Protoss_Cybernetics_Core, 1.00);
        buildQueue[Protoss_Templar_Archives] =      atPercent(Protoss_Citadel_of_Adun, 1.00);
        buildQueue[Protoss_Forge] =                 s >= 70;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 5;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Dark_Templar] =         com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 2;
    }

    void PvP_2G_Robo()
    {
        // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"
        inTransition =                              total(Protoss_Robotics_Facility) > 0;
        inOpening =                                 com(Protoss_Robotics_Facility) == 0;
        focusUnit =                                 enemyMaybeDT() ? Protoss_Observer : Protoss_Reaver;

        // Buildings
        buildQueue[Protoss_Gateway] =               2 + (com(Protoss_Robotics_Facility) > 0);
        buildQueue[Protoss_Nexus] =                 1;
        buildQueue[Protoss_Assimilator] =           total(Protoss_Zealot) >= 3;
        buildQueue[Protoss_Cybernetics_Core] =      (total(Protoss_Zealot) >= 5 && vis(Protoss_Assimilator) >= 1);
        buildQueue[Protoss_Robotics_Facility] =     com(Protoss_Dragoon) >= 2;
        buildQueue[Protoss_Observatory] =           enemyMaybeDT() || total(Protoss_Reaver) > 0;
        buildQueue[Protoss_Robotics_Support_Bay] =  !enemyMaybeDT() || total(Protoss_Observer) > 0;

        // Upgrades
        upgradeQueue[Singularity_Charge] =          vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 5;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Reaver] =               com(Protoss_Robotics_Facility) > 0 && com(Protoss_Robotics_Support_Bay) > 0;
        protossUnitPump[Protoss_Observer] =             com(Protoss_Robotics_Facility) > 0 && com(Protoss_Observatory) > 0;
        protossUnitPump[Protoss_Shuttle] =              com(Protoss_Robotics_Facility) > 0 && vis(Protoss_Reaver) > vis(Protoss_Shuttle) * 2;
    }

    void PvP_2G_Main()
    {
        // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)" - 10/12
        unitLimits[Protoss_Zealot] =                    s <= 80 ? 5 : 0;
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        transitionReady =                               vis(Protoss_Gateway) >= 2;
        desiredDetection =                              Protoss_Forge;
        gasLimit =                                      total(Protoss_Zealot) >= 3 ? INT_MAX : 0;

        // Buildings
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 24);
        buildQueue[Protoss_Shield_Battery] =            Spy::enemyRush() && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 5;
    }

    void PvP_2G()
    {
        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyBuild() == "FFE" || Spy::getEnemyTransition() == "DT")
                currentTransition = "Robo";
            else if (Spy::getEnemyBuild() == "CannonRush")
                currentTransition = "Robo";
            else if (Spy::enemyPressure())
                currentTransition = "DT";
        }

        // Openers
        if (currentOpener == "Main")
            PvP_2G_Main();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "Robo")
                PvP_2G_Robo();
            if (currentTransition == "DT")
                PvP_2G_DT();
        }
    }
}