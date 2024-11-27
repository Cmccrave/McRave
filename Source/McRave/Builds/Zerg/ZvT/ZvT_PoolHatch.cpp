#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void ZvT_PH_4Pool()
    {
        auto enemyHeldRush = Players::getCompleteCount(PlayerState::Enemy, Terran_Bunker) > 0 || Spy::enemyWalled() || Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) > 0;

        // 4p
        scout =                                         scout || (vis(Zerg_Spawning_Pool) > 0 && com(Zerg_Drone) >= 4 && !Terrain::getEnemyStartingPosition().isValid());
        transitionReady =                               (enemyHeldRush && vis(Zerg_Drone) >= 10);
        rush =                                          !enemyHeldRush || total(Zerg_Drone) < 8;
        inTransition =                                  true;

        // Buildings
        buildQueue[Zerg_Spawning_Pool] =                s >= 8;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      (!enemyHeldRush && vis(Zerg_Drone) < 4) || (enemyHeldRush && vis(Zerg_Drone) < 10);
        zergUnitPump[Zerg_Zergling] =                   !enemyHeldRush && com(Zerg_Spawning_Pool) > 0;
    }

    void ZvT_PH_Overpool()
    {
        // 9o 9p
        transitionReady =                               hatchCount() >= 2;
        scout =                                         scout || (hatchCount() >= 2);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 22);
        buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < (12 - vis(Zerg_Hatchery));
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvT();
    }

    void ZvT_PH_12Pool()
    {
        // 13p 12h
        transitionReady =                               hatchCount() >= 2;
        scout =                                         scout || (com(Zerg_Drone) >= 9);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 24 && vis(Zerg_Spawning_Pool) > 0);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);
        buildQueue[Zerg_Spawning_Pool] =                s >= 26;

        // Pumping
        zergUnitPump[Zerg_Drone] =                      vis(Zerg_Drone) < (13 - vis(Zerg_Extractor) - vis(Zerg_Spawning_Pool));
        zergUnitPump[Zerg_Zergling] =                   vis(Zerg_Zergling) < lingsNeeded_ZvT();
    }

    void ZvT_PH()
    {
        // Openers
        if (currentOpener == "4Pool")
            ZvT_PH_4Pool();
        if (currentOpener == "Overpool")
            ZvT_PH_Overpool();
        if (currentOpener == "12Pool")
            ZvT_PH_12Pool();
    }
}