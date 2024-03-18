#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../TerranBuildOrder.h"

namespace McRave::BuildOrder::Terran {

    namespace {

        void TvP_1FactFE()
        {
            transitionReady =                               vis(Terran_Factory) >= 1;
            gasLimit =                                      3;

            unitLimits[Terran_Marine] =                     4;

            scout =                                         scout || (s >= 26 && vis(Terran_Refinery) == 1);

            buildQueue[Terran_Supply_Depot] =               (s >= 18);
            buildQueue[Terran_Barracks] =                   (s >= 22);
            buildQueue[Terran_Refinery] =                   (s >= 24);
            buildQueue[Terran_Factory] =                    (s >= 32);
        }

        void TvP_2FactFE()
        {
            transitionReady =                               total(Terran_Vulture) >= 2;
            gasLimit =                                      (vis(Terran_Factory) >= 2) ? 1 : 3;

            unitLimits[Terran_Marine] =                     2;

            scout =                                         scout || (s >= 26 && vis(Terran_Refinery) == 1);

            buildQueue[Terran_Supply_Depot] =               (s >= 18) + (vis(Terran_Factory) >= 1 && s >= 32) + (s >= 44);
            buildQueue[Terran_Barracks] =                   (s >= 22);
            buildQueue[Terran_Refinery] =                   (s >= 24);
            buildQueue[Terran_Factory] =                    (s >= 32) + (s >= 36);
        }
    }

    void TvP_RaxFact()
    {
        // Openers
        if (currentOpener == "1FactFE")
            TvP_1FactFE();
        if (currentOpener == "2FactFE")
            TvP_2FactFE();
    }
}