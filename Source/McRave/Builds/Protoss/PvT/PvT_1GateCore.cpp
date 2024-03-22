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

            // Upgrades
            upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

            // Army Composition
            armyComposition[Protoss_Zealot] =               0.05;
            armyComposition[Protoss_Dragoon] =              0.75;
            armyComposition[Protoss_Reaver] =               0.50;
            armyComposition[Protoss_Observer] =             0.25;
            armyComposition[Protoss_Shuttle] =              0.25;
        }

        void PvT_1GC_DT()
        {
            // "https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)"
            inTransition =                                  total(Protoss_Citadel_of_Adun) > 0;
            inOpening =                                     s <= 80;
            hideTech =                                      com(Protoss_Dark_Templar) <= 0;
            focusUnit =                                     Protoss_Dark_Templar;

            // Buildings
            buildQueue[Protoss_Gateway] =                   (s >= 20) + (vis(Protoss_Templar_Archives) > 0);
            buildQueue[Protoss_Nexus] =                     1 + (vis(Protoss_Dark_Templar) > 0);
            buildQueue[Protoss_Assimilator] =               (s >= 24) + (vis(Protoss_Nexus) >= 2);
            buildQueue[Protoss_Citadel_of_Adun] =           s >= 36;
            buildQueue[Protoss_Templar_Archives] =          s >= 48;

            // Upgrades
            upgradeQueue[Singularity_Charge] =              vis(Protoss_Dark_Templar) >= 2;

            // Army composition
            armyComposition[Protoss_Dragoon] =              0.80;
            armyComposition[Protoss_Zealot] =               0.10;
            armyComposition[Protoss_Dark_Templar] =         0.10;
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

            // Army composition
            armyComposition[Protoss_Dragoon] =              0.95;
            armyComposition[Protoss_Zealot] =               0.05;
        }

        void PvT_1GC_NZCore()
        {
            // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)"
            scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
            unitLimits[Protoss_Zealot] =                    0;

            // Buildings
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 30);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 26;

            // Army composition
            armyComposition[Protoss_Dragoon] =              0.95;
            armyComposition[Protoss_Zealot] =               0.05;
        }

        void PvT_1GC_ZCore()
        {
            // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)"
            scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
            unitLimits[Protoss_Zealot] =                    1;

            // Buildings
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 36;

            // Army composition
            armyComposition[Protoss_Dragoon] =              0.95;
            armyComposition[Protoss_Zealot] =               0.05;
        }
    }

    void PvT_1GC()
    {
        // Reactions
        if (!inTransition) {
            if (currentBuild == "1GateCore") {
                if (Spy::enemyRush()) {
                    currentBuild = "2Gate";
                    currentOpener = "Main";
                    currentTransition = "DT";
                }
                else if (Spy::enemyFastExpand())
                    currentTransition = "DT";
            }
            if (currentBuild == "2Gate") {
                if (Spy::enemyFastExpand())
                    currentTransition = "DT";
            }
        }

        // Openers
        if (currentOpener == "0Zealot")
            PvT_1GC_NZCore();
        if (currentOpener == "1Zealot")
            PvT_1GC_ZCore();

        // Transitions
        if (currentTransition == "Robo")
            PvT_1GC_Robo();
        if (currentTransition == "DT")
            PvT_1GC_DT();
        if (currentTransition == "4Gate")
            PvT_1GC_4Gate();
    }
}