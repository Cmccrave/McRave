#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Scouts {

    namespace {

        set<Position> scoutTargets;
        int scoutCount;
        bool proxyCheck = false;
        UnitType proxyType = UnitTypes::None;
        Position proxyPosition = Positions::Invalid;

        void updateScoutCount()
        {
            scoutCount = 1;

            // If we have seen an enemy Probe before we've scouted the enemy, follow it
            if (Units::getEnemyCount(UnitTypes::Protoss_Probe) == 1 && Units::getEnemyCount(UnitTypes::Protoss_Zealot) == 0) {
                auto &enemyProbe = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType() == UnitTypes::Protoss_Probe;
                });
                proxyCheck = (enemyProbe && !Terrain::getEnemyStartingPosition().isValid() && enemyProbe->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0);
            }
            else
                proxyCheck = false;
            
            // Calculate the number of unexplored bases
            int unexploredBases = 0;
            for (auto &tile : mapBWEM.StartingLocations()) {
                Position center = Position(tile) + Position(64, 48);
                if (!Broodwar->isExplored(TilePosition(center)))
                    unexploredBases++;
            }

            // If we are playing PvZ
            if (Players::PvZ()) {

                // Don't scout vs 4Pool
                if (Strategy::getEnemyBuild() == "4Pool")
                    scoutCount = 0;

                // Send a 2nd scout after 1st scout
                else if (!Terrain::getEnemyStartingPosition().isValid() && mapBWEM.StartingLocations().size() == 4 && unexploredBases == 2)
                    scoutCount = 2;
            }

            // If we are playing PvP
            if (Players::PvP()) {

                // Send a 2nd scout versus a proxy
                if ((Strategy::enemyProxy() || proxyCheck) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1)
                    scoutCount = 2;
            }

            if (Broodwar->self()->getRace() == Races::Zerg && Terrain::getEnemyStartingPosition().isValid())
                scoutCount = 0;

            if (Strategy::enemyPressure() && BuildOrder::isPlayPassive())
                scoutCount = 0;
        }

        void updateScoutTargets()
        {
            scoutTargets.clear();

            const auto explored = [](Position here) {
                return Broodwar->isExplored(TilePosition(here));
            };

            const auto addTarget = [](Position here) {
                if (here.isValid())
                    scoutTargets.insert(here);
            };

            // If it's a proxy, scout for the proxy building
            if (Strategy::enemyProxy()) {
                auto proxyType = Players::vP() ? UnitTypes::Protoss_Pylon : UnitTypes::Terran_Barracks;
                if (Units::getEnemyCount(proxyType) == 0) {
                    addTarget(mapBWEM.Center());
                    proxyPosition = mapBWEM.Center();
                }
                else {
                    auto &proxyBuilding = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Enemy, [&](auto &u) {
                        return u.getType() == proxyType;
                    });
                    if (proxyBuilding) {
                        addTarget(proxyBuilding->getPosition());
                        proxyPosition = proxyBuilding->getPosition();
                    }
                }
            }

            // If it's a cannon rush, scout the main
            else if (Strategy::getEnemyBuild() == "CannonRush")
                addTarget(BWEB::Map::getMainPosition());

            // If enemy start is valid and explored, add a target to the most recent one to scout
            else if (Terrain::foundEnemy()) {

                // Add each enemy station as a target
                for (auto &[_, station] : Stations::getEnemyStations()) {
                    auto tile = station->getBWEMBase()->Center();
                    addTarget(Position(tile));
                }

                // Add extra co-ordinates for better exploring
                addTarget(Terrain::getEnemyStartingPosition() + Position(0, -320));
                addTarget(Terrain::getEnemyStartingPosition() + Position(0, 320));
                addTarget(Terrain::getEnemyStartingPosition() + Position(-320, 0));
                addTarget(Terrain::getEnemyStartingPosition() + Position(320, 0));

                if (Players::vZ() && Stations::getEnemyStations().size() == 1)
                    addTarget(Position(Terrain::getEnemyExpand()));
            }

            // If we know where it is but it isn't explored
            else if (Terrain::getEnemyStartingTilePosition().isValid())
                addTarget(Terrain::getEnemyStartingPosition());

            // If we have no idea where the enemy is
            else {
                auto basesExplored = 0;
                multimap<double, Position> startsByDist;

                // Sort unexplored starts by distance
                for (auto &start : mapBWEM.StartingLocations()) {
                    auto center = Position(start) + Position(64, 48);
                    auto dist = center.getDistance(BWEB::Map::getMainPosition());

                    if (!explored(center))
                        startsByDist.emplace(1.0 / dist, center);
                    else
                        basesExplored++;
                }

                // Assign n scout targets where n is scout count
                for (auto &[_, position] : startsByDist) {
                    addTarget(position);
                    if (int(scoutTargets.size()) == scoutCount)
                        break;
                }

                // Scout the popular middle proxy location if it's walkable
                if (basesExplored == 1 && !Players::vZ() && !explored(mapBWEM.Center()) && BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), mapBWEM.Center()) != DBL_MAX)
                    addTarget(mapBWEM.Center());
            }
        }

        void updateAssignment(UnitInfo& unit)
        {
            auto start = unit.getWalkPosition();
            auto distBest = DBL_MAX;

            const auto isClosestAvailableScout = [&](Position here) {
                auto &closestScout = Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Scout && !u.getDestination().isValid();
                });
                return unit.shared_from_this() == closestScout;
            };

            if (!BuildOrder::firstReady() || BuildOrder::isOpener() || !Terrain::getEnemyStartingPosition().isValid()) {

                // If it's a main or natural proxy
                if (Strategy::enemyProxy() && Strategy::getEnemyBuild() == "2Gate") {
                    auto &enemyPylon = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto u) {
                        return u.getType() == UnitTypes::Protoss_Pylon;
                    });

                    if (enemyPylon && enemyPylon->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0) {
                        if (enemyPylon->unit() && enemyPylon->unit()->exists()) {
                            unit.unit()->attack(enemyPylon->unit());
                            return;
                        }
                        else
                            unit.setDestination(enemyPylon->getPosition());
                        return;
                    }
                }

                // If it's a center of map proxy
                if ((proxyPosition.isValid() && isClosestAvailableScout(proxyPosition)) || proxyCheck) {

                    // Determine what proxy type to expect
                    if (Units::getEnemyCount(UnitTypes::Terran_Barracks) > 0)
                        proxyType = UnitTypes::Terran_Barracks;
                    else if (Units::getEnemyCount(UnitTypes::Protoss_Pylon) > 0)
                        proxyType = UnitTypes::Protoss_Pylon;
                    else if (Units::getEnemyCount(UnitTypes::Protoss_Gateway) > 0)
                        proxyType = UnitTypes::Protoss_Gateway;

                    // Find the closet of the proxy type we expect
                    auto &enemyWorker = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto u) {
                        return u.getType().isWorker();
                    });
                    auto &enemyStructure = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto u) {
                        return u.getType() == proxyType;
                    });

                    auto enemyWorkerClose = enemyWorker && enemyWorker->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0;
                    auto enemyWorkerConstructing = enemyWorker && enemyStructure && enemyWorker->getPosition().getDistance(enemyStructure->getPosition()) < 128.0;
                    auto enemyStructureProxy = enemyStructure && !Terrain::isInEnemyTerritory(enemyStructure->getTilePosition());

                    // Attempt to kill the worker if we find it - TODO: Check if the Attack command takes care of this
                    if (Strategy::getEnemyBuild() != "2Gate" && (enemyWorkerClose || enemyWorkerConstructing)) {
                        if (enemyWorker->unit() && enemyWorker->unit()->exists()) {
                            unit.unit()->attack(enemyWorker->unit());
                            return;
                        }
                        else
                            unit.setDestination(enemyWorker->getPosition());
                    }
                    else if (enemyStructureProxy) {
                        if (enemyStructure->unit() && enemyStructure->unit()->exists()) {
                            unit.unit()->attack(enemyStructure->unit());
                            return;
                        }
                        else
                            unit.setDestination(enemyStructure->getPosition());
                    }
                    else
                        unit.setDestination(BWEB::Map::getMainPosition());
                }

                // If we have scout targets, find the closest scout target
                else if (!scoutTargets.empty()) {

                    auto best = 0.0;
                    for (auto &target : scoutTargets) {
                        auto dist = target.getDistance(unit.getPosition());
                        auto time = 1.0 + (double(Grids::lastVisibleFrame((TilePosition)target)));
                        auto timeDiff = Broodwar->getFrameCount() - time;
                        auto score = time / dist;

                        if (!isClosestAvailableScout(target))
                            continue;

                        if (score > best && timeDiff > 500) {
                            best = score;
                            unit.setDestination(target);
                        }
                    }
                }

                // Make sure we always do something
                if (!unit.getDestination().isValid())
                    unit.setDestination(Terrain::getEnemyStartingPosition());
            }

            else
            {
                int best = INT_MAX;
                for (auto &area : mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        if (area.AccessibleNeighbours().size() == 0
                            || Terrain::isInEnemyTerritory(base.Location())
                            || Terrain::isInAllyTerritory(base.Location())
                            || Command::overlapsActions(unit.unit(), Position(base.Location()), UnitTypes::None, PlayerState::Self))
                            continue;

                        int time = Grids::lastVisibleFrame(base.Location());
                        if (time < best) {
                            best = time;
                            unit.setDestination(Position(base.Location()));
                        }
                    }
                }
            }
        }

        constexpr tuple commands{ Command::attack, Command::kite, Command::hunt, Command::move };
        void updateDecision(UnitInfo& unit)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Attack"),
                make_pair(1, "Kite"),
                make_pair(2, "Hunt"),
                make_pair(3, "Move"),
            };

            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);

            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateScouts()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Scout) {
                    updateAssignment(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateScoutCount();
        updateScoutTargets();
        updateScouts();
        Visuals::endPerfTest("Scouts");
    }

    int getScoutCount() { return scoutCount; }
}