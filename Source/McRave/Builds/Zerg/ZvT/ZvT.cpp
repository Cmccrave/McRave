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

        wantNatural =                               hatchCount() >= 3 || (Spy::getEnemyTransition() != "WorkerRush");
        wantThird =                                 hatchCount() >= 3;

        mineralThird =                              false;
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        gasTrick =                                  false;
        reserveLarva =                              0;

        gasLimit =                                  gasMax();

        desiredDetection =                          Zerg_Overlord;
        focusUnit =                                 UnitTypes::None;
    }

    int inboundUnits_ZvT()
    {
        static map<UnitType, double> trackables ={ {Terran_Marine, 1.0}, {Terran_Medic, 1.0}, {Terran_Firebat, 1.0} };

        // Economic estimate (have information on army, they aren't close):
        // For each unit, assume it arrives with enough time for us to create defenders
        auto arrivalValue = 0.0;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;

            auto idx = trackables.find(unit.getType());
            if (idx != trackables.end()) {
                arrivalValue += idx->second;
                if (Units::inBoundUnit(unit))
                    arrivalValue += idx->second / 2.0;
            }
        }

        // Make less if we have some other units outside our opening
        if (total(Zerg_Zergling) >= 10 && vis(Zerg_Zergling) >= 6) {
            arrivalValue -= vis(Zerg_Mutalisk);
            arrivalValue -= (vis(Zerg_Sunken_Colony) + vis(Zerg_Creep_Colony)) * 6.0;
        }

        return int(arrivalValue);
    }

    int lingsNeeded_ZvT() {

        auto initialValue = 2;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        // 2Rax
        if (Spy::getEnemyBuild() == "2Rax") {
            initialValue = 6;
            if (Spy::getEnemyOpener() == "BBS")
                initialValue = 10;
        }

        // RaxCC
        if (Spy::getEnemyBuild() == "RaxCC") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "8Rax")
                initialValue = 10;
        }

        // RaxFact
        if (Spy::getEnemyBuild() == "RaxFact" || Spy::enemyWalled()) {
            initialValue = 2;
            if (Util::getTime() > Time(3, 45))
                initialValue = 6;
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

    void ZvT_1HatchLurker()
    {
        inTransition =                                  true;
        inOpening =                                     true;
        inBookSupply =                                  total(Zerg_Overlord) < 3;

        focusUnit =                                     Zerg_Lurker;
        pressure =                                      com(Zerg_Hydralisk_Den) > 0;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     1;
        buildQueue[Zerg_Extractor] =                    vis(Zerg_Drone) >= 8;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Lair] =                         (com(Zerg_Hydralisk_Den) > 0 && gas(80));
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 20 && gas(50));

        // Research
        techQueue[TechTypes::Lurker_Aspect] =           com(Zerg_Hydralisk_Den) && com(Zerg_Lair);

        // Pumping
        zergUnitPump[Zerg_Drone] |=                     vis(Zerg_Drone) < 11 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] =                   false;
        zergUnitPump[Zerg_Hydralisk] =                  com(Zerg_Hydralisk_Den) > 0;
        zergUnitPump[Zerg_Lurker] =                     com(Zerg_Hydralisk) > 0 && Players::hasResearched(PlayerState::Self, TechTypes::Lurker_Aspect);

        // Gas
        gasLimit = gasMax();
    }

    void ZvT_2HatchMuta()
    {
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) <= 9;
        inBookSupply =                                  total(Zerg_Overlord) < 3;

        focusUnit =                                     Zerg_Mutalisk;
        reserveLarva =                                  6;

        auto thirdHatch = (com(Zerg_Spire) == 0 && s >= 50 && vis(Zerg_Drone) >= 22) || (com(Zerg_Spire) == 1 && total(Zerg_Mutalisk) >= 6 && vis(Zerg_Drone) >= 22);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 2 && vis(Zerg_Drone) >= 10 && vis(Zerg_Spawning_Pool) > 0) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 20);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        atPercent(Zerg_Lair, 0.95);

        // Research
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Reactions
        if (Spy::getEnemyOpener() == "8Rax" && total(Zerg_Zergling) < 6)
            buildQueue[Zerg_Lair] = 0;

        // Pumping
        zergUnitPump[Zerg_Drone] |=                     vis(Zerg_Drone) < 28 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] |=                  lingsNeeded_ZvT() > vis(Zerg_Zergling);
        zergUnitPump[Zerg_Mutalisk] =                   com(Zerg_Spire) > 0;

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyFastExpand()) {
            if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 10)
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && Spy::getEnemyBuild() == "2Rax" && Util::getTime() < Time(4, 00))
                gasLimit = 0;
            else if (Spy::enemyProxy() && Util::getTime() < Time(3, 00))
                gasLimit = 0;
        }
    }

    void ZvT_3HatchMuta()
    {
        // 13h 12g
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     total(Zerg_Mutalisk) <= 12;
        inBookSupply =                                  vis(Zerg_Overlord) < 4;

        focusUnit =                                     Zerg_Mutalisk;
        reserveLarva =                                  9;

        auto thirdHatch = Spy::enemyProxy() ? total(Zerg_Zergling) >= 6 : (s >= 26 && vis(Zerg_Drone) >= 11);
        auto fourthHatch = (Spy::getEnemyBuild() == "RaxFact" || !Spy::enemyFastExpand()) ? com(Zerg_Mutalisk) > 0 : (vis(Zerg_Spire) > 0 && s >= 66);

        auto secondGas = Spy::enemyFastExpand() ? (s >= 44 && vis(Zerg_Drone) >= 20) : (com(Zerg_Lair) > 0);

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch + fourthHatch;
        buildQueue[Zerg_Extractor] =                    (hatchCount() >= 3) + secondGas;
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 42 && atPercent(Zerg_Lair, 0.80));

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 (Spy::getEnemyBuild() != "RaxFact" && vis(Zerg_Zergling) >= 6 && gas(80)) || (vis(Zerg_Spire) > 0 && Spy::enemyFastExpand());

        // Research
        techQueue[TechTypes::Burrowing] =               Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 15);

        // Ling pump
        auto firstLingPump = lingsNeeded_ZvT() > vis(Zerg_Zergling);
        auto secondLingPump = false;// Spy::enemyFastExpand() && total(Zerg_Mutalisk) >= 9 && total(Zerg_Zergling) < 24;

        // Pumping
        zergUnitPump[Zerg_Drone] |=                     vis(Zerg_Drone) < 33 && com(Zerg_Spawning_Pool) > 0;
        zergUnitPump[Zerg_Zergling] |=                  firstLingPump || secondLingPump;
        zergUnitPump[Zerg_Mutalisk] =                   com(Zerg_Spire) > 0 && gas(80);

        // Gas
        gasLimit = gasMax();
        if (!Spy::enemyFastExpand()) {
            if (vis(Zerg_Drone) + vis(Zerg_Extractor) < 10)
                gasLimit = 0;
            else if (vis(Zerg_Lair) > 0 && Spy::getEnemyBuild() == "2Rax" && Util::getTime() < Time(4, 30))
                gasLimit = 0;
            else if (Spy::enemyProxy() && Util::getTime() < Time(3, 30))
                gasLimit = 0;
        }
    }

    void ZvT()
    {
        defaultZvT();

        // Reactions
        if (!inTransition) {
            if (Spy::getEnemyTransition() == "WorkerRush") {
                currentBuild = "PoolHatch";
                currentOpener = "12Pool";
                currentTransition = "2HatchMuta";
            }
        }

        // Builds
        if (currentBuild == "HatchPool")
            ZvT_HP();
        if (currentBuild == "PoolHatch")
            ZvT_PH();
        if (currentBuild == "PoolLair")
            ZvT_PL();

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta")
                ZvT_2HatchMuta();
            if (currentTransition == "3HatchMuta")
                ZvT_3HatchMuta();
            if (currentTransition == "1HatchLurker")
                ZvT_1HatchLurker();
        }
    }
}