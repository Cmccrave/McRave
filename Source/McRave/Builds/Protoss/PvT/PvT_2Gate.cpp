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

        void PvT_2G_DT()
        {
            inTransition =                                  total(Protoss_Citadel_of_Adun) > 0;
            inOpening =                                     total(Protoss_Dark_Templar) < 4;

            unitLimits[Protoss_Zealot] =                    Spy::getEnemyBuild() == "2Rax" ? 2 : 0;
            gasLimit =                                      Spy::getEnemyBuild() == "2Rax" && total(Protoss_Gateway) < 2 ? 0 : 3;
            focusUnit =                                     Protoss_Dark_Templar;
            hideTech =                                      true;

            // Buildings
            buildQueue[Protoss_Nexus] =                     1 + (vis(Protoss_Templar_Archives) > 0);
            buildQueue[Protoss_Citadel_of_Adun] =           vis(Protoss_Dragoon) >= 3;
            buildQueue[Protoss_Templar_Archives] =          atPercent(Protoss_Citadel_of_Adun, 1.00);

            // Upgrades
            upgradeQueue[Singularity_Charge] =              vis(Protoss_Citadel_of_Adun) > 0;

            // Composition
            if (com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 4)
                armyComposition[Protoss_Dark_Templar] =         1.00;
            else
                armyComposition[Protoss_Dragoon] =              1.00;
        }

        void PvT_2G_Robo()
        {
            inTransition =                                  total(Protoss_Robotics_Facility) > 0;
            inOpening =                                     s < 70;

            focusUnit =                                     Protoss_Reaver;

            // Buildings
            buildQueue[Protoss_Robotics_Facility] =         total(Protoss_Dragoon) >= 3;

            // Upgrades
            upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

            // Composition
            armyComposition[Protoss_Zealot] =               0.10;
            armyComposition[Protoss_Dragoon] =              0.90;

            armyComposition[Protoss_Reaver] =               0.50;
            armyComposition[Protoss_Observer] =             0.25;
            armyComposition[Protoss_Shuttle] =              0.25;
        }

        void PvT_2G_Expand()
        {
            inTransition =                                  true;
            inOpening =                                     total(Protoss_Nexus) < 2;

            // Buildings
            buildQueue[Protoss_Nexus] =                     1 + (s >= 50);

            // Upgrades
            upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

            // Composition
            armyComposition[Protoss_Dragoon] =              1.00;
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
        }
    }

    void PvT_2G()
    {
        // Reactions
        if (!inTransition) {
            if (currentBuild == "2Gate") {
                if (Spy::enemyFastExpand())
                    currentTransition = "DT";
            }
        }

        // Builds
        if (currentBuild == "2Gate")
            PvT_2G_1015();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "Robo")
                PvT_2G_Robo();
            if (currentTransition == "DT")
                PvT_2G_DT();
            if (currentTransition == "Expand")
                PvT_2G_Expand();
        }
    }
}