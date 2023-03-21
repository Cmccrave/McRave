#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    int lingsNeeded_ZvZ() {
        if (currentBuild == "2HatchMuta")
            return max(18, int(0.9 * Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling)));
        else if (vis(Zerg_Drone) >= 11 || Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) > 1)
            return 18;
        else if (vis(Zerg_Spire) > 0)
            return 12;
        else if (Spy::getEnemyTransition() == "2HatchSpeedling")
            return 6;
        return 8;
    }

    int dronesNeeded_ZvZ() {
        if (Spy::getEnemyOpener() == "12Pool" || Spy::getEnemyOpener() == "12Hatch")
            return 12;
        if (hatchCount() >= 2)
            return 11;
        if (Broodwar->getStartLocations().size() >= 4)
            return 10;
        return 9;
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
        reserveLarva =                                  true;

        gasLimit =                                      gasMax();
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvZ();
        unitLimits[Zerg_Drone] =                        dronesNeeded_ZvZ();

        desiredDetection =                              Zerg_Overlord;
        firstUpgrade =                                  ((vis(Zerg_Zergling) >= 6 && gas(100)) || lingSpeed()) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        firstTech =                                     TechTypes::None;
        firstUnit =                                     None;

        armyComposition[Zerg_Drone] =                   0.60;
        armyComposition[Zerg_Zergling] =                0.40;
    }

    void ZvZ_PL_1HatchMuta()
    {
        inOpening =                                     total(Zerg_Mutalisk) < 4;
        inTransition =                                  vis(Zerg_Lair) > 0;
        unitLimits[Zerg_Drone] =                        (vis(Zerg_Lair) > 0) ? 11 : 9;;
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvZ();
        gasLimit =                                      ((lingSpeed() || gas(80)) && total(Zerg_Lair) == 0) ? 2 : gasMax();
        firstUnit =                                     Zerg_Mutalisk;
        inBookSupply =                                  total(Zerg_Mutalisk) < 3;

        auto secondHatch = (Spy::getEnemyTransition() == "1HatchMuta" && total(Zerg_Mutalisk) >= 4)
            || (Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) >= 2 && Util::getTime() < Time(3, 30));
        wantNatural = secondHatch;

        // Build
        buildQueue[Zerg_Lair] =                         lingSpeed() && gas(100) && vis(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) >= 6 && vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Spire] =                        lingSpeed() && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 7;
        buildQueue[Zerg_Hatchery] =                     1 + secondHatch;
        buildQueue[Zerg_Overlord] =                     (com(Zerg_Spire) == 0 && atPercent(Zerg_Spire, 0.35) && (s >= 34)) ? 4 : 1 + ((vis(Zerg_Extractor) + Spy::enemyGasSteal() >= 1) && s >= 15) + (s >= 32) + (s >= 46);

        // Army Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvZ() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
            armyComposition[Zerg_Mutalisk] =            0.00;
        }
        else if (atPercent(Zerg_Spire, 0.9)) {
            armyComposition[Zerg_Drone] =               0.40;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.60;
        }
        else {
            armyComposition[Zerg_Drone] =               1.00;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.00;
        }
    }

    void ZvZ_PH_2HatchMuta()
    {
        inOpening =                                     total(Zerg_Mutalisk) < 3;
        inTransition =                                  vis(Zerg_Lair) > 0;
        unitLimits[Zerg_Drone] =                        dronesNeeded_ZvZ();
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvZ();
        gasLimit =                                      lingSpeed() ? 2 : gasMax();

        firstUnit =                                     Zerg_Mutalisk;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        // Build
        buildQueue[Zerg_Extractor] =                    (s >= 24) + (atPercent(Zerg_Spire, 0.5));
        buildQueue[Zerg_Lair] =                         gas(100) && vis(Zerg_Spawning_Pool) > 0 && total(Zerg_Zergling) >= 10 && vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Spire] =                        lingSpeed() && atPercent(Zerg_Lair, 0.95) && com(Zerg_Drone) >= 11;
        buildQueue[Zerg_Overlord] =                     1 + (vis(Zerg_Extractor) + Spy::enemyGasSteal() >= 1) + (s >= 32) + (atPercent(Zerg_Spire, 0.5) && s >= 38);

        // Army Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvZ() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.40;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.60;
        }
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
                ZvZ_PL_1HatchMuta();
            if (currentTransition == "2HatchMuta")
                ZvZ_PH_2HatchMuta();
        }
    }
}