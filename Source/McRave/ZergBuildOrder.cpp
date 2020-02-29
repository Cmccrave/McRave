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
        auto hatchPerBase = 2.0;
    }

    void opener()
    {
        if (Players::vT()) {
            if (currentBuild == "HatchPool")
                ZvTHatchPool();
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
        const auto techVal = int(techList.size()) + skipOneTech - (Players::vT() && Strategy::enemyPressure());
        techSat = (techVal >= int(Stations::getMyStations().size()));

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = None;

        // Adding tech
        if (!inOpeningBook && techUnit == None && !techSat && com(Zerg_Hatchery) == vis(Zerg_Hatchery))
            getTech = true;

        //// Add Scourge if we have Mutas and enemy has air to air
        //if (isTechUnit(Zerg_Mutalisk) && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
        //    techList.insert(Zerg_Scourge);
        //    unlockedType.insert(Zerg_Scourge);
        //}

        // Add Hydralisks if we want Lurkers
        if (isTechUnit(Zerg_Lurker)) {
            techList.insert(Zerg_Hydralisk);
            unlockedType.insert(Zerg_Hydralisk);
        }

        getNewTech();
        getTechBuildings();
    }

    void situational()
    {
        auto baseVal = int(Stations::getMyStations().size());
        auto techVal = int(techList.size()) + 2;
        buildQueue[Zerg_Creep_Colony] = vis(Zerg_Creep_Colony) + vis(Zerg_Spore_Colony) + vis(Zerg_Sunken_Colony);

        // Executing gas Trick
        if (gasTrick) {
            if (Broodwar->self()->minerals() >= 80 && total(Zerg_Extractor) == 0)
                buildQueue[Zerg_Extractor] = 1;
            if (vis(Zerg_Extractor) > 0)
                buildQueue[Zerg_Extractor] = (vis(Zerg_Drone) < 9);
        }

        // Adding Wall Defenses
        if (vis(Zerg_Drone) >= 10 && atPercent(Zerg_Spawning_Pool, 0.5) && Terrain::getNaturalWall()) {

            int colonies = 0;
            for (auto& tile : Terrain::getNaturalWall()->getDefenses()) {
                if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                    colonies++;
            }

            if (Terrain::needAirDefenses(*Terrain::getNaturalWall()) > colonies || Terrain::needGroundDefenses(*Terrain::getNaturalWall()) > colonies) {
                Broodwar->drawCircleMap(Terrain::getNaturalWall()->getCentroid(), 4, Colors::Orange, false);
                buildQueue[Zerg_Creep_Colony] += 1;
            }
        }

        // Adding Station Defenses
        if (com(Zerg_Spawning_Pool) >= 1) {
            for (auto &station : Stations::getMyStations()) {
                auto &s = *station.second;

                int colonies = 0;
                for (auto& tile : s.getDefenseLocations()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Stations::needGroundDefenses(s) > colonies || Stations::needAirDefenses(s) > colonies) {
                    Broodwar->drawCircleMap(s.getBWEMBase()->Center(), 6, Colors::Red, false);
                    buildQueue[Zerg_Creep_Colony] += 1;
                    break;
                }
            }
        }

        // Adding Overlords
        if (!inBookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(22, Players::getSupply(PlayerState::Self) / providers);
            if (vis(Zerg_Overlord) >= 3)
                buildQueue[Zerg_Overlord] = count;
        }

        // Adding Evolution Chambers
        if ((Players::getSupply(PlayerState::Self) >= 160 && Stations::getMyStations().size() >= 3) || (Strategy::enemyAir() && (!Players::vZ() || vis(Zerg_Spire) == 0)))
            buildQueue[Zerg_Evolution_Chamber] = 2 - Strategy::enemyAir();

        // Outside of opening book
        if (!inOpeningBook) {
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            // Figuring out how many gas workers we need
            auto totalMin = 0;
            auto totalGas = 0;
            for (auto &[type, percent] : armyComposition) {
                totalMin += int(percent * type.mineralPrice());
                totalGas += int(percent * type.gasPrice());
            }
            gasLimit = int(ceil(double(vis(Zerg_Drone) * totalGas * Resources::getGasCount() * 3 * 103) / double(totalMin * Resources::getMinCount() * 2 * 67)));

            // Adding Hatcheries
            productionSat = (vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive)) >= int(hatchPerBase * Stations::getMyStations().size());
            if (shouldExpand() || shouldAddProduction())
                buildQueue[Zerg_Hatchery] = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + 1;

            // Adding Extractors
            if (shouldAddGas())
                buildQueue[Zerg_Extractor] = gasCount;

            // Hive upgrades
            if (Broodwar->self()->getRace() == Races::Zerg && Players::getSupply(PlayerState::Self) >= 300) {
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive] = com(Zerg_Queens_Nest) >= 1;
                buildQueue[Zerg_Lair] = com(Zerg_Queens_Nest) < 1;
            }
        }
    }

    void composition()
    {
        if (inOpeningBook)
            return;

        armyComposition.clear();

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
    }

    void unlocks()
    {
        unlockedType.insert(Zerg_Overlord);
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

            if (Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) < 2 || Util::getTime() > Time(20, 0))
                unlockedType.insert(Zerg_Zergling);

            unlockedType.insert(Zerg_Drone);
        }
    }
}