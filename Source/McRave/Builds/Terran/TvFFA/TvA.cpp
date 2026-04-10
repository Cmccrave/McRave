#include "Builds/Terran/TerranBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

namespace McRave::BuildOrder::Terran {
    void TvA()
    {
        if (currentTransition == T_2FactVulture) {
            inOpening    = s < 70;
            inBookSupply = vis(Terran_Supply_Depot) < 0;
            scout        = s >= 20 && vis(Terran_Supply_Depot) > 0;
            gasLimit     = INT_MAX;

            // Buildings
            buildQueue[Terran_Supply_Depot] = s >= 16;
            buildQueue[Terran_Barracks]     = s >= 20;
            buildQueue[Terran_Refinery]     = s >= 24;
            buildQueue[Terran_Factory]      = (s >= 30) + (s >= 36) + (s >= 46);
            buildQueue[Terran_Machine_Shop] = (s >= 30) + (com(Terran_Factory) >= 2);

            // Upgrades
            upgradeQueue[Ion_Thrusters] = com(Terran_Machine_Shop) > 0;

            // Pumping
            terranUnitPump[Terran_SCV]                  = true;
            terranUnitPump[Terran_Marine]               = total(Terran_Marine) < 4;
            terranUnitPump[Terran_Vulture]              = (total(Terran_Siege_Tank_Siege_Mode) + total(Terran_Siege_Tank_Siege_Mode)) >= 2;
            terranUnitPump[Terran_Siege_Tank_Tank_Mode] = (total(Terran_Siege_Tank_Siege_Mode) + total(Terran_Siege_Tank_Siege_Mode)) < 2;
        }

        if (currentTransition == "NukeRush") {
            inBookSupply = vis(Terran_Supply_Depot) < 3;
            scout        = scout || vis(Terran_Barracks) > 0;
            inOpening    = com(Terran_Ghost) == 0;
            focusUnit    = Terran_Ghost;
            gasLimit     = INT_MAX;
            wantNatural  = false;

            // Buildings
            buildQueue[Terran_Supply_Depot]     = (s >= 16) + (s >= 30) + (s >= 48);
            buildQueue[Terran_Barracks]         = (s >= 20) + (s >= 54);
            buildQueue[Terran_Refinery]         = s >= 26;
            buildQueue[Terran_Factory]          = s >= 40;
            buildQueue[Terran_Starport]         = s >= 54;
            buildQueue[Terran_Science_Facility] = s >= 66;
            buildQueue[Terran_Covert_Ops]       = atPercent(Terran_Science_Facility, 0.8);
            buildQueue[Terran_Academy]          = s >= 68;
            buildQueue[Terran_Control_Tower]    = atPercent(Terran_Starport, 0.8);
            buildQueue[Terran_Nuclear_Silo]     = atPercent(Terran_Science_Facility, 0.8);

            // Research
            techQueue[Personnel_Cloaking] = com(Terran_Covert_Ops) > 0;

            // Pumping
            terranUnitPump[Terran_SCV]             = true;
            terranUnitPump[Terran_Marine]          = total(Terran_Marine) < 4;
            terranUnitPump[Terran_Vulture]         = com(Terran_Factory) > 0;
            terranUnitPump[Terran_Ghost]           = com(Terran_Covert_Ops) > 0 && total(Terran_Ghost) < 4;
            terranUnitPump[Terran_Dropship]        = com(Terran_Control_Tower) > 0 && total(Terran_Dropship) < 2;
            terranUnitPump[Terran_Nuclear_Missile] = com(Terran_Covert_Ops) > 0 && com(Terran_Nuclear_Silo) > 0;
        }
    }
} // namespace McRave::BuildOrder::Terran