#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {

        void ZvZ_PL_9Pool()
        {
            transitionReady =                               com(Zerg_Extractor) > 0;
            unitLimits[Zerg_Drone] =                        9 - (vis(Zerg_Extractor) > 0) + (vis(Zerg_Overlord) > 1);
            unitLimits[Zerg_Zergling] =                     Spy::enemyRush() ? INT_MAX : 10;
            gasLimit =                                      (Spy::enemyRush() && com(Zerg_Sunken_Colony) == 0) ? 0 : gasMax();
            playPassive =                                   (com(Zerg_Mutalisk) == 0 && Spy::enemyRush())
                                                            || (Spy::getEnemyOpener() == "9Pool" && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 8 && !Spy::enemyRush() && !Spy::enemyPressure() && total(Zerg_Mutalisk) == 0)
                                                            || (Broodwar->getStartLocations().size() >= 3 && Util::getTime() < Time(3,00) && !Terrain::getEnemyStartingPosition().isValid())
                                                            || (com(Zerg_Mutalisk) == 0 && Spy::enemyFastExpand() && Util::getTime() > Time(4, 10));

            buildQueue[Zerg_Spawning_Pool] =                s >= 18;
            buildQueue[Zerg_Extractor] =                    s >= 18 && vis(Zerg_Spawning_Pool) > 0;
            buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) >= 1) + (s >= 32);
        }
    }

    void ZvZ_PL()
    {
        // Openers
        if (currentOpener == "9Pool")
            ZvZ_PL_9Pool();
    }
}