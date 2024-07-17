#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

// Notes:
// Lings: 3 drones per hatch
// Hydra: 5.5 drones per hatch - 0.5 gas per hatch
// Muta: 8 drones per hatch - 1 gas per hatch

#include "ZergBuildOrder.h"

namespace McRave::BuildOrder::Zerg {

    namespace {
        bool againstRandom = false;
        bool needSpores = false;
        bool needSunks = false;
        string nodeName = "[Zerg]: ";

        bool logFlags[10];

        void queueGasTrick()
        {
            // Executing gas trick
            if (gasTrick) {
                if (Broodwar->self()->minerals() >= 80 && total(Zerg_Extractor) == 0)
                    buildQueue[Zerg_Extractor] = 1;
                if (vis(Zerg_Extractor) > 0)
                    buildQueue[Zerg_Extractor] = (vis(Zerg_Drone) < 9);
            }
        }

        void queueWallDefenses()
        {
            // Adding Wall defenses
            auto needLarvaSpending = vis(Zerg_Larva) > 3 && Broodwar->self()->supplyUsed() < Broodwar->self()->supplyTotal() && Util::getTime() < Time(4, 30) && com(Zerg_Sunken_Colony) >= 2;
            if (!needLarvaSpending && !rush && !pressure && (vis(Zerg_Drone) >= 9 || Players::ZvZ())) {
                for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                    if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()))
                        continue;

                    auto colonies = 0;
                    auto stationNeeds = wall.getStation() && (Stations::needAirDefenses(wall.getStation()) > 0 || Stations::needGroundDefenses(wall.getStation()) > 0);
                    for (auto& tile : wall.getDefenses()) {
                        if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                            colonies++;
                        if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony && stationNeeds && wall.getStation()->getDefenses().find(tile) != wall.getStation()->getDefenses().end())
                            colonies--;
                    }

                    auto airNeeded = Walls::needAirDefenses(wall);
                    auto grdNeeded = Walls::needGroundDefenses(wall);
                    if (airNeeded > colonies)
                        needSpores = true;
                    if (grdNeeded > colonies)
                        needSunks = true;

                    if ((atPercent(Zerg_Evolution_Chamber, 0.50) && airNeeded > colonies) || (vis(Zerg_Spawning_Pool) > 0 && grdNeeded > colonies)) {
                        buildQueue[Zerg_Creep_Colony] += 1;
                        return; // TODO: Only queue 1 for now otherwise sometimes we plan 2 (2 walls separately request a colony)
                    }
                }
            }
        }

        void queueStationDefenses()
        {
            // Adding Station defenses
            if (vis(Zerg_Drone) >= 8 || Players::ZvZ()) {
                for (auto &station : Stations::getStations(PlayerState::Self)) {
                    auto colonies = Stations::getColonyCount(station);
                    auto airNeeded = Stations::needAirDefenses(station);
                    auto grdNeeded = Stations::needGroundDefenses(station);

                    if (airNeeded > colonies)
                        needSpores = true;
                    if (grdNeeded > colonies)
                        needSunks = true;

                    if ((vis(Zerg_Spawning_Pool) > 0 && grdNeeded > colonies) || (atPercent(Zerg_Evolution_Chamber, 0.50) && airNeeded > colonies)) {
                        buildQueue[Zerg_Creep_Colony] += 1;
                        return; // TODO: Only queue 1 for now otherwise sometimes we plan 2 (2 stations separately request a colony)
                    }
                }
            }

            // Prepare evo chamber just in case and not a hydra build
            if (focusUnit != Zerg_Hydralisk) {
                if (Players::ZvT() && Spy::getEnemyBuild() == "RaxFact" && Util::getTime() > Time(4, 30)) {
                    needSpores = true;
                    wallNat = true;
                }
                if (Players::ZvP() && Spy::getEnemyTransition() == "Unknown" && ((Spy::getEnemyBuild() == "2Gate" && Util::getTime() > Time(4, 45)) || (Spy::getEnemyBuild() == "1GateCore" && Util::getTime() > Time(4, 15)))) {
                    needSpores = true;
                    wallNat = false;
                }
                if (Players::ZvZ() && Spy::getEnemyOpener() != "4Pool" && !Spy::enemyTurtle() && Spy::getEnemyOpener() != "7Pool" && Spy::getEnemyTransition() == "Unknown" && Util::getTime() > Time(5, 00)) {
                    needSpores = true;
                    wallNat = true;
                }
            }

            // Log needing sunks
            if (needSunks && !logFlags[0]) {
                logFlags[0] = true;
                Util::debug(nodeName + "Need sunkens for defense");
            }

            // Log needing spores
            if (needSpores && !logFlags[1]) {
                logFlags[1] = true;
                Util::debug(nodeName + "Need spores for defense");
            }
        }

        void queueSupply()
        {
            // Adding Overlords outside opening book supply, offset by hatch count
            if (!inBookSupply) {
                int offset = hatchCount() * 2;
                int supplyPerOvie = 15 - (hatchCount() / 2);
                int count = 1 + min(26, (s - offset) / supplyPerOvie);
                buildQueue[Zerg_Overlord] = min(25, count);
            }

            // Adding Overlords if we are sacrificing a scout or know we will lose one
            if (Scouts::isSacrificeScout() && vis(Zerg_Overlord) == total(Zerg_Overlord) && Util::getTime() < Time(5, 30)
                && (Players::getStrength(PlayerState::Enemy).airToAir > 0.0 || Players::getStrength(PlayerState::Enemy).groundToAir > 0.0)
                && (Spy::getEnemyTransition() == "Corsair" || Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "Unknown")) {
                buildQueue[Zerg_Overlord]++;
            }

            // Remove spending money on Overlords if they'll have no protection anyways
            if (Players::ZvP() && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > 0 && vis(Zerg_Lair) == 0 && vis(Zerg_Hydralisk_Den) == 0 && vis(Zerg_Spore_Colony) == 0 && Util::getTime() < Time(7, 00))
                buildQueue[Zerg_Overlord] = 0;
        }

        void queueGeysers()
        {
            // Adding Extractors
            if (!inOpening) {

                auto minDronesAllGas = 40;
                if (Players::ZvZ())
                    minDronesAllGas = 10;

                auto minDronesPerGas = 10 + (4 * vis(Zerg_Extractor));
                auto takeAllGeysers = com(Zerg_Drone) >= minDronesAllGas;
                auto allowNewGeyser =  com(Zerg_Drone) >= minDronesPerGas && (Resources::isHalfMineralSaturated() || productionSat || takeAllGeysers);
                auto needGeyser = gasLimit > vis(Zerg_Extractor) * 3;
                gasDesired = allowNewGeyser && needGeyser;

                buildQueue[Zerg_Extractor] = min(vis(Zerg_Extractor) + gasDesired, Resources::getGasCount());

                // Try to cancel extractors we don't need
                if (gasLimit == 0 && !Players::ZvZ())
                    buildQueue[Zerg_Extractor] = 0;
            }
        }

        void queueUpgradeStructures()
        {
            // Adding Evolution Chambers
            if ((s >= 200 && Stations::getStations(PlayerState::Self).size() >= 3)
                || (focusUnit == Zerg_Ultralisk && vis(Zerg_Queens_Nest) > 0))
                buildQueue[Zerg_Evolution_Chamber] = 1 + (Stations::getStations(PlayerState::Self).size() >= 4);
            if (needSpores)
                buildQueue[Zerg_Evolution_Chamber] = max(buildQueue[Zerg_Evolution_Chamber], 1);

            // Adding a Lair
            if (Spy::enemyInvis() && !Players::ZvZ() && Util::getTime() > Time(4, 45)) {
                buildQueue[Zerg_Lair] = 1;
                buildQueue[Zerg_Extractor] = max(vis(Zerg_Extractor), 2);
            }

            // Hive upgrades
            if (int(Stations::getStations(PlayerState::Self).size()) >= 4 - Players::ZvT() && vis(Zerg_Drone) >= 36) {
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive] = com(Zerg_Queens_Nest) >= 1;
                buildQueue[Zerg_Lair] = com(Zerg_Queens_Nest) < 1;
            }
        }

        void queueExpansions()
        {
            if (!inOpening) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto availableGas = Broodwar->self()->gas() - BuildOrder::getGasQueued();
                const auto incompleteHatch = vis(Zerg_Hatchery) - com(Zerg_Hatchery);

                const auto waitForMinerals = 300 + (150 * incompleteHatch);
                const auto resourceSat = (availableMinerals >= waitForMinerals && Resources::isHalfMineralSaturated() && Resources::isGasSaturated());

                expandDesired = (resourceSat && techSat && productionSat)
                    || (Players::ZvZ() && Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0 && Stations::getStations(PlayerState::Self).size() < Stations::getStations(PlayerState::Enemy).size() && availableMinerals > waitForMinerals && availableGas < 150)
                    || (Stations::getStations(PlayerState::Self).size() >= 4 && Stations::getMiningStationsCount() <= 2)
                    || (Stations::getStations(PlayerState::Self).size() >= 4 && Stations::getGasingStationsCount() <= 1);

                buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + expandDesired);

                if (expandDesired && int(Stations::getStations(PlayerState::Self).size()) <= 1)
                    wantNatural = true;
                if (expandDesired && int(Stations::getStations(PlayerState::Self).size()) <= 2)
                    wantThird = true;
            }
        }

        void queueProduction()
        {
            if (!inOpening) {

                // Calculate hatcheries per base with current composition
                static double hatchPerBase = 0.0;
                hatchPerBase = armyComposition[Zerg_Zergling] * 2.5
                    + armyComposition[Zerg_Mutalisk] * 1.0
                    + armyComposition[Zerg_Hydralisk] * 2.0
                    + armyComposition[Zerg_Ultralisk] * 1.0
                    + armyComposition[Zerg_Defiler] * 1.0;

                // Calculate hatcheries per base with next composition
                // TODO: Create these
                if (Players::ZvZ() && int(Stations::getStations(PlayerState::Self).size()) >= 2)
                    hatchPerBase = 1.5;

                // Log if we have a change in production expectations
                static double oldHatchPerBase = 0.0;
                if (hatchPerBase != oldHatchPerBase) {
                    Util::debug("[BuildOrder]: Changed production to " + to_string(hatchPerBase) + " from " + to_string(oldHatchPerBase));
                }
                oldHatchPerBase = hatchPerBase;

                // Check if we are maxed on production
                auto desiredProduction = int(round(hatchPerBase * double(Stations::getStations(PlayerState::Self).size())));
                productionSat = hatchCount() >= min(6, desiredProduction);
                auto maxSat = productionSat * 2;

                // Queue production if we need it
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto incompleteHatch = vis(Zerg_Hatchery) - com(Zerg_Hatchery);
                const auto waitForMinerals = 300 + (150 * incompleteHatch);

                const auto resourceSat = (availableMinerals >= waitForMinerals && Resources::isHalfMineralSaturated() && Resources::isGasSaturated() && !productionSat && vis(Zerg_Larva) <= 3);
                const auto larvaBankrupt = (availableMinerals >= waitForMinerals && (vis(Zerg_Larva) + (3 * incompleteHatch)) < min(3, hatchCount()) && !productionSat);

                rampDesired = resourceSat || larvaBankrupt;
                buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + rampDesired);
            }

            // Above 3 hatcheries we need to carefully add, don't add hatcheries if we have larva to spend
            if (inOpening && hatchCount() >= 3 && inTransition) {
                if (vis(Zerg_Larva) >= min(3, hatchCount()))
                    buildQueue[Zerg_Hatchery] = hatchCount();
            }
        }

        void calculateGasLimit()
        {
            // Calculate the number of gas workers we need outside of the opening book
            if (!inOpening)
                gasLimit = vis(Zerg_Drone) / 3;

            // If enemy has invis and we are trying to get lair for detection
            if (Spy::enemyInvis() && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0 && Util::getTime() > Time(4, 45) && atPercent(Zerg_Lair, 0.5))
                gasLimit += 1;

            // If we have very limited mineral mining (maybe we pulled workers to fight) we need to cut all gas
            if (!Players::ZvZ() && Workers::getMineralWorkers() <= 5 && Util::getTime() < Time(5, 00))
                gasLimit = 0;

            // If we have to pull everything we have
            if (Players::ZvZ() && Spy::getEnemyOpener() == "4Pool" && currentOpener == "12Pool" && total(Zerg_Zergling) < 18)
                gasLimit = 0;
        }

        void removeExcessGas()
        {
            // Removing gas workers if we are adding Sunkens or have excess gas
            if (Util::getTime() > Time(3, 00) || Players::ZvZ()) {
                auto gasRemaining       = Broodwar->self()->gas() - BuildOrder::getGasQueued();
                auto minRemaining       = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                auto dropGasRush        = !Players::ZvZ() && Spy::enemyRush();
                auto dropGasLarva       = !Players::ZvZ() && vis(Zerg_Larva) >= hatchCount() && unitReservations.empty();
                auto dropGasDefenses    = needSunks && (Players::ZvZ() || Spy::enemyProxy() || Spy::getEnemyOpener() == "9/9" || Spy::getEnemyOpener() == "8Rax");

                auto mineralToGasRatio  = minRemaining < max(100, 8 * vis(Zerg_Drone)) && gasRemaining > max(150, 13 * vis(Zerg_Drone));


                if (mineralToGasRatio && !rush && !pressure) {
                    if (dropGasRush
                        || dropGasLarva
                        || dropGasDefenses
                        || Roles::getMyRoleCount(Role::Worker) < 5
                        || Players::ZvZ())
                        gasLimit = 0;
                }
            }
        }

        void queueUpgrades()
        {
            using namespace UpgradeTypes;

            // Overlord speed can be done inside openings
            const auto queueOvieSpeed = (Players::ZvT() && Spy::getEnemyTransition() == "2PortWraith" && Util::getTime() > Time(6, 00))
                || (Players::ZvP() && Players::getStrength(PlayerState::Enemy).airToAir > 0 && Players::getSupply(PlayerState::Self, Races::Zerg) >= 160 && total(Zerg_Hydralisk) > 0)
                || (Spy::enemyInvis() && (BuildOrder::isFocusUnit(Zerg_Hydralisk) || BuildOrder::isFocusUnit(Zerg_Ultralisk)))
                || (!Players::ZvZ() && Players::getSupply(PlayerState::Self, Races::Zerg) >= 200);

            // Need to do this otherwise our queue isnt empty
            if (queueOvieSpeed)
                upgradeQueue[Pneumatized_Carapace] = 1;

            if (inOpening)
                return;

            // Speed upgrades
            upgradeQueue[Metabolic_Boost] = vis(Zerg_Zergling) >= 20 || total(Zerg_Hive) > 0 || total(Zerg_Defiler_Mound) > 0;
            upgradeQueue[Muscular_Augments] = (BuildOrder::isFocusUnit(Zerg_Hydralisk) && Players::ZvP()) || Broodwar->self()->getUpgradeLevel(Grooved_Spines) > 0;
            upgradeQueue[Anabolic_Synthesis] = Players::getTotalCount(PlayerState::Enemy, Terran_Marine) < 20 || Broodwar->self()->getUpgradeLevel(Chitinous_Plating) > 0;

            // Range upgrades
            upgradeQueue[Grooved_Spines] = (BuildOrder::isFocusUnit(Zerg_Hydralisk) && !Players::ZvP()) || Broodwar->self()->getUpgradeLevel(Muscular_Augments) > 0;

            // Other upgrades
            upgradeQueue[Chitinous_Plating] = Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 20 || Broodwar->self()->getUpgradeLevel(Anabolic_Synthesis) > 0;
            upgradeQueue[Adrenal_Glands] = Broodwar->self()->getUpgradeLevel(Metabolic_Boost) > 0;

            // Ground unit upgrades
            auto upgradingGrdArmor = (Broodwar->self()->getUpgradeLevel(Zerg_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Melee_Attacks)) || Broodwar->self()->isUpgrading(Zerg_Carapace);
            upgradeQueue[Zerg_Melee_Attacks] = upgradingGrdArmor && (BuildOrder::getCompositionPercentage(Zerg_Zergling) > 0.0 || BuildOrder::getCompositionPercentage(Zerg_Ultralisk) > 0.0);
            upgradeQueue[Zerg_Missile_Attacks] = upgradingGrdArmor && (BuildOrder::getCompositionPercentage(Zerg_Hydralisk) > 0.0 || BuildOrder::getCompositionPercentage(Zerg_Lurker) > 0.0);
            upgradeQueue[Zerg_Carapace] = (BuildOrder::getCompositionPercentage(Zerg_Hydralisk) > 0.0
                || BuildOrder::getCompositionPercentage(Zerg_Zergling) > 0.0
                || BuildOrder::getCompositionPercentage(Zerg_Ultralisk) > 0.0) && Players::getSupply(PlayerState::Self, Races::Zerg) > 100;

            // Want 3x upgrades by default
            upgradeQueue[Zerg_Carapace] *= 3;
            upgradeQueue[Zerg_Missile_Attacks] *= 3;
            upgradeQueue[Zerg_Melee_Attacks] *= 3;

            // Air unit upgrades
            auto upgradingAirArmor = (Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Attacks)) || Broodwar->self()->isUpgrading(Zerg_Flyer_Carapace);
            auto ZvZAirArmor = (Players::ZvZ() && int(Stations::getStations(PlayerState::Self).size()) >= 2);
            auto ZvTAirArmor = Players::ZvT();
            auto ZvPAirArmor = (Players::ZvP() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0);

            upgradeQueue[Zerg_Flyer_Attacks] = BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && total(Zerg_Mutalisk) >= 12 && upgradingAirArmor;
            upgradeQueue[Zerg_Flyer_Carapace] = BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && total(Zerg_Mutalisk) >= 12 && (ZvZAirArmor || ZvTAirArmor || ZvPAirArmor);

            // Want 3x upgrades by default
            upgradeQueue[Zerg_Flyer_Attacks] *= 3;
            upgradeQueue[Zerg_Flyer_Carapace] *= 3;
        }

        void queueResearch()
        {
            using namespace TechTypes;
            if (inOpening)
                return;

            techQueue[Lurker_Aspect] = !BuildOrder::isFocusUnit(Zerg_Hydralisk) || (Upgrading::haveOrUpgrading(UpgradeTypes::Grooved_Spines, 1) && Upgrading::haveOrUpgrading(UpgradeTypes::Muscular_Augments, 1));
            techQueue[Burrowing] = Stations::getStations(PlayerState::Self).size() >= 3 && Players::getSupply(PlayerState::Self, Races::Zerg) > 140;
            techQueue[Consume] = true;
            techQueue[Plague] = Broodwar->self()->hasResearched(Consume);
        }

        void queueAllin()
        {
            Allin Z_6HatchCrackling;
            Z_6HatchCrackling.workerCount = 30;
            Z_6HatchCrackling.productionCount = 6;
            Z_6HatchCrackling.typeCount = 64;
            Z_6HatchCrackling.type = Zerg_Zergling;
            Z_6HatchCrackling.name = "6HatchCrackling";

            Allin Z_5HatchSpeedling;
            Z_5HatchSpeedling.workerCount = 20;
            Z_5HatchSpeedling.productionCount = 5;
            Z_5HatchSpeedling.typeCount = 48;
            Z_5HatchSpeedling.type = Zerg_Zergling;
            Z_5HatchSpeedling.name = "5HatchSpeedling";

            Allin Z_4HatchSpeedling;
            Z_4HatchSpeedling.workerCount = 18;
            Z_4HatchSpeedling.productionCount = 4;
            Z_4HatchSpeedling.typeCount = 32;
            Z_4HatchSpeedling.type = Zerg_Zergling;
            Z_4HatchSpeedling.name = "4HatchSpeedling";

            // Get allin struct
            if (activeAllinType == AllinType::Z_6HatchCrackling)
                activeAllin = Z_6HatchCrackling;
            if (activeAllinType == AllinType::Z_5HatchSpeedling)
                activeAllin = Z_5HatchSpeedling;
            if (activeAllinType == AllinType::Z_4HatchSpeedling)
                activeAllin = Z_4HatchSpeedling;

            // Ling all-in
            if (Players::ZvZ() && (Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) > 0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0 || Spy::enemyFortress()))
                activeAllin = Z_6HatchCrackling;

            // Turn off all-ins against splash
            auto antiZergling = (Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Archon) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0)
                && total(Zerg_Zergling) > activeAllin.typeCount * 5;
            if (antiZergling)
                return;

            // Common attributes
            if (activeAllin.name != "") {
                armyComposition.clear();
                reserveLarva = 0;
                inOpening = false;
                wantThird = false;
                focusUnit = None;

                // Clear typical buildings we wont need
                buildQueue[Zerg_Lair] = 0;
                buildQueue[Zerg_Hydralisk_Den] = 0;
                buildQueue[Zerg_Spire] = 0;
                buildQueue[Zerg_Extractor] = 0;

                // Pumping
                if (vis(Zerg_Drone) < activeAllin.workerCount)
                    armyComposition[Zerg_Drone] = 1.00;
                else
                    armyComposition[activeAllin.type] = 1.00;

                // Log active all-in
                if (!logFlags[2] && activeAllin.name != "") {
                    McRave::Util::debug("[BuildOrder] Started " + activeAllin.name + " all-in");
                    logFlags[2] = true;
                }
            }

            // 6HatchCrackling
            if (activeAllin.name == "6HatchCrackling") {
                gasLimit = 3;

                // Buildings
                buildQueue[Zerg_Extractor] = 1;
                buildQueue[Zerg_Lair] = 1;
                if (hatchCount() < activeAllin.productionCount && (vis(Zerg_Lair) > 0 || vis(Zerg_Hive) > 0) && vis(Zerg_Larva) <= 2)
                    buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + 1);
                if (hatchCount() >= 5)
                    buildQueue[Zerg_Evolution_Chamber] = 2;

                // Hive tech
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive] = atPercent(Zerg_Queens_Nest, 0.95);

                // Upgrades
                upgradeQueue[UpgradeTypes::Metabolic_Boost] = 1;
                upgradeQueue[UpgradeTypes::Zerg_Carapace] = 3;
                upgradeQueue[UpgradeTypes::Zerg_Melee_Attacks] = 3;
                upgradeQueue[UpgradeTypes::Adrenal_Glands] = com(Zerg_Hive) > 0;
            }

            // 5HatchSpeedling
            if (activeAllin.name == "5HatchSpeedling") {
                gasLimit = (vis(Zerg_Evolution_Chamber) || !lingSpeed()) ? 1 : 0;

                // Buildings
                buildQueue[Zerg_Extractor] = 1;
                if (hatchCount() >= 5)
                    buildQueue[Zerg_Evolution_Chamber] = 2;
                if (hatchCount() < activeAllin.productionCount && vis(Zerg_Larva) <= 2)
                    buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + 1);
                if (Upgrading::haveOrUpgrading(UpgradeTypes::Zerg_Melee_Attacks, 1) && Upgrading::haveOrUpgrading(UpgradeTypes::Zerg_Carapace, 1))
                    buildQueue[Zerg_Lair] = 1;

                // Upgrades
                upgradeQueue[UpgradeTypes::Metabolic_Boost] = 1;
                upgradeQueue[UpgradeTypes::Zerg_Carapace] = 2;
                upgradeQueue[UpgradeTypes::Zerg_Melee_Attacks] = 2;
            }

            // 4HatchSpeedling
            if (activeAllin.name == "4HatchSpeedling") {
                gasLimit = lingSpeed() ? 0 : 1;

                // Buildings
                buildQueue[Zerg_Extractor] = 1;
                if (hatchCount() < activeAllin.productionCount && vis(Zerg_Larva) <= 2)
                    buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + 1);

                // Upgrades
                upgradeQueue[UpgradeTypes::Metabolic_Boost] = 1;
            }
        }
    }

    void opener()
    {
        if (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0 && !Players::ZvFFA() && !Players::ZvTVB())
            againstRandom = true;

        // TODO: Team melee / Team FFA support
        if (Broodwar->getGameType() == GameTypes::Team_Free_For_All || Broodwar->getGameType() == GameTypes::Team_Melee) {
            buildQueue[Zerg_Hatchery] = Players::getSupply(PlayerState::Self, Races::None) >= 30;
            currentBuild = "HatchPool";
            currentOpener = "10Hatch";
            currentTransition = "3HatchMuta";
            ZvFFA();
            return;
        }

        if (Players::vT())
            ZvT();
        else if (Players::vP() || againstRandom)
            ZvP();
        else if (Players::vZ())
            ZvZ();
        else if (Players::ZvFFA())
            ZvFFA();
    }

    void tech()
    {
        const auto vsGoonsGols = Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "5GateGoon" || Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5FactGoliath" || Spy::getEnemyTransition() == "3FactGoliath";
        const auto vsAnnoyingShit = Spy::getEnemyTransition() == "2FactVulture" || Spy::getEnemyTransition() == "2PortWraith";
        auto techOffset = 0;

        // ZvP
        if (Players::ZvP()) {
            techOffset = 2;
        }

        // ZvT
        if (Players::ZvT()) {
            techOffset = 2;
            if (vsAnnoyingShit) {
                unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk };
                techOffset = 1;
            }
            else
                unitOrder ={ Zerg_Mutalisk, Zerg_Ultralisk, Zerg_Defiler };
        }

        // ZvZ
        if (Players::ZvZ()) {
            if (focusUnit == Zerg_Hydralisk)
                unitOrder ={ Zerg_Hydralisk };
            else
                unitOrder ={ Zerg_Mutalisk };
        }

        // ZvFFA
        if (Players::ZvFFA())
            unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker };

        // If we have our tech unit, set to none
        if (techComplete())
            focusUnit = None;

        // Adding tech
        const auto endOfTech = !unitOrder.empty() && isFocusUnit(unitOrder.back());
        const auto techVal = int(focusUnits.size()) + techOffset + mineralThird;
        techSat = !focusUnits.empty() && (techVal >= int(Stations::getStations(PlayerState::Self).size()) || endOfTech);
        auto readyToTech = vis(Zerg_Extractor) >= 2 || int(Stations::getStations(PlayerState::Self).size()) >= 4 || focusUnits.empty();
        if (!inOpening && readyToTech && focusUnit == None && !techSat && productionSat && vis(Zerg_Drone) >= 10)
            getTech = true;

        getNewTech();
        getTechBuildings();
    }

    void situational()
    {
        // Queue up defenses
        needSunks = false;
        needSpores = false;
        buildQueue[Zerg_Creep_Colony] = vis(Zerg_Creep_Colony) + vis(Zerg_Spore_Colony) + vis(Zerg_Sunken_Colony);
        queueWallDefenses();
        queueStationDefenses();

        // Queue up supply, upgrade structures
        queueSupply();
        queueUpgradeStructures();
        queueGasTrick();

        // Optimize our gas mining by dropping gas mining at specific excessive values
        calculateGasLimit();
        removeExcessGas();

        // Outside of opening book, book no longer is in control of queuing production, expansions or gas limits
        queueExpansions();
        queueProduction();
        queueGeysers();

        // Queue upgrades/research
        queueUpgrades();
        queueResearch();

        // Queue an all-in if it will win
        queueAllin();
    }

    void composition()
    {
        // Composition
        armyComposition.clear();
        if (inOpening) {
            if (pumpDefiler)
                armyComposition[Zerg_Defiler] =             1.00;
            else if (pumpScourge)
                armyComposition[Zerg_Scourge] =             1.00;
            else if (pumpMutas)
                armyComposition[Zerg_Mutalisk] =            1.00;
            else if (pumpHydras)
                armyComposition[Zerg_Hydralisk] =           1.00;
            else if (pumpLings)
                armyComposition[Zerg_Zergling] =            1.00;
            else
                armyComposition[Zerg_Drone] =               1.00;
            return;
        }

        const auto vsGoonsGols = Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "5GateGoon" || Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5FactGoliath" || Spy::getEnemyTransition() == "3FactGoliath";

        // ZvT
        if (Players::vT() && !inOpening) {

            // Defiler tech
            if (isFocusUnit(Zerg_Defiler)) {
                armyComposition[Zerg_Drone] =               0.55;
                armyComposition[Zerg_Zergling] =            0.30;
                armyComposition[Zerg_Ultralisk] =           0.10;
                armyComposition[Zerg_Defiler] =             0.05;
            }

            // Ultralisk tech
            else if (isFocusUnit(Zerg_Ultralisk) && com(Zerg_Hive)) {
                armyComposition[Zerg_Drone] =               0.45;
                armyComposition[Zerg_Mutalisk] =            0.00;
                armyComposition[Zerg_Zergling] =            0.45;
                armyComposition[Zerg_Ultralisk] =           0.10;
            }

            // Lurker tech
            else if (isFocusUnit(Zerg_Lurker) && atPercent(TechTypes::Lurker_Aspect, 0.8)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.15;
                armyComposition[Zerg_Hydralisk] =               0.05;
                armyComposition[Zerg_Lurker] =                  1.00;
                armyComposition[Zerg_Mutalisk] =                0.20;
            }

            // Hydralisk tech
            else if (isFocusUnit(Zerg_Hydralisk) && isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.00;
                armyComposition[Zerg_Hydralisk] =               0.30;
                armyComposition[Zerg_Mutalisk] =                0.10;
            }

            // Mutalisk tech
            else if (isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =               0.60;
                armyComposition[Zerg_Zergling] =            0.20;
                armyComposition[Zerg_Mutalisk] =            0.20;
            }
        }

        // ZvP
        if (Players::vP() && !inOpening) {
            if (isFocusUnit(Zerg_Defiler) && Researching::haveOrResearching(TechTypes::Consume)) {
                armyComposition[Zerg_Drone] =                   0.50;
                armyComposition[Zerg_Zergling] =                0.01;
                armyComposition[Zerg_Hydralisk] =               0.40;
                armyComposition[Zerg_Defiler] =                 0.05;
            }
            else if (isFocusUnit(Zerg_Hydralisk) && isFocusUnit(Zerg_Mutalisk) && isFocusUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] =                   0.50;
                armyComposition[Zerg_Zergling] =                0.01;
                armyComposition[Zerg_Hydralisk] =               0.35;
                armyComposition[Zerg_Lurker] =                  0.05;
                armyComposition[Zerg_Mutalisk] =                0.10;
            }
            else if (isFocusUnit(Zerg_Hydralisk) && isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.00;
                armyComposition[Zerg_Hydralisk] =               0.30;
                armyComposition[Zerg_Mutalisk] =                0.10;
            }
            else if (isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.10;
                armyComposition[Zerg_Mutalisk] =                0.30;
            }
            else if (isFocusUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.20;
                armyComposition[Zerg_Hydralisk] =               0.20;
                armyComposition[Zerg_Lurker] =                  1.00;
            }
            else if (isFocusUnit(Zerg_Hydralisk)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.00;
                armyComposition[Zerg_Hydralisk] =               0.40;
            }
        }

        // ZvZ
        if (Players::ZvZ() && !inOpening) {
            if (Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) > 0) {
                if (vis(Zerg_Drone) > 30 && Stations::getStations(PlayerState::Self).size() >= 3)
                    armyComposition[Zerg_Zergling] =                1.00;
                else
                    armyComposition[Zerg_Drone] =                   1.00;
            }
            else if (isFocusUnit(Zerg_Mutalisk) && vis(Zerg_Drone) >= 18) {
                armyComposition[Zerg_Drone] =                   0.30;
                armyComposition[Zerg_Zergling] =                0.40;
                armyComposition[Zerg_Mutalisk] =                0.30;
            }
            else if (isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.75;
                armyComposition[Zerg_Mutalisk] =                0.25;
            }
            else if (isFocusUnit(Zerg_Hydralisk)) {
                armyComposition[Zerg_Hydralisk] =               1.00;
            }
        }

        // ZvFFA
        if ((Players::ZvTVB() || Players::ZvFFA()) && !inOpening) {
            if (isFocusUnit(Zerg_Hydralisk) && isFocusUnit(Zerg_Mutalisk) && isFocusUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] =                   0.60;
                armyComposition[Zerg_Zergling] =                0.00;
                armyComposition[Zerg_Hydralisk] =               0.25;
                armyComposition[Zerg_Lurker] =                  0.05;
                armyComposition[Zerg_Mutalisk] =                0.10;
            }
            else if (isFocusUnit(Zerg_Hydralisk) && isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.70;
                armyComposition[Zerg_Zergling] =                0.00;
                armyComposition[Zerg_Hydralisk] =               0.20;
                armyComposition[Zerg_Mutalisk] =                0.10;
            }
            else if (isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.70;
                armyComposition[Zerg_Zergling] =                0.00;
                armyComposition[Zerg_Mutalisk] =                0.30;
            }
            else {
                armyComposition[Zerg_Drone] =                   0.80;
                armyComposition[Zerg_Zergling] =                0.20;
            }
        }

        // Turn off drones if we hit a cap
        if (armyComposition[Zerg_Zergling] > 0.0) {
            if (pumpLings || vis(Zerg_Drone) >= 60 || Resources::isMineralSaturated()) {
                armyComposition[Zerg_Zergling] += armyComposition[Zerg_Drone];
                armyComposition[Zerg_Drone] = 0.0;
            }
            else {
                armyComposition[Zerg_Drone] += armyComposition[Zerg_Zergling];
                armyComposition[Zerg_Zergling] = 0.0;
            }
        }

        // Cleanup enemy
        if (Util::getTime() > Time(15, 0) && Stations::getStations(PlayerState::Enemy).size() == 0 && Terrain::foundEnemy()) {
            armyComposition[Zerg_Drone] =                   0.40;
            armyComposition[Zerg_Mutalisk] =                0.60;
        }
    }

    void unlocks()
    {
        // Saving larva to burst out tech units
        const auto reserveAt = Players::ZvZ() ? 10 : 16;
        unitReservations.clear();
        if (vis(Zerg_Drone) >= reserveAt && inOpening && reserveLarva > 0) {
            if (atPercent(Zerg_Spire, 0.50) && focusUnit == Zerg_Mutalisk)
                unitReservations[Zerg_Mutalisk] = max(0, reserveLarva - total(Zerg_Mutalisk));
            if (atPercent(Zerg_Spire, 0.50) && focusUnit == Zerg_Scourge)
                unitReservations[Zerg_Scourge] = max(0, reserveLarva - total(Zerg_Scourge) * 2);
            if (vis(Zerg_Hydralisk_Den) > 0 && focusUnit == Zerg_Hydralisk)
                unitReservations[Zerg_Hydralisk] = max(0, reserveLarva - total(Zerg_Hydralisk));

            if (pumpScourge && reserveLarva > 0)
                unitReservations[Zerg_Scourge] = 1;
        }

        // Queue enough overlords to fit the reservations
        if (reserveLarva > 0 && atPercent(Zerg_Spire, 0.25) && total(focusUnit) < reserveLarva && com(Zerg_Spire) == 0) {
            auto enemyAirThreat = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0;
            auto expectedSupply = s + ((reserveLarva - total(focusUnit)) * 4);
            auto expectedOverlords = int(ceil(double(expectedSupply) / 16.0));
            buildQueue[Zerg_Overlord] = expectedOverlords + int(enemyAirThreat) + 1;
        }

        // Unlocking units
        unlockedType.clear();
        for (auto &[type, per] : armyComposition) {
            if (per > 0.0)
                unlockedType.insert(type);
        }

        // Unit limiting in opening book
        if (inOpening) {
            for (auto &[type, limit] : unitLimits) {
                if (limit > vis(type))
                    unlockedType.insert(type);
                else
                    unlockedType.erase(type);
            }
        }

        // Always allowed to make Overlords
        unlockedType.insert(Zerg_Overlord);
    }



    bool lingSpeed() {
        return BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Metabolic_Boost)
            || BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost);
    }

    bool hydraSpeed() {
        return BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Muscular_Augments)
            || BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Muscular_Augments);
    }

    bool hydraRange() {
        return BWAPI::Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Grooved_Spines)
            || BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines);
    }

    int hatchCount() {
        return vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);
    }

    int colonyCount() {
        return vis(Zerg_Creep_Colony) + vis(Zerg_Sunken_Colony);
    }

    bool gas(int amount) {
        return Broodwar->self()->gas() >= amount;
    }

    bool minerals(int amount) {
        return Broodwar->self()->minerals() >= amount;
    }

    int gasMax() {
        return vis(Zerg_Extractor) * 3;
    }

    int capGas(int value) {
        auto onTheWay = 0;
        for (auto &w : Units::getUnits(PlayerState::Self)) {
            auto &worker = *w;
            if (worker.unit()->isCarryingGas() || (worker.hasResource() && worker.getResource().lock()->getType().isRefinery()))
                onTheWay+=8;
        }

        return int(round(double(value - Broodwar->self()->gas() - onTheWay) / 8.0));
    }
}