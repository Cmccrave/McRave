#include "Builds/Zerg/ZergBuildOrder.h"
#include "Info/Player/Players.h"
#include "Main/Common.h"
#include "Map/Terrain/Terrain.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void ZvT_PL_4Pool()
    {
        auto enemyHeldRush = Players::getCompleteCount(PlayerState::Enemy, Terran_Bunker) > 0 || Spy::enemyWalled() || Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) > 0 ||
                             total(Zerg_Zergling) >= 12;

        // 4p
        scout           = scout || (vis(Zerg_Spawning_Pool) > 0 && com(Zerg_Drone) >= 4 && !Terrain::getEnemyStartingPosition().isValid() && minerals(100));
        transitionReady = enemyHeldRush;
        rush            = !enemyHeldRush || total(Zerg_Drone) < 8;
        inTransition    = true;

        // Buildings
        buildQueue[Zerg_Spawning_Pool] = s >= 8;
        buildQueue[Zerg_Overlord]      = 1 + (s >= 18) + (s >= 30);

        // Pumping
        zergUnitPump[Zerg_Drone]    = (!enemyHeldRush && vis(Zerg_Drone) < 4) || (enemyHeldRush && vis(Zerg_Drone) < 10);
        zergUnitPump[Zerg_Zergling] = !enemyHeldRush && com(Zerg_Spawning_Pool) > 0;
    }

    void ZvT_PL()
    {
        // Openers
        if (currentOpener == Z_4Pool)
            ZvT_PL_4Pool();
    }
} // namespace McRave::BuildOrder::Zerg