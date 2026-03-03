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

    namespace {

        void TvP_1FactFE()
        {
            transitionReady =                               vis(Terran_Factory) >= 1;
            gasLimit =                                      3;
            scout =                                         scout || (s >= 26 && vis(Terran_Refinery) == 1);

            wallNat = vis(Terran_Barracks) > 0;

            // Buildings
            buildQueue[Terran_Supply_Depot] =               (s >= 18) + (vis(Terran_Factory) >= 1 && s >= 32);
            buildQueue[Terran_Barracks] =                   (s >= 22);
            buildQueue[Terran_Refinery] =                   (s >= 24);
            buildQueue[Terran_Factory] =                    (s >= 32);

            terranUnitPump[Terran_SCV] = vis(Terran_SCV) < 24;
            terranUnitPump[Terran_Marine] = com(Terran_Barracks) > 0 && total(Terran_Marine) < 4;
            terranUnitPump[Terran_Vulture] = com(Terran_Factory) > 0;
        }

        void TvP_2FactFE()
        {
            transitionReady =                               total(Terran_Vulture) >= 2;
            gasLimit =                                      (vis(Terran_Factory) >= 2) ? 1 : 3;
            scout =                                         scout || (s >= 26 && vis(Terran_Refinery) == 1);

            wallNat = vis(Terran_Barracks) > 0;

            // Buildings
            buildQueue[Terran_Supply_Depot] =               (s >= 18) + (vis(Terran_Factory) >= 1 && s >= 32) + (s >= 44);
            buildQueue[Terran_Barracks] =                   (s >= 22);
            buildQueue[Terran_Refinery] =                   (s >= 24);
            buildQueue[Terran_Factory] =                    (s >= 32) + (s >= 36);

            terranUnitPump[Terran_SCV] = vis(Terran_SCV) < 24;
            terranUnitPump[Terran_Marine] = com(Terran_Barracks) > 0 && total(Terran_Marine) < 2;
            terranUnitPump[Terran_Vulture] = com(Terran_Factory) > 0;
        }
    }

    void TvP_RaxFact()
    {
        // Openers
        if (currentOpener == T_1FactFE)
            TvP_1FactFE();
        if (currentOpener == T_2FactFE)
            TvP_2FactFE();
    }
}