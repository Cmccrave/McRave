#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void defaultZvFFA() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wallNat =                                   hatchCount() >= 4;
        wallMain =                                  false;
        wantNatural =                               true;
        wantThird =                                 Spy::getEnemyBuild() == "FFE";
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        reserveLarva =                              0;

        gasLimit =                                  gasMax();

        desiredDetection =                          Zerg_Overlord;
        focusUnit =                                 UnitTypes::None;
    }

    int lingsNeeded_ZvFFA() {
        auto lings = 0;
        auto timingValue = 0;
        auto initialValue = 6;

        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        auto time = double((Util::getTime().minutes - 1) * 60 + (Util::getTime().seconds)) / 60.0;
        return int(time * 4);
    }

    void ZvFFA()
    {
        defaultZvFFA();

        // Builds
        if (currentBuild == "HatchPool")
            ZvFFA_HP();
    }
}