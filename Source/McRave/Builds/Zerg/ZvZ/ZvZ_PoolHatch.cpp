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
            transitionReady =                               total(Zerg_Zergling) >= 6 || (Spy::enemyFastExpand() && com(Zerg_Spawning_Pool) > 0);
            unitLimits[Zerg_Drone] =                        Spy::enemyFastExpand() ? 16 : 10;
            unitLimits[Zerg_Zergling] =                     6;
            gasLimit =                                      capGas(100);
            
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 22);
            buildQueue[Zerg_Spawning_Pool] =                (vis(Zerg_Overlord) >= 2);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        }

        void ZvZ_PH_12Pool()
        {
            transitionReady =                               total(Zerg_Zergling) >= 6;
            unitLimits[Zerg_Drone] =                        vis(Zerg_Extractor) > 0 ? 9 : 12;
            unitLimits[Zerg_Zergling] =                     12;
            gasLimit =                                      com(Zerg_Drone) >= 10 ? gasMax() : 0;
            focusUpgrade =                                  (vis(Zerg_Zergling) >= 6 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
            
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Extractor) > 0 && !Spy::enemyRush());
            buildQueue[Zerg_Spawning_Pool] =                s >= 24;
            buildQueue[Zerg_Extractor] =                    (s >= 24 && vis(Zerg_Spawning_Pool) > 0);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18);
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