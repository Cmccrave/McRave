#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../TerranBuildOrder.h"

namespace McRave::BuildOrder::Terran
{
    void defaultTvZ() {
        inOpening =                                 true;
        inBookSupply =                              true;
        mineralThird =                              true;
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;

        desiredDetection =                          Terran_Missile_Turret;
        focusUpgrade =                              UpgradeTypes::None;
        focusTech =                                 TechTypes::None;
        focusUnit =                                 None;

        armyComposition[Terran_Marine] =            0.80;
        armyComposition[Terran_Medic] =             0.20;
        armyComposition[Terran_SCV] =               1.00;

        wallNat =                                   false;
        wallMain =                                  false;
    }

    void TvZ_Academy()
    {
        rampType = Terran_Barracks;
        gasLimit = 3;
        inBookSupply = vis(Terran_Supply_Depot) < 3;
        inOpening = vis(Terran_Comsat_Station) == 0;

        buildQueue[Terran_Supply_Depot] =                   1 + (s >= 28) + (s >= 48);
        buildQueue[Terran_Barracks] =                       (s >= 22) + (s >= 26);
        buildQueue[Terran_Refinery] =                       (s >= 36);
        buildQueue[Terran_Academy] =                        (s >= 38);
        buildQueue[Terran_Comsat_Station] =                 (s >= 56);

        techQueue[TechTypes::Stim_Packs] =                  (s >= 52);
    }

    void TvZ()
    {
        defaultTvZ();

        // Builds
        if (currentBuild == "2Rax")
            TvZ_2Rax();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "Academy")
                TvZ_Academy();
        }
    }
}