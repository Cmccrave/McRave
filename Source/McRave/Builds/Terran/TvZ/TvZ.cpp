#include "Builds/All/All.h"
#include "Builds/Terran/TerranBuildOrder.h"
#include "Macro/Researching/Researching.h"
#include "Main/Common.h"
#include "Map/Stations/Stations.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

namespace McRave::BuildOrder::Terran {
    void defaultTvZ()
    {
        inOpening       = true;
        inBookSupply    = true;
        mineralThird    = true;
        proxy           = false;
        hideTech        = false;
        rush            = false;
        pressure        = false;
        transitionReady = false;
        planEarly       = false;

        wallNat   = Stations::getStations(PlayerState::Self).size() >= 2;
        wallMain  = false;
        wallThird = Stations::getStations(PlayerState::Self).size() >= 3;
    }

    void TvZ_Academy()
    {
        rampType     = Terran_Barracks;
        gasLimit     = gasMax();
        inBookSupply = vis(Terran_Supply_Depot) < 3;
        inOpening    = vis(Terran_Comsat_Station) == 0;

        // Buildings
        buildQueue[Terran_Supply_Depot]   = 1 + (s >= 28) + (vis(Terran_Academy) > 0);
        buildQueue[Terran_Barracks]       = (s >= 22) + (s >= 26);
        buildQueue[Terran_Refinery]       = (s >= 38);
        buildQueue[Terran_Academy]        = (s >= 46);
        buildQueue[Terran_Comsat_Station] = (s >= 56);

        // Research
        techQueue[Stim_Packs]      = com(Terran_Academy) > 0;
        upgradeQueue[U_238_Shells] = Researching::haveOrResearching(Stim_Packs);

        // Pumping
        terranUnitPump[Terran_SCV]     = true;
        terranUnitPump[Terran_Marine]  = total(Terran_Marine) < 6 || total(Terran_Supply_Depot) >= 3;
        terranUnitPump[Terran_Medic]   = com(Terran_Academy) > 0 && vis(Terran_Medic) < vis(Terran_Marine) / 1.5;
        terranUnitPump[Terran_Firebat] = com(Terran_Academy) > 0 && vis(Terran_Medic) < vis(Terran_Marine) / 1.5;
    }

    void TvZ_2PortWraith()
    {
        rampType     = Terran_Barracks;
        gasLimit     = gasMax();
        inBookSupply = vis(Terran_Supply_Depot) < 2;
        inOpening    = total(Terran_Wraith) < 4;
        focusUnit    = Terran_Wraith;

        buildQueue[Terran_Starport]      = (com(Terran_Factory) > 0) + (vis(Terran_Starport) > 0);
        buildQueue[Terran_Control_Tower] = total(Terran_Wraith) >= 2;

        techQueue[Cloaking_Field] = com(Terran_Control_Tower) > 0;

        terranUnitPump[Terran_SCV]     = vis(Terran_SCV) < 24;
        terranUnitPump[Terran_Marine]  = vis(Terran_Factory) > 0 && total(Terran_Marine) < 2;
        terranUnitPump[Terran_Vulture] = vis(Terran_Starport) >= 2 && total(Terran_Vulture) < 1;
        terranUnitPump[Terran_Wraith]  = true;
    }

    void TvZ_3FactGoliath() {}

    void TvZ_5FactGoliath()
    {
        rampType     = Terran_Factory;
        gasLimit     = gasMax();
        inBookSupply = vis(Terran_Supply_Depot) < 4;
        inOpening    = Util::getTime() < Time(9, 00);

        auto mutaBuild  = Spy::getEnemyTransition().find("Muta") != string::npos || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk, Zerg_Spire) > 0;
        auto hydraBuild = Spy::getEnemyTransition().find("Hydra") != string::npos || Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk, Zerg_Hydralisk_Den) > 0;

        buildQueue[Terran_Command_Center]  = 2;
        buildQueue[Terran_Supply_Depot]    = 2 + (s >= 48) + (s >= 80);
        buildQueue[Terran_Refinery]        = 1 + (s >= 104);
        buildQueue[Terran_Factory]         = 1 + (s >= 32) + (s >= 104) + (s >= 120) * 2;
        buildQueue[Terran_Armory]          = (s >= 50);
        buildQueue[Terran_Engineering_Bay] = (s >= 66);
        buildQueue[Terran_Machine_Shop]    = vis(Terran_Vulture) >= 2 || (s >= 50) || vis(Terran_Armory) > 0;

        upgradeQueue[Charon_Boosters]        = com(Terran_Armory) > 0;
        upgradeQueue[Terran_Vehicle_Plating] = com(Terran_Armory) > 0;

        techQueue[Tank_Siege_Mode] = hydraBuild && vis(Terran_Factory) >= 5;

        terranUnitPump[Terran_SCV]                  = vis(Terran_SCV) < 42;
        terranUnitPump[Terran_Marine]               = total(Terran_Marine) < 4;
        terranUnitPump[Terran_Vulture]              = total(Terran_Vulture) < 2;
        terranUnitPump[Terran_Goliath]              = !hydraBuild || total(Terran_Goliath) <= 10;
        terranUnitPump[Terran_Siege_Tank_Tank_Mode] = hydraBuild && total(Terran_Goliath) >= 10;
    }

    void TvZ()
    {
        defaultTvZ();

        // Builds
        if (currentBuild == T_2Rax)
            TvZ_2Rax();
        if (currentBuild == T_RaxFact)
            TvZ_RaxFact();

        // Transitions
        if (transitionReady) {
            if (currentTransition == T_Academy)
                TvZ_Academy();
            if (currentTransition == T_2PortWraith)
                TvZ_2PortWraith();
            if (currentTransition == T_5FactGoliath)
                TvZ_5FactGoliath();
        }
    }
} // namespace McRave::BuildOrder::Terran