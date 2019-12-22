#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Zerg {

    void opener()
    {
        if (currentBuild == "HatchPool")
            HatchPool();
        else if (currentBuild == "PoolHatch")
            PoolHatch();
        else if (currentBuild == "PoolLair")
            PoolLair();
    }

    void tech()
    {
        const auto firstTechUnit = !techList.empty() ? *techList.begin() : None;
        const auto skipOneTech = true;
        const auto techVal = int(techList.size()) + skipOneTech;
        techSat = techVal >= com(Zerg_Hatchery) + com(Zerg_Lair) + com(Zerg_Hive) - 1;

        // If we have our tech unit, set to none
        if (techComplete())
            techUnit = None;

        // Adding tech
        if (!inOpeningBook && techUnit == None && !techSat)
            getTech = true;

        // Add Scourge if we have Mutas and enemy has air to air
        if (isTechUnit(Zerg_Mutalisk) && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
            techList.insert(Zerg_Scourge);
            unlockedType.insert(Zerg_Scourge);
        }

        // Add Hydralisks if we want Lurkers as they are needed first
        if (isTechUnit(Zerg_Lurker)) {
            techList.insert(Zerg_Hydralisk);
            unlockedType.insert(Zerg_Hydralisk);
        }

        // ZvP
        if (Players::ZvP()) {
            if (firstTechUnit == Zerg_Mutalisk)
                techOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
            if (firstTechUnit == Zerg_Hydralisk)
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
            else
                techOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
        }

        // ZvT
        if (Players::ZvT()) {
            if (firstTechUnit == Zerg_Mutalisk)
                techOrder ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
            if (firstTechUnit == Zerg_Hydralisk)
                techOrder ={ Zerg_Mutalisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
            else
                techOrder ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker, Zerg_Defiler, Zerg_Ultralisk };
        }

        // ZvZ
        if (Players::ZvZ())
            techOrder ={ Zerg_Mutalisk, Zerg_Devourer };

        if (isTechUnit(Zerg_Ultralisk)) {
            armyComposition[Zerg_Drone] = 0.35;
            armyComposition[Zerg_Zergling] = 0.25;
            armyComposition[Zerg_Lurker] = 0.10;
            armyComposition[Zerg_Mutalisk] = 0.10;
            armyComposition[Zerg_Ultralisk] = 0.10;
            armyComposition[Zerg_Defiler] = 0.10;
        }
        else if (isTechUnit(Zerg_Defiler)) {
            armyComposition[Zerg_Drone] = 0.35;
            armyComposition[Zerg_Zergling] = 0.25;
            armyComposition[Zerg_Lurker] = 0.15;
            armyComposition[Zerg_Mutalisk] = 0.10;
            armyComposition[Zerg_Defiler] = 0.10;
        }
        else if (isTechUnit(Zerg_Lurker) && isTechUnit(Zerg_Mutalisk)) {
            armyComposition[Zerg_Drone] = 0.45;
            armyComposition[Zerg_Zergling] = 0.05;
            armyComposition[Zerg_Hydralisk] = 0.20;
            armyComposition[Zerg_Lurker] = 0.05;
            armyComposition[Zerg_Mutalisk] = 0.20;
            armyComposition[Zerg_Scourge] = 0.05;
        }
        else if (isTechUnit(Zerg_Lurker) && !isTechUnit(Zerg_Mutalisk)) {
            armyComposition[Zerg_Drone] = 0.50;
            armyComposition[Zerg_Zergling] = 0.05;
            armyComposition[Zerg_Hydralisk] = 0.20;
            armyComposition[Zerg_Lurker] = 0.25;
        }
        else if (isTechUnit(Zerg_Mutalisk) && isTechUnit(Zerg_Hydralisk)) {
            armyComposition[Zerg_Drone] = 0.50;
            armyComposition[Zerg_Zergling] = 0.00;
            armyComposition[Zerg_Hydralisk] = 0.20;
            armyComposition[Zerg_Mutalisk] = 0.20;
            armyComposition[Zerg_Scourge] = 0.10;
        }
        else if (isTechUnit(Zerg_Hydralisk)) {
            armyComposition[Zerg_Drone] = 0.60;
            armyComposition[Zerg_Zergling] = 0.10;
            armyComposition[Zerg_Hydralisk] = 0.30;
        }

        getNewTech();
        getTechBuildings();
    }

    void situational()
    {
        auto baseVal = int(Stations::getMyStations().size());
        auto techVal = int(techList.size()) + 2;

        // Executing gas Trick
        if (gasTrick) {
            if (Broodwar->self()->minerals() >= 80 && total(Zerg_Extractor) == 0)
                buildQueue[Zerg_Extractor] = 1;
            if (vis(Zerg_Extractor) > 0)
                buildQueue[Zerg_Extractor] = (vis(Zerg_Drone) < 9);
        }

        // Adding Sunkens/Spores
        if (vis(Zerg_Drone) >= 8) {
            auto sunkenCount = 0;
            if (Players::vP())
                sunkenCount = int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon)) / 2 - (com(Zerg_Zergling) / 8) + (com(Zerg_Hydralisk) / 2) + (com(Zerg_Mutalisk) / 2);
            else if (Players::vT() && vis(Zerg_Drone) >= 16)
                sunkenCount = 2;

            buildQueue[Zerg_Creep_Colony] = sunkenCount;
            wallDefenseDesired = sunkenCount;
        }

        // Adding Overlords
        if (!inBookSupply) {
            int providers = vis(Zerg_Overlord) > 0 ? 14 : 16;
            int count = 1 + min(22, Players::getSupply(PlayerState::Self) / providers);
            if (vis(Zerg_Overlord) >= 3)
                buildQueue[Zerg_Overlord] = count;
        }

        // Outside of opening book
        if (!inOpeningBook) {
            int gasCount = min(vis(Zerg_Extractor) + 1, Resources::getGasCount());

            // Figuring out how many gas workers we need
            auto totalMin = 0;
            auto totalGas = 0;
            auto miningSpeedRatio = 65.0 / 103.0;
            for (auto &[type, _] : armyComposition) {
                totalMin += type.mineralPrice();
                totalGas += type.gasPrice();
            }
            gasLimit = int(max(0.0, com(Zerg_Drone) * (miningSpeedRatio * double(totalGas) / double(totalMin))));
            Broodwar << gasLimit << endl;

            // Adding Hatcheries
            if (shouldExpand() || shouldAddProduction()) {
                auto hatchPerBase = 2.5;
                buildQueue[Zerg_Hatchery] = min(int(Stations::getMyStations().size() * hatchPerBase), vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) + 1);
                productionSat = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive) >= Stations::getMyStations().size() * hatchPerBase;
            }

            // Adding Extractors
            if (shouldAddGas())
                buildQueue[Zerg_Extractor] = gasCount;

            // Adding Evolution Chambers
            if (Players::getSupply(PlayerState::Self) >= 100)
                buildQueue[Zerg_Evolution_Chamber] = 2;

            // Adding Defenses
            if (com(Zerg_Spawning_Pool) >= 1 && int(Stations::getMyStations().size()) >= 3) {
                buildQueue[Zerg_Creep_Colony] = vis(Zerg_Sunken_Colony) + vis(Zerg_Spore_Colony);

                for (auto &station : Stations::getMyStations()) {
                    auto &s = *station.second;

                    if (Stations::needDefenses(s) > 0 )
                        buildQueue[Zerg_Creep_Colony] = vis(Zerg_Sunken_Colony) + 1;
                }
            }

            // Hive upgrades
            if (Broodwar->self()->getRace() == Races::Zerg && Players::getSupply(PlayerState::Self) >= 200) {
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive] = com(Zerg_Queens_Nest) >= 1;
                buildQueue[Zerg_Lair] = com(Zerg_Queens_Nest) < 1;
            }
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