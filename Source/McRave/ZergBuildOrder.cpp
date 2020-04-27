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

    void opener()
    {
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
        else if (Players::vZ()) {
            if (currentBuild == "PoolLair")
                ZvZPoolLair();
        }
    }

    void tech()
    {
        const auto firstTechUnit = !techList.empty() ? *techList.begin() : None;
        const auto skipOneTech = true;
        const auto techVal = int(techList.size()) + skipOneTech;
        techSat = (techVal >= int(Stations::getMyStations().size()));

        // ZvP
        if (Players::ZvP()) {
            if (firstTechUnit == Zerg_Mutalisk)
                techOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
            else if (firstTechUnit == Zerg_Hydralisk)
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
            else
                techOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
        }

        // ZvT
        if (Players::ZvT()) {

            auto vsMech = (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 4)
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) + Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode)) > 0
                || Strategy::getEnemyTransition() == "1FactTanks";

            if (vsMech)
                techOrder ={ Zerg_Mutalisk, Zerg_Ultralisk, Zerg_Defiler };
            else
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
        }

        // ZvZ
        if (Players::ZvZ())
            techOrder ={ Zerg_Mutalisk };

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = None;

        // Adding tech
        if (!inOpeningBook && techUnit == None && !techSat && (productionSat || (techList.size() == 0 && Resources::isMinSaturated())))
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
        auto techVal = int(techList.size()) + 2;
        auto saveForDefenses = false;

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
        if (atPercent(Zerg_Spawning_Pool, 0.5) && vis(Zerg_Drone) >= 9) {
            for (auto &[_, wall] : BWEB::Walls::getWalls()) {

                int colonies = 0;
                for (auto& tile : wall.getDefenses()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Walls::needGroundDefenses(wall) > 0 || Walls::needAirDefenses(wall) > 0)
                    saveForDefenses = true;
                if (Walls::needAirDefenses(wall) > colonies || Walls::needGroundDefenses(wall) > colonies) {
                    buildQueue[Zerg_Creep_Colony] += 1;
                    colonies++;
                }
            }
        }

        // Adding Station defenses
        if (atPercent(Zerg_Spawning_Pool, 0.5) && (vis(Zerg_Drone) >= 9 || Players::ZvZ())) {
            for (auto &station : Stations::getMyStations()) {
                auto &s = *station.second;

                int colonies = 0;
                for (auto& tile : s.getDefenseLocations()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Stations::needGroundDefenses(s) > 0 || Stations::needAirDefenses(s) > 0)
                    saveForDefenses = true;
                if (Stations::needGroundDefenses(s) > colonies || Stations::needAirDefenses(s) > colonies) {
                    buildQueue[Zerg_Creep_Colony] += 1;
                    colonies++;
                }
            }
        }

        // Adding Overlords outside opening book supply
        if (!inBookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(22, Players::getSupply(PlayerState::Self) / providers);
            if (vis(Zerg_Overlord) >= 3)
                buildQueue[Zerg_Overlord] = count;
        }

        // Adding Evolution Chambers
        if ((Players::getSupply(PlayerState::Self) >= 180 && Stations::getMyStations().size() >= 3)
            || (Strategy::enemyAir() && vis(Zerg_Spire) == 0 && firstUnit != Zerg_Mutalisk))
            buildQueue[Zerg_Evolution_Chamber] = 2 - Strategy::enemyAir();

        // Outside of opening book
        if (!inOpeningBook) {
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            // Calculate the number of gas workers we need
            auto totalMin = 0;
            auto totalGas = 0;
            for (auto &[type, percent] : armyComposition) {
                if (type.whatBuilds().first == Zerg_Larva) {
                    totalMin += int(percent * type.mineralPrice());
                    totalGas += int(percent * type.gasPrice());
                }
                else {
                    totalMin += int(percent * type.mineralPrice()) + (int(percent * type.whatBuilds().first.mineralPrice()));
                    totalGas += int(percent * type.gasPrice()) + (int(percent * type.whatBuilds().first.gasPrice()));
                }
            }
            gasLimit = int(ceil(double(vis(Zerg_Drone) * totalGas * Resources::getGasCount() * 3 * 103) / double(totalMin * Resources::getMinCount() * 2 * 67))) + techList.size();

            // Adding Hatcheries
            auto hatchPerBase = Players::ZvZ() ? 1.5 : 2.0;
            productionSat = (vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive)) >= int(hatchPerBase * Stations::getMyStations().size());
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
        if (!saveLarva && Broodwar->self()->minerals() < 200 && (saveForDefenses || (com(Zerg_Sunken_Colony) < vis(Zerg_Sunken_Colony)) || (Broodwar->self()->gas() > 400)))
            gasLimit = 0;
    }

    void composition()
    {
        // Opening books have compositions defined
        if (inOpeningBook)
            return;

        // Clear composition before setting
        armyComposition.clear();

        // ZvT
        if (Players::vT()) {
            if (isTechUnit(Zerg_Defiler)) {
                armyComposition[Zerg_Drone] = 0.45;
                armyComposition[Zerg_Zergling] = 0.30;
                armyComposition[Zerg_Ultralisk] = 0.15;
                armyComposition[Zerg_Scourge] = 0.05;
                armyComposition[Zerg_Defiler] = 0.05;
            }
            else if (isTechUnit(Zerg_Ultralisk)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.30;
                armyComposition[Zerg_Ultralisk] = 0.15;
                armyComposition[Zerg_Scourge] = 0.05;
            }
            else if (isTechUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Hydralisk] = 0.10;
                armyComposition[Zerg_Lurker] = 1.00;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Hydralisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.00;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.35;
            }
            else {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.60;
            }
        }

        // ZvP
        if (Players::vP()) {
            if (isTechUnit(Zerg_Ultralisk)) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Lurker] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.10;
                armyComposition[Zerg_Ultralisk] = 0.10;
                armyComposition[Zerg_Defiler] = 0.10;
            }
            else if (isTechUnit(Zerg_Defiler)) {
                armyComposition[Zerg_Drone] = 0.45;
                armyComposition[Zerg_Zergling] = 0.20;
                armyComposition[Zerg_Lurker] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.10;
                armyComposition[Zerg_Defiler] = 0.10;
            }
            else if (isTechUnit(Zerg_Lurker) && isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.45;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Lurker] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.15;
            }
            else if (isTechUnit(Zerg_Lurker) && !isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Lurker] = 0.25;
            }
            else if (isTechUnit(Zerg_Mutalisk) && isTechUnit(Zerg_Hydralisk)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.30;
            }
            else if (isTechUnit(Zerg_Hydralisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Hydralisk] = 0.40;
            }
            else {
                armyComposition[Zerg_Drone] = 0.80;
                armyComposition[Zerg_Zergling] = 0.20;
            }
        }

        // ZvZ
        if (Players::ZvZ()) {
            if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.10;
                armyComposition[Zerg_Zergling] = 0.35;
                armyComposition[Zerg_Mutalisk] = 0.65;
            }
            else {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.40;
            }
        }

        // Add Scourge if we have Mutas and enemy has air to air
        if (isTechUnit(Zerg_Mutalisk) && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
            if (Players::ZvZ()) {
                if (vis(Zerg_Mutalisk) > 3 && vis(Zerg_Scourge) < vis(Zerg_Mutalisk) * 2) {
                    armyComposition[Zerg_Scourge] = armyComposition[Zerg_Mutalisk];
                    armyComposition[Zerg_Mutalisk] = 0.00;
                }
                else
                    armyComposition[Zerg_Scourge] = 0.00;
            }
            else {
                if (vis(Zerg_Scourge) == 0) {
                    armyComposition[Zerg_Scourge] = armyComposition[Zerg_Mutalisk];
                    armyComposition[Zerg_Mutalisk] = 0.00;
                }
                else {
                    armyComposition[Zerg_Scourge] += 0.05;
                    armyComposition[Zerg_Mutalisk] -= 0.05;
                }
            }
        }

        // Check if we are in ZvT and going Lurker, only make when Lurker tech almost ready
        if (isTechUnit(Zerg_Lurker) && (!Players::ZvT() || !atPercent(TechTypes::Lurker_Aspect, 0.8))) {
            armyComposition[Zerg_Hydralisk] = 0.00;
            armyComposition[Zerg_Lurker] = 0.00;
        }
    }

    void unlocks()
    {
        // Saving larva to burst out tech units
        saveLarva = inOpeningBook && ((atPercent(Zerg_Spire, 0.5) && com(Zerg_Spire) == 0) || (atPercent(Zerg_Hydralisk_Den, 0.6) && com(Zerg_Hydralisk_Den) == 0));

        // Drone and Ling limiting in opening book
        if (inOpeningBook) {
            droneLimit > vis(Zerg_Drone) ? void(unlockedType.insert(Zerg_Drone)) : void(unlockedType.erase(Zerg_Drone));
            lingLimit > vis(Zerg_Zergling) ? void(unlockedType.insert(Zerg_Zergling)) : void(unlockedType.erase(Zerg_Zergling));
        }
        else {
            unlockedType.insert(Zerg_Zergling);
            unlockedType.insert(Zerg_Drone);
        }
    }
}