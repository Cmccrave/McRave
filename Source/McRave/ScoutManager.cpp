#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Scouts {

    namespace {

        set<Position> scoutTargets;
        int scoutCount;
        int scoutDeadFrame = 0;
        bool proxyCheck = false;
        UnitType proxyType = UnitTypes::None;
        Position proxyPosition = Positions::Invalid;

        void updateScoutRoles()
        {
            // If we have seen an enemy Probe before we've scouted the enemy, follow it
            if (Units::getEnemyCount(UnitTypes::Protoss_Probe) == 1 && com(UnitTypes::Protoss_Zealot) < 1) {
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
                if (Strategy::getEnemyBuild() == "4Pool" && Units::getEnemyCount(UnitTypes::Zerg_Zergling) >= 2)
                    scoutCount = 0;

                // Send a 2nd scout after 1st scout
                else if (!Terrain::getEnemyStartingPosition().isValid() && mapBWEM.StartingLocations().size() == 4 && unexploredBases == 2)
                    scoutCount = 2;

                // Default to 1 scout
                else
                    scoutCount = 1;
            }

            // If we are playing PvP
            if (Players::PvP()) {

                // No scouting if we see proxy cannons
                if (Strategy::enemyProxy() && Units::getEnemyCount(UnitTypes::Protoss_Photon_Cannon) > 0)
                    scoutCount = 0;

                // Send a 2nd scout to find any proxies
                else if ((Strategy::enemyProxy() || proxyCheck) && com(UnitTypes::Protoss_Zealot) < 1)
                    scoutCount = 2;

                // Default to 1 scout
                else
                    scoutCount = 1;
            }

            // If we are playing PvT
            if (Players::PvT()) {

                // Default to 1 scout
                scoutCount = 1;
            }

            if (Broodwar->self()->getRace() == Races::Zerg && Terrain::getEnemyStartingPosition().isValid())
                scoutCount = 0;

            if (Strategy::enemyPressure() && BuildOrder::isPlayPassive())
                scoutCount = 0;

            // Assign new scouts after the last one died 10 seconds ago
            if (BWEB::Map::getNaturalChoke() && BuildOrder::shouldScout() && Units::getMyRoleCount(Role::Scout) < Scouts::getScoutCount() && Broodwar->getFrameCount() - scoutDeadFrame > 240) {
                shared_ptr<UnitInfo> scout = nullptr;

                if (BuildOrder::isProxy()) {
                    scout = Util::getFurthestUnit(Position(BWEB::Map::getNaturalChoke()->Center()), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && (!u.hasResource() || !u.getResource().getType().isRefinery()) && u.getBuildingType() == UnitTypes::None && !u.unit()->isCarryingMinerals() && !u.unit()->isCarryingGas();
                    });
                }
                else {
                    scout = Util::getClosestUnit(Position(BWEB::Map::getNaturalChoke()->Center()), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && (!u.hasResource() || !u.getResource().getType().isRefinery()) && u.getBuildingType() == UnitTypes::None && !u.unit()->isCarryingMinerals() && !u.unit()->isCarryingGas();
                    });
                }

                if (scout) {
                    scout->setRole(Role::Scout);
                    scout->setBuildingType(UnitTypes::None);
                    scout->setBuildPosition(TilePositions::Invalid);

                    if (scout->hasResource())
                        Workers::removeUnit(*scout);
                }
            }
            else if (Units::getMyRoleCount(Role::Scout) > Scouts::getScoutCount()) {

                // Look at scout targets and find the least useful scout, remove it
                auto &scout = Util::getClosestUnitGround(BWEB::Map::getMainPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Scout;
                });
                if (scout)
                    scout->setRole(Role::Worker);

            }
        }

        void updateScoutTargets()
        {
            scoutTargets.clear();
            proxyPosition = Positions::Invalid;

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
                if (scoutCount == 1)
                    return true;

                auto &closestScout = Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Scout;
                });
                return unit.shared_from_this() == closestScout;
            };

            if (!BuildOrder::firstReady() || BuildOrder::isOpener() || !Terrain::getEnemyStartingPosition().isValid()) {

                // If it's a center of map proxy
                if ((proxyPosition.isValid() && isClosestAvailableScout(proxyPosition)) || (proxyCheck && isClosestAvailableScout(BWEB::Map::getMainPosition()))) {

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

                    auto enemyWorkerClose = enemyWorker && enemyWorker->getPosition().getDistance(BWEB::Map::getMainPosition()) < 1280.0;
                    auto enemyWorkerConstructing = enemyWorker && enemyStructure && enemyWorker->getPosition().getDistance(enemyStructure->getPosition()) < 128.0;
                    auto enemyStructureProxy = enemyStructure && !Terrain::isInEnemyTerritory(enemyStructure->getTilePosition());

                    // Attempt to kill the worker if we find it - TODO: Check if the Attack command takes care of this
                    if (Strategy::getEnemyBuild() != "2Gate" && (enemyWorkerClose || enemyWorkerConstructing))
                        unit.setDestination(enemyWorker->getPosition());
                    else if (enemyStructureProxy)
                        unit.setDestination(enemyStructure->getPosition());
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

                if (!unit.getDestination().isValid() && Terrain::getEnemyStartingPosition().isValid())
                    unit.setDestination(Terrain::getEnemyStartingPosition());
            }

            // Make sure we always do something
            else if (!unit.getDestination().isValid())
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
                        if (time < best && isClosestAvailableScout(Position(base.Location()))) {
                            best = time;
                            unit.setDestination(Position(base.Location()));
                        }
                    }
                }
            }
        }

        void updatePath(UnitInfo& unit)
        {
            BWEB::Path newPath;
            newPath.createUnitPath(unit.getPosition(), unit.getDestination());
            unit.setAttackPath(newPath);

            auto newDestination = Util::findPointOnPath(unit.getAttackPath(), [&](Position p) {
                return p.getDistance(unit.getPosition()) >= 64.0;
            });

            if (newDestination.isValid())
                unit.setDestination(newDestination);
        }

        constexpr tuple commands{ Command::attack, Command::kite, Command::hunt/*, Command::move*/ };
        void updateDecision(UnitInfo& unit)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Attack"),
                make_pair(1, "Kite"),
                make_pair(2, "Explore"),
                //make_pair(3, "Move"),
            };

            // Gas steal tester
            if (Broodwar->self()->getName() == "McRaveGasSteal" && Terrain::foundEnemy()) {
                auto gas = Broodwar->getClosestUnit(Terrain::getEnemyStartingPosition(), Filter::GetType == UnitTypes::Resource_Vespene_Geyser);
                Broodwar->drawLineMap(gas->getPosition(), unit.getPosition(), Colors::Red);
                if (gas && gas->exists() && gas->getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 320 && unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 160) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Build)
                        unit.unit()->build(Broodwar->self()->getRace().getRefinery(), gas->getTilePosition());
                    return;
                }
                unit.unit()->move(gas->getPosition());
            }

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
                    updatePath(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateScoutTargets();
        updateScoutRoles();
        updateScouts();
        Visuals::endPerfTest("Scouts");
    }

    void removeUnit(UnitInfo& unit)
    {
        scoutDeadFrame = Broodwar->getFrameCount();
    }

    int getScoutCount() { return scoutCount; }
}