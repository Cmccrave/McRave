#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    namespace {

        void PvZ_1GC_DT()
        {
            // Experimental build from Best
            inOpening =                                     s < 70;
            inTransition =                                  total(Protoss_Citadel_of_Adun) > 0;
            focusUnit =                                     Protoss_Dark_Templar;

            // Buildings
            buildQueue[Protoss_Gateway] =                   (s >= 20) + (s >= 42);
            buildQueue[Protoss_Citadel_of_Adun] =           s >= 34;
            buildQueue[Protoss_Templar_Archives] =          vis(Protoss_Gateway) >= 2;

            // Research
            techQueue[Psionic_Storm] =                      vis(Protoss_Dark_Templar) >= 2;

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
            protossUnitPump[Protoss_Dark_Templar] = com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0;
        }

        void PvZ_1GC_ZCore()
        {
            scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
            gasLimit =                                      INT_MAX;
            transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

            // Buildings
            buildQueue[Protoss_Nexus] =                     1;
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 34;

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
        }

        void PvZ_1GC_ZZCore()
        {
            scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
            gasLimit =                                      INT_MAX;
            transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

            // Buildings
            buildQueue[Protoss_Nexus] =                     1;
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 24);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 32;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 40;

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
        }
    }

    void PvZ_1GC()
    {
        // Reactions
        if (!inTransition) {

            // If enemy is rushing, pressuring or stole gas
            if (Spy::enemyRush() || Spy::enemyPressure() || Spy::enemyGasSteal()) {
                currentBuild = P_2Gate;
                currentOpener = P_10_12;
                currentTransition = P_4Gate;
            }
        }

        // Openers
        if (currentOpener == P_ZCore)
            PvZ_1GC_ZCore();
        if (currentOpener == P_ZZCore)
            PvZ_1GC_ZZCore();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_DT)
                PvZ_1GC_DT();
        }
    }
}