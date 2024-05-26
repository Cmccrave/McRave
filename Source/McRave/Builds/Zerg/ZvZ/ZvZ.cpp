#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    int lingsNeeded_ZvZ() {
        auto initialValue = 10;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        auto macroHatch = (currentBuild == "PoolHatch" || currentBuild == "HatchPool") ? 1 : 0;
        auto enemyDrones = Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone);
        auto minDrones = max(enemyDrones + 1, (Stations::getGasingStationsCount() * 8) + int(macroHatch * 3));

        // 1Hatch builds can't make too many lings
        if (currentTransition == "1HatchMuta" && total(Zerg_Mutalisk) == 0) {
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) > 1)
                initialValue = 10;
            else
                initialValue = 18;
        }

        // 2Hatch builds can make more lings
        if (currentTransition == "2HatchMuta" && total(Zerg_Mutalisk) == 0) {
            initialValue = 24;
            if (vis(Zerg_Drone) >= minDrones)
                initialValue = 36;
        }

        return initialValue;
    }

    void defaultZvZ() {
        inOpening =                                     true;
        inBookSupply =                                  true;
        wallNat =                                       hatchCount() >= 3;
        wallMain =                                      false;
        wantNatural =                                   false;
        wantThird =                                     false;
        proxy =                                         false;
        hideTech =                                      false;
        rush =                                          false;
        pressure =                                      false;
        transitionReady =                               false;
        gasTrick =                                      false;
        reserveLarva =                                  0;

        gasLimit =                                      gasMax();

        desiredDetection =                              Zerg_Overlord;
        focusUnit =                                     UnitTypes::None;

        armyComposition[Zerg_Drone] =                   0.60;
        armyComposition[Zerg_Zergling] =                0.40;
    }

    void ZvZ_1HatchMuta()
    {
        inOpening =                                     total(Zerg_Mutalisk) < 7;
        inTransition =                                  vis(Zerg_Lair) > 0;
        inBookSupply =                                  total(Zerg_Mutalisk) < 3;

        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 18 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvZ();
        focusUnit =                                     Zerg_Mutalisk;
        reserveLarva =                                  3;

        auto secondHatch = (Spy::getEnemyTransition() == "1HatchMuta" && atPercent(Zerg_Spire, 0.5))
            || (Spy::enemyTurtle() && vis(Zerg_Drone) >= 15);
        wantNatural = Spy::enemyTurtle();

        // Build
        buildQueue[Zerg_Lair] =                         lingSpeed() && gas(100) && vis(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) >= 6 && vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Spire] =                        lingSpeed() && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 7;
        buildQueue[Zerg_Hatchery] =                     1 + secondHatch;
        buildQueue[Zerg_Overlord] =                     (com(Zerg_Spire) == 0 && atPercent(Zerg_Spire, 0.35) && (s >= 34)) ? 4 : 1 + ((vis(Zerg_Extractor) + Spy::enemyGasSteal() >= 1) && s >= 15) + (s >= 32) + (s >= 46);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 (vis(Zerg_Zergling) >= 6 && gas(100));

        // Pumping
        pumpLings = lingsNeeded_ZvZ() > total(Zerg_Zergling) || (com(Zerg_Spire) == 1 && !gas(100)) || vis(Zerg_Drone) >= unitLimits[Zerg_Drone];
        pumpMutas = com(Zerg_Spire) == 1 && gas(100);

        // Reactions
        if (Spy::enemyTurtle()) {
            wantNatural = true;
            pumpLings = false;
        }

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyTurtle() && (lingSpeed() || gas(80)) && total(Zerg_Lair) == 0)
            gasLimit = 2;
    }

    void ZvZ_2HatchMuta()
    {
        inOpening =                                     total(Zerg_Mutalisk) < 3;
        inTransition =                                  vis(Zerg_Lair) > 0;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 18 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvZ();
        focusUnit =                                     Zerg_Mutalisk;
        reserveLarva =                                  3;

        // Build
        buildQueue[Zerg_Extractor] =                    (s >= 24) + (atPercent(Zerg_Spire, 0.5));
        buildQueue[Zerg_Lair] =                         gas(100) && vis(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) >= 10 && vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Spire] =                        lingSpeed() && atPercent(Zerg_Lair, 0.95) && com(Zerg_Drone) >= 9;
        buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) + Spy::enemyGasSteal() >= 1) + (s >= 32) + (s >= 46);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 (vis(Zerg_Zergling) >= 6 && gas(100));

        // Pumping
        pumpLings = lingsNeeded_ZvZ() > total(Zerg_Zergling) || (com(Zerg_Spire) == 1 && !gas(100)) || vis(Zerg_Drone) >= unitLimits[Zerg_Drone];
        pumpMutas = com(Zerg_Spire) == 1 && gas(100);

        // Reactions
        if (Spy::enemyTurtle()) {
            buildQueue[Zerg_Hatchery] = 3;
            wantNatural = true;
            pumpLings = false;
        }

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyTurtle() && (lingSpeed() || gas(80)) && total(Zerg_Lair) == 0)
            gasLimit = 2;
    }

    void ZvZ_2HatchHydra()
    {
        inOpening =                                     total(Zerg_Hydralisk) < 6;
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        20;
        unitLimits[Zerg_Zergling] =                     max(lingsNeeded_ZvZ(), Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) + 4);
        focusUnit =                                     Zerg_Hydralisk;
        wantNatural =                                   true;

        // Build
        buildQueue[Zerg_Overlord] =                     2 + (s >= 32) + (s >= 46);
        buildQueue[Zerg_Extractor] =                    1;
        buildQueue[Zerg_Hatchery] =                     2 + (vis(Zerg_Drone) >= 11 && total(Zerg_Zergling) >= 10) + (vis(Zerg_Drone) >= 18);
        buildQueue[Zerg_Hydralisk_Den] =                hatchCount() >= 4;

        // Upgrades
        upgradeQueue[UpgradeTypes::Muscular_Augments] = vis(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[UpgradeTypes::Grooved_Spines] = vis(Zerg_Hydralisk_Den) > 0 && hydraSpeed();

        // Pumping
        pumpLings = vis(Zerg_Zergling) < unitLimits[Zerg_Zergling];
        pumpHydras = Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 && com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) + 6;

        // Gas
        gasLimit = 3;
        if (vis(Zerg_Hydralisk_Den) > 0)
            gasLimit = gasMax();
        else if (lingSpeed())
            gasLimit = 1;
    }

    void ZvZ()
    {
        defaultZvZ();

        // Builds
        if (currentBuild == "PoolHatch")
            ZvZ_PH();
        if (currentBuild == "PoolLair")
            ZvZ_PL();

        // Reactions
        if (!inTransition) {
            if (Spy::enemyGasSteal())
                currentTransition = "2HatchMuta";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "1HatchMuta")
                ZvZ_1HatchMuta();
            if (currentTransition == "2HatchMuta")
                ZvZ_2HatchMuta();
            if (currentTransition == "2HatchHydra")
                ZvZ_2HatchHydra();
        }
    }
}