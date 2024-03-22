#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void ZvP4HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/4_Hatch_Lair_(vs._Protoss)'
        inTransition =                                  atPercent(Zerg_Lair, 0.25) || total(Zerg_Mutalisk) > 0;
        inOpening =                                     total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 6 || total(Zerg_Mutalisk) < 6;

        focusUnit =                                     Zerg_Mutalisk;
        pressure =                                      vis(Zerg_Zergling) >= 36;
        unitLimits[Zerg_Drone] =                        32;
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvP();
        wantThird =                                     Spy::getEnemyBuild() == "FFE";

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28) + (vis(Zerg_Drone) >= 18 && vis(Zerg_Overlord) >= 3);
        buildQueue[Zerg_Extractor] =                    (vis(Zerg_Drone) >= 18) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 18) + (total(Zerg_Mutalisk) >= 9);
        buildQueue[Zerg_Lair] =                         (gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 36 && atPercent(Zerg_Lair, 0.95));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 36) + (s >= 46) + (s >= 54) + (s >= 62) + (2 * (s >= 88));

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;

        // Pumping
        pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && gas(80);

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 12)
            gasLimit = gasMax();
    }

    void ZvP2HatchSpeedling()
    {
        inTransition =                                  true;
        inOpening =                                     total(Zerg_Zergling) < 90;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        10 - hatchCount() - vis(Zerg_Extractor);
        unitLimits[Zerg_Zergling] =                     INT_MAX;
        gasLimit =                                      !lingSpeed() ? capGas(100) : 0;
        wallNat =                                       true;
        pressure =                                      !Spy::enemyProxy();
        wantNatural =                                   !Spy::enemyProxy();

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 20 && vis(Zerg_Spawning_Pool) > 0 && (!Spy::enemyProxy() || vis(Zerg_Zergling) >= 6));
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && com(Zerg_Drone) >= 8 && vis(Zerg_Zergling) >= 6 && (!Spy::enemyProxy() || vis(Zerg_Sunken_Colony) > 0));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 gas(100);

        // Pumping
        pumpLings =                                     vis(Zerg_Extractor) > 0;
        
    }

    void ZvP3HatchSpeedling()
    {
        // 'https://liquipedia.net/starcraft/3_Hatch_Zergling_(vs._Protoss)'
        inTransition =                              true;
        inOpening =                                 total(Zerg_Zergling) < 56 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) == 0;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        13;
        unitLimits[Zerg_Zergling] =                     INT_MAX;
        gasLimit =                                      !lingSpeed() ? capGas(100) : 0;
        wallNat =                                       true;
        pressure =                                      com(Zerg_Hatchery) >= 3 && inOpening;
        wantThird =                                     false;

        // Build
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (s >= 24);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 (hatchCount() >= 3 && gas(100));

        // Pumping
        pumpLings =                                     hatchCount() >= 3;
    }

    void ZvP2HatchLurker()
    {
        // 'https://liquipedia.net/starcraft/2_Hatch_Lurker_(vs._Terran)'
        inTransition =                              true;
        inOpening =                                 total(Zerg_Lurker) < 3;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        20;
        unitLimits[Zerg_Zergling] =                     12;
        unitLimits[Zerg_Hydralisk] =                    3;
        focusUnit =                                     Zerg_Lurker;
        gasLimit =                                      gasMax();

        // Buildings
        buildQueue[Zerg_Extractor] =                    (s >= 24) + (atPercent(Zerg_Lair, 0.90));
        buildQueue[Zerg_Lair] =                         (vis(Zerg_Drone) >= 10 && gas(80));
        buildQueue[Zerg_Hydralisk_Den] =                atPercent(Zerg_Lair, 0.5);

        // Research
        techQueue[Lurker_Aspect] =                      (com(Zerg_Lair) > 0);

        // Composition
        armyComposition[Zerg_Drone] =                   0.60;
        armyComposition[Zerg_Zergling] =                0.20;
        armyComposition[Zerg_Hydralisk] =               0.20;
        armyComposition[Zerg_Lurker] =                  1.00;
    }

    void ZvP2HatchCrackling()
    {
        inTransition =                                  true;
        inOpening =                                     total(Zerg_Zergling) < 100;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        20;
        unitLimits[Zerg_Zergling] =                     200;
        
        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32 && vis(Zerg_Extractor) > 0);
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 60) + (s >= 70) + (s >= 76) + (s >= 82) + (s >= 88);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Extractor] =                    (s >= 24 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10);
        buildQueue[Zerg_Evolution_Chamber] =            (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 18);
        buildQueue[Zerg_Queens_Nest] =                  (com(Zerg_Lair) == 1 && vis(Zerg_Drone) >= 18);
        buildQueue[Zerg_Hive] =                         (com(Zerg_Queens_Nest) == 1 && vis(Zerg_Drone) >= 18);

        // Upgrades
        upgradeQueue[Zerg_Carapace] = vis(Zerg_Lair) > 0;
        upgradeQueue[Metabolic_Boost] = vis(Zerg_Lair) > 0;
        upgradeQueue[Adrenal_Glands] = com(Zerg_Hive) > 0;

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10 && Broodwar->self()->gatheredGas() < 800)
            gasLimit = gasMax();

        // Pumping
        pumpLings = vis(Zerg_Drone) >= 20 || lingsNeeded_ZvP() > vis(Zerg_Zergling);
    }
}