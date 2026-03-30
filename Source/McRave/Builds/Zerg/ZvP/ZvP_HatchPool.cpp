#include "Builds/Zerg/ZergBuildOrder.h"
#include "Main/Common.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void ZvP_HP_12Hatch()
    {
        // 12h 11p
        transitionReady = vis(Zerg_Spawning_Pool) > 0;
        scout           = scout || (hatchCount() == 1 && s >= 22 && Util::getTime() > Time(1, 30));
        planEarly       = !Spy::enemyProxy() && (hatchCount() == 1 && s >= 22 && Util::getTime() > Time(1, 30));

        // Buildings
        buildQueue[Zerg_Hatchery]      = 1 + (s >= 24);
        buildQueue[Zerg_Spawning_Pool] = (hatchCount() >= 2);
        buildQueue[Zerg_Overlord]      = 1 + (s >= 16) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone]    = vis(Zerg_Drone) < 13;
        zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < lingsNeeded_ZvP();
    }

    void ZvP_HP_10Hatch()
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
        zergUnitPump[Zerg_Zergling] = vis(Zerg_Zergling) < lingsNeeded_ZvP();
    }

    void ZvP_HP()
    {
        // If we scout a proxy early enough, switch to pool first
        if (Spy::getEnemyOpener() == P_Proxy_9_9 && currentOpener == Z_12Hatch && total(Zerg_Hatchery) < 2) {
            currentOpener = Z_10Hatch;
        }

        // Openers
        if (currentOpener == Z_10Hatch)
            ZvP_HP_10Hatch();
        if (currentOpener == Z_12Hatch)
            ZvP_HP_12Hatch();
    }
} // namespace McRave::BuildOrder::Zerg