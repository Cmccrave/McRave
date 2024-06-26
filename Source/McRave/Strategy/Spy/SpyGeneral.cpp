#include "Main\McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Spy::General {

    namespace {
        map<Unit, UnitType> unitsStored; // A bit hacky way to say if we've stored a unit
        string nodeName = "[Spy]: ";

        void enemyUnitTimings(PlayerInfo& player, StrategySpy& theSpy)
        {
            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // Don't track these
                if (unit.getType() == Zerg_Egg || unit.getType() == Zerg_Larva || unit.getType() == Zerg_Lurker_Egg
                    || unit.getType() == Zerg_Creep_Colony
                    || !unit.unit()->exists())
                    continue;

                // If unit type is changing now, remove it from the timings
                if ((unit.getType() == Zerg_Lair || unit.getType() == Zerg_Hive
                    || unit.getType() == Zerg_Greater_Spire
                    || unit.getType() == Zerg_Sunken_Colony || unit.getType() == Zerg_Spore_Colony) && !unit.isCompleted()) {
                    unitsStored.erase(unit.unit());
                    continue;
                }

                // Attempt to remove drones that morphed into a building
                auto itr = unitsStored.find(unit.unit());
                if (itr != unitsStored.end() && itr->second != unit.getType())
                    unitsStored.erase(unit.unit());

                // If not tracked, store timing metrics
                if (unitsStored.find(unit.unit()) == unitsStored.end()) {
                    unitsStored[unit.unit()] = unit.getType();
                    auto &ut = theSpy.unitTimings[unit.getType()];
                    ut.countStartedWhen.push_back(unit.timeStartedWhen());
                    ut.countCompletedWhen.push_back(unit.timeCompletesWhen());
                    ut.countArrivesWhen.push_back(unit.timeArrivesWhen());

                    // Debug log
                    auto count = int(ut.countStartedWhen.size());
                    if (!unit.getType().isBuilding())
                        Util::debug(nodeName + string(unit.getType().c_str()) + " " + to_string(count) + " arrives at " + unit.timeArrivesWhen().toString());
                    else
                        Util::debug(nodeName + string(unit.getType().c_str()) + " " + to_string(count) + " completes at " + unit.timeCompletesWhen().toString());

                    // If this timing is sooner than arrival, start or completion, overwrite existing data
                    if (ut.firstArrivesWhen.isUnknown() || unit.timeArrivesWhen() < ut.firstArrivesWhen)
                        ut.firstArrivesWhen = unit.timeArrivesWhen();
                    if (ut.firstStartedWhen.isUnknown() || unit.timeStartedWhen() < ut.firstStartedWhen)
                        ut.firstStartedWhen = unit.timeStartedWhen();
                    if (ut.firstCompletedWhen.isUnknown() || unit.timeCompletesWhen() < ut.firstCompletedWhen)
                        ut.firstCompletedWhen = unit.timeCompletesWhen();                    
                }
            }
        }

        void enemyUpgradeTimings(PlayerInfo& player, StrategySpy& theSpy) 
        {
            // TODO: More than 1 upgrade level
            for (auto &upgrade : UpgradeTypes::allUpgradeTypes()) {
                auto &ut = theSpy.upgradeTimings[upgrade];
                if (player.hasUpgrade(upgrade) && theSpy.upgradeTimings[upgrade].firstCompletedWhen.isUnknown()) {
                    ut.firstCompletedWhen = Util::getTime();
                    ut.countCompletedWhen.push_back(Util::getTime());
                    Util::debug(nodeName + string(upgrade.c_str()) +  " completes at " + Util::getTime().toString());
                }
            }
        }

        void checkEnemyUnits(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Monitor for a wall
            if (Players::vT() && Terrain::getEnemyStartingPosition().isValid() && Util::getTime() < Time(5, 00)) {
                auto pathPoint = Terrain::getEnemyStartingPosition();
                auto closest = DBL_MAX;

                for (int x = Terrain::getEnemyStartingTilePosition().x - 2; x < Terrain::getEnemyStartingTilePosition().x + 5; x++) {
                    for (int y = Terrain::getEnemyStartingTilePosition().y - 2; y < Terrain::getEnemyStartingTilePosition().y + 5; y++) {
                        auto center = Position(TilePosition(x, y)) + Position(16, 16);
                        auto dist = center.getDistance(Terrain::getEnemyStartingPosition());
                        if (dist < closest && BWEB::Map::isUsed(TilePosition(x, y)) == UnitTypes::None) {
                            closest = dist;
                            pathPoint = center;
                        }
                    }
                }

                BWEB::Path newPath(Position(Terrain::getMainChoke()->Center()), pathPoint, Zerg_Zergling);
                newPath.generateJPS([&](auto &t) { return newPath.unitWalkable(t); });
                if (!newPath.isReachable())
                    theSpy.wall.possible = true;
            }

            theSpy.workersPulled = 0;
            theSpy.productionCount = (player.getCurrentRace() == Races::Zerg ? 1 : 0); // Starting hatcheries always exist
            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // Monitor the soonest the enemy will scout us
                if (unit.getType().isWorker()) {
                    if (Terrain::inTerritory(PlayerState::Self, unit.getPosition()) || unit.isProxy())
                        theSpy.workersPulled++;
                }

                // Monitor gas intake or gas steal
                if (unit.getType().isRefinery() && unit.unit()->exists()) {
                    if (Terrain::inTerritory(PlayerState::Self, unit.getPosition()))
                        theSpy.steal.possible = true;
                    else
                        theSpy.gasMined = unit.unit()->getInitialResources() - unit.unit()->getResources();
                }

                // Monitor for any upgrades coming
                if (unit.unit()->isUpgrading() && theSpy.typeUpgrading.find(unit.getType()) == theSpy.typeUpgrading.end())
                    theSpy.typeUpgrading.insert(unit.getType());

                // Monitor for a fast expand
                if (unit.getType().isResourceDepot()) {
                    for (auto &base : Terrain::getAllBases()) {
                        if (!base->Starting() && base->Center().getDistance(unit.getPosition()) < 64.0)
                            theSpy.expand.possible = true;
                    }
                }

                // Monitor for a proxy
                if (Util::getTime() < Time(5, 00)) {
                    if (unit.getType().isBuilding() && unit.isProxy())
                        theSpy.proxy.possible = true;
                }

                // Non starting hatchery incrementing
                if (unit.getType() == Zerg_Hatchery || unit.getType() == Zerg_Lair || unit.getType() == Zerg_Hive) {
                    if (find(mapBWEM.StartingLocations().begin(), mapBWEM.StartingLocations().end(), unit.getTilePosition()) == mapBWEM.StartingLocations().end())
                        theSpy.productionCount++;
                }

                // Monitor for invis units
                if (unit.isHidden())
                    theSpy.invis.possible = true;                
            }
        }

        void checkEnemyRush(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Rush builds are immediately aggresive builds
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self, Races::None) >= 70 : Players::getSupply(PlayerState::Self, Races::None) >= 90;
            theSpy.rush.possible = !supplySafe && (Spy::getEnemyTransition() == "MarineRush" || Spy::getEnemyTransition() == "ZealotRush" || Spy::getEnemyTransition() == "LingRush" || Spy::getEnemyTransition() == "WorkerRush");
            if (supplySafe)
                theSpy.rush.possible = false;
        }

        void checkEnemyPressure(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Pressure builds are delayed aggresive builds
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self, Races::None) >= 80 : Players::getSupply(PlayerState::Self, Races::None) >= 120;
            theSpy.pressure.possible = !supplySafe && (Spy::getEnemyTransition() == "4Gate" || Spy::getEnemyTransition() == "2HatchSpeedling" || Spy::getEnemyTransition() == "3HatchSpeedling" || Spy::getEnemyTransition() == "Sparks" || Spy::getEnemyTransition() == "2Fact" || Spy::getEnemyTransition() == "Academy");
            if (supplySafe)
                theSpy.pressure.possible = false;
        }

        void checkEnemyInvis(PlayerInfo& player, StrategySpy& theSpy)
        {
            // DTs, Vultures, Lurkers
            theSpy.invis.possible = (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) >= 1 || (Players::getTotalCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) > 0) || Players::getTotalCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1)
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Ghost) >= 1 || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 4)
                || (Players::getTotalCount(PlayerState::Enemy, Zerg_Lurker) >= 1 || (Players::getTotalCount(PlayerState::Enemy, Zerg_Lair) >= 1 && Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk_Den) >= 1))
                || (Spy::getEnemyTransition() == "1HatchLurker" || Spy::getEnemyTransition() == "2HatchLurker" || Spy::getEnemyTransition() == "DT"  || Spy::getEnemyTransition() == "2PortWraith");

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (com(Protoss_Observer) > 0)
                    theSpy.invis.possible = false;
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Pneumatized_Carapace))
                    theSpy.invis.possible = false;
            }

            // Terran
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (com(Terran_Comsat_Station) > 0)
                    theSpy.invis.possible = false;
                else if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) > 0 || Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0)
                    theSpy.invis.possible = true;
            }
        }

        void checkEnemyEarly(PlayerInfo& player, StrategySpy& theSpy)
        {
            // If we have seen an enemy worker before we've scouted the enemy, follow it
            if ((Players::getVisibleCount(PlayerState::Enemy, Protoss_Probe) > 0 || Players::getVisibleCount(PlayerState::Enemy, Terran_SCV) > 0) && Util::getTime() < Time(2, 00)) {
                auto enemyWorker = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u->getType().isWorker() && u->isProxy();
                });
                if (enemyWorker)
                    theSpy.early.possible = true;
            }
            if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) > 0 || Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) > 0 || Util::getTime() > Time(2, 00))
                theSpy.early.possible = false;
        }

        void checkEnemyProxy(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Proxy builds are built closer to me than the enemy
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self, Races::None) >= 40 : Players::getSupply(PlayerState::Self, Races::None) >= 80;
            if (supplySafe)
                theSpy.proxy.possible = false;
        }

        void checkEnemyGreedy(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Greedy detection
            theSpy.greedy.possible = (Players::ZvP() && Spy::getEnemyBuild() != "FFE" && int(Stations::getStations(PlayerState::Enemy).size()) >= 3 && Util::getTime() < Time(10, 00))
                || (Players::ZvP() && Spy::getEnemyBuild() != "FFE" && theSpy.expand.confirmed && Util::getTime() < Time(6, 45))
                || (Players::ZvT() && int(Stations::getStations(PlayerState::Enemy).size()) >= 3 && Util::getTime() < Time(10, 00));
            if (Util::getTime() > Time(10, 00))
                theSpy.greedy.possible = false;
        }
    }

    void updateGeneral(StrategySpy& theSpy)
    {
        // Update enemy general strategy
        for (auto &p : Players::getPlayers()) {
            PlayerInfo &player = p.second;

            if (player.isEnemy()) {
                enemyUnitTimings(player, theSpy);
                enemyUpgradeTimings(player, theSpy);
                checkEnemyUnits(player, theSpy);

                // TODO: Impl multiple players
                checkEnemyRush(player, theSpy);
                checkEnemyPressure(player, theSpy);
                checkEnemyInvis(player, theSpy);
                checkEnemyEarly(player, theSpy);
                checkEnemyProxy(player, theSpy);
                checkEnemyGreedy(player, theSpy);
            }
        }
    }
}