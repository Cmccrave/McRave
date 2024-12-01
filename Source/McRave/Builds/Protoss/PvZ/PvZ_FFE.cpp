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

        void PvZ_FFE_Forge()
        {
            wantNatural =                                   true;
            wallNat =                                       true;
            scout =                                         vis(Protoss_Pylon) > 0;
            transitionReady =                               vis(Protoss_Gateway) >= 1;

            // Buildings
            buildQueue[Protoss_Nexus] =                     1 + (s >= 28);
            buildQueue[Protoss_Pylon] =                     (s >= 14) + (s >= 30), (s >= 16) + (s >= 30);
            buildQueue[Protoss_Assimilator] =               !Spy::enemyRush() && s >= 34;
            buildQueue[Protoss_Gateway] =                   s >= 32;
            buildQueue[Protoss_Forge] =                     s >= 20;

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
        }

        void PvZ_2Stargate()
        {
            inOpening =                                 s < 100;
            inTransition =                              total(Protoss_Stargate) >= 2;
            focusUnit =                                 Protoss_Corsair;

            // Buildings
            buildQueue[Protoss_Assimilator] =           (s >= 38) + (atPercent(Protoss_Cybernetics_Core, 0.75));
            buildQueue[Protoss_Cybernetics_Core] =      s >= 36;
            buildQueue[Protoss_Citadel_of_Adun] =       0;
            buildQueue[Protoss_Templar_Archives] =      0;
            buildQueue[Protoss_Stargate] =              (vis(Protoss_Corsair) > 0) + (atPercent(Protoss_Cybernetics_Core, 1.00));

            // Upgrades
            upgradeQueue[Protoss_Air_Weapons] =         vis(Protoss_Stargate) > 0;

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
            protossUnitPump[Protoss_Dark_Templar] = com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 2;
            protossUnitPump[Protoss_High_Templar] = com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_High_Templar) < 2;
            protossUnitPump[Protoss_Corsair] = com(Protoss_Stargate) > 0;
        }

        void PvZ_5GateGoon()
        {
            // "https://liquipedia.net/starcraft/5_Gate_Ranged_Goons_(vs._Zerg)"
            inOpening =                                 s < 160;
            inTransition =                              total(Protoss_Gateway) >= 3;
            pressure =                                  Util::getTime() > Time(6, 30);

            // Buildings
            buildQueue[Protoss_Cybernetics_Core] =      s >= 40;
            buildQueue[Protoss_Gateway] =               (vis(Protoss_Cybernetics_Core) > 0) + (4 * (s >= 64));
            buildQueue[Protoss_Assimilator] =           1 + (s >= 116);

            // Upgrades
            upgradeQueue[Singularity_Charge] =          true;

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0 && total(Protoss_Zealot) < 2;
            protossUnitPump[Protoss_Dragoon] = com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        }

        void PvZ_NeoBisu()
        {
            // "https://liquipedia.net/starcraft/%2B1_Sair/Speedlot_(vs._Zerg)"
            inOpening =                                 s < 100;
            inTransition =                              total(Protoss_Citadel_of_Adun) > 0 && total(Protoss_Stargate) > 0;
            focusUnit =                                 Protoss_Corsair;

            // Buildings
            buildQueue[Protoss_Assimilator] =           (s >= 34) + (vis(Protoss_Cybernetics_Core) > 0);
            buildQueue[Protoss_Gateway] =               1 + (vis(Protoss_Citadel_of_Adun) > 0);
            buildQueue[Protoss_Cybernetics_Core] =      s >= 36;
            buildQueue[Protoss_Citadel_of_Adun] =       vis(Protoss_Corsair) > 0;
            buildQueue[Protoss_Stargate] =              com(Protoss_Cybernetics_Core) >= 1;
            buildQueue[Protoss_Templar_Archives] =      Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements);

            if (vis(Protoss_Templar_Archives) > 0) {
                focusUnits.insert(Protoss_High_Templar);
                unlockedType.insert(Protoss_High_Templar);
                focusUnits.insert(Protoss_Dark_Templar);
                unlockedType.insert(Protoss_Dark_Templar);
            }

            // Upgrades
            upgradeQueue[Protoss_Ground_Weapons] =      com(Protoss_Cybernetics_Core) > 0;
            upgradeQueue[Protoss_Air_Weapons] =         com(Protoss_Cybernetics_Core) > 0 && Upgrading::haveOrUpgrading(Protoss_Ground_Weapons, 1);

            // Pumping
            protossUnitPump[Protoss_Probe] = true;
            protossUnitPump[Protoss_Zealot] = com(Protoss_Gateway) > 0;
            protossUnitPump[Protoss_Dark_Templar] = com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_Dark_Templar) < 2;
            protossUnitPump[Protoss_High_Templar] = com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0 && total(Protoss_High_Templar) < 2;
            protossUnitPump[Protoss_Corsair] = com(Protoss_Stargate) > 0;
        }
    }

    void PvZ_FFE()
    {
        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyTransition() == Z_2HatchMuta || Spy::getEnemyTransition() == Z_3HatchMuta)
                currentTransition = P_2Stargate;
        }

        // Openers
        if (currentOpener == P_Forge)
            PvZ_FFE_Forge();

        // Transitions
        if (transitionReady) {
            if (currentTransition == P_2Stargate)
                PvZ_2Stargate();
            if (currentTransition == P_5GateGoon)
                PvZ_5GateGoon();
            if (currentTransition == P_NeoBisu)
                PvZ_NeoBisu();
        }
    }
}