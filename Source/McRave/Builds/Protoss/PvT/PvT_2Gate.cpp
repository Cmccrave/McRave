#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void PvT_2G_DT()
    {
        inTransition =                                  total(Protoss_Citadel_of_Adun) > 0;
        inOpening =                                     total(Protoss_Dark_Templar) < 4;

        gasLimit =                                      Spy::getEnemyBuild() == T_2Rax && total(Protoss_Gateway) < 2 ? 0 : 3;
        focusUnit =                                     Protoss_Dark_Templar;
        hideTech =                                      true;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1 + (vis(Protoss_Templar_Archives) > 0);
        buildQueue[Protoss_Citadel_of_Adun] =           vis(Protoss_Dragoon) >= 3;
        buildQueue[Protoss_Templar_Archives] =          atPercent(Protoss_Citadel_of_Adun, 1.00);

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Citadel_of_Adun) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Dark_Templar] =         com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 4;
    }

    void PvT_2G_Robo()
    {
        inTransition =                                  total(Protoss_Robotics_Facility) > 0;
        inOpening =                                     s < 70;
        focusUnit =                                     Protoss_Reaver;

        // Buildings
        buildQueue[Protoss_Robotics_Facility] =         total(Protoss_Dragoon) >= 3;
        buildQueue[Protoss_Robotics_Support_Bay] =      com(Protoss_Robotics_Facility) > 0;

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Reaver] =               com(Protoss_Robotics_Facility) > 0 && com(Protoss_Robotics_Support_Bay) > 0;
        protossUnitPump[Protoss_Observer] =             com(Protoss_Robotics_Facility) > 0 && com(Protoss_Observatory) > 0;
        protossUnitPump[Protoss_Shuttle] =              com(Protoss_Robotics_Facility) > 0 && vis(Protoss_Reaver) > vis(Protoss_Shuttle) * 2;
    }

    void PvT_2G_1015()
    {
        // "https://liquipedia.net/starcraft/10/15_Gates_(vs._Terran)"
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        transitionReady =                               vis(Protoss_Gateway) >= 2;

        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 30);
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 30);
        buildQueue[Protoss_Cybernetics_Core] =          s >= 26;
        buildQueue[Protoss_Assimilator] =               s >= 22;

        // Composition
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);

    }

    void PvT_2G()
    {
        // Reactions
        if (!inTransition) {
            if (currentBuild == P_2Gate) {
                if (Spy::enemyFastExpand())
                    currentTransition = P_DT;
            }
        }

        // Builds
        if (currentBuild == P_2Gate)
            PvT_2G_1015();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_Robo)
                PvT_2G_Robo();
            if (currentTransition == P_DT)
                PvT_2G_DT();
        }
    }
}