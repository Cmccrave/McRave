#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UpgradeTypes;
using namespace McRave::BuildOrder::All;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void PvP_1GC_Robo()
    {
        // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"
        focusUnit =                                 enemyMaybeDT() ? Protoss_Observer : Protoss_Reaver;
        inTransition =                              total(Protoss_Robotics_Facility) > 0;
        inOpening =                                 com(Protoss_Robotics_Facility) == 0;

        // Buildings
        buildQueue[Protoss_Gateway] =               (s >= 20) + (vis(Protoss_Robotics_Facility) > 0);
        buildQueue[Protoss_Robotics_Facility] =     s >= 50;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 2;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Reaver] =               com(Protoss_Robotics_Facility) > 0 && com(Protoss_Robotics_Support_Bay) > 0;
        protossUnitPump[Protoss_Observer] =             com(Protoss_Robotics_Facility) > 0 && com(Protoss_Observatory) > 0;
        protossUnitPump[Protoss_Shuttle] =              com(Protoss_Robotics_Facility) > 0 && vis(Protoss_Reaver) > vis(Protoss_Shuttle) * 2;
    }

    void PvP_1GC_3Gate()
    {
        // -nolink-
        inTransition =                              total(Protoss_Gateway) >= 3;
        inOpening =                                 Spy::enemyPressure() ? Broodwar->getFrameCount() < 9000 : Broodwar->getFrameCount() < 8000;
        gasLimit =                                  vis(Protoss_Gateway) >= 2 && com(Protoss_Gateway) < 3 ? 2 : INT_MAX;
        wallNat =                                   Util::getTime() > Time(4, 30);

        // Build
        buildQueue[Protoss_Gateway] =               (s >= 20) + (s >= 38) + (s >= 40);

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 2;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
    }

    void PvP_1GC_4Gate()
    {
        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)"
        inTransition =                              total(Protoss_Gateway) >= 3;
        inOpening =                                 s < 140;
        desiredDetection =                          Protoss_Forge;

        // Buildings
        if (Spy::getEnemyBuild() == P_2Gate) {
            unitLimits[Protoss_Zealot] =            s < 60 ? 4 : 0;
            gasLimit =                              vis(Protoss_Dragoon) > 0 ? 3 : 1;
            buildQueue[Protoss_Shield_Battery] =    enemyMoreZealots() && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;
            buildQueue[Protoss_Gateway] =           (s >= 20) + (vis(Protoss_Pylon) >= 3) + (2 * (s >= 62));
            buildQueue[Protoss_Cybernetics_Core] =  s >= 34;
        }
        else {
            buildQueue[Protoss_Shield_Battery] =    0;
            buildQueue[Protoss_Gateway] =           (s >= 20) + (s >= 40) + (2 * (s >= 62));
            buildQueue[Protoss_Cybernetics_Core] =  s >= 34;
        }

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 2;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
    }

    void PvP_1GC_DT()
    {
        // "https://liquipedia.net/starcraft/2_Gate_DT_(vs._Protoss)"
        focusUnit =                                 Protoss_Dark_Templar;
        inTransition =                              total(Protoss_Citadel_of_Adun) > 0;
        inOpening =                                 s < 90;
        wallNat =                                   (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
        desiredDetection =                          Protoss_Forge;
        hideTech =                                  com(Protoss_Dark_Templar) <= 0;
        unitLimits[Protoss_Zealot] =                vis(Protoss_Photon_Cannon) >= 2 && s < 60 ? INT_MAX : unitLimits[Protoss_Zealot];

        // Buildings
        buildQueue[Protoss_Gateway] =               (s >= 20) + (vis(Protoss_Templar_Archives) > 0);
        buildQueue[Protoss_Forge] =                 s >= 70;
        buildQueue[Protoss_Photon_Cannon] =         2 * (com(Protoss_Forge) > 0);
        buildQueue[Protoss_Citadel_of_Adun] =       vis(Protoss_Dragoon) > 0;
        buildQueue[Protoss_Templar_Archives] =      atPercent(Protoss_Citadel_of_Adun, 1.00);

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 2;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Dark_Templar] =         com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 2;
    }

    void PvP_1GC_NZCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)"
        scout =                                         vis(Protoss_Pylon) > 0;
        transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1;
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 30);
        buildQueue[Protoss_Gateway] =                   s >= 20;
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 26;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               false;
    }

    void PvP_1GC_ZCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)"
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1;
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway] =                   s >= 20;
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 34;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 1;
    }

    void PvP_1GC_ZZCore()
    {
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1;
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway] =                   s >= 20;
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 34;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < (1 + (vis(Protoss_Cybernetics_Core) > 0));
    }

    void PvP_1GC()
    {
        // Reactions
        if (!inTransition) {

            // If enemy is 2gate, switch to some form of 2gate if we haven't gone core yet
            if (Spy::getEnemyBuild() == P_2Gate && vis(Protoss_Cybernetics_Core) == 0) {
                currentBuild = P_2Gate;
                currentOpener = P_10_12;
                currentTransition = P_Robo;
            }

            // Otherwise try to go 3gate after core
            else if (Spy::getEnemyBuild() == P_2Gate && vis(Protoss_Cybernetics_Core) > 0)
                currentTransition = P_3Gate;

            // If our 4Gate would likely kill us
            else if (Scouts::enemyDeniedScout() && currentTransition == P_4Gate)
                currentTransition = P_Robo;

            // If we didn't see enemy info by 3:30
            else if (!Scouts::gotFullScout() && Util::getTime() > Time(3, 30))
                currentTransition = P_Robo;

            // If we're not doing Robo vs potential DT, switch
            else if (currentTransition != P_Robo && Players::getVisibleCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) > 0)
                currentTransition = P_Robo;

            // If we see a FFE, 3Gate with an expansion
            else if (Spy::getEnemyBuild() == P_FFE)
                currentTransition = P_3Gate;
        }

        // Openers
        if (currentOpener == P_NZCore)
            PvP_1GC_NZCore();
        if (currentOpener == P_ZCore)
            PvP_1GC_ZCore();
        if (currentOpener == P_ZZCore)
            PvP_1GC_ZZCore();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_Robo)
                PvP_1GC_Robo();
            if (currentTransition == P_3Gate)
                PvP_1GC_3Gate();
            if (currentTransition == P_4Gate)
                PvP_1GC_4Gate();
            if (currentTransition == P_DT)
                PvP_1GC_DT();
        }
    }
}