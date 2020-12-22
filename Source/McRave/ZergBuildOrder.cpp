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
        bool againstRandom = false;
    }

    void opener()
    {
        if (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0)
            againstRandom = true;

        if (againstRandom) {
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
        else if (Players::vP() || Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0) {
            if (currentBuild == "PoolHatch")
                ZvPPoolHatch();
        }
        else if (Players::vZ()) {
            if (currentBuild == "PoolLair")
                ZvZPoolLair();
            if (currentBuild == "PoolHatch")
                ZvZPoolHatch();
        }
    }

    void tech()
    {
        const auto techVal = int(techList.size()) + (vis(Zerg_Hive) > 0) + (2 * Players::ZvT()) + Players::ZvP();
        const auto endOfTech = !techOrder.empty() && isTechUnit(techOrder.back());
        techSat = (techVal >= int(Stations::getMyStations().size())) || endOfTech;

        // ZvP
        if (Players::ZvP()) {
            auto vsGoons = Strategy::getEnemyTransition() == "4Gate"
                || Strategy::getEnemyTransition() == "5GateGoon";

            if (Strategy::getEnemyTransition() == "Carriers")
                techOrder ={ Zerg_Mutalisk, Zerg_Hydralisk };
            else if (vsGoons)
                techOrder ={ Zerg_Mutalisk, Zerg_Defiler };
            else
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Hydralisk, Zerg_Defiler };
        }

        // ZvT
        if (Players::ZvT()) {
            auto vsMech = Strategy::getEnemyTransition() == "2Fact"
                || Strategy::getEnemyTransition() == "1FactTanks"
                || Strategy::getEnemyTransition() == "5FactGoliath";

            if (isTechUnit(Zerg_Lurker) && vsMech) {
                techList.erase(Zerg_Lurker);
                unlockedType.erase(Zerg_Lurker);
                unlockedType.erase(Zerg_Hydralisk);
            }

            if (vsMech)
                techOrder ={ Zerg_Mutalisk, Zerg_Hydralisk/*, Zerg_Defiler, Zerg_Ultralisk*/ };
            else
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Ultralisk, Zerg_Defiler };
        }

        // ZvZ
        if (Players::ZvZ())
            techOrder ={ Zerg_Mutalisk };

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = None;

        // Adding tech
        auto readyToTech = vis(Zerg_Extractor) >= int(Stations::getMyStations().size()) || int(Stations::getMyStations().size()) >= 4 || techList.empty();
        if (!inOpeningBook && readyToTech && techUnit == None && !techSat && productionSat && vis(Zerg_Drone) >= 10)
            getTech = true;

        // Add Scourge if we have Mutas
        if (isTechUnit(Zerg_Mutalisk))
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
        auto needSunks = false;
        if (!rush && !pressure && (vis(Zerg_Drone) >= 9 || Players::ZvZ())) {
            for (auto &[_, wall] : BWEB::Walls::getWalls()) {

                int colonies = 0;
                for (auto& tile : wall.getDefenses()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Walls::needAirDefenses(wall) > colonies || Walls::needGroundDefenses(wall) > colonies) {
                    auto difference = Walls::needGroundDefenses(wall) + Walls::needAirDefenses(wall) - colonies;
                    buildQueue[Zerg_Creep_Colony] += difference;
                    colonies += difference;
                }

                Broodwar->drawTextMap(wall.getCentroid(), "%d, %d", Walls::needAirDefenses(wall), Walls::needGroundDefenses(wall));

                if (Walls::needAirDefenses(wall) > 0)
                    needSpores = true;
                if (Walls::needGroundDefenses(wall) > 0)
                    needSunks = true;
            }
        }

        // Adding Station defenses
        if (!rush && !pressure && (vis(Zerg_Drone) >= 9 || Players::ZvZ())) {
            for (auto &[_, s] : Stations::getMyStations()) {
                auto &station = *s;

                int colonies = 0;
                for (auto& tile : station.getDefenseLocations()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Stations::needGroundDefenses(station) > colonies || Stations::needAirDefenses(station) > colonies) {
                    auto difference = Stations::needGroundDefenses(station) + Stations::needAirDefenses(station) - colonies;

                    // Queued only if we have the required structure
                    if ((vis(Zerg_Spawning_Pool) > 0 && Stations::needGroundDefenses(station) > colonies) || (vis(Zerg_Evolution_Chamber) > 0 && Stations::needAirDefenses(station) > colonies))
                        buildQueue[Zerg_Creep_Colony] += difference;

                    colonies += difference;
                }

                Broodwar->drawTextMap(station.getBase()->Center() + Position(0, 16), "%d, %d", Stations::needAirDefenses(station), Stations::needGroundDefenses(station));

                if (Stations::needAirDefenses(station) > 0)
                    needSpores = true;
                if (Stations::needGroundDefenses(station) > 0)
                    needSunks = true;
            }
        }

        defensesNow = buildQueue[Zerg_Creep_Colony] > 0 && vis(Zerg_Sunken_Colony) < 1 && (Strategy::getEnemyBuild() == "RaxFact" || Strategy::getEnemyBuild() == "2Gate" || Strategy::getEnemyBuild() == "1GateCore" || Strategy::enemyRush() || Players::ZvZ() || Util::getTime() > Time(6, 30));

        // Adding Overlords outside opening book supply
        if (!inBookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(26, Players::getSupply(PlayerState::Self) / providers);
            if (vis(Zerg_Overlord) >= 3)
                buildQueue[Zerg_Overlord] = count;
        }

        // Adding Overlords if we are sacrificing a scout
        if (Scouts::isSacrificeScout() && Util::getTime() > Time(3, 30))
            buildQueue[Zerg_Overlord]++;

        // Adding Evolution Chambers
        if ((Players::getSupply(PlayerState::Self) >= 180 && Stations::getMyStations().size() >= 3)
            || (techUnit == Zerg_Ultralisk && vis(Zerg_Queens_Nest) > 0)
            || needSpores)
            buildQueue[Zerg_Evolution_Chamber] = needSpores ? 1 : 1 + (Stations::getMyStations().size() >= 4);

        // Outside of opening book
        if (!inOpeningBook) {
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            // Calculate the number of gas workers we need
            auto totalMin = 0;
            auto totalGas = 0;
            for (auto &[type, percent] : armyComposition) {
                auto percentExists = double(vis(type) * type.supplyRequired()) / double(s);
                totalMin += max(1, int(round(percent - percentExists) * type.mineralPrice()));
                totalGas += max(1, int(round(percent - percentExists) * type.gasPrice()));
            }

            gasLimit = int(ceil(double(vis(Zerg_Drone) * totalGas * Resources::getGasCount() * 3 * 103) / double(totalMin * Resources::getMinCount() * 2 * 67)));
            if (Players::ZvZ())
                gasLimit += int(1.0 * double(Stations::getMyStations().size()));

            // Adding Hatcheries
            auto desiredProduction = INT_MAX;
            if (Players::ZvT())
                desiredProduction = int(Stations::getMyStations().size()) + max(0, int(Stations::getMyStations().size()) - 3) + max(0, int(Stations::getMyStations().size()) - 4);
            if (Players::ZvP())
                desiredProduction = int(Stations::getMyStations().size()) + max(0, int(Stations::getMyStations().size()) - 3) + (Strategy::getEnemyBuild() != "FFE");
            if (Players::ZvZ())
                desiredProduction = int(Stations::getMyStations().size()) + max(0, int(Stations::getMyStations().size()) - 1);

            if (Players::ZvP() && Strategy::getEnemyBuild() != "FFE")
                wantThird = false;

            productionSat = vis(Zerg_Hive) + vis(Zerg_Lair) + vis(Zerg_Hatchery) >= desiredProduction;
            checkExpand();
            checkRamp();
            buildQueue[Zerg_Hatchery] = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + expandDesired + rampDesired;

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
        if (larvaLimit == 0 && vis(Zerg_Spire) == com(Zerg_Spire) && Broodwar->self()->minerals() < 100 * int(Stations::getMyStations().size()) && ((defensesNow && Broodwar->self()->minerals() < 100) || Broodwar->self()->gas() > 100 * int(Stations::getMyStations().size()) || (Strategy::enemyRush() && Broodwar->self()->gas() > 200)))
            gasLimit = 0;
        if (needSpores && Players::ZvZ() && com(Zerg_Evolution_Chamber) == 0)
            gasLimit = 0;
        if (buildCount(Zerg_Extractor) == 0)
            gasLimit = 0;
        if (Units::getMyRoleCount(Role::Worker) < 5)
            gasLimit = 0;
    }

    void composition()
    {
        // Clear composition before setting
        if (!inOpeningBook)
            armyComposition.clear();

        // ZvT
        if (Players::vT() && !inOpeningBook) {
            auto vsMech = Strategy::getEnemyTransition() == "2Fact"
                || Strategy::getEnemyTransition() == "1FactTanks"
                || Strategy::getEnemyTransition() == "5FactGoliath";

            if (Util::getTime() > Time(15, 0) && Stations::getEnemyStations().size() == 0 && Terrain::foundEnemy()) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Mutalisk] = 0.60;
            }
            else if (isTechUnit(Zerg_Defiler) && s >= 360) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.15;
                armyComposition[Zerg_Mutalisk] = 0.20;
                armyComposition[Zerg_Ultralisk] = 0.10;
                armyComposition[Zerg_Defiler] = 0.05;
            }
            else if (isTechUnit(Zerg_Defiler)) {
                armyComposition[Zerg_Drone] = 0.55;
                armyComposition[Zerg_Zergling] = 0.30;
                armyComposition[Zerg_Ultralisk] = 0.10;
                armyComposition[Zerg_Defiler] = 0.05;
            }
            else if (isTechUnit(Zerg_Ultralisk) && s >= 360) {
                armyComposition[Zerg_Drone] = 0.55;
                armyComposition[Zerg_Mutalisk] = 0.20;
                armyComposition[Zerg_Zergling] = 0.15;
                armyComposition[Zerg_Ultralisk] = 0.10;
            }
            else if (isTechUnit(Zerg_Ultralisk) && com(Zerg_Ultralisk_Cavern)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.30;
                armyComposition[Zerg_Ultralisk] = 0.10;
            }
            else if (isTechUnit(Zerg_Lurker) && atPercent(TechTypes::Lurker_Aspect, 0.8)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.30;
                armyComposition[Zerg_Hydralisk] = 0.10;
                armyComposition[Zerg_Lurker] = 1.00;
            }
            else if (isTechUnit(Zerg_Hydralisk)) {
                armyComposition[Zerg_Drone] = 0.40;
                armyComposition[Zerg_Zergling] = 0.00;
                armyComposition[Zerg_Hydralisk] = 0.40;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.00;
                armyComposition[Zerg_Mutalisk] = 0.40;
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
            if (isTechUnit(Zerg_Defiler)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.25;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Lurker] = 1.00;
                armyComposition[Zerg_Defiler] = 0.05;
            }
            else if (isTechUnit(Zerg_Hydralisk) && isTechUnit(Zerg_Mutalisk) && isTechUnit(Zerg_Lurker)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.25;
                armyComposition[Zerg_Lurker] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.10;
            }
            else if (isTechUnit(Zerg_Lurker) && isTechUnit(Zerg_Mutalisk) && (Broodwar->self()->isResearching(TechTypes::Lurker_Aspect) || Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect))) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Hydralisk] = 0.10;
                armyComposition[Zerg_Lurker] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Hydralisk) && isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Hydralisk] = 0.20;
                armyComposition[Zerg_Mutalisk] = 0.20;
            }
            else if (isTechUnit(Zerg_Mutalisk) && Strategy::getEnemyBuild() != "FFE") {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.45;
            }
            else if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.60;
                armyComposition[Zerg_Zergling] = 0.05;
                armyComposition[Zerg_Mutalisk] = 0.35;
            }
            else {
                armyComposition[Zerg_Drone] = 0.80;
                armyComposition[Zerg_Zergling] = 0.20;
            }
        }

        // ZvZ
        if (Players::ZvZ() && !inOpeningBook) {
            if (isTechUnit(Zerg_Mutalisk)) {
                armyComposition[Zerg_Drone] = 0.50;
                armyComposition[Zerg_Zergling] = 0.10;
                armyComposition[Zerg_Mutalisk] = 0.40;
            }
            else {
                armyComposition[Zerg_Drone] = 0.55;
                armyComposition[Zerg_Zergling] = 0.45;
            }
        }

        if (Resources::isMinSaturated() && Resources::isGasSaturated()) {
            armyComposition[Zerg_Zergling] = armyComposition[Zerg_Drone];
            armyComposition[Zerg_Drone] = 0.0;
        }

        // Add Scourge if we have Mutas and enemy has air to air
        if (isTechUnit(Zerg_Mutalisk)) {
            auto airCount = Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) + Players::getVisibleCount(PlayerState::Enemy, Terran_Wraith);
            auto needScourgeZvP = Players::ZvP() && (airCount >= 3 || vis(Zerg_Mutalisk) == 0) && ((vis(Zerg_Scourge) / 2) - 1 < airCount && airCount < 6 && Players::getStrength(PlayerState::Enemy).airToAir > 0.0);
            auto needScourgeZvZ = Players::ZvZ() && (airCount / 2) > vis(Zerg_Scourge) && (total(Zerg_Mutalisk) >= 3 || currentTransition == "2HatchMuta");
            auto needScourgeZvT = Players::ZvT() &&
                ((Strategy::getEnemyTransition() == "2PortWraith" && (airCount >= 3 || vis(Zerg_Mutalisk) == 0) && ((vis(Zerg_Scourge) / 2) - 1 < airCount && airCount < 6 && Players::getStrength(PlayerState::Enemy).airToAir > 0.0))
                    || (Util::getTime() > Time(10, 00) && vis(Zerg_Scourge) < 4));

            if (needScourgeZvP || needScourgeZvZ || needScourgeZvT) {
                armyComposition[Zerg_Scourge] = max(0.20, armyComposition[Zerg_Mutalisk]);
                armyComposition[Zerg_Mutalisk] = 0.00;
            }
        }

        // Specific compositions
        if (isTechUnit(Zerg_Hydralisk) && !Terrain::isIslandMap() && (Players::getVisibleCount(PlayerState::Enemy, Protoss_Stargate) >= 3 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Carrier) >= 4 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Fleet_Beacon) >= 1)) {
            armyComposition.clear();
            armyComposition[Zerg_Drone] = 0.50;
            armyComposition[Zerg_Hydralisk] = 0.50;
        }

        // Determine if we should drone up instead of build army
        if (Util::getTime() < Time(10, 00) && !inOpeningBook && !Strategy::enemyProxy() && !Strategy::enemyRush()) {
            auto enemyGroundRatio =     0.5 - clamp(Players::getStrength(PlayerState::Enemy).groundDefense / (0.01 + Players::getStrength(PlayerState::Enemy).groundToGround + Players::getStrength(PlayerState::Enemy).airToGround + Players::getStrength(PlayerState::Enemy).groundDefense), 0.0, 1.0);
            auto enemyAirRatio =        0.5 - clamp(Players::getStrength(PlayerState::Enemy).airDefense / (0.01 + Players::getStrength(PlayerState::Enemy).airToAir + Players::getStrength(PlayerState::Enemy).groundToAir + Players::getStrength(PlayerState::Enemy).airDefense), 0.0, 1.0);

            auto offsetBy = 0.2 * enemyAirRatio;

            armyComposition[Zerg_Drone] =       max(0.0, armyComposition[Zerg_Drone] - offsetBy);
            armyComposition[Zerg_Mutalisk] =    max(0.0, armyComposition[Zerg_Mutalisk] + offsetBy);
        }
    }

    void unlocks()
    {
        // Always unlock Overlords
        unlockedType.insert(Zerg_Overlord);

        // Saving larva to burst out tech units
        int limitBy = int(Stations::getMyStations().size()) * 3;
        larvaLimit = limitBy * int((inOpeningBook || techList.size() == 1) && ((atPercent(Zerg_Spire, 0.50) && com(Zerg_Spire) == 0) || (atPercent(Zerg_Hydralisk_Den, 0.6) && com(Zerg_Hydralisk_Den) == 0)));

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

        // Remove Mutalisks if we're against Valks
        if (Players::getVisibleCount(PlayerState::Enemy, Terran_Valkyrie) >= 3 && vis(Zerg_Mutalisk) >= 9)
            unlockedType.erase(Zerg_Mutalisk);

        // Remove Lings if we're against Vults
        if (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 && total(Zerg_Ultralisk_Cavern) == 0 && total(Zerg_Defiler_Mound) == 0)
            unlockedType.erase(Zerg_Zergling);

        // UMS Unlocking
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings) {
            for (auto &type : BWAPI::UnitTypes::allUnitTypes()) {
                if (!type.isBuilding() && type.getRace() == Races::Zerg && vis(type) >= 2) {
                    unlockedType.insert(type);
                    if (!type.isWorker())
                        techList.insert(type);
                }
            }
        }
    }
}