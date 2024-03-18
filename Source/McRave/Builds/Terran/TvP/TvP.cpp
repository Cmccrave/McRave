#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../TerranBuildOrder.h"

namespace McRave::BuildOrder::Terran
{
    void defaultTvP() {
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

        armyComposition[Terran_Vulture] =                   0.50;
        armyComposition[Terran_Siege_Tank_Tank_Mode] =      0.40;
        armyComposition[Terran_Goliath] =                   0.10;
        armyComposition[Terran_SCV] =                       1.00;
        armyComposition[Terran_Marine] =                    1.00;

        wallNat =                                   vis(Terran_Factory) > 0;
        wallMain =                                  false;
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
        buildQueue[Terran_Command_Center] =             1 + (s >= 46);
        buildQueue[Terran_Academy] =                    (s >= 62);
        buildQueue[Terran_Armory] =                     (s >= 72);
        buildQueue[Terran_Comsat_Station] =             2 * (s >= 88);
        buildQueue[Terran_Factory] =                    1 + (s >= 88) + 3 * (s >= 94);
        buildQueue[Terran_Machine_Shop] =               vis(Terran_Factory) > 0;

        // Research
        techQueue[TechTypes::Spider_Mines] =            (s >= 60);
        techQueue[TechTypes::Tank_Siege_Mode] =         (s >= 96);
        upgradeQueue[UpgradeTypes::Ion_Thrusters] =     (s >= 100);

        // Upgrades
        upgradeQueue[UpgradeTypes::Terran_Vehicle_Weapons] = (com(Terran_Armory) > 0);

        // Gas
        gasLimit = 1;
        if (vis(Terran_Command_Center) >= 2)
            gasLimit = Resources::getGasCount() * 3;

        // Pumping TODO
        auto pumpTanks = (total(Terran_Siege_Tank_Tank_Mode) == 0);
        auto pumpMarines = total(Terran_Marine) < 4;
        if (pumpTanks) {
            armyComposition.clear();
            armyComposition[Terran_SCV] = 1.00;
            armyComposition[Terran_Siege_Tank_Tank_Mode] = 1.00;
        }

        if (pumpMarines) {
            armyComposition[Terran_Marine] = 1.00;
        }
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