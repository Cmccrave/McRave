#include "Builds/Zerg/ZergBuildOrder.h"
#include "Main/Common.h"
#include "Map/Terrain.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void ZvFFA_PH_Overpool()
    {
        // 9o 9p 11h
        transitionReady = hatchCount() >= 2;
        scout           = scout || (com(Zerg_Drone) >= 11 && minerals(200));

        // Buildings
        buildQueue[Zerg_Hatchery]      = 1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0);
        buildQueue[Zerg_Spawning_Pool] = (vis(Zerg_Overlord) >= 2);
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone]    = vis(Zerg_Drone) < (11 - (hatchCount() >= 2));
        zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < lingsNeeded_ZvFFA();
    }

    void ZvFFA_PH()
    {
        // Openers
        if (currentOpener == Z_Overpool)
            ZvFFA_PH_Overpool();
    }

} // namespace McRave::BuildOrder::Zerg