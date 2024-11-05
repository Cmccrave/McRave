#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void ZvT_HP_12Hatch()
    {
        // 12h 11p
        transitionReady =                               vis(Zerg_Spawning_Pool) > 0;
        scout =                                         scout || (hatchCount() == 1 && s >= 22 && Util::getTime() > Time(1, 30));
        planEarly =                                     (hatchCount() == 1 && s >= 22 && Util::getTime() > Time(1, 30));

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
        buildQueue[Zerg_Spawning_Pool] =                (hatchCount() >= 2);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 16) + (s >= 32);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < 13;
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvT();
    }

    void ZvT_HP()
    {
        // Openers
        if (currentOpener == "12Hatch")
            ZvT_HP_12Hatch();
    }
}