#include "Builds/Zerg/ZergBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void ZvFFA_HP_10Hatch()
    {
        // 10h 9p
        transitionReady = vis(Zerg_Spawning_Pool) > 0;
        scout           = scout || (hatchCount() == 1 && s == 20 && Broodwar->self()->minerals() >= 150);
        planEarly       = !Spy::enemyProxy() && hatchCount() == 1 && s == 20 && Broodwar->self()->minerals() >= 150;
        gasTrick        = true;

        // Buildings
        buildQueue[Zerg_Hatchery]      = 1 + (s >= 20);
        buildQueue[Zerg_Spawning_Pool] = hatchCount() >= 2;
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18 && vis(Zerg_Spawning_Pool) > 0) + (s >= 36);

        // Pumping
        zergUnitPump[Zerg_Drone]    = vis(Zerg_Drone) < 11;
        zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < lingsNeeded_ZvFFA();
    }

    void ZvFFA_HP()
    {
        // Openers
        if (currentOpener == Z_10Hatch)
            ZvFFA_HP_10Hatch();
    }
} // namespace McRave::BuildOrder::Zerg