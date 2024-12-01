#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {

        void ZvZ_PL_9Pool()
        {
            transitionReady =                               com(Zerg_Extractor) > 0 || Spy::enemyGasSteal();
            gasLimit =                                      (Spy::enemyRush() && com(Zerg_Sunken_Colony) == 0) ? 0 : gasMax();

            zergUnitPump[Zerg_Drone] = vis(Zerg_Drone) < (9 - (vis(Zerg_Extractor) > 0) + (vis(Zerg_Overlord) > 1));

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Extractor] =                    s >= 18 && vis(Zerg_Spawning_Pool) > 0;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }
    }

    void ZvZ_PL()
    {
        // Openers
        if (currentOpener == Z_9Pool)
            ZvZ_PL_9Pool();
    }
}