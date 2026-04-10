#include "Builds/Zerg/ZergBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void defaultZvFFA()
    {
        inOpening       = true;
        inBookSupply    = true;
        wallNat         = hatchCount() >= 4;
        wallMain        = false;
        wantNatural     = true;
        wantThird       = Spy::getEnemyBuild() == P_FFE;
        proxy           = false;
        hideTech        = false;
        rush            = false;
        pressure        = false;
        transitionReady = false;
        planEarly       = false;
        reserveLarva    = 0;
        mineralThird    = true;

        gasLimit = gasMax();

        focusUnit = UnitTypes::None;
    }

    int lingsNeeded_ZvFFA()
    {
        auto initialValue = 6;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        auto totalEstimate = lingsNeeded_ZvP() + lingsNeeded_ZvT() + lingsNeeded_ZvZ();
        return totalEstimate;
    }

    void ZvFFA_HP_3HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/3_Hatch_Spire_(vs._Protoss)'
        inTransition = hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpening    = total(Zerg_Mutalisk) < 9;
        inBookSupply = vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;

        focusUnit = Zerg_Mutalisk;
        wantThird = Spy::enemyFastExpand() || hatchCount() >= 3;
        gasLimit  = (vis(Zerg_Drone) >= 11) ? gasMax() : 0;

        auto fourthHatch    = com(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 16;
        auto spireOverlords = (3 * (s >= 66)) + (s >= 82);

        // Buildings
        buildQueue[Zerg_Hatchery]  = 2 + (s >= 26) + fourthHatch;
        buildQueue[Zerg_Extractor] = (s >= 32 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
        buildQueue[Zerg_Lair]      = (s >= 32 && vis(Zerg_Drone) >= 15 && gas(100));
        buildQueue[Zerg_Spire]     = (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord]  = 1 + (s >= 18) + (s >= 32) + (s >= 48) + spireOverlords;

        // Pumping
        zergUnitPump[Zerg_Drone] |= vis(Zerg_Drone) < 33 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] = lingsNeeded_ZvFFA() > vis(Zerg_Zergling);
        zergUnitPump[Zerg_Mutalisk] = vis(Zerg_Drone) >= 20 && com(Zerg_Spire) > 0;
    }

    void ZvFFA()
    {
        defaultZvFFA();

        // Builds
        if (currentBuild == Z_HatchPool)
            ZvFFA_HP();
        if (currentBuild == Z_PoolHatch)
            ZvFFA_PH();

        // Transitions
        if (transitionReady) {
            if (currentTransition == Z_3HatchMuta)
                ZvFFA_HP_3HatchMuta();
        }
    }
} // namespace McRave::BuildOrder::Zerg