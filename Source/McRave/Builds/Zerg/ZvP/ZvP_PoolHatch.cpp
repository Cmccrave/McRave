#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void ZvP_PH_Overpool()
    {
        // 9o 9p 11h
        transitionReady =                               hatchCount() >= 2;
        scout =                                         scout || (com(Zerg_Drone) >= 11 && minerals(200));

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0);
        buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < (12 - vis(Zerg_Hatchery));
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvP();
    }

    void ZvP_PH_4Pool()
    {
        // 4p
        transitionReady =                               total(Zerg_Zergling) >= 24;
        scout =                                         scout || (vis(Zerg_Spawning_Pool) > 0 && com(Zerg_Drone) >= 4 && !Terrain::getEnemyStartingPosition().isValid());
        rush =                                          true;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1;
        buildQueue[Zerg_Spawning_Pool] =                s >= 8;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < 4;
        zergUnitPump[Zerg_Zergling] =                   com(Zerg_Spawning_Pool) > 0;
    }

    void ZvP_PH_9Pool()
    {
        // 9p 10o 10h
        transitionReady =                               hatchCount() >= 2;
        gasTrick =                                      vis(Zerg_Spawning_Pool) > 0 && total(Zerg_Overlord) < 2;
        scout =                                         scout || (vis(Zerg_Spawning_Pool) > 0 && s >= 22);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 20 && vis(Zerg_Spawning_Pool) > 0 && atPercent(Zerg_Spawning_Pool, 0.8 && total(Zerg_Zergling) >= 6) && vis(Zerg_Overlord) >= 2 && (!Spy::enemyProxy() || vis(Zerg_Sunken_Colony) >= 2));
        buildQueue[Zerg_Spawning_Pool] =                s >= 18;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 20 && vis(Zerg_Spawning_Pool) > 0) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < (12 - vis(Zerg_Hatchery));
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvP();
    }

    void ZvP_PH()
    {
        // Openers
        if (currentOpener == "Overpool")
            ZvP_PH_Overpool();
        if (currentOpener == "4Pool")
            ZvP_PH_4Pool();
        if (currentOpener == "9Pool")
            ZvP_PH_9Pool();
    }
}