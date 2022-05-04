#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    namespace {
        bool lingSpeed() {
            return Broodwar->self()->isUpgrading(UpgradeTypes::Metabolic_Boost) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost);
        }

        bool gas(int amount) {
            return Broodwar->self()->gas() >= amount;
        }

        int gasMax() {
            return Resources::getGasCount() * 3;
        }

        int capGas(int value) {
            auto onTheWay = 0;
            for (auto &w : Units::getUnits(PlayerState::Self)) {
                auto &worker = *w;
                if (worker.unit()->isCarryingGas() || (worker.hasResource() && worker.getResource().getType().isRefinery()))
                    onTheWay+=8;
            }

            return int(round(double(value - Broodwar->self()->gas() - onTheWay) / 8.0));
        }

        int hatchCount() {
            return vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);
        }

        int colonyCount() {
            return vis(Zerg_Creep_Colony) + vis(Zerg_Sunken_Colony);
        }

        int lingsNeeded() {
            auto lings = 0;
            auto timingValue = 0;
            auto initialValue = 6;

            if (com(Zerg_Spawning_Pool) == 0)
                return 0;

            if (total(Zerg_Zergling) < initialValue)
                return initialValue;

            auto time = double((Util::getTime().minutes - 1) * 60 + (Util::getTime().seconds)) / 60.0;
            return int(time * 3);
        }

        void defaultZvFFA() {
            inOpeningBook =                             true;
            inBookSupply =                              true;
            wallNat =                                   hatchCount() >= 4 || Terrain::isShitMap();
            wallMain =                                  false;
            wantNatural =                               true;
            wantThird =                                 Spy::getEnemyBuild() == "FFE";
            proxy =                                     false;
            hideTech =                                  false;
            playPassive =                               false;
            rush =                                      false;
            pressure =                                  false;
            transitionReady =                           false;
            planEarly =                                 false;

            gasLimit =                                  gasMax();
            unitLimits[Zerg_Zergling] =                 lingsNeeded();
            unitLimits[Zerg_Drone] =                    INT_MAX;

            desiredDetection =                          Zerg_Overlord;
            firstUpgrade =                              vis(Zerg_Zergling) >= 6 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
            firstTech =                                 TechTypes::None;
            firstUnit =                                 None;

            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.40;
        }
    }

    void ZvFFA3HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/3_Hatch_Spire_(vs._Protoss)'
        lockedTransition =                              hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpeningBook =                                 total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;
        firstUpgrade =                                  UpgradeTypes::None;
        firstUnit =                                     Zerg_Mutalisk;
        hideTech =                                      true;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : 15 - hatchCount();
        unitLimits[Zerg_Zergling] =                     lingsNeeded();
        playPassive =                                   (com(Zerg_Mutalisk) == 0 || Util::getTime() < Time(7, 00));
        wantThird =                                     Spy::enemyFastExpand() || hatchCount() >= 3;
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;

        auto fourthHatch = com(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 16;
        auto spireOverlords = (3 * (s >= 66)) + (s >= 82);

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + fourthHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 32 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
        buildQueue[Zerg_Lair] =                         (s >= 32 && vis(Zerg_Drone) >= 15 && gas(100));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32 && vis(Zerg_Extractor) > 0) + (s >= 48) + spireOverlords;

        // Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            com(Zerg_Spire) > 0 ? 0.00 : 0.40;
            armyComposition[Zerg_Mutalisk] =            com(Zerg_Spire) > 0 ? 0.40 : 0.00;
        }
    }

    void ZvFFA10Hatch()
    {
        // 'https://liquipedia.net/starcraft/10_Hatch'
        transitionReady =                               total(Zerg_Overlord) >= 2;
        unitLimits[Zerg_Zergling] =                     lingsNeeded();
        gasLimit =                                      0;
        scout =                                         false;
        wantNatural =                                   !Spy::enemyProxy();
        playPassive =                                   true;
        unitLimits[Zerg_Drone] =                        10;
        planEarly =                                     hatchCount() == 1 && s == 20 && Broodwar->self()->minerals() >= 150;
        gasTrick =                                      s >= 18 && hatchCount() < 2 && total(Zerg_Spawning_Pool) == 0;

        buildQueue[Zerg_Hatchery] =                     1 + (s >= 20);
        buildQueue[Zerg_Spawning_Pool] =                hatchCount() >= 2;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18 && vis(Zerg_Spawning_Pool) > 0) + (s >= 36);
    }

    void ZvFFA()
    {
        defaultZvFFA();

        // Openers
        if (currentOpener == "10Hatch")
            ZvFFA10Hatch();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "3HatchMuta")
                ZvFFA3HatchMuta();
        }
    }
}