#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

// Notes:
// Lings: 3 drones per hatch
// Hydra: 5.5 drones per hatch - 0.5 gas per hatch
// Muta: 8 drones per hatch - 1 gas per hatch

namespace McRave::BuildOrder::Zerg {

    namespace {
        bool forceHiveTransition = false;
        bool forceLairTransition = false;
        bool skipOneTech = false;
    }

    void opener()
    {
        if (currentTransition == "Unknown") {
            if (Players::vZ()) {
                currentBuild = "PoolHatch";
                currentOpener = "Overpool";
                currentTransition = "2HatchSpeedling";
            }
            else if (Players::vT() || Players::vP()) {
                currentBuild = "PoolHatch";
                currentOpener = "Overpool";
                currentTransition = "2HatchMuta";
            }
        }

        if (Players::vT()) {
            if (currentBuild == "HatchPool")
                ZvTHatchPool();
            if (currentBuild == "PoolHatch")
                ZvTPoolHatch();
        }
        else if (Players::vP()) {
            if (currentBuild == "PoolHatch")
                ZvPPoolHatch();
        }
        else if (Players::vZ() || Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0) {
            if (currentBuild == "PoolLair")
                ZvZPoolLair();
            if (currentBuild == "PoolHatch")
                ZvZPoolHatch();
        }
    }

    void tech()
    {
        const auto techVal = int(techList.size()) + skipOneTech - int(forceHiveTransition) - int(forceLairTransition);
        const auto endOfTech = !techOrder.empty() && isTechUnit(techOrder.back());
        techSat = (techVal >= int(Stations::getMyStations().size())) || endOfTech;

        // ZvP
        if (Players::ZvP()) {
            techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Hydralisk };
            skipOneTech = true;
        }

        // ZvT
        if (Players::ZvT()) {

            auto vsMech = (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 4)
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) + Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode)) > 0
                || Strategy::getEnemyTransition() == "1FactTanks"
                || Strategy::getEnemyTransition() == "5FactGoliath"
                || Strategy::getEnemyTransition() == "SiegeExpand";

            if (vsMech) {
                techOrder ={ Zerg_Mutalisk, Zerg_Ultralisk };
                skipOneTech = true;
            }
            else {
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Ultralisk };
                skipOneTech = false;
            }
        }

        // ZvZ
        if (Players::ZvZ())
            techOrder ={ Zerg_Mutalisk };

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = None;

        // Adding tech
        auto readyToTech = com(Zerg_Extractor) == int(Stations::getMyStations().size()) || int(Stations::getMyStations().size()) >= 4 || (techList.empty() && vis(Zerg_Drone) >= 12);
        if (!inOpeningBook && readyToTech && techUnit == None && !techSat && productionSat && vis(Zerg_Drone) >= 14)
            getTech = true;

        // Add Scourge if we have Mutas and enemy has air
        if (isTechUnit(Zerg_Mutalisk) && Strategy::enemyAir())
            unlockedType.insert(Zerg_Scourge);

        // Add Hydras if we want Lurkers
        if (isTechUnit(Zerg_Lurker))
            unlockedType.insert(Zerg_Hydralisk);

        getNewTech();
        getTechBuildings();
    }

    void situational()
    {
        auto baseVal = int(Stations::getMyStations().size());

        // Executing gas trick
        if (gasTrick) {
            if (Broodwar->self()->minerals() >= 80 && total(Zerg_Extractor) == 0)
                buildQueue[Zerg_Extractor] = 1;
            if (vis(Zerg_Extractor) > 0)
                buildQueue[Zerg_Extractor] = (vis(Zerg_Drone) < 9);
        }

        // Set our defense count to current visible count
        buildQueue[Zerg_Creep_Colony] = vis(Zerg_Creep_Colony) + vis(Zerg_Spore_Colony) + vis(Zerg_Sunken_Colony);

        // Adding Wall defenses
        auto needSpores = false;
        if (!rush && vis(Zerg_Drone) >= 9) {
            for (auto &[_, wall] : BWEB::Walls::getWalls()) {

                int colonies = 0;
                for (auto& tile : wall.getDefenses()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Walls::needAirDefenses(wall) > colonies || Walls::needGroundDefenses(wall) > colonies) {
                    buildQueue[Zerg_Creep_Colony] += 1;
                    colonies++;
                }

                if (Walls::needAirDefenses(wall) > 0)
                    needSpores = true;
            }
        }

        // Adding Station defenses
        if (!rush && (vis(Zerg_Drone) >= 9 || Players::ZvZ())) {
            for (auto &station : Stations::getMyStations()) {
                auto &s = *station.second;

                int colonies = 0;
                for (auto& tile : s.getDefenseLocations()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Stations::needGroundDefenses(s) > colonies || Stations::needAirDefenses(s) > colonies) {
                    buildQueue[Zerg_Creep_Colony] += 1;
                    colonies++;
                }

                if (Stations::needAirDefenses(s) > 0)
                    needSpores = true;
            }
        }

        defensesNow = buildQueue[Zerg_Creep_Colony] > 0 && vis(Zerg_Sunken_Colony) < 2 && (Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 || Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) > 0 || Strategy::getEnemyBuild() == "2Gate" || Strategy::enemyRush() || Players::ZvZ() || Util::getTime() > Time(6,30));

        // Adding Overlords outside opening book supply
        if (!inBookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(26, Players::getSupply(PlayerState::Self) / providers);
            if (vis(Zerg_Overlord) >= 3)
                buildQueue[Zerg_Overlord] = count;
        }

        // Adding Evolution Chambers
        if ((Players::getSupply(PlayerState::Self) >= 180 && Stations::getMyStations().size() >= 3)
            || needSpores)
            buildQueue[Zerg_Evolution_Chamber] = 2 - Strategy::enemyAir();

        // Outside of opening book
        if (!inOpeningBook) {
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            // Calculate the number of gas workers we need
            auto totalMin = 0;
            auto totalGas = 0;
            for (auto &[type, percent] : armyComposition) {
                auto percentExists = double(vis(type) * type.supplyRequired()) / double(s);

                if (type.whatBuilds().first == Zerg_Larva) {
                    totalMin += int(round(percent * (1.0 - percentExists) * type.mineralPrice()));
                    totalGas += int(round(percent * (1.0 - percentExists)* type.gasPrice()));
                }
                else {
                    auto parentMin = int(percent * type.whatBuilds().first.mineralPrice());
                    auto parentGas = int(percent * type.whatBuilds().first.gasPrice());
                    totalMin += int(percent * type.mineralPrice() * (1.0 - percentExists)) + parentMin;
                    totalGas += int(percent * type.gasPrice() * (1.0 - percentExists)) + parentGas;
                }
            }
            gasLimit = int(ceil(double(vis(Zerg_Drone) * totalGas * Resources::getGasCount() * 3 * 103) / double(totalMin * Resources::getMinCount() * 67)));

            if (Players::ZvT())
                gasLimit += (vis(Zerg_Lair)) + (3 * vis(Zerg_Hive));

            // Adding Hatcheries
            auto hatchPerBase = Players::ZvP() ? 1.5 : 1.0;
            productionSat = vis(Zerg_Hive) + vis(Zerg_Lair) + vis(Zerg_Hatchery) >= hatchPerBase * Stations::getMyStations().size();
            if (shouldExpand() || shouldAddProduction())
                buildQueue[Zerg_Hatchery] = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + 1;

            // Adding Extractors
            if (shouldAddGas())
                buildQueue[Zerg_Extractor] = gasCount;

            // Hive upgrades
            if (Players::getSupply(PlayerState::Self) >= 300) {
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive] = com(Zerg_Queens_Nest) >= 1;
                buildQueue[Zerg_Lair] = com(Zerg_Queens_Nest) < 1;
            }
        }

        // Removing gas workers if we are adding Sunkens or have excess gas
        if (!saveLarva && Stations::getMyStations().size() <= 3 && Broodwar->self()->minerals() < 150 && (defensesNow || Broodwar->self()->gas() > 400 || (Strategy::enemyRush() && Broodwar->self()->gas() > 200)))
            gasLimit = 0;
    }

    void composition()
    {
        // Clear composition before setting
        if (!inOpeningBook)
            armyComposition.clear();

        // ZvT
        if (Players::vT() && !inOpeningBook) {
            auto vsMech = (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 4)
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) + Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode)) > 0
                || Strategy::getEnemyTransition() == "1FactTanks";

            if (Util::getTime() > Time(15, 0) && Stations::getEnemyStations().size() == 0 && Terrain::foundEnemy()) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Mutalisk] = 0.60;
            }
            else if (isTechUnit(Zerg_Defiler) && com(Zerg_Defiler_Mound)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Ultralisk] = 0.15;
                armyComposition[Zerg_Defiler] = 0.05;
            }
            else if (isTechUnit(Zerg_Ultralisk) && com(Zerg_Ultralisk_Cavern)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.25;
                armyComposition[Zerg_Ultralisk] = 0.15;
            }
            else if (isTechUnit(Zerg_Lurker) && vsMech) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.35;
            }
            else if (isTechUnit(Zerg_Lurker) && (Broodwar->self()->isResearching(TechTypes::Lurker_Aspect) || Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect))) {
                armyComposition[Zerg_Drone] = 0.70;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Lurker] = 1.00;
                armyComposition[Zerg_Mutalisk] = 0.00;
            }
            else if (isTechUnit(Zerg_Hydralisk) && com(Zerg_Hydralisk_Den)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.00;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Mutalisk) && total(Zerg_Mutalisk) >= 12 && com(Zerg_Spire) && Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) >= 2) {
                armyComposition[Zerg_Drone] = 0.85;
                armyComposition[Zerg_Zergling] = 0.00;
                armyComposition[Zerg_Mutalisk] = 0.00;
                armyComposition[Zerg_Scourge] = 0.15;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.35;
            }
            else if (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0) {
                armyComposition[Zerg_Drone] = 1.00;
                armyComposition[Zerg_Zergling] = 0.00;
            }
            else {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.60;
            }
        }

        // ZvP
        if (Players::vP() && !inOpeningBook) {
            if (isTechUnit(Zerg_Ultralisk) && com(Zerg_Ultralisk_Cavern)) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.40;
                armyComposition[Zerg_Mutalisk] = 0.10;
                armyComposition[Zerg_Ultralisk] = 0.10;
            }
            else if (isTechUnit(Zerg_Hydralisk) && isTechUnit(Zerg_Mutalisk) && isTechUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.30;
                armyComposition[Zerg_Lurker] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.10;
            }
            else if (isTechUnit(Zerg_Lurker) && isTechUnit(Zerg_Mutalisk) && (Broodwar->self()->isResearching(TechTypes::Lurker_Aspect) || Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect))) {
                armyComposition[Zerg_Drone] = 0.45;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Lurker] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.30;
            }
            else {
                armyComposition[Zerg_Drone] = 0.80;
                armyComposition[Zerg_Zergling] = 0.20;
            }
        }

        // ZvZ
        if (Players::ZvZ() && !inOpeningBook) {
            if (int(Stations::getMyStations().size()) >= 3) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.40;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.00;
                armyComposition[Zerg_Mutalisk] = 0.50;
            }
            else {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.40;
            }
        }

        // Add Scourge if we have Mutas and enemy has air to air
        if (isTechUnit(Zerg_Mutalisk)) {
            auto airCount = Players::getCurrentCount(PlayerState::Enemy, Protoss_Corsair) + Players::getCurrentCount(PlayerState::Enemy, Terran_Wraith) + Players::getCurrentCount(PlayerState::Enemy, Terran_Valkyrie) + Players::getCurrentCount(PlayerState::Enemy, Zerg_Mutalisk);

            if (((vis(Zerg_Scourge) / 2) - 1 < airCount && airCount < 6 && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) || (total(Zerg_Scourge) < 4 && Players::ZvZ() && total(Zerg_Mutalisk) >= 3 && Strategy::getEnemyTransition() == "1HatchMuta")) {
                armyComposition[Zerg_Scourge] = max(0.20, armyComposition[Zerg_Mutalisk]);
                armyComposition[Zerg_Mutalisk] = 0.00;
            }
        }

        // Specific compositions
        if (isTechUnit(Zerg_Hydralisk) && !Terrain::isIslandMap() && (Players::getCurrentCount(PlayerState::Enemy, Protoss_Stargate) >= 3 || Players::getCurrentCount(PlayerState::Enemy, Protoss_Carrier) >= 4 || Players::getCurrentCount(PlayerState::Enemy, Protoss_Fleet_Beacon) >= 1)) {
            armyComposition[Zerg_Drone] = 0.50;
            armyComposition[Zerg_Hydralisk] = 0.50;
        }

        // Check if we are going Lurker, only make when Lurker tech almost ready or not in ZvT
        if (isTechUnit(Zerg_Lurker) && Players::ZvT() && !atPercent(TechTypes::Lurker_Aspect, 0.8)) {
            armyComposition[Zerg_Hydralisk] = 0.00;
            armyComposition[Zerg_Lurker] = 0.00;
        }

        // Check if we are in ZvT and against Vultures, make more Drones instead of lings
        if (Players::ZvT() && Strategy::getEnemyTransition() == "2Fact" && total(Zerg_Ultralisk) < 4) {
            armyComposition[Zerg_Drone] += armyComposition[Zerg_Zergling];
            armyComposition[Zerg_Zergling] = 0.00;
        }
    }

    void unlocks()
    {
        // Always unlock Overlords
        unlockedType.insert(Zerg_Overlord);

        // Saving larva to burst out tech units
        saveLarva = (inOpeningBook || techList.size() == 1) && ((atPercent(Zerg_Spire, 0.50) && com(Zerg_Spire) == 0) || (atPercent(Zerg_Hydralisk_Den, 0.6) && com(Zerg_Hydralisk_Den) == 0));

        // Drone and Ling limiting in opening book
        if (inOpeningBook) {
            if (droneLimit > vis(Zerg_Drone))
                unlockedType.insert(Zerg_Drone);
            else
                unlockedType.erase(Zerg_Drone);
            if (lingLimit > vis(Zerg_Zergling))
                unlockedType.insert(Zerg_Zergling);
            else
                unlockedType.erase(Zerg_Zergling);
        }
        else {
            unlockedType.insert(Zerg_Zergling);
            unlockedType.insert(Zerg_Drone);
        }
    }
}