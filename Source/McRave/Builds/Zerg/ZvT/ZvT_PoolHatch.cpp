#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {

        void ZvT_PH_4Pool()
        {
            scout =                                         scout || (vis(Zerg_Spawning_Pool) > 0 && com(Zerg_Drone) >= 4 && !Terrain::getEnemyStartingPosition().isValid());
            unitLimits[Zerg_Drone] =                        4;
            unitLimits[Zerg_Zergling] =                     INT_MAX;
            transitionReady =                               total(Zerg_Zergling) >= 24;
            rush =                                          true;

            // Buildings
            buildQueue[Zerg_Spawning_Pool] =                s >= 8;
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

            pumpLings = com(Zerg_Spawning_Pool) > 0;
        }

        void ZvT_PH_Overpool()
        {
            transitionReady =                               hatchCount() >= 2;
            unitLimits[Zerg_Drone] =                        (12 - vis(Zerg_Hatchery));
            unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
            gasLimit =                                      0;
            scout =                                         scout || (hatchCount() >= 2);

            // Buildings
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 22);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);
        }

        void ZvT_PH_12Pool()
        {
            transitionReady =                               hatchCount() >= 2;
            unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
            gasLimit =                                      com(Zerg_Drone) >= 11 ? gasMax() : 0;
            unitLimits[Zerg_Drone] =                        (13 - vis(Zerg_Extractor) - vis(Zerg_Spawning_Pool));
            scout =                                         scout || (com(Zerg_Drone) >= 9);

            // Buildings
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 24 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);
            buildQueue[Zerg_Spawning_Pool] =                s >= 26;
        }
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