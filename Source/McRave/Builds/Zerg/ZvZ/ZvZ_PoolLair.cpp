#include "Builds/Zerg/ZergBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void ZvZ_PL_Overpool()
    {
        transitionReady = com(Zerg_Extractor) > 0 || Spy::enemyGasSteal();
        gasLimit        = (Spy::enemyRush() && com(Zerg_Sunken_Colony) == 0) ? 0 : gasMax();

        // Buildings
        buildQueue[Zerg_Hatchery]      = 1;
        buildQueue[Zerg_Spawning_Pool] = (s >= 18 && vis(Zerg_Overlord) >= 2);
        buildQueue[Zerg_Extractor]     = (s >= 18 && vis(Zerg_Spawning_Pool) > 0);
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18) + (s >= 32);

        // Pumping
        zergUnitPump[Zerg_Drone]    = vis(Zerg_Drone) < (11 - vis(Zerg_Extractor));
        zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < lingsNeeded_ZvZ();
    }

    void ZvZ_PL_9Pool()
    {
        transitionReady = com(Zerg_Extractor) > 0 || Spy::enemyGasSteal();
        gasLimit        = (Spy::enemyRush() && com(Zerg_Sunken_Colony) == 0) ? 0 : gasMax();

        // Buildings
        buildQueue[Zerg_Spawning_Pool] = s >= 18;
        buildQueue[Zerg_Extractor]     = s >= 18 && vis(Zerg_Spawning_Pool) > 0;
        buildQueue[Zerg_Overlord]      = 1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);

        // Pumping
        zergUnitPump[Zerg_Drone] = vis(Zerg_Drone) < (9 - (vis(Zerg_Extractor) > 0) + (vis(Zerg_Overlord) > 1));
    }

    void ZvZ_PL_GasPool()
    {
        transitionReady = vis(Zerg_Extractor) > 0 || Spy::enemyGasSteal();
        gasLimit        = (Spy::enemyRush() && com(Zerg_Sunken_Colony) == 0) ? 0 : gasMax();

        // Buildings
        buildQueue[Zerg_Spawning_Pool] = s >= 18 && vis(Zerg_Extractor) >= 1;
        buildQueue[Zerg_Extractor]     = s >= 18 && vis(Zerg_Overlord) >= 2;
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18) + (s >= 32);

        // Pumping
        zergUnitPump[Zerg_Drone]    = vis(Zerg_Drone) < 10;
        zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < lingsNeeded_ZvZ();
    }

    void ZvZ_PL()
    {
        // Openers
        if (currentOpener == Z_9Pool)
            ZvZ_PL_9Pool();
        if (currentOpener == Z_Overpool)
            ZvZ_PL_Overpool();
        if (currentOpener == Z_Gaspool)
            ZvZ_PL_GasPool();
    }
} // namespace McRave::BuildOrder::Zerg