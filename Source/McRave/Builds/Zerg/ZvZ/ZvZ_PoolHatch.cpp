#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {

        void ZvZ_PH_Overpool()
        {
            transitionReady =                               com(Zerg_Spawning_Pool) > 0;
            unitLimits[Zerg_Drone] =                        11 - vis(Zerg_Extractor);
            gasLimit =                                      capGas(100);

            auto secondHatch = Spy::enemyRush() ? com(Zerg_Sunken_Colony) >= 1 : (s >= 22 && vis(Zerg_Extractor) > 0 && total(Zerg_Zergling) >= 6);
            
            // Buildings
            buildQueue[Zerg_Hatchery] =                     1 + secondHatch;
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Extractor] =                    (s >= 22 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        }

        void ZvZ_PH_12Pool()
        {
            transitionReady =                               com(Zerg_Spawning_Pool) > 0;
            unitLimits[Zerg_Drone] =                        13 - vis(Zerg_Hatchery) - vis(Zerg_Spawning_Pool);
            gasLimit =                                      com(Zerg_Drone) >= 10 ? gasMax() : 0;
            wantNatural =                                   !Spy::enemyRush();

            auto secondHatch = Spy::enemyRush() ? com(Zerg_Sunken_Colony) >= 1 : (s >= 22 && vis(Zerg_Extractor) > 0);
            
            buildQueue[Zerg_Hatchery] =                     1 + secondHatch;
            buildQueue[Zerg_Spawning_Pool] =                s >= 24;
            buildQueue[Zerg_Extractor] =                    (s >= 22 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        }
    }

    void ZvZ_PH()
    {
        // Openers
        if (currentOpener == "Overpool")
            ZvZ_PH_Overpool();
        if (currentOpener == "12Pool")
            ZvZ_PH_12Pool();
    }
}