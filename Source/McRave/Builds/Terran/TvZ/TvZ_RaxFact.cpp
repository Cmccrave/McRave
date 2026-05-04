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

    void TvZ_111()
    {
        transitionReady = vis(Terran_Factory) >= 1;
        gasLimit        = gasMax();
        scout           = scout || (s >= 26 && vis(Terran_Refinery) == 1);

        wallNat = vis(Terran_Barracks) > 0;

        // Buildings
        buildQueue[Terran_Supply_Depot] = (s >= 18) + (vis(Terran_Marine) >= 1 && s >= 32);
        buildQueue[Terran_Barracks]     = (s >= 20);
        buildQueue[Terran_Refinery]     = vis(Terran_Barracks) > 0;
        buildQueue[Terran_Factory]      = (s >= 28);

        terranUnitPump[Terran_SCV]    = vis(Terran_SCV) < 24;
        terranUnitPump[Terran_Marine] = vis(Terran_Factory) > 0;
    }

    void TvZ_1FactFE()
    {
        transitionReady = vis(Terran_Command_Center) >= 2;
        gasLimit        = gasMax();
        scout           = scout || s >= 24;

        wallNat = vis(Terran_Barracks) > 0;

        // Buildings
        buildQueue[Terran_Command_Center] = 1 + (s >= 42);
        buildQueue[Terran_Supply_Depot]   = (s >= 18) + (vis(Terran_Marine) >= 1 && s >= 32);
        buildQueue[Terran_Barracks]       = (s >= 22);
        buildQueue[Terran_Refinery]       = (s >= 24);
        buildQueue[Terran_Factory]        = (s >= 28);

        if (gas(88) || vis(Terran_Factory) > 0)
            gasLimit = 1;

        terranUnitPump[Terran_SCV]    = vis(Terran_SCV) < 24;
        terranUnitPump[Terran_Marine] = vis(Terran_Factory) > 0;
    }

    void TvZ_RaxFact()
    {
        // Openers
        if (currentOpener == T_111)
            TvZ_111();
        if (currentOpener == T_1FactFE)
            TvZ_1FactFE();
    }
} // namespace McRave::BuildOrder::Terran