#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {

        void ZvFFA_HP_3HatchMuta()
        {
            // 'https://liquipedia.net/starcraft/3_Hatch_Spire_(vs._Protoss)'
            inTransition =                                  hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
            inOpening =                                     total(Zerg_Mutalisk) < 9;
            inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;

            focusUnit =                                     Zerg_Mutalisk;
            unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : 15 - hatchCount();
            unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvFFA();
            wantThird =                                     Spy::enemyFastExpand() || hatchCount() >= 3;
            gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;

            auto fourthHatch = com(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 16;
            auto spireOverlords = (3 * (s >= 66)) + (s >= 82);

            // Buildings
            buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + fourthHatch;
            buildQueue[Zerg_Extractor] =                    (s >= 32 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
            buildQueue[Zerg_Lair] =                         (s >= 32 && vis(Zerg_Drone) >= 15 && gas(100));
            buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + spireOverlords;

            // Pumping
            pumpLings = lingsNeeded_ZvFFA() > vis(Zerg_Zergling);
            pumpMutas = com(Zerg_Spire) > 0;
        }

        void ZvFFA_HP_10Hatch()
        {
            // 'https://liquipedia.net/starcraft/10_Hatch'
            transitionReady =                               total(Zerg_Overlord) >= 2;
            unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvFFA();
            gasLimit =                                      0;
            wantNatural =                                   !Spy::enemyProxy();
            unitLimits[Zerg_Drone] =                        10;
            planEarly =                                     hatchCount() == 1 && s == 20 && Broodwar->self()->minerals() >= 150;
            gasTrick =                                      s >= 18 && hatchCount() < 2 && total(Zerg_Spawning_Pool) == 0;

            // Buildings
            buildQueue[Zerg_Hatchery] =                     1 + (s >= 20);
            buildQueue[Zerg_Spawning_Pool] =                hatchCount() >= 2;
            buildQueue[Zerg_Overlord] =                     1 + (s >= 18 && vis(Zerg_Spawning_Pool) > 0) + (s >= 36);
        }
    }

    void ZvFFA_HP()
    {
        // Openers
        if (currentOpener == "10Hatch")
            ZvFFA_HP_10Hatch();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "3HatchMuta")
                ZvFFA_HP_3HatchMuta();
        }
    }
}