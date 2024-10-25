#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

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

        wallNat =                                   false;
        wallMain =                                  false;
    }

    void TvZ_Academy()
    {
        rampType =          Terran_Barracks;
        gasLimit =          3;
        inBookSupply =      vis(Terran_Supply_Depot) < 3;
        inOpening =         vis(Terran_Comsat_Station) == 0;

        // Buildings
        buildQueue[Terran_Supply_Depot] =                   1 + (s >= 28) + (vis(Terran_Academy) > 0);
        buildQueue[Terran_Barracks] =                       (s >= 22) + (s >= 26);
        buildQueue[Terran_Refinery] =                       (s >= 38);
        buildQueue[Terran_Academy] =                        (s >= 46);
        buildQueue[Terran_Comsat_Station] =                 (s >= 56);

        // Research
        techQueue[Stim_Packs] =                             com(Terran_Academy) > 0;
        upgradeQueue[U_238_Shells] =                        Researching::haveOrResearching(Stim_Packs);

        // Pumping
        terranUnitPump[Terran_SCV] = true;
        terranUnitPump[Terran_Marine] = total(Terran_Marine) < 4 || total(Terran_Supply_Depot) >= 3;
        terranUnitPump[Terran_Medic] = com(Terran_Academy) > 0 && vis(Terran_Medic) < 2;
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