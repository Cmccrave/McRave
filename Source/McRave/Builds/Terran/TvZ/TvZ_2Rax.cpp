#include "Builds/Terran/TerranBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Terran {

    namespace {

        void TvZ_2Rax_1113()
        {
            transitionReady = vis(Terran_Barracks) >= 2;
            gasLimit        = 0;
            scout           = scout || (vis(Terran_Barracks) == 1);

            buildQueue[Terran_Supply_Depot] = (s >= 18);
            buildQueue[Terran_Barracks]     = (s >= 22) + (s >= 26);

            terranUnitPump[Terran_SCV]    = true;
            terranUnitPump[Terran_Marine] = true;
        }
    } // namespace

    void TvZ_2Rax()
    {
        // Openers
        if (currentOpener == T_11_13)
            TvZ_2Rax_1113();
    }
} // namespace McRave::BuildOrder::Terran