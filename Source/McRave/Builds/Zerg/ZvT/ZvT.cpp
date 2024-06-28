#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void defaultZvT() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wallNat =                                   false;
        wallMain =                                  false;
        wantNatural =                               true;
        wantThird =                                 false;
        mineralThird =                              false;
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        gasTrick =                                  false;
        reserveLarva =                              0;

        baseToHatchRatio ={ { 1, 1 }, {2, 2}, {3, 4}, {4, 7} };

        gasLimit =                                  gasMax();
        unitLimits[Zerg_Zergling] =                 lingsNeeded_ZvT();
        unitLimits[Zerg_Drone] =                    INT_MAX;

        desiredDetection =                          Zerg_Overlord;
        focusUnit =                                 UnitTypes::None;

        armyComposition[Zerg_Drone] =               0.60;
        armyComposition[Zerg_Zergling] =            0.40;
    }

    int inboundUnits_ZvT()
    {
        static map<UnitType, double> trackables ={ {Terran_Marine, 1.0}, {Terran_Medic, 1.0}, {Terran_Firebat, 1.0} };
        auto inBoundUnit = [&](auto &u) {
            if (!Terrain::getEnemyMain())
                return true;
            const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();

            // Check if we know they weren't at home and are missing on the map for 35 seconds
            if (!Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), u->getPosition()) && !Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), u->getPosition()))
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 35);
            return false;
        };

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end()) {
                arrivalValue += idx->second;
                if (inBoundUnit(u))
                    arrivalValue += idx->second / 2.0;
            }
        }
        return int(arrivalValue);
    }

    int lingsNeeded_ZvT() {

        auto initialValue = 6;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        // 2Rax
        if (Spy::getEnemyBuild() == "2Rax") {
            initialValue = 6;
            if (Spy::getEnemyOpener() == "11/13")
                initialValue = 6;
            else if (Spy::getEnemyOpener() == "BBS")
                initialValue = 10;
        }

        // RaxCC
        if (Spy::getEnemyBuild() == "RaxCC") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "8Rax")
                initialValue = 10;
            else
                initialValue = 2;
        }

        // RaxFact
        if (Spy::getEnemyBuild() == "RaxFact") {
            initialValue = 2;
            if (Util::getTime() > Time(4, 00))
                initialValue = 10;
        }

        // TODO: Fix T spy
        if (Spy::getEnemyOpener() == "8Rax" || Spy::enemyProxy())
            initialValue = 10;
        if (Spy::getEnemyTransition() == "WorkerRush")
            initialValue = 24;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // If this is a 2Hatch build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("2Hatch") != string::npos)
            return 0;

        auto inbound = inboundUnits_ZvT();
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 36)
            return inbound;
        return 0;
    }

    void ZvT_2HatchMuta()
    {
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        inBookSupply =                                  total(Zerg_Mutalisk) < 3;

        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 28 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
        reserveLarva =                                  6;

        auto thirdHatch = (Spy::getEnemyBuild() == "RaxFact") ? total(Zerg_Mutalisk) >= 6 : (vis(Zerg_Spire) > 0);
        wantThird = (Spy::getEnemyBuild() == "RaxFact");

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        atPercent(Zerg_Lair, 0.95);

        // Research
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Reactions
        if (Spy::getEnemyOpener() == "8Rax" && total(Zerg_Zergling) < 6) 
            buildQueue[Zerg_Lair] = 0;

        // Pumping
        pumpLings = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && gas(80);

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10)
            gasLimit = gasMax();
    }

    void ZvT_3HatchMuta()
    {
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 4;

        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     lingsNeeded_ZvT();
        wantThird = (Spy::getEnemyBuild() == "RaxFact") || hatchCount() >= 3;
        reserveLarva =                                  9;

        auto thirdHatch = Spy::enemyProxy() ? total(Zerg_Zergling) >= 6 : (s >= 26 && vis(Zerg_Drone) >= 11);
        auto fourthHatch = (Spy::getEnemyBuild() == "RaxFact") ? total(Zerg_Mutalisk) >= 9 : (vis(Zerg_Spire) > 0 && s >= 66);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 32) + (s >= 44 && vis(Zerg_Drone) >= 20);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 42 && atPercent(Zerg_Lair, 0.80));

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Zergling) >= 6 && gas(80);

        // Research
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Pumping
        pumpLings = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        pumpMutas = com(Zerg_Spire) > 0 && gas(80);

        // Gas
        gasLimit = 0;
        if (vis(Zerg_Drone) >= 10)
            gasLimit = gasMax();
    }

    void ZvT_2HatchSpeedling()
    {
        inTransition =                                  total(Zerg_Zergling) >= 10;
        inOpening =                                     total(Zerg_Zergling) < 36;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        total(Zerg_Zergling) >= 12 ? 11 : 9;
        rush =                                          true;

        // Buildings
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 9);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 gas(90);

        // Pumping
        pumpLings = hatchCount() >= 3;

        // Gas
        gasLimit = 0;
        if (!lingSpeed())
            gasLimit = capGas(100);
    }

    void ZvT_3HatchSpeedling()
    {
        inTransition =                                  total(Zerg_Zergling) >= 10;
        inOpening =                                     total(Zerg_Zergling) < 40;
        inBookSupply =                                  vis(Zerg_Overlord) < 3;

        unitLimits[Zerg_Drone] =                        13;
        rush =                                          true;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1 + (s >= 22 && vis(Zerg_Spawning_Pool) > 0) + (s >= 26);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 26);
        buildQueue[Zerg_Extractor] =                    (s >= 24);

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 gas(90);

        // Pumping
        pumpLings = hatchCount() >= 3;

        // Gas
        gasLimit = 0;
        if (!lingSpeed())
            gasLimit = capGas(100);
    }

    void ZvT()
    {
        defaultZvT();

        // Builds
        if (currentBuild == "HatchPool")
            ZvT_HP();
        if (currentBuild == "PoolHatch")
            ZvT_PH();

        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyTransition() == "WorkerRush") {
                currentBuild = "PoolHatch";
                currentOpener = "12Pool";
                currentTransition = "2HatchMuta";
            }
            if (currentBuild == "3HatchMuta" && (Spy::enemyRush() || Spy::enemyProxy()))
                currentTransition = "2HatchMuta";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta")
                ZvT_2HatchMuta();
            if (currentTransition == "3HatchMuta")
                ZvT_3HatchMuta();
            if (currentTransition == "2HatchSpeedling")
                ZvT_2HatchSpeedling();
            if (currentTransition == "3HatchSpeedling")
                ZvT_3HatchSpeedling();
        }
    }
}