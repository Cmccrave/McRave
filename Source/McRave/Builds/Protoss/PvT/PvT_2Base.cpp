#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void PvT_2B_12Nexus()
    {
        // 8p 12n 12g 13g 15c
        unitLimits[Protoss_Zealot] =                    1;
        scout =                                         vis(Protoss_Pylon) > 0;
        transitionReady =                               vis(Protoss_Gateway) >= 2;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1 + (s >= 24);
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 44);
        buildQueue[Protoss_Assimilator] =               (s >= 26);
        buildQueue[Protoss_Gateway] =                   (vis(Protoss_Nexus) >= 2) + (s >= 34);
        buildQueue[Protoss_Cybernetics_Core] =          (s >= 30);

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                true;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && vis(Protoss_Cybernetics_Core) > 0 && total(Protoss_Zealot) < 2;
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;

        // Gas
        gasLimit = gasMax();
        if (goonRange() && vis(Protoss_Nexus) < 2)
            gasLimit = 2;
    }

    void PvT_2B_21Nexus()
    {
        // "http://liquipedia.net/starcraft/21_Nexus"
        scout =                                         Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
        transitionReady =                               vis(Protoss_Gateway) >= 2;

        // Buildings
        buildQueue[Protoss_Nexus] =                     1 + (s >= 42);
        buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 30);
        buildQueue[Protoss_Assimilator] =               s >= 24;
        buildQueue[Protoss_Gateway] =                   (s >= 20) + (vis(Protoss_Nexus) >= 2);
        buildQueue[Protoss_Cybernetics_Core] =          s >= 26;

        // Upgrades
        upgradeQueue[Singularity_Charge] =              vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                total(Protoss_Probe) < 19 || total(Protoss_Pylon) >= 3;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;

        // Gas
        gasLimit = gasMax();
        if (goonRange() && vis(Protoss_Nexus) < 2)
            gasLimit = 2;
    }

    void PvT_2B_20Nexus()
    {
        // "https://liquipedia.net/starcraft/2_Gate_Range_Expand"
        scout =                                             vis(Protoss_Cybernetics_Core) > 0;
        gasLimit =                                          goonRange() && vis(Protoss_Pylon) < 3 ? 2 : INT_MAX;
        transitionReady =                                   vis(Protoss_Gateway) >= 2;

        buildQueue[Protoss_Nexus] =                         1 + (s >= 40);
        buildQueue[Protoss_Pylon] =                         (s >= 16) + (s >= 30);
        buildQueue[Protoss_Assimilator] =                   s >= 24;
        buildQueue[Protoss_Gateway] =                       (s >= 20) + (s >= 34);
        buildQueue[Protoss_Cybernetics_Core] =              s >= 26;

        // Upgrades
        upgradeQueue[Singularity_Charge] =                  vis(Protoss_Dragoon) > 0;

        // Pumping
        protossUnitPump[Protoss_Probe] =                total(Protoss_Probe) < 20 || total(Protoss_Dragoon) >= 2;
        protossUnitPump[Protoss_Zealot] =               com(Protoss_Gateway) > 0 && zealotsNeeded_PvT() > total(Protoss_Zealot);
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;

        // Gas
        gasLimit = gasMax();
        if (vis(Protoss_Pylon) < 3)
            gasLimit = 2;
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
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Observer] =             com(Protoss_Gateway) > 0 && com(Protoss_Observatory) > 0;
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
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Carrier] =              com(Protoss_Stargate) > 0 && com(Protoss_Fleet_Beacon) > 0;
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
        upgradeQueue[Protoss_Air_Weapons] =             vis(Protoss_Stargate) > 0;
        upgradeQueue[Carrier_Capacity] =                com(Protoss_Fleet_Beacon) > 0;

        // Composition
        protossUnitPump[Protoss_Dragoon] =              com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        protossUnitPump[Protoss_Reaver] =               com(Protoss_Robotics_Facility) > 0 && com(Protoss_Robotics_Support_Bay) > 0;
        protossUnitPump[Protoss_Observer] =             com(Protoss_Robotics_Facility) > 0 && com(Protoss_Observatory) > 0;
        protossUnitPump[Protoss_Shuttle] =              com(Protoss_Robotics_Facility) > 0 && vis(Protoss_Reaver) > vis(Protoss_Shuttle) * 2;
        protossUnitPump[Protoss_Carrier] =              com(Protoss_Stargate) > 0 && com(Protoss_Fleet_Beacon) > 0;
    }

    void PvT_2B()
    {
        // Reactions
        if (!inTransition) {
            if (currentBuild == P_2Base) {
                if (Spy::enemyFastExpand() || Spy::getEnemyTransition() == T_SiegeExpand)
                    currentTransition = P_ReaverCarrier;
                else if (Spy::enemyPressure())
                    currentTransition = P_Obs;
            }
        }

        // Openers
        if (currentOpener == P_12Nexus)
            PvT_2B_12Nexus();
        if (currentOpener == P_20Nexus)
            PvT_2B_20Nexus();
        if (currentOpener == P_21Nexus)
            PvT_2B_21Nexus();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_Obs)
                PvT_2B_Obs();
            if (currentTransition == P_Carrier)
                PvT_2B_Carrier();
            if (currentTransition == P_ReaverCarrier)
                PvT_2B_ReaverCarrier();
        }
    }
}