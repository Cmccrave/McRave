#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
using namespace UpgradeTypes;
using namespace TechTypes;

#include "../ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    void defaultZvP() {
        inOpening =                                 true;
        inBookSupply =                              true;
        wallNat =                                   int(Stations::getStations(PlayerState::Self).size()) >= 3 && Spy::getEnemyBuild() == "FFE";
        wallMain =                                  false;
        wantNatural =                               !Spy::enemyProxy() || Spy::getEnemyBuild() == "CannonRush" || hatchCount() >= 2;
        wantThird =                                 Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        proxy =                                     false;
        hideTech =                                  false;
        rush =                                      false;
        pressure =                                  false;
        transitionReady =                           false;
        planEarly =                                 false;
        gasTrick =                                  false;
        mineralThird =                              false;
        reserveLarva =                              true;

        gasLimit =                                  gasMax();
        unitLimits[Zerg_Zergling] =                 lingsNeeded_ZvP();
        unitLimits[Zerg_Drone] =                    INT_MAX;

        desiredDetection =                          Zerg_Overlord;
        focusUpgrade =                              UpgradeTypes::None;
        focusTech =                                 TechTypes::None;
        focusUnit =                                 UnitTypes::None;

        armyComposition[Zerg_Drone] =               0.60;
        armyComposition[Zerg_Zergling] =            0.40;
    }

    int inboundUnits_ZvP()
    {
        set<UnitType> trackables ={ Protoss_Zealot, Protoss_Dragoon, Protoss_Dark_Templar };
        auto inBoundUnit = [&](auto &u) {
            if (trackables.find(u->getType()) != trackables.end()) {
                if (!Terrain::getEnemyMain())
                    return true;
                auto distSelf = BWEB::Map::getGroundDistance(u->getPosition(), Terrain::getNaturalPosition());
                auto distEnemy = BWEB::Map::getGroundDistance(u->getPosition(), Terrain::getEnemyNatural()->getBase()->Center());
                if (distSelf < distEnemy)
                    return true;
                return !Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), u->getPosition()) && !Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), u->getPosition());
            }
            return false;
        };

        // Economic estimate (have information on army, they aren't close):
        // For each Zealot or Dragoon, assume it arrives with 45 seconds for us to create Zerglings/Hydralisks
        auto arrivalValue = int(0.5 * count_if(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (inBoundUnit(u)) {
                const auto visDiff = !Terrain::inTerritory(PlayerState::Enemy, u->getPosition()) ? Broodwar->getFrameCount() - u->getLastVisibleFrame() : 0;
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 50);
            }
            return false;
        }));

        // Inbound estimate (have information on army, they're inbound):
        arrivalValue += int(1.5 * count_if(Units::getUnits(PlayerState::Enemy).begin(), Units::getUnits(PlayerState::Enemy).end(), [&](auto &u) {
            if (inBoundUnit(u)) {
                const auto visDiff = !Terrain::inTerritory(PlayerState::Enemy, u->getPosition()) ? Broodwar->getFrameCount() - u->getLastVisibleFrame() : 0;
                return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 30);
            }
            return false;
        }));
        return arrivalValue;
    }

    int lingsNeeded_ZvP() {
        auto lings = 0;
        auto initialValue = 6;
        if (com(Zerg_Spawning_Pool) == 0)
            return 0;

        // 2Gate
        if (Spy::getEnemyBuild() == "2Gate") {
            if (Spy::getEnemyOpener() == "10/15")
                initialValue = 6;
            else if (Spy::getEnemyOpener() == "Proxy9/9" || Spy::getEnemyOpener() == "9/9")
                initialValue = 16;
            else
                initialValue = 10;
        }

        // FFE
        if (Spy::getEnemyBuild() == "FFE") {
            initialValue = 2;
            if (Spy::getEnemyOpener() == "Gateway")
                initialValue = 6;
        }

        // 1GC
        if (Spy::getEnemyBuild() == "1GateCore") {
            initialValue = 6;
            if (Spy::getEnemyOpener() == "1Zealot" || Spy::getEnemyOpener() == "2Zealot")
                initialValue = 6;
        }

        // CannonRush
        if (Spy::getEnemyBuild() == "CannonRush")
            initialValue = 12;

        if (total(Zerg_Zergling) < initialValue)
            return initialValue;

        // Specifically for proxy defense, we can allow pretty much any amount of lings after we've droned
        if (vis(Zerg_Drone) >= 18 && Spy::getEnemyOpener() == "Proxy9/9" && Spy::getEnemyBuild() == "2Gate")
            return 24;

        // If this is a 2Hatch build, we prefer sunkens over lings, don't make any after initial
        if (currentTransition.find("2Hatch") != string::npos)
            return 0;

        auto inbound = inboundUnits_ZvP();
        if (vis(Zerg_Zergling) < inbound && vis(Zerg_Zergling) < 36)
            return inbound;
        return 0;
    }

    void ZvP2HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/2_Hatch_Muta_(vs._Protoss)'
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     Spy::getEnemyOpener() == "Proxy9/9" ? total(Zerg_Mutalisk) < 3 : total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 5 || total(Zerg_Mutalisk) < 6;
        focusUnit =                                     Zerg_Mutalisk;
        focusUpgrade =                                  (Spy::getEnemyOpener() == "Proxy9/9" && hatchCount() >= 4 && gas(100)) ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 26 : unitLimits[Zerg_Drone] - vis(Zerg_Extractor);

        wantThird =                                     Spy::enemyFastExpand() || hatchCount() >= 3 || Spy::getEnemyTransition() == "Corsair";
        gasLimit =                                      (vis(Zerg_Drone) >= 10) ? gasMax() : 0;


        auto thirdHatch = total(Zerg_Mutalisk) >= 6 && vis(Zerg_Drone) >= 18;
        if (Spy::getEnemyBuild() == "FFE") {
            thirdHatch = (s >= 26 && vis(Zerg_Drone) >= 11 && vis(Zerg_Lair) > 0);
            planEarly = false;
        }

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + thirdHatch;
        buildQueue[Zerg_Extractor] =                    (s >= 20 && hatchCount() >= 2 && vis(Zerg_Drone) >= 10) + (vis(Zerg_Spire) > 0 && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Lair] =                         (s >= 24 && gas(80));
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95));
        buildQueue[Zerg_Overlord] =                     (com(Zerg_Spire) == 0 && atPercent(Zerg_Spire, 0.35)) ? 5 : 1 + (s >= 18) + (s >= 32) + (2 * (s >= 50)) + (s >= 80);

        // Composition
        if (com(Zerg_Spire) == 0 && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvP3HatchMuta()
    {
        // 'https://liquipedia.net/starcraft/3_Hatch_Spire_(vs._Protoss)'
        inTransition =                                  hatchCount() >= 3 || total(Zerg_Mutalisk) > 0;
        inOpening =                                     total(Zerg_Mutalisk) < 9;
        inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;
        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 33 : unitLimits[Zerg_Drone];

        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);

        auto spireOverlords = (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "NeoBisu") ? (4 * (s >= 66)) : (3 * (s >= 66)) + (s >= 82);

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Extractor) > 0);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 13) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 20);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (hatchCount() >= 2 && s >= 32) + (s >= 48) + spireOverlords;

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;

        // Composition
        if ((com(Zerg_Spire) == 0 || Units::getImmThreat() > 0.0) && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvP3HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 32;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;
        focusUnit =                                     Zerg_Hydralisk;
        focusUpgrade =                                  UpgradeTypes::Muscular_Augments;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 19 : unitLimits[Zerg_Drone];
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? 3 : 0;

        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 26;
        mineralThird =                                  true;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26 && vis(Zerg_Drone) >= 11);
        buildQueue[Zerg_Extractor] =                    (s >= 30 && vis(Zerg_Drone) >= 11 && hatchCount() >= 3) + (vis(Zerg_Lair));
        buildQueue[Zerg_Hydralisk_Den] =                (vis(Zerg_Drone) >= 16 && s >= 38);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 54);

        // Upgrades
        upgradeQueue[Muscular_Augments] = com(Zerg_Hydralisk_Den) > 0;
        upgradeQueue[Grooved_Spines] = hydraSpeed();

        // Pumping
        auto pumpHydras = com(Zerg_Hydralisk_Den) > 0;
        auto pumpDrones = com(Zerg_Spawning_Pool) > 0 && vis(Zerg_Drone) < 19;
        auto pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);

        // Composition
        armyComposition.clear();
        if (pumpHydras)
            armyComposition[Zerg_Hydralisk] =           1.00;
        else if (pumpLings)
            armyComposition[Zerg_Zergling] =            1.00;
        else
            armyComposition[Zerg_Drone] =               1.00;
    }

    void ZvP4HatchHydra()
    {
        inTransition =                                  vis(Zerg_Hydralisk_Den) > 0;
        inOpening =                                     total(Zerg_Hydralisk) < 64 && total(Zerg_Zergling) < 100;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;
        focusUnit =                                     Zerg_Hydralisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 28 : unitLimits[Zerg_Drone];

        planEarly =                                     false;
        wantThird =                                     hatchCount() >= 4 && Spy::enemyFastExpand();
        reserveLarva =                                  false;

        // Buildings
        buildQueue[Zerg_Hatchery] =                     2 + (total(Zerg_Zergling) >= 6 && vis(Zerg_Drone) >= 11) + (s >= 54 && vis(Zerg_Drone) >= 18) + (s >= 84 && vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Extractor] =                    (s >= 26 && vis(Zerg_Drone) >= 13 && hatchCount() >= 3) + (vis(Zerg_Drone) >= 24);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 38 && vis(Zerg_Drone) >= 16 && gas(30));
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + (s >= 60) + (s >= 74);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore";
        if (total(Zerg_Hydralisk) >= 6) {
            upgradeQueue[Grooved_Spines] = rangeFirst ? vis(Zerg_Hydralisk_Den) > 0 : hydraSpeed();
            upgradeQueue[Muscular_Augments] = rangeFirst ? hydraRange() : vis(Zerg_Hydralisk_Den) > 0;
            upgradeQueue[Zerg_Carapace] = com(Zerg_Evolution_Chamber) > 0;
            upgradeQueue[Metabolic_Boost] = 1;
        }

        // Pumping
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 2;
        auto pumpHydras = vis(Zerg_Drone) >= 28 || needMinimumHydras;
        auto pumpLings = lingsNeeded_ZvP() > vis(Zerg_Zergling);

        // All-in
        if (Spy::enemyFastExpand() && !needMinimumHydras) {
            pumpHydras = false;
            pumpLings = true;
            unitLimits[Zerg_Zergling] = INT_MAX;
            upgradeQueue[Metabolic_Boost] = 1;
        }

        // Gas
        gasLimit = 0;
        if (pumpHydras)
            gasLimit = min(5, gasMax());
        else if (vis(Zerg_Hydralisk_Den) > 0)
            gasLimit = 1;
        else if (vis(Zerg_Drone) >= 11)
            gasLimit = 3;

        // Composition
        armyComposition.clear();
        if (pumpHydras)
            armyComposition[Zerg_Hydralisk] =           1.00;
        else if (pumpLings)
            armyComposition[Zerg_Zergling] =            1.00;
        else
            armyComposition[Zerg_Drone] =               1.00;
    }

    void ZvP6HatchHydra()
    {
        //'https://liquipedia.net/starcraft/6_hatch_(vs._Protoss)'
        inTransition =                                  Util::getTime() > Time(3, 15);
        inOpening =                                     total(Zerg_Hydralisk) < 60;
        inBookSupply =                                  vis(Zerg_Overlord) < 5;

        focusUnit =                                     Zerg_Hydralisk;
        focusUpgrade =                                  UpgradeTypes::None;

        wantThird =                                     true;
        mineralThird =                                  false;
        reserveLarva =                                  false;
        planEarly =                                     wantThird && hatchCount() < 3 && s >= 24;

        gasLimit =                                      (vis(Zerg_Drone) >= 16) ? gasMax() : 0;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 45 : unitLimits[Zerg_Drone];

        // Soulkey style (early 4th hatch, early den, skip spire)
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 46) + (s >= 62);
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 26) + (s >= 52) + (com(Zerg_Drone) >= 26 && s >= 64) + (com(Zerg_Drone) >= 26 && s >= 80);
        buildQueue[Zerg_Extractor] =                    (s >= 30) + (vis(Zerg_Drone) >= 28 && s >= 70) + (hatchCount() >= 6);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3);
        buildQueue[Zerg_Hydralisk_Den] =                (s >= 60);
        buildQueue[Zerg_Evolution_Chamber] =            (s >= 70) + (s >= 150);
        buildQueue[Zerg_Spire] =                        (s >= 140);

        // Upgrades
        auto rangeFirst = Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Spy::getEnemyBuild() == "1GateCore";
        if (total(Zerg_Hydralisk) > 0) {
            upgradeQueue[Grooved_Spines] = rangeFirst ? vis(Zerg_Hydralisk_Den) : hydraSpeed();
            upgradeQueue[Muscular_Augments] = rangeFirst ? hydraRange() : vis(Zerg_Hydralisk_Den);
            upgradeQueue[Zerg_Missile_Attacks] = (com(Zerg_Evolution_Chamber) > 0) + (com(Zerg_Evolution_Chamber) > 1);
            upgradeQueue[Zerg_Carapace] = (com(Zerg_Evolution_Chamber) > 1);
            techQueue[Burrowing] = com(Zerg_Drone) >= 36;
        }

        // Pumping
        auto needMinimumHydras = com(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Hydralisk) < Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + 2;
        auto pumpHydras = (com(Zerg_Hydralisk_Den) > 0 && needMinimumHydras) || (com(Zerg_Drone) >= 44 && (hydraRange() || hydraSpeed()));
        auto pumpLings = Util::getTime() < Time(7, 00) && lingsNeeded_ZvP() > vis(Zerg_Zergling);

        // Composition
        armyComposition.clear();
        if (pumpHydras)
            armyComposition[Zerg_Hydralisk] =           1.00;
        else if (pumpLings)
            armyComposition[Zerg_Zergling] =            1.00;
        else
            armyComposition[Zerg_Drone] =               1.00;
    }

    void ZvP6HatchCrackling()
    {
        // :)
        inTransition =                                  vis(Zerg_Lair) > 0;
        inOpening =                                     true;
        inBookSupply =                                  vis(Zerg_Overlord) < 8 || total(Zerg_Mutalisk) < 9;
        focusUpgrade =                                  vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
        focusUnit =                                     Zerg_Mutalisk;
        unitLimits[Zerg_Drone] =                        com(Zerg_Spawning_Pool) > 0 ? 24 : unitLimits[Zerg_Drone];
        unitLimits[Zerg_Zergling] =                     INT_MAX;

        wantThird =                                     Util::getTime() < Time(2, 45) || Spy::getEnemyBuild() == "FFE";
        gasLimit =                                      (vis(Zerg_Drone) >= 11) ? gasMax() : 0;
        planEarly =                                     wantThird && hatchCount() < 3 && Util::getTime() > Time(2, 30);

        auto spireOverlords = (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "NeoBisu") ? (4 * (s >= 66)) : (3 * (s >= 66)) + (s >= 82);

        if (vis(Zerg_Hive) > 0) {
            unitLimits[Zerg_Drone] = 30;
            focusUpgrade = UpgradeTypes::Adrenal_Glands;
        }

        if (com(Zerg_Hive) > 0)
            gasLimit = 3;

        // Build
        buildQueue[Zerg_Hatchery] =                     2 + (s >= 28 && vis(Zerg_Drone) >= 11 && vis(Zerg_Extractor) > 0) + (com(Zerg_Hive) > 0) + 2 * (s >= 100);
        buildQueue[Zerg_Extractor] =                    (s >= 28 && vis(Zerg_Drone) >= 11) + (vis(Zerg_Lair) > 0 && vis(Zerg_Drone) >= 21);
        buildQueue[Zerg_Lair] =                         (s >= 32 && gas(100) && hatchCount() >= 3 && vis(Zerg_Overlord) >= 3);
        buildQueue[Zerg_Spire] =                        (s >= 32 && atPercent(Zerg_Lair, 0.95) && vis(Zerg_Drone) >= 16);
        buildQueue[Zerg_Queens_Nest] =                  total(Zerg_Mutalisk) >= 9;
        buildQueue[Zerg_Hive] =                         atPercent(Zerg_Queens_Nest, 0.95);
        buildQueue[Zerg_Overlord] =                     1 + (s >= 18) + (s >= 32) + (s >= 48) + spireOverlords;

        // Upgrades
        upgradeQueue[Metabolic_Boost] =                 vis(Zerg_Lair) > 0;
        upgradeQueue[Adrenal_Glands] =                  com(Zerg_Hive) > 0;

        // Composition
        if (com(Zerg_Hive) > 0 || vis(Zerg_Drone) >= 30) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            0.70;
            armyComposition[Zerg_Mutalisk] =            0.30;
        }
        else if ((com(Zerg_Spire) == 0 || Units::getImmThreat() > 0.0) && lingsNeeded_ZvP() > vis(Zerg_Zergling)) {
            armyComposition[Zerg_Drone] =               0.00;
            armyComposition[Zerg_Zergling] =            1.00;
        }
        else if (vis(Zerg_Drone) < 30) {
            armyComposition[Zerg_Drone] =               0.60;
            armyComposition[Zerg_Zergling] =            0.00;
            armyComposition[Zerg_Mutalisk] =            0.40;
        }
    }

    void ZvP()
    {
        defaultZvP();

        // Builds
        if (currentBuild == "HatchPool")
            ZvP_HP();
        if (currentBuild == "PoolHatch")
            ZvP_PH();

        // Reactions
        if (!inTransition) {
            if (Spy::enemyProxy() && Util::getTime() < Time(2, 00)) {
                currentBuild = "PoolHatch";
                currentOpener = "Overpool";
                currentTransition = "3HatchMuta";
            }

            if (Spy::getEnemyBuild() != "FFE" && (currentTransition == "3HatchHydra" || currentTransition == "6HatchHydra") && com(Zerg_Spawning_Pool) > 0)
                currentTransition = "4HatchHydra";
            if (Spy::getEnemyBuild() == "FFE" && currentTransition == "4HatchHydra")
                currentTransition = "6HatchHydra";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "2HatchMuta")
                ZvP2HatchMuta();
            if (currentTransition == "3HatchMuta")
                ZvP3HatchMuta();
            if (currentTransition == "3HatchHydra")
                ZvP3HatchHydra();
            if (currentTransition == "4HatchHydra")
                ZvP4HatchHydra();
            if (currentTransition == "6HatchHydra")
                ZvP6HatchHydra();
            if (currentTransition == "6HatchCrackling")
                ZvP6HatchCrackling();
        }
    }
}