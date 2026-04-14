#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"
#include "Info/Player/Players.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss {

    void defaultPvP() {
        inOpening =                                     true;
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

        focusUnit =                                     UnitTypes::None;
    }

    bool enemyMoreZealots() {
        return com(Protoss_Zealot) <= Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) || Spy::enemyProxy();
    }

    bool enemyMaybeDT() {
        return (Players::getVisibleCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) > 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) <= 2)
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 3
            || Spy::enemyInvis()
            || Spy::getEnemyTransition() == P_DT;
    }

    int zealotsNeeded_PvP()
    {
        if (Spy::getEnemyOpener() == P_9_9 || Spy::getEnemyOpener() == P_Proxy_9_9)
            return 5;

        // 2Gate
        if (currentOpener == P_10_12)
            return 3;
        if (currentOpener == P_10_15)
            return 1;

        // 1GC
        if (currentOpener == P_NZCore)
            return 0;
        if (currentOpener == P_ZCore)
            return 1;
        if (currentOpener == P_ZZCore)
            return 1 + (vis(Protoss_Cybernetics_Core) > 0);
        return 1;
    }

    void PvP()
    {
        defaultPvP();

        // Builds
        if (currentBuild == P_2Gate)
            PvP_2G();
        if (currentBuild == P_1GateCore)
            PvP_1GC();
    }
}