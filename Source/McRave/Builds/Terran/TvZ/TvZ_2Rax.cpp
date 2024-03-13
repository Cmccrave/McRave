#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../TerranBuildOrder.h"

namespace McRave::BuildOrder::Terran {

    namespace {

        void TvZ_2Rax_1113()
        {
            transitionReady =                               vis(Terran_Barracks) >= 2;
            gasLimit =                                      0;

            scout =                                         scout || (vis(Terran_Barracks) == 1);

            buildQueue[Terran_Supply_Depot] =               (s >= 18);
            buildQueue[Terran_Barracks] =                   (s >= 22) + (s >= 26);
        }
    }

    void TvZ_2Rax()
    {
        // Openers
        if (currentOpener == "11/13")
            TvZ_2Rax_1113();
    }
}