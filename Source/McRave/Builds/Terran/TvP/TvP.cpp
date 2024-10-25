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
    void defaultTvP() {
        inOpening =                                         true;
        inBookSupply =                                      true;
        mineralThird =                                      true;
        proxy =                                             false;
        hideTech =                                          false;
        rush =                                              false;
        pressure =                                          false;
        transitionReady =                                   false;
        planEarly =                                         false;

        desiredDetection =                                  Terran_Missile_Turret;

        wallNat =                                           false;
        wallMain =                                          false;
    }

    void TvP_5Fact()
    {
        inOpening = Util::getTime() < Time(9, 00);
        inBookSupply = vis(Terran_Supply_Depot) < 3;
        rampType = Terran_Factory;

        // Build
        buildQueue[Terran_Supply_Depot] =               2 + (vis(Terran_Command_Center) >= 2);
        buildQueue[Terran_Barracks] =                   1;
        buildQueue[Terran_Bunker] =                     vis(Terran_Factory) > 0;
        buildQueue[Terran_Refinery] =                   1 + (vis(Terran_Armory) >= 1);
        buildQueue[Terran_Command_Center] =             1 + (s >= 46) + (s >= 116);
        buildQueue[Terran_Academy] =                    (s >= 62);
        buildQueue[Terran_Armory] =                     (s >= 72) + vis(Terran_Science_Facility) > 0;
        buildQueue[Terran_Comsat_Station] =             2 * (s >= 88);
        buildQueue[Terran_Factory] =                    1 + (s >= 88) + 3 * (s >= 94);
        buildQueue[Terran_Machine_Shop] =               vis(Terran_Factory) > 0 && total(Terran_Vulture) > 0;
        buildQueue[Terran_Starport] =                   vis(Terran_Command_Center) >= 3;
        buildQueue[Terran_Science_Facility] =           com(Terran_Starport) > 0;

        // Research
        techQueue[TechTypes::Spider_Mines] =            (s >= 60);
        techQueue[TechTypes::Tank_Siege_Mode] =         (s >= 96);
        upgradeQueue[UpgradeTypes::Ion_Thrusters] =     (s >= 100);

        // Upgrades
        upgradeQueue[UpgradeTypes::Terran_Vehicle_Weapons] = (com(Terran_Armory) > 0) * 2;

        // Pumping
        terranUnitPump[Terran_SCV] = vis(Terran_SCV) < 45;
        terranUnitPump[Terran_Siege_Tank_Tank_Mode] = true;
        terranUnitPump[Terran_Vulture] = true;

        // Gas
        gasLimit = 1;
        if (com(Terran_Command_Center) >= 2)
            gasLimit = gasMax();
    }

    void TvP()
    {
        defaultTvP();

        // Builds
        if (currentBuild == "RaxFact")
            TvP_RaxFact();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "5Fact")
                TvP_5Fact();
        }
    }
}