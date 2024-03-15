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
        map<UnitType, int> forcedCount;

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
            auto needLarvaSpending = vis(Zerg_Larva) > 3 && Broodwar->self()->supplyUsed() < Broodwar->self()->supplyTotal() && unitLimits[Zerg_Larva] == 0 && Util::getTime() < Time(4, 30) && com(Zerg_Sunken_Colony) >= 2;
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
        }

        void queueSupply()
        {
            // Adding Overlords outside opening book supply
            if (!inBookSupply) {
                int count = 1 + min(26, s / 14);
                if (vis(Zerg_Overlord) >= 3)
                    buildQueue[Zerg_Overlord] = count;
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
                auto minDronesPerGas = 10 + (4 * vis(Zerg_Extractor));
                gasDesired = gasLimit > vis(Zerg_Extractor) * 3 && com(Zerg_Drone) >= minDronesPerGas
                    && (Resources::isHalfMineralSaturated() || productionSat || com(Zerg_Drone) >= (Players::ZvT() ? 40 : 50));

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
            if (Spy::enemyInvis() && Util::getTime() > Time(4, 45)) {
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
            if (!inOpening && unitLimits[Zerg_Larva] == 0) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto availableGas = Broodwar->self()->gas() - BuildOrder::getGasQueued();
                const auto resourceSat = Resources::isGasSaturated() && (int(Stations::getStations(PlayerState::Self).size()) >= 4 ? Resources::isMineralSaturated() : Resources::isHalfMineralSaturated());
                const auto droneMin = Players::ZvZ() ? 10 : 16;

                expandDesired = (focusUnit == None && resourceSat && techSat && productionSat)
                    || (Players::ZvZ() && Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0 && Stations::getStations(PlayerState::Self).size() < Stations::getStations(PlayerState::Enemy).size() && availableMinerals > 300 && availableGas < 150)
                    || (focusUnit == None && vis(Zerg_Drone) >= droneMin && int(Stations::getStations(PlayerState::Self).size()) <= 1);

                buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + expandDesired);

                if (expandDesired && int(Stations::getStations(PlayerState::Self).size()) <= 1)
                    wantNatural = true;
                if (expandDesired && int(Stations::getStations(PlayerState::Self).size()) <= 2)
                    wantThird = true;
            }
        }

        void queueProduction()
        {
            //// Adding Hatcheries
            //auto desiredProduction = int(Stations::getStations(PlayerState::Self).size());
            //if (Players::ZvT())
            //    desiredProduction = int(Stations::getStations(PlayerState::Self).size()) + max(0, int(Stations::getStations(PlayerState::Self).size()) - 3) + max(0, int(Stations::getStations(PlayerState::Self).size()) - 4);
            //if (Players::ZvP())
            //    desiredProduction = int(Stations::getStations(PlayerState::Self).size()) + (2 * max(0, int(Stations::getStations(PlayerState::Self).size()) - 2));
            //if (Players::ZvZ())
            //    desiredProduction = int(Stations::getStations(PlayerState::Self).size()) + max(0, int(Stations::getStations(PlayerState::Self).size()) - 1) - (int(Stations::getStations(PlayerState::Enemy).size() >= 2));
            //if (Players::ZvFFA())
            //    desiredProduction = int(Stations::getStations(PlayerState::Self).size()) + max(0, int(Stations::getStations(PlayerState::Self).size()) - 1);

            //// ZvP trapped in base
            //if (Spy::getEnemyTransition() == "4Gate" && int(Stations::getStations(PlayerState::Self).size()) <= 2)
            //    desiredProduction = 6;

            //// ZvT going hydras
            //if (Players::ZvT() && vis(Zerg_Drone) >= 22 && total(Zerg_Hydralisk) == 0 && find(unitOrder.begin(), unitOrder.end(), Zerg_Hydralisk) != unitOrder.end() && int(Stations::getStations(PlayerState::Self).size()) <= 3) {
            //    desiredProduction = 5;
            //    forcedCount[Zerg_Hatchery] = 5;
            //    forcedCount[Zerg_Drone] = 28;
            //}

            // Idk
            auto hatchPerbase = 1.0;
            if (armyComposition[Zerg_Zergling] > 0.0)
                hatchPerbase = 3.0;
            else if (armyComposition[Zerg_Hydralisk] > 0.0)
                hatchPerbase = 2.0;
            else if (armyComposition[Zerg_Mutalisk] > 0.0)
                hatchPerbase = 1.5;
            auto desiredProduction = int(round(hatchPerbase * double(Stations::getStations(PlayerState::Self).size())));

            productionSat = hatchCount() >= min(7, desiredProduction);
            auto maxSat = productionSat * 2;

            if (!inOpening && unitLimits[Zerg_Larva] == 0) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto incompleteHatch = vis(Zerg_Hatchery) - com(Zerg_Hatchery);
                const auto waitForMinerals = 300 * (1 + incompleteHatch);

                rampDesired = (availableMinerals >= waitForMinerals && Resources::isHalfMineralSaturated() && Resources::isGasSaturated() && !productionSat)
                    || (availableMinerals >= waitForMinerals && vis(Zerg_Larva) + (3 * incompleteHatch) < min(3, hatchCount()) && !productionSat)
                    || (availableMinerals >= 600 && hatchCount() < maxSat && productionSat && vis(Zerg_Larva) < 12) // ???
                    || (vis(Zerg_Drone) >= forcedCount[Zerg_Drone] && hatchCount() < forcedCount[Zerg_Hatchery]);

                buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + rampDesired);
            }
        }

        void calculateGasLimit()
        {
            // Calculate the number of gas workers we need outside of the opening book
            if (!inOpening) {
                auto totalMin = 1;
                auto totalGas = 0;
                for (auto &[type, percent] : armyComposition) {
                    if (unitLimits[type] > 0)
                        continue;
                    auto percentExists = max(0.01, double(vis(type) * type.supplyRequired()) / double(s));
                    totalMin += max(0, int(round(percent * double(type.mineralPrice()) / percentExists)));
                    totalGas += max(0, int(round(percent * double(type.gasPrice()) / percentExists)));
                }

                auto resourceRate = (3.0 * 103.0 / (67.1));
                gasLimit = int(double(vis(Zerg_Drone) * totalGas * resourceRate) / (double(totalMin)));
                if (Players::ZvZ())
                    gasLimit += int(Stations::getStations(PlayerState::Self).size());
                if (!Players::ZvZ() && vis(Zerg_Lair))
                    gasLimit = max(gasLimit, 6);
                gasLimit += vis(Zerg_Evolution_Chamber) + vis(Zerg_Lair);
            }

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
                auto dropGasRush        = (Spy::enemyRush() && Broodwar->self()->gas() > 200);
                auto dropGasExcess      = gasRemaining > 15 * vis(Zerg_Drone) && !inOpening;
                auto dropGasDefenses    = needSunks && Util::getTime() < Time(3, 30) && (Players::ZvZ() || Spy::enemyProxy() || Spy::getEnemyOpener() == "9/9" || Spy::getEnemyOpener() == "8Rax");
                auto dropGasBroke       = minRemaining < 75 && gasRemaining >= 100 && (Util::getTime() < Time(4, 30) || Players::ZvZ());
                auto dropGasDrones      = minRemaining < 75 && gasRemaining >= 100 && !Players::ZvZ() && vis(Zerg_Lair) > 0 && vis(Zerg_Drone) < 18;
                auto dropGasLarva       = minRemaining < 100 && gasRemaining >= 100 && !Players::ZvZ() && vis(Zerg_Larva) >= 3 && Util::getTime() < Time(5, 00) && unitReservations.empty();

                if ((dropGasBroke)
                    || dropGasLarva
                    || Roles::getMyRoleCount(Role::Worker) < 5
                    || (unitLimits[Zerg_Larva] < 3 && !rush && !pressure && minRemaining < 100 && (dropGasRush || dropGasExcess || dropGasDefenses || dropGasDrones)))
                    gasLimit = 0;
            }
        }

        void queueUpgrades()
        {
            using namespace UpgradeTypes;

            // Overlord speed can be done inside openings
            const auto queueOvieSpeed = (Players::ZvT() && Spy::getEnemyTransition() == "2PortWraith" && Util::getTime() > Time(6, 00))
                || (Players::ZvP() && Players::getStrength(PlayerState::Enemy).airToAir > 0 && Players::getSupply(PlayerState::Self, Races::Zerg) >= 160)
                || (Spy::enemyInvis() && (BuildOrder::isFocusUnit(Zerg_Hydralisk) || BuildOrder::isFocusUnit(Zerg_Ultralisk)))
                || (Players::getSupply(PlayerState::Self, Races::Zerg) >= 200);

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

            // Air unit upgrades
            auto upgradingAirArmor = (Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Attacks)) || Broodwar->self()->isUpgrading(Zerg_Flyer_Carapace);
            auto ZvZAirArmor = (Players::ZvZ() && int(Stations::getStations(PlayerState::Self).size()) >= 2);
            auto ZvTAirArmor = Players::ZvT();
            auto ZvPAirArmor = (Players::ZvP() && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) >= 2);

            if (BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && total(Zerg_Mutalisk) >= 12 && Stations::getStations(PlayerState::Self).size() >= 3) {
                upgradeQueue[Zerg_Flyer_Attacks] = upgradingAirArmor;
                upgradeQueue[Zerg_Flyer_Carapace] = ZvZAirArmor || ZvTAirArmor || ZvPAirArmor;
            }
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
        const auto endOfTech = !unitOrder.empty() && isFocusUnit(unitOrder.back());
        const auto vsAnnoyingShit = Spy::getEnemyTransition() == "2FactVulture" || Spy::getEnemyTransition() == "2PortWraith";
        auto techOffset = 0;

        // ZvP
        if (Players::ZvP()) {
            techOffset = 1;
            if (Spy::getEnemyTransition() == "Carriers")
                unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk };
            else if (focusUnit == Zerg_Hydralisk)
                unitOrder ={ Zerg_Hydralisk };
            else if (vsGoonsGols)
                unitOrder ={ Zerg_Mutalisk };
            else
                unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker };
        }

        // ZvT
        if (Players::ZvT()) {
            techOffset = 2;
            if (vsGoonsGols || vsAnnoyingShit) {
                unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk };
                techOffset = 1;
            }
            else
                unitOrder ={ Zerg_Mutalisk, Zerg_Ultralisk, Zerg_Defiler };
        }

        // ZvZ
        if (Players::ZvZ())
            unitOrder ={ Zerg_Mutalisk };

        // ZvFFA
        if (Players::ZvFFA())
            unitOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker };

        // If we have our tech unit, set to none
        if (techComplete())
            focusUnit = None;

        // Adding tech
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
    }

    void composition()
    {
        // Clear composition before setting
        if (!inOpening)
            armyComposition.clear();

        const auto vsGoonsGols = Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "5GateGoon" || Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5FactGoliath";

        // ZvT
        if (Players::vT() && !inOpening) {

            // Cleanup enemy
            if (Util::getTime() > Time(15, 0) && Stations::getStations(PlayerState::Enemy).size() == 0 && Terrain::foundEnemy()) {
                armyComposition[Zerg_Drone] =                   0.40;
                armyComposition[Zerg_Mutalisk] =                0.60;
            }

            // Defiler tech
            else if (isFocusUnit(Zerg_Defiler)) {

                if (s > 360 || (Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) + Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode)) > 12) {
                    armyComposition[Zerg_Drone] =               0.50;
                    armyComposition[Zerg_Zergling] =            0.35;
                    armyComposition[Zerg_Mutalisk] =            0.00;
                    armyComposition[Zerg_Ultralisk] =           0.10;
                    armyComposition[Zerg_Defiler] =             0.05;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.55;
                    armyComposition[Zerg_Zergling] =            0.30;
                    armyComposition[Zerg_Ultralisk] =           0.10;
                    armyComposition[Zerg_Defiler] =             0.05;
                }
            }

            // Ultralisk tech
            else if (isFocusUnit(Zerg_Ultralisk) && com(Zerg_Hive)) {
                if (s > 360 || (Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) + Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode)) > 6) {
                    armyComposition[Zerg_Drone] =               0.40;
                    armyComposition[Zerg_Mutalisk] =            0.20;
                    armyComposition[Zerg_Zergling] =            0.40;
                    armyComposition[Zerg_Ultralisk] =           0.00;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.45;
                    armyComposition[Zerg_Mutalisk] =            0.00;
                    armyComposition[Zerg_Zergling] =            0.45;
                    armyComposition[Zerg_Ultralisk] =           0.10;
                }
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
                if (total(Zerg_Mutalisk) >= 32 && vis(Zerg_Hive) > 0) {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            0.20;
                    armyComposition[Zerg_Mutalisk] =            0.20;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            0.00;
                    armyComposition[Zerg_Mutalisk] =            0.40;
                }
            }

            // No tech
            else {
                if (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0) {
                    armyComposition[Zerg_Drone] =               1.00;
                    armyComposition[Zerg_Zergling] =            0.00;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.40;
                    armyComposition[Zerg_Zergling] =            0.60;
                }
            }
        }

        // ZvP
        if (Players::vP() && !inOpening) {
            if (isFocusUnit(Zerg_Defiler)) {
                armyComposition[Zerg_Drone] =                   0.58;
                armyComposition[Zerg_Zergling] =                0.02;
                armyComposition[Zerg_Hydralisk] =               0.25;
                armyComposition[Zerg_Mutalisk] =                0.10;
                armyComposition[Zerg_Defiler] =                 0.05;
            }
            else if (isFocusUnit(Zerg_Hydralisk) && isFocusUnit(Zerg_Mutalisk) && isFocusUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] =                   0.50;
                armyComposition[Zerg_Zergling] =                0.00;
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
                if (total(Zerg_Mutalisk) >= 32 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Adrenal_Glands) > 0) {
                    armyComposition[Zerg_Drone] =               0.40;
                    armyComposition[Zerg_Zergling] =            0.30;
                    armyComposition[Zerg_Mutalisk] =            0.30;
                }
                else {
                    armyComposition[Zerg_Drone] =               0.60;
                    armyComposition[Zerg_Zergling] =            0.00;
                    armyComposition[Zerg_Mutalisk] =            0.40;
                }
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
            else {
                armyComposition[Zerg_Drone] =                   0.80;
                armyComposition[Zerg_Zergling] =                0.20;
            }
        }

        // ZvZ
        if (Players::ZvZ() && !inOpening) {
            Broodwar << Stations::getMiningStationsCount() << endl;
            if (isFocusUnit(Zerg_Mutalisk) && hatchCount() > Stations::getMiningStationsCount()) {
                armyComposition[Zerg_Drone] =                   0.45;
                armyComposition[Zerg_Zergling] =                0.35;
                armyComposition[Zerg_Mutalisk] =                0.20;
            }
            else if (isFocusUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] =                   0.50;
                armyComposition[Zerg_Mutalisk] =                0.50;
            }
            else {
                armyComposition[Zerg_Drone] =                   0.50;
                armyComposition[Zerg_Zergling] =                0.50;
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

        // When switching techs, wait for ideal production before starting
        if (!inOpening) {
            auto switching = false;
            for (auto &[type, cnt] : armyComposition) {
                if (total(type) <= 0)
                    switching = true;
            }
            if (switching && (hatchCount() < forcedCount[Zerg_Hatchery] || vis(Zerg_Drone) < forcedCount[Zerg_Drone])) {
                armyComposition.clear();
                armyComposition[Zerg_Drone] =               0.60;
                armyComposition[Zerg_Zergling] =            0.00;
                armyComposition[Zerg_Mutalisk] =            0.40;
            }
        }

        // Make lings if we're fully saturated, need to pump lings or we are behind on sunkens
        if (!inOpening) {
            int pumpLings = 0;
            if (Players::ZvP() && Util::getTime() > Time(7, 30) && focusUnits.find(Zerg_Hydralisk) == focusUnits.end() && hatchCount() >= 6 && int(Stations::getStations(PlayerState::Self).size()) >= 3)
                pumpLings = 12;
            if (Spy::getEnemyTransition() == "Robo")
                pumpLings = 12;
            if (Spy::getEnemyBuild() == "FFE") {
                if (Spy::getEnemyTransition() == "Speedlot" && Util::getTime() > Time(6, 45) && Util::getTime() < Time(7, 45))
                    pumpLings = 24;
            }
            if (Players::ZvZ() && Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) == 0 && vis(Zerg_Drone) > Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone))
                pumpLings = Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling);

            if (vis(Zerg_Zergling) < pumpLings) {
                armyComposition[Zerg_Zergling] = armyComposition[Zerg_Drone];
                armyComposition[Zerg_Drone] = 0.0;
                unitLimits[Zerg_Zergling] = pumpLings;
            }
        }

        // Specific compositions
        if (isFocusUnit(Zerg_Hydralisk) && !Terrain::isIslandMap() && (Players::getVisibleCount(PlayerState::Enemy, Protoss_Stargate) >= 3 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Carrier) >= 4 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Fleet_Beacon) >= 1 || Spy::getEnemyTransition() == "DoubleStargate" || Spy::getEnemyTransition() == "Carriers")) {
            armyComposition.clear();
            armyComposition[Zerg_Drone] = 0.50;
            armyComposition[Zerg_Hydralisk] = 0.50;
        }

        // Add Scourge if we have Mutas and enemy has air to air
        if (com(Zerg_Spire) > 0) {
            auto airCount = Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) + Players::getVisibleCount(PlayerState::Enemy, Terran_Wraith);
            auto needScourgeZvP = Players::ZvP() && airCount > 0 && vis(Zerg_Mutalisk) < 3 && vis(Zerg_Scourge) < min(6, airCount * 2);
            auto needScourgeZvZ = Players::ZvZ() && (airCount / 2) > vis(Zerg_Scourge) && (com(Zerg_Extractor) >= 3 || currentTransition == "2HatchMuta") && vis(Zerg_Scourge) < 2;
            auto needScourgeZvT = Players::ZvT() && Players::getVisibleCount(PlayerState::Enemy, Terran_Valkyrie) >= 2 && vis(Zerg_Scourge) < 4;

            if (needScourgeZvP || needScourgeZvZ || needScourgeZvT) {
                armyComposition.clear();
                armyComposition[Zerg_Scourge] = 1.00;
            }
        }
    }

    void unlocks()
    {
        // Saving larva to burst out tech units
        const auto limitBy = int(Stations::getStations(PlayerState::Self).size()) * 3;
        const auto reserveAt = Players::ZvZ() ? 10 : 16;
        unitReservations.clear();
        if ((inOpening && reserveLarva) || (!inOpening && focusUnits.size() <= 1)) {
            if (atPercent(Zerg_Spire, 0.50) && vis(Zerg_Drone) >= reserveAt && focusUnit == Zerg_Mutalisk && (armyComposition[Zerg_Mutalisk] > 0.0 || armyComposition[Zerg_Scourge] > 0.0)) {
                unitReservations[Zerg_Mutalisk] = max(0, limitBy - total(Zerg_Mutalisk) - int(armyComposition[Zerg_Scourge] > 0.0));
                unitReservations[Zerg_Scourge] = max(0, 2 * int(armyComposition[Zerg_Scourge] > 0.0) - total(Zerg_Scourge));
            }
            if (vis(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Drone) >= reserveAt && focusUnit == Zerg_Hydralisk && armyComposition[Zerg_Hydralisk] > 0.0)
                unitReservations[Zerg_Hydralisk] = max(0, limitBy - total(Zerg_Hydralisk));
        }

        // Unlocking units
        unlockedType.clear();
        unlockedType.insert(Zerg_Overlord);
        for (auto &[type, per] : armyComposition) {
            if (per > 0.0)
                unlockedType.insert(type);
        }

        // Unit limiting in opening book
        if (inOpening) {
            for (auto &type : unitLimits) {
                if (type.second > vis(type.first))
                    unlockedType.insert(type.first);
                else
                    unlockedType.erase(type.first);
            }
        }

        // Remove Lings if we're against Vults, adding at Hive
        if (Players::ZvT()) {
            if (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 && total(Zerg_Hive) == 0 && unitLimits[Zerg_Zergling] <= vis(Zerg_Zergling))
                unlockedType.erase(Zerg_Zergling);
            if (total(Zerg_Hive) > 0)
                unlockedType.insert(Zerg_Zergling);
        }

        // Removing mutas temporarily if they overdefended
        const auto vsGoonsGols = Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "5GateGoon" || Spy::getEnemyTransition() == "CorsairGoon" || Spy::getEnemyTransition() == "5FactGoliath";
        if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Photon_Cannon) >= 6 && Util::getTime() < Time(9, 00) && total(Zerg_Mutalisk) >= 9 && !vsGoonsGols) {
            unlockedType.erase(Zerg_Mutalisk);
        }

        // UMS Unlocking
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings) {
            for (auto &type : BWAPI::UnitTypes::allUnitTypes()) {
                if (!type.isBuilding() && type.getRace() == Races::Zerg && vis(type) >= 2) {
                    unlockedType.insert(type);
                    if (!type.isWorker())
                        focusUnits.insert(type);
                }
            }
        }
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

    int gasMax() {
        return Resources::getGasCount() * 3;
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