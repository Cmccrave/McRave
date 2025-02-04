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

        void PvT_2B_12Nexus()
        {
            // "http://liquipedia.net/starcraft/12_Nexus"
            gasLimit =                                          goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;
            unitLimits[Protoss_Zealot] =                        1;
            scout =                                             vis(Protoss_Pylon) > 0;
            transitionReady =                                   vis(Protoss_Gateway) >= 2;

            // Buildings
            buildQueue[Protoss_Nexus] =                         1 + (s >= 24);
            buildQueue[Protoss_Pylon] =                         (s >= 16) + (s >= 44);
            buildQueue[Protoss_Assimilator] =                   s >= 30;
            buildQueue[Protoss_Gateway] =                       (s >= 28);
            buildQueue[Protoss_Cybernetics_Core] =              vis(Protoss_Gateway) >= 2;

            // Upgrades
            upgradeQueue[Singularity_Charge] =                  vis(Protoss_Dragoon) > 0;
        }

        void PvT_2B_21Nexus()
        {
            // "http://liquipedia.net/starcraft/21_Nexus"
            scout =                                             Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
            gasLimit =                                          goonRange() && vis(Protoss_Nexus) < 2 ? 2 : INT_MAX;
            unitLimits[Protoss_Dragoon] =                       Util::getTime() > Time(4, 0) || vis(Protoss_Nexus) >= 2 ? INT_MAX : 1;
            unitLimits[Protoss_Probe] =                         20;
            transitionReady =                                   vis(Protoss_Gateway) >= 2;

            // Buildings
            buildQueue[Protoss_Nexus] =                         1 + (s >= 42);
            buildQueue[Protoss_Pylon] =                         (s >= 16) + (s >= 30);
            buildQueue[Protoss_Assimilator] =                   s >= 24;
            buildQueue[Protoss_Gateway] =                       (s >= 20) + (vis(Protoss_Nexus) >= 2);
            buildQueue[Protoss_Cybernetics_Core] =              s >= 26;

            // Upgrades
            upgradeQueue[Singularity_Charge] =                  vis(Protoss_Dragoon) > 0;
        }

        void PvT_2B_20Nexus()
        {
            // "https://liquipedia.net/starcraft/2_Gate_Range_Expand"
            scout =                                             vis(Protoss_Cybernetics_Core) > 0;            
            gasLimit =                                          goonRange() && vis(Protoss_Pylon) < 3 ? 2 : INT_MAX;
            unitLimits[Protoss_Dragoon] =                       Util::getTime() > Time(4, 0) || vis(Protoss_Nexus) >= 2 || s >= 40 ? INT_MAX : 0;
            unitLimits[Protoss_Probe] =                         20;
            transitionReady =                                   vis(Protoss_Gateway) >= 2;

            buildQueue[Protoss_Nexus] =                         1 + (s >= 40);
            buildQueue[Protoss_Pylon] =                         (s >= 16) + (s >= 30);
            buildQueue[Protoss_Assimilator] =                   s >= 24;
            buildQueue[Protoss_Gateway] =                       (s >= 20) + (s >= 34);
            buildQueue[Protoss_Cybernetics_Core] =              s >= 26;

            // Upgrades
            upgradeQueue[Singularity_Charge] =                  vis(Protoss_Dragoon) > 0;
        }

        void PvT_2B_Obs()
        {
            inOpening =                                     s < 80;
            inTransition =                                  total(Protoss_Nexus) >= 2;
            focusUnit =                                     Protoss_Observer;

            // Buildings
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 26;
            buildQueue[Protoss_Robotics_Facility] =         s >= 62;
            buildQueue[Protoss_Observatory] =               com(Protoss_Robotics_Facility) > 0;

            // Composition
            armyComposition[Protoss_Dragoon] =              1.00;

            armyComposition[Protoss_Observer] =             1.00;

        }

        void PvT_2B_Carrier()
        {
            inOpening =                                     total(Protoss_Stargate) < 2;
            focusUnit =                                     Protoss_Carrier;
            inTransition =                                  total(Protoss_Stargate) > 0;

            // Buildings
            buildQueue[Protoss_Assimilator] =               (s >= 24) + (s >= 60);
            buildQueue[Protoss_Cybernetics_Core] =          s >= 26;
            buildQueue[Protoss_Stargate] =                  (vis(Protoss_Dragoon) >= 6) + (vis(Protoss_Carrier) > 0);
            buildQueue[Protoss_Fleet_Beacon] =              atPercent(Protoss_Stargate, 1.00);

            // Upgrades
            upgradeQueue[Protoss_Air_Weapons] =             vis(Protoss_Stargate) > 0;
            upgradeQueue[Carrier_Capacity] =                com(Protoss_Fleet_Beacon) > 0;

            // Composition
            armyComposition[Protoss_Zealot] =               0.10;
            armyComposition[Protoss_Dragoon] =              0.90;

            armyComposition[Protoss_Carrier] =              1.00;
        }

        void PvT_2B_ReaverCarrier()
        {
            inOpening =                                     s < 200;
            inTransition =                                  total(Protoss_Stargate) > 0;
            focusUnit =                                     Players::getTotalCount(PlayerState::Enemy, Terran_Medic) > 0 ? Protoss_Reaver : Protoss_Carrier;

            // Buildings
            buildQueue[Protoss_Assimilator] =               1 + (s >= 56);
            buildQueue[Protoss_Robotics_Facility] =         s >= 58;
            buildQueue[Protoss_Robotics_Support_Bay] =      atPercent(Protoss_Robotics_Facility, 0.95);
            buildQueue[Protoss_Stargate] =                  (s >= 80) + (vis(Protoss_Carrier) > 0);
            buildQueue[Protoss_Fleet_Beacon] =              atPercent(Protoss_Stargate, 0.95);

            // Upgrades
            upgradeQueue[Gravitic_Drive] =                  total(Protoss_Shuttle) > 0;

            // Composition
            armyComposition[Protoss_Zealot] =               0.10;
            armyComposition[Protoss_Dragoon] =              0.90;

            armyComposition[Protoss_Reaver] =               0.50;
            armyComposition[Protoss_Observer] =             0.25;
            armyComposition[Protoss_Shuttle] =              0.25;

            armyComposition[Protoss_Carrier] =              1.00;
        }
    }

    void PvT_2B()
    {
        // Reactions
        if (!inTransition) {
            if (currentBuild == "2Base") {
                if (Spy::enemyFastExpand() || Spy::getEnemyTransition() == "SiegeExpand")
                    currentTransition = "ReaverCarrier";
                else if (Terrain::getEnemyStartingPosition().isValid() && !Spy::enemyFastExpand() && currentTransition == "DoubleExpand")
                    currentTransition = "Obs";
                else if (Spy::enemyPressure())
                    currentTransition = "Obs";
            }
        }

        // Openers
        if (currentOpener == "12Nexus")
            PvT_2B_12Nexus();
        if (currentOpener == "20Nexus")
            PvT_2B_20Nexus();
        if (currentOpener == "21Nexus")
            PvT_2B_21Nexus();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "Obs")
                PvT_2B_Obs();
            if (currentTransition == "Carrier")
                PvT_2B_Carrier();
            if (currentTransition == "ReaverCarrier")
                PvT_2B_ReaverCarrier();
        }
    }
}