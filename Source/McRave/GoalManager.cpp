#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave {

}

namespace McRave::Goals {

    namespace {

        void assignNumberToGoal(Position here, UnitType type, int count, GoalType gType = GoalType::None)
        {
            if (!here.isValid())
                return;

            for (int current = 0; current < count; current++) {
                shared_ptr<UnitInfo> closest;
                if (type.isFlyer())
                    closest = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                    return u.getType() == type && !u.getGoal().isValid();
                });
                else {
                    closest = Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &u) {
                        return u.getType() == type && !u.getGoal().isValid();
                    });
                }

                if (closest) {
                    closest->setGoal(here);
                    closest->setGoalType(gType);
                }
            }
        }

        void assignNumberToGoal(WalkPosition here, UnitType type, int count, GoalType gType = GoalType::None)
        {
            assignNumberToGoal(Position(here) + Position(4, 4), type, count, gType);
        }

        void assignNumberToGoal(TilePosition here, UnitType type, int count, GoalType gType = GoalType::None)
        {
            assignNumberToGoal(Position(here) + Position(16, 16), type, count, gType);
        }

        void assignPercentToGoal(Position here, UnitType type, double percent, GoalType gType = GoalType::None)
        {
            int count = int(percent * double(vis(type)));
            assignNumberToGoal(here, type, count, gType);
        }

        void assignPercentToGoal(WalkPosition here, UnitType type, double percent, GoalType gType = GoalType::None)
        {
            assignPercentToGoal(Position(here) + Position(4, 4), type, percent, gType);
        }

        void assignPercentToGoal(TilePosition here, UnitType type, double percent, GoalType gType = GoalType::None)
        {
            assignPercentToGoal(Position(here) + Position(16, 16), type, percent, gType);
        }

        void updateGenericGoals()
        {
            auto rangedType = UnitTypes::None;
            auto meleeType = UnitTypes::None;
            auto airType = UnitTypes::None;
            auto detector = UnitTypes::None;
            auto base = UnitTypes::None;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                rangedType = Protoss_Dragoon;
                meleeType = Protoss_Zealot;
                airType = Protoss_Corsair;
                detector = Protoss_Observer;
                base = Protoss_Nexus;
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                rangedType = Zerg_Hydralisk;
                meleeType = Zerg_Zergling;
                airType = Zerg_Mutalisk;
                detector = Zerg_Overlord;
                base = Zerg_Hatchery;
            }
            if (Broodwar->self()->getRace() == Races::Terran) {
                rangedType = Terran_Vulture;
                meleeType = Terran_Firebat;
                airType = Terran_Wraith;
                detector = Terran_Science_Vessel;
                base = Terran_Command_Center;
            }

            // Clear out base early game
            if (Util::getTime() < Time(5, 00) && !Strategy::enemyRush() && !Strategy::enemyPressure() && !Players::ZvZ()) {
                auto oldestTile = Terrain::getOldestPosition(BWEB::Map::getMainArea());

                if (oldestTile.isValid())
                    assignNumberToGoal(oldestTile, Zerg_Zergling, 1, GoalType::Explore);
            }

            // Defend my expansions
            if (Stations::getMyStations().size() >= 3) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = *s.second;

                    if (station.getGroundDefenseCount() < 2) {

                        if (Broodwar->self()->getRace() == Races::Zerg && (station.isMain() || station.isNatural()))
                            continue;

                        // TODO: Removed GoalType::Defend due to lag
                        // If it's a main, defend at the natural
                        if (station.isMain()) {
                            auto closestNatural = BWEB::Stations::getClosestNaturalStation(station.getBase()->Location());
                            if (closestNatural && closestNatural->getBase()->Center()) {
                                assignNumberToGoal(closestNatural->getBase()->Center(), rangedType, 2, GoalType::None);
                                continue;
                            }
                        }

                        // TODO: Removed GoalType::Defend due to lag
                        // Otherwise defend at this base
                        assignNumberToGoal(station.getBase()->Center(), rangedType, max(0, 2 - station.getGroundDefenseCount()), GoalType::None);
                    }
                }
            }

            // Send detector to next expansion
            auto nextExpand = Position(Buildings::getCurrentExpansion()) + Position(64, 48);
            auto needDetector = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 || Players::ZvZ();
            if (nextExpand.isValid() && needDetector && BWEB::Map::isUsed(Buildings::getCurrentExpansion()) == UnitTypes::None) {
                if (Stations::getMyStations().size() >= 2 && BuildOrder::buildCount(base) > vis(base))
                    assignNumberToGoal(nextExpand, detector, 1);
            }

            // Escort expanders
            if (nextExpand.isValid() && !Players::ZvP() && !Players::ZvZ() && (!Players::ZvT() || Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) >= 2)) {
                auto closestBuilder = Util::getClosestUnit(nextExpand, PlayerState::Self, [&](auto &u) {
                    return u.getBuildType().isResourceDepot();
                });

                auto type = (vis(airType) > 0 && Broodwar->self()->getRace() == Races::Zerg) ? airType : rangedType;

                if (closestBuilder) {
                    if (closestBuilder->hasTarget())
                        assignNumberToGoal(closestBuilder->getTarget().getPosition(), type, 1, GoalType::Escort);
                    assignNumberToGoal(closestBuilder->getPosition(), type, 4, GoalType::Escort);
                }
            }

            // Aggresively deny enemy expansions
            if (int(Stations::getEnemyStations().size()) >= 2 && Terrain::getEnemyStartingPosition().isValid() && Util::getTime() > Time(5, 00) && !BuildOrder::isRush() && !Players::ZvZ()) {
                multimap<double, BWEB::Station> stationsByDistance;
                int multi = Broodwar->self()->getRace() == Races::Zerg ? 2 : 1;

                for (auto &station : BWEB::Stations::getStations()) {

                    auto closestSelfStation = Stations::getClosestStation(PlayerState::Self, station.getBase()->Center());
                    auto closestEnemyStation = Stations::getClosestStation(PlayerState::Enemy, station.getBase()->Center());

                    if (!closestSelfStation || !closestEnemyStation)
                        continue;

                    if (Terrain::isInEnemyTerritory(station.getBase()->GetArea()))
                        continue;
                    if (station == Terrain::getEnemyNatural() || station == Terrain::getEnemyMain())
                        continue;
                    if (station == Terrain::getMyNatural() || station == Terrain::getMyMain())
                        continue;
                    if (Stations::ownedBy(&station) == PlayerState::Self || Stations::ownedBy(&station) == PlayerState::Enemy)
                        continue;
                    if (station.getBase()->Center().getDistance(closestSelfStation->getBase()->Center()) < station.getBase()->Center().getDistance(closestEnemyStation->getBase()->Center())
                        && BWEB::Map::getGroundDistance(station.getBase()->Center(), closestSelfStation->getBase()->Center()) < BWEB::Map::getGroundDistance(station.getBase()->Center(), closestEnemyStation->getBase()->Center()))
                        continue;

                    // Create a path and check if areas are already enemy owned
                    auto badStation = false;
                    auto path = mapBWEM.GetPath(station.getBase()->Center(), BWEB::Map::getMainPosition());
                    for (auto &choke : path) {
                        if (Terrain::isInEnemyTerritory(choke->GetAreas().first) || Terrain::isInEnemyTerritory(choke->GetAreas().second))
                            badStation = true;
                    }
                    if (badStation)
                        continue;

                    auto dist = station.getBase()->Center().getDistance(Terrain::getEnemyStartingPosition());
                    stationsByDistance.emplace(make_pair(dist, station));
                }

                // Zerg deny every base
                if (Broodwar->self()->getRace() == Races::Zerg) {
                    int i = 0;
                    for (auto &station : stationsByDistance) {
                        auto type = meleeType;
                        assignNumberToGoal(station.second.getBase()->Center(), type, 1);
                        i++;
                    }
                }

                // Terran plant a mine at every base
                // TODO

                // Protoss denies closest base
                if (Players::PvT()) {
                    int i = 0;
                    for (auto &station : stationsByDistance) {
                        auto type = (Players::PvT() || Players::PvP()) ? rangedType : meleeType;
                        assignNumberToGoal(station.second.getBase()->Center(), type, 2);
                        i++;
                        if (i > 1)
                            break;
                    }
                }
            }

        }

        void updateProtossGoals()
        {
            if (Broodwar->self()->getRace() != Races::Protoss)
                return;

            map<UnitType, int> unitTypes;
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myRace = Broodwar->self()->getRace();

            // Attack enemy expansions with a small force
            if (Players::vP() || Players::vT()) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBase()->Center();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }

                if (Players::vP())
                    assignPercentToGoal(posBest, Protoss_Dragoon, 0.15);
                else
                    assignPercentToGoal(posBest, Protoss_Zealot, 0.15);
            }

            // Send a DT / Zealot + Goon squad to enemys furthest station
            if (Stations::getMyStations().size() >= 2) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBase()->Center();
                    auto dist = pos.getDistance(Terrain::getEnemyStartingPosition());

                    if (dist > distBest) {
                        posBest = pos;
                        distBest = dist;
                    }
                }

                if (Actions::overlapsDetection(nullptr, posBest, PlayerState::Enemy) || vis(Protoss_Dark_Templar) == 0) {
                    if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) > 0)
                        assignNumberToGoal(posBest, Protoss_Zealot, 4);
                    else
                        assignNumberToGoal(posBest, Protoss_Dragoon, 4);
                }
                else
                    assignNumberToGoal(posBest, Protoss_Dark_Templar, vis(Protoss_Dark_Templar));
            }

            // Escort shuttles
            if (Players::vZ() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    UnitInfo &unit = *u;
                    if (unit.getRole() == Role::Transport)
                        assignPercentToGoal(unit.getPosition(), Protoss_Corsair, 0.25);
                }
            }
        }

        void updateTerranGoals()
        {

        }

        void updateZergGoals()
        {
            if (Broodwar->self()->getRace() != Races::Zerg)
                return;

            auto enemyStrength = Players::getStrength(PlayerState::Enemy);

            // Assign an Overlord to watch our Choke early on
            if ((Util::getTime() < Time(3, 00) && !Strategy::enemyProxy()) || (Util::getTime() < Time(2, 15) && Strategy::enemyProxy()) || (Players::ZvZ() && enemyStrength.airToAir <= 0.0))
                assignNumberToGoal(Position(BWEB::Map::getNaturalChoke()->Center()), Zerg_Overlord, 1, GoalType::Escort);

            // Assign an Overlord to each Station
            for (auto &[_, station] : Stations::getMyStations()) {
                auto closestSunk = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                    return u.getType() == Zerg_Sunken_Colony && station->getDefenseLocations().find(u.getTilePosition()) != station->getDefenseLocations().end();
                });
                auto closestSpore = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                    return u.getType() == Zerg_Spore_Colony && station->getDefenseLocations().find(u.getTilePosition()) != station->getDefenseLocations().end();
                });

                if (closestSpore)
                    assignNumberToGoal(closestSpore->getPosition(), Zerg_Overlord, 1, GoalType::Escort);
                else if (closestSunk)
                    assignNumberToGoal(closestSunk->getPosition(), Zerg_Overlord, 1, GoalType::Escort);
                else
                    assignNumberToGoal(station->getBase()->Center(), Zerg_Overlord, 1, GoalType::Escort);
            }

            // Assign an Overlord to each Wall
            if (!Players::vT()) {
                for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                    if (!Terrain::isInAllyTerritory(wall.getArea()))
                        continue;

                    auto closestSunk = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                        return u.getType() == Zerg_Sunken_Colony && wall.getDefenses().find(u.getTilePosition()) != wall.getDefenses().end();
                    });

                    if (closestSunk)
                        assignNumberToGoal(closestSunk->getPosition(), Zerg_Overlord, 1, GoalType::Escort);
                    else
                        assignNumberToGoal(wall.getCentroid(), Zerg_Overlord, 1, GoalType::Escort);
                }
            }

            // Attack enemy expansions with a small force         
            if (Util::getTime() > Time(8, 0)) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;

                    if (station == Terrain::getEnemyNatural()
                        || station == Terrain::getEnemyMain())
                        continue;

                    auto pos = station.getResourceCentroid();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }
                if (posBest.isValid())
                    assignPercentToGoal(posBest, Zerg_Zergling, 0.50);
            }

            // Assign Scourge to watch for drops coming in
            if (Players::ZvT() && Util::getTime() > Time(10, 00)) {
                vector<Position> validSecondaryNodes;
                Position firstNode = Positions::Invalid;
                Position secondNode = Positions::Invalid;

                auto playableWidth = Broodwar->mapWidth() * 32 - 320;
                auto playableHeight = Broodwar->mapHeight() * 32 - 320;
                auto widthIncrement = playableWidth / 10;
                auto heightIncrement = playableHeight / 10;
                vector<Position> nodes;

                for (int x = 160; x < Broodwar->mapWidth() * 32 - 160; x+=widthIncrement) {
                    nodes.push_back(Position(x, 160));
                    nodes.push_back(Position(x, Broodwar->mapHeight() * 32 - 160));
                }
                for (int y = 160; y < Broodwar->mapHeight() * 32 - 160; y+=heightIncrement) {
                    nodes.push_back(Position(160, y));
                    nodes.push_back(Position(Broodwar->mapWidth() * 32 - 160, y));
                }
                sort(nodes.begin(), nodes.end(), [&](auto &l, auto &r) { return l.getDistance(Terrain::getEnemyStartingPosition()) < r.getDistance(Terrain::getEnemyStartingPosition()); });

                int i = 0;
                for (auto &pos : nodes) {

                    auto closestEnemyStation = Stations::getClosestStation(PlayerState::Enemy, pos);
                    auto closestSelfStation = Stations::getClosestStation(PlayerState::Self, pos);

                    if (!closestEnemyStation
                        || !closestSelfStation
                        || pos.getDistance(closestEnemyStation->getBase()->Center()) < pos.getDistance(closestSelfStation->getBase()->Center())
                        || Terrain::isInEnemyTerritory(TilePosition(pos)))
                        continue;

                    validSecondaryNodes.push_back(pos);
                    Broodwar->drawTextMap(pos, "%d", i);
                    Broodwar->drawCircleMap(pos, 8, Colors::Yellow);
                    i++;
                }

                // First node is automatically the closest to the enemy
                if (!validSecondaryNodes.empty()) {
                    firstNode = validSecondaryNodes.front();
                    Broodwar->drawCircleMap(firstNode, 4, Colors::Red);
                }

                // Second node is furthest from first node
                if (!validSecondaryNodes.empty()) {
                    sort(validSecondaryNodes.begin(), validSecondaryNodes.end(), [&](auto &l, auto &r) { return l.getDistance(firstNode) > r.getDistance(firstNode); });
                    secondNode = validSecondaryNodes.front();
                    Broodwar->drawCircleMap(secondNode, 4, Colors::Green);
                }

                if (firstNode.isValid())
                    assignNumberToGoal(firstNode, Zerg_Scourge, 2, GoalType::Contain);
                if (secondNode.isValid())
                    assignNumberToGoal(secondNode, Zerg_Scourge, 2, GoalType::Contain);
            }

            // Send an Overlord to last visited expansion to check
            if (Util::getTime() > Time(8, 00) && false) {
                auto minTimeDiff = 1000;
                auto desiredCount = com(Zerg_Overlord) - int(Stations::getMyStations().size());
                auto count = 0;

                for (auto &base : Terrain::getAllBases()) {
                    auto dist = base->Center().getDistance(BWEB::Map::getMainPosition());
                    auto time = 1 + Grids::lastVisibleFrame((TilePosition)base->Center());
                    auto timeDiff = max(Broodwar->getFrameCount(), 2 * minTimeDiff) - time;

                    if (timeDiff > minTimeDiff && BWEB::Map::isUsed(TilePosition(base->Center())) == UnitTypes::None) {
                        assignNumberToGoal(base->Center(), Zerg_Overlord, 1, GoalType::Explore);
                        count++;
                    }

                    if (count >= desiredCount)
                        break;
                }
            }
        }
    }

    void onFrame()
    {
        updateGenericGoals();
        updateProtossGoals();
        updateTerranGoals();
        updateZergGoals();
    }
}