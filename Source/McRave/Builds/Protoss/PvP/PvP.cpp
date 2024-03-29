#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ProtossBuildOrder.h"

namespace McRave::BuildOrder::Protoss {

    void defaultPvP() {
        inOpening =                                 true;
        inBookSupply =                                  vis(Protoss_Pylon) < 2;
        wallNat =                                       vis(Protoss_Nexus) >= 2;
        wallMain =                                      false;
        scout =                                         vis(Protoss_Gateway) > 0;
        wantNatural =                                   false;
        wantThird =                                     false;
        proxy =                                         false;
        hideTech =                                      false;
        rush =                                          false;
        transitionReady =                               false;

        gasLimit =                                      INT_MAX;
        unitLimits[Protoss_Zealot] =                    1;
        unitLimits[Protoss_Dragoon] =                   INT_MAX;
        unitLimits[Protoss_Probe] =                     INT_MAX;

        desiredDetection =                              Protoss_Observer;
        focusUnit =                                     UnitTypes::None;

        armyComposition[Protoss_Probe] =                1.00;
        armyComposition[Protoss_Zealot] =               0.10;
        armyComposition[Protoss_Dragoon] =              0.90;
    }

    bool enemyMoreZealots() {
        return com(Protoss_Zealot) <= Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) || Spy::enemyProxy();
    }

    bool enemyMaybeDT() {
        return (Players::getVisibleCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) > 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) <= 2)
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 3
            || Spy::enemyInvis()
            || Spy::getEnemyTransition() == "DT";
    }

    void PvP()
    {
        defaultPvP();

        // Builds
        if (currentBuild == "2Gate")
            PvP_2G();
        if (currentBuild == "1GateCore")
            PvP_1GC();
    }
}