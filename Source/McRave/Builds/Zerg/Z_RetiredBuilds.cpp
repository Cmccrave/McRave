#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void ZvP4HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/4_Hatch_Lair_(vs._Protoss)'
        inTransition =                              atPercent(Zerg_Lair, 0.25) || total(Zerg_Mutalisk) > 0;
        inOpening =                                 total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 6 || total(Zerg_Mutalisk) < 6;
        firstUpgrade =                                  (com(Zerg_Lair) > 0 && vis(Zerg_Overlord) >= 5 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        firstUnit =                                     Zerg_Mutalisk;
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

        // Composition
        if (Spy::getEnemyBuild() == "2Gate" && Spy::getEnemyTransition() != "Corsair" && (com(Zerg_Sunken_Colony) == 0 || com(Zerg_Drone) < 9)) {
            armyComposition[Zerg_Drone] =               0.20;
            armyComposition[Zerg_Zergling] =            0.80;
            gasLimit =                                  vis(Zerg_Drone) >= 12 ? gasMax() : 0;
        }
        else {
            armyComposition[Zerg_Drone] =               com(Zerg_Spire) > 0 ? 0.10 : 0.60;
            armyComposition[Zerg_Zergling] =            com(Zerg_Spire) > 0 ? 0.50 : 0.40;
            armyComposition[Zerg_Mutalisk] =            com(Zerg_Spire) > 0 ? 0.40 : 0.00;
            gasLimit =                                  vis(Zerg_Drone) >= 12 ? gasMax() : 0;
        }
    }

    void ZvP2HatchSpeedling()
    {
        inTransition =                              true;
        inOpening =                                 total(Zerg_Zergling) < 90;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;
        unitLimits[Zerg_Drone] =                        10 - hatchCount() - vis(Zerg_Extractor);
        unitLimits[Zerg_Zergling] =                     INT_MAX;
        gasLimit =                                      !lingSpeed() ? capGas(100) : 0;
        wallNat =                                       true;
        pressure =                                      !Spy::enemyProxy();
        wantNatural =                                   !Spy::enemyProxy();
        firstUpgrade =                                  gas(100) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;

        // Build
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 20 && vis(Zerg_Spawning_Pool) > 0 && (!Spy::enemyProxy() || vis(Zerg_Zergling) >= 6));
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && com(Zerg_Drone) >= 8 && vis(Zerg_Zergling) >= 6 && (!Spy::enemyProxy() || vis(Zerg_Sunken_Colony) > 0));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 30);

        // Composition
        if (vis(Zerg_Drone) < 6) {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.40;
        }
        else {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
    }

    void ZvP3HatchSpeedling()
    {
        // 'https://liquipedia.net/starcraft/3_Hatch_Zergling_(vs._Protoss)'
        inTransition =                              true;
        inOpening =                                 total(Zerg_Zergling) < 56 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) == 0;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;
        firstUpgrade =                                  (hatchCount() >= 3 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
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

        // Composition
        armyComposition[Zerg_Drone] =                   0.20;
        armyComposition[Zerg_Zergling] =                0.80;
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
        firstUpgrade =                                  UpgradeTypes::None;
        firstTech =                                     TechTypes::Lurker_Aspect;
        firstUnit =                                     Zerg_Lurker;
        gasLimit =                                      gasMax();

        buildQueue[Zerg_Extractor] =                    (s >= 24) + (atPercent(Zerg_Lair, 0.90));
        buildQueue[Zerg_Lair] =                         (vis(Zerg_Drone) >= 10 && gas(80));
        buildQueue[Zerg_Hydralisk_Den] =                atPercent(Zerg_Lair, 0.5);

        // Composition
        armyComposition[Zerg_Drone] =                   0.60;
        armyComposition[Zerg_Zergling] =                0.20;
        armyComposition[Zerg_Hydralisk] =               0.20;
        armyComposition[Zerg_Lurker] =                  1.00;
    }

    void ZvP2HatchCrackling()
    {
        inTransition =                              true;
        inOpening =                                 total(Zerg_Zergling) < 100;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;
        firstUpgrade =                                  lingSpeed() ? UpgradeTypes::Zerg_Carapace : UpgradeTypes::Metabolic_Boost;
        firstUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Hive) > 0 ? 21 : 18;
        unitLimits[Zerg_Zergling] =                     200;
        gasLimit =                                      (vis(Zerg_Drone) >= 10 && Broodwar->self()->gatheredGas() < 800) ? gasMax() : 0;

        if (vis(Zerg_Lair) > 0)
            firstUpgrade = UpgradeTypes::Zerg_Carapace;
        if (Broodwar->self()->isUpgrading(UpgradeTypes::Zerg_Carapace))
            firstUpgrade = UpgradeTypes::Metabolic_Boost;
        if (com(Zerg_Hive) > 0)
            firstUpgrade = UpgradeTypes::Adrenal_Glands;

        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32 && vis(Zerg_Extractor) > 0);
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 60) + (s >= 70) + (s >= 76) + (s >= 82) + (s >= 88);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Extractor] =                    (s >= 24 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10);
        buildQueue[Zerg_Evolution_Chamber] =            (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 18);
        buildQueue[Zerg_Queens_Nest] =                  (com(Zerg_Lair) == 1 && vis(Zerg_Drone) >= 18);
        buildQueue[Zerg_Hive] =                         (com(Zerg_Queens_Nest) == 1 && vis(Zerg_Drone) >= 18);

        // Composition
        if (lingsNeeded_ZvP() > vis(Zerg_Zergling) || vis(Zerg_Drone) == unitLimits[Zerg_Drone]) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               1.00;
            armyComposition[Zerg_Zergling] =            0.00;
        }
    }
}