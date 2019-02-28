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

        void misc()
        {
            scoutCount = 1;

            // If we have seen an enemy Probe before we've scouted the enemy, follow it
            if (Units::getEnemyCount(UnitTypes::Protoss_Probe) == 1 && Units::getEnemyCount(UnitTypes::Protoss_Zealot) == 0) {
                auto w = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType() == UnitTypes::Protoss_Probe;
                });
                proxyCheck = (w && !Terrain::getEnemyStartingPosition().isValid() && w->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1);
            }
            else
                proxyCheck = false;

            // If we know a proxy possibly exists, we need a second scout
            auto foundProxyGates = Strategy::enemyProxy() && Strategy::getEnemyBuild() == "2Gate" && Units::getEnemyCount(UnitTypes::Protoss_Gateway) > 0;
            if ((Strategy::enemyProxy() || proxyCheck || foundProxyGates) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1)
                scoutCount++;

            if (Broodwar->self()->getRace() == Races::Zerg && Terrain::getEnemyStartingPosition().isValid())
                scoutCount = 0;
            if (Strategy::getEnemyBuild() == "4Pool")
                scoutCount = 0;
            if (Strategy::enemyPressure() && BuildOrder::isPlayPassive())
                scoutCount = 0;
        }

        void updateScoutTargets()
        {
            scoutTargets.clear();

            // If enemy start is valid and explored, add a target to the most recent one to scout
            if (Terrain::foundEnemy()) {
                for (auto &s : Stations::getEnemyStations()) {
                    auto &station = *s.second;
                    TilePosition tile(station.getBWEMBase()->Center());
                    if (tile.isValid())
                        scoutTargets.insert(Position(tile));
                }
                if (Players::vZ() && Stations::getEnemyStations().size() == 1 && Strategy::getEnemyBuild() != "Unknown")
                    scoutTargets.insert((Position)Terrain::getEnemyExpand());
            }

            // If we know where it is but it isn't explored
            else if (Terrain::getEnemyStartingTilePosition().isValid())
                scoutTargets.insert(Terrain::getEnemyStartingPosition());

            // If we have no idea where the enemy is
            else if (!Terrain::getEnemyStartingTilePosition().isValid()) {
                double best = DBL_MAX;
                Position pos = Positions::Invalid;
                int basesExplored = 0;
                for (auto &tile : mapBWEM.StartingLocations()) {
                    Position center = Position(tile) + Position(64, 48);
                    double dist = center.getDistance(BWEB::Map::getMainPosition());
                    if (Broodwar->isExplored(tile))
                        basesExplored++;

                    if (!Broodwar->isExplored(tile))
                        scoutTargets.insert(center);
                }

                // Scout the middle for a proxy if it's walkable
                if (basesExplored == 1 && !Players::vZ() && !Broodwar->isExplored((TilePosition)mapBWEM.Center()) && BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), mapBWEM.Center()) != DBL_MAX)
                    scoutTargets.insert(mapBWEM.Center());
            }

            // If it's a proxy, scout for the proxy building
            if (Strategy::enemyProxy()) {
                auto proxyType = Players::vP() ? UnitTypes::Protoss_Pylon : UnitTypes::Terran_Barracks;
                if (Units::getEnemyCount(proxyType) == 0) {
                    scoutTargets.insert(mapBWEM.Center());
                    proxyPosition = mapBWEM.Center();
                }
                else {
                    auto proxyBuilding = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Enemy, [&](auto &u) {
                        return u.getType() == proxyType;
                    });
                    if (proxyBuilding) {
                        scoutTargets.insert(proxyBuilding->getPosition());
                        proxyPosition = proxyBuilding->getPosition();
                    }
                }
            }

            // If it's a cannon rush, scout the main
            if (Strategy::getEnemyBuild() == "CannonRush")
                scoutTargets.insert(BWEB::Map::getMainPosition());
        }

        void updateAssignment(const shared_ptr<UnitInfo>& u)
        {
            auto &unit = *u;
            auto start = unit.getWalkPosition();
            auto distBest = DBL_MAX;
            auto posBest = unit.getDestination();

            const auto isClosestScout = [&](Position here) {
                auto closestScout = Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Scout;
                });
                return u == closestScout;
            };

            if (!BuildOrder::firstReady() || BuildOrder::isOpener() || !Terrain::getEnemyStartingPosition().isValid()) {

                // If it's a main or natural proxy
                if (Strategy::enemyProxy() && Strategy::getEnemyBuild() == "2Gate") {
                    auto enemyPylon = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto u) {
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
                if (isClosestScout(proxyPosition)) {

                    // If it's a proxy (maybe cannon rush), try to find the unit to kill
                    if (Strategy::enemyProxy() || proxyCheck) {

                        if (Units::getEnemyCount(UnitTypes::Terran_Barracks) > 0)
                            proxyType = UnitTypes::Terran_Barracks;
                        else if (Units::getEnemyCount(UnitTypes::Protoss_Pylon) > 0)
                            proxyType = UnitTypes::Protoss_Pylon;
                        else if (Units::getEnemyCount(UnitTypes::Protoss_Gateway) > 0)
                            proxyType = UnitTypes::Protoss_Gateway;

                        auto enemyWorker = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto u) {
                            return u.getType().isWorker();
                        });
                        auto enemyStructure = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto u) {
                            return u.getType() == proxyType;
                        });

                        auto enemyWorkerClose = enemyWorker && enemyWorker->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0;
                        auto enemyWorkerConstructing = enemyWorker && enemyStructure && enemyWorker->getPosition().getDistance(enemyStructure->getPosition()) < 128.0;
                        auto enemyStructureProxy = enemyStructure && !Terrain::isInEnemyTerritory(enemyStructure->getTilePosition());

                        // Priority on killing a worker if it's possible
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
                }

                // If we have scout targets, find the closest target
                else if (!scoutTargets.empty()) {
                    for (auto &target : scoutTargets) {
                        double dist = target.getDistance(unit.getPosition());
                        double time = 1.0 + (double(Grids::lastVisibleFrame((TilePosition)target)));
                        double timeDiff = Broodwar->getFrameCount() - time;

                        if (time < distBest && timeDiff > 500) {
                            distBest = time;
                            posBest = target;
                        }
                    }
                    if (posBest.isValid())
                        unit.setDestination(posBest);
                }

                // HACK: Make sure we always do something
                if (!unit.getDestination().isValid())
                    unit.setDestination(Terrain::getEnemyStartingPosition());
            }          

            else
            {
                int best = INT_MAX;
                for (auto &area : mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        if (area.AccessibleNeighbours().size() == 0 || Terrain::isInEnemyTerritory(base.Location()) || Terrain::isInAllyTerritory(base.Location()))
                            continue;

                        int time = Grids::lastVisibleFrame(base.Location());
                        if (time < best)
                            best = time, posBest = Position(base.Location());
                    }
                }
                if (posBest.isValid() && unit.unit()->getOrderTargetPosition() != posBest)
                    unit.setDestination(posBest);
            }
        }

        constexpr tuple commands{ Command::attack, Command::kite, Command::hunt, Command::move };
        void updateDecision(const shared_ptr<UnitInfo>& u)
        {
            auto &unit = *u;
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Attack"),
                make_pair(1, "Kite"),
                make_pair(2, "Hunt"),
                make_pair(3, "Move"),
            };

            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, u);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateScouts()
        {
            misc();

            for (auto &p : Players::getPlayers()) {
                if (!p.second.isSelf())
                    continue;

                for (auto &u : p.second.getUnits()) {
                    const shared_ptr<UnitInfo> &unit = u;
                    if (unit->getRole() == Role::Scout) {
                        updateAssignment(unit);
                        updateDecision(unit);
                    }
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateScoutTargets();
        updateScouts();
        Visuals::endPerfTest("Scouts");
    }

    int getScoutCount() { return scoutCount; }
}