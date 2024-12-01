#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void PvT_1GC_Robo()
    {
        // "http://liquipedia.net/starcraft/1_Gate_Reaver" 
        inTransition =                                  total(Protoss_Robotics_Facility) > 0;
        inOpening =                                     s < 60;
        hideTech =                                      com(Protoss_Reaver) <= 0;
        focusUnit =                                     Spy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1 + (s >= 74);
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 60) + (s >= 62);
        buildQueue[Protoss_Robotics_Facility] =         s >= 52;
        buildQueue[Protoss_Robotics_Support_Bay] =      com(Protoss_Robotics_Facility) > 0 && focusUnit == Protoss_Reaver;
        buildQueue[Protoss_Observatory] =               com(Protoss_Robotics_Facility) > 0 && focusUnit == Protoss_Observer;

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

    void PvT_1GC_DT()
    {
        // "https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)"
        inTransition =                                  total(Protoss_Citadel_of_Adun) > 0;
        inOpening =                                     total(Protoss_Dark_Templar) < 4;
        hideTech =                                      com(Protoss_Dark_Templar) <= 0;
        focusUnit =                                     Protoss_Dark_Templar;

        // Buildings
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (vis(Protoss_Templar_Archives) > 0);
        buildQueue[Protoss_Nexus] =                     1 + (total(Protoss_Dark_Templar) >= 2);
        buildQueue[Protoss_Assimilator] =               (s >= 24) + (vis(Protoss_Nexus) >= 2);
        buildQueue[Protoss_Citadel_of_Adun] =           (com(Protoss_Cybernetics_Core) > 0) && s >= 36;
        buildQueue[Protoss_Templar_Archives] =          (com(Protoss_Citadel_of_Adun) > 0) && s >= 48;

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Citadel_of_Adun) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Dark_Templar] =         com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 4;
    }

    void PvT_1GC_4Gate()
    {
        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)"
        inTransition =                                  total(Protoss_Gateway) >= 4;
        inOpening =                                     s < 80;

        // Buildings
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 30) + (2 * (s >= 62));
        buildQueue[Protoss_Assimilator] =               s >= 22;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 26;

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
    }

    void PvT_NZCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)"
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

        // Buildings
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 30);
        buildQueue[Protoss_Gateway] =                   s >= 20;
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 26;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
    }

    void PvT_ZCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)"
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

        // Buildings
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
        buildQueue[Protoss_Gateway] =                   s >= 20;
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Cybernetics_Core] =          s >= 36;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
    }

    void PvT_1GC()
    {
        // Reactions
        if (!inTransition) {
            if (currentBuild == P_1GateCore) {
                if (Spy::enemyRush()) {
                    currentBuild = P_2Gate;
                    currentOpener = P_10_12;
                    currentTransition = P_DT;
                }
                else if (Spy::enemyFastExpand())
                    currentTransition = P_DT;
            }
            if (currentBuild == P_2Gate) {
                if (Spy::enemyFastExpand())
                    currentTransition = P_DT;
            }
        }

        // Openers
        if (currentOpener == P_NZCore)
            PvT_NZCore();
        if (currentOpener == P_ZCore)
            PvT_ZCore();

        // Transitions
        if (currentTransition == P_Robo)
            PvT_1GC_Robo();
        if (currentTransition == P_DT)
            PvT_1GC_DT();
        if (currentTransition == P_4Gate)
            PvT_1GC_4Gate();
    }
}