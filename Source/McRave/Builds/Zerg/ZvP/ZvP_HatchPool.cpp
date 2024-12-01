#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void ZvP_HP_12Hatch()
    {
        // 12h 11p
        transitionReady =                               vis(Zerg_Spawning_Pool) > 0;
        scout =                                         scout || (hatchCount() == 1 && s >= 22 && Util::getTime() > Time(1, 30));
        planEarly =                                     !Spy::enemyProxy() && (hatchCount() == 1 && s >= 22 && Util::getTime() > Time(1, 30));

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 24);
        buildQueue[Zerg_Spawning_Pool] =                (hatchCount() >= 2);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 16) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < 12;
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvP();
    }

    void ZvP_HP_10Hatch()
    {
        // 10h 9p
        transitionReady =                               vis(Zerg_Spawning_Pool) > 0;
        scout =                                         scout || (hatchCount() == 1 && s == 20 && Broodwar->self()->minerals() >= 150);
        planEarly =                                     !Spy::enemyProxy() && hatchCount() == 1 && s == 20 && Broodwar->self()->minerals() >= 150;
        gasTrick =                                      s >= 18 && hatchCount() < 2 && total(Zerg_Spawning_Pool) == 0;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 20);
        buildQueue[Zerg_Spawning_Pool] =                hatchCount() >= 2;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18 && vis(Zerg_Spawning_Pool) > 0) + (s >= 36);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < 10;
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvP();
    }

    void ZvP_HP()
    {
        // Openers
        if (currentOpener == Z_10Hatch)
            ZvP_HP_10Hatch();
        if (currentOpener == Z_12Hatch)
            ZvP_HP_12Hatch();
    }
}