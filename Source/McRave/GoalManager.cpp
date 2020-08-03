#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave {
    enum class GoalType {
        None, Contain, Explore, Escort // ...possible future improvements here for better goal positioning
    };
}

namespace McRave::Goals {

    namespace {

        map<Position, int> goalFrameUpdate;

        void assignNumberToGoal(Position here, UnitType type, int count, GoalType gType = GoalType::None)
        {
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

                if (closest)
                    closest->setGoal(here);
            }
        }

        void assignPercentToGoal(Position here, UnitType type, double percent, GoalType gType = GoalType::None)
        {
            // Clamp between 1 and 4
            int count = int(percent * double(vis(type)));
            assignNumberToGoal(here, type, count, gType);
        }

        void updateProtossGoals()
        {
            if (Broodwar->self()->getRace() != Races::Protoss)
                return;

            map<UnitType, int> unitTypes;
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myRace = Broodwar->self()->getRace();

            // Defend my expansions
            if (enemyStrength.groundToGround > 0) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = *s.second;

                    if (station.getBase()->Location() != BWEB::Map::getNaturalTile() && station.getBase()->Location() != BWEB::Map::getMainTile() && station.getGroundDefenseCount() == 0)
                        assignNumberToGoal(station.getResourceCentroid(), Protoss_Dragoon, 2, GoalType::Contain);
                }
            }

            // Attack enemy expansions with a small force
            // PvP / PvT
            if (Players::vP() || Players::vT()) {
                auto distBest = DBL_MAX;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBase()->Center();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist < distBest) {
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
            // PvE
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

            // Secure our own future expansion position
            // PvE
            Position nextExpand(Buildings::getCurrentExpansion());
            if (nextExpand.isValid()) {

                auto building = Broodwar->self()->getRace().getResourceDepot();

                if (vis(building) >= 2 && BuildOrder::buildCount(building) > vis(building)) {
                    if (Players::vZ())
                        assignPercentToGoal(nextExpand, Protoss_Zealot, 0.15);
                    else {
                        assignPercentToGoal(nextExpand, Protoss_Dragoon, 0.25);
                    }

                    assignNumberToGoal(nextExpand, Protoss_Observer, 1);
                }
            }

            // Escort shuttles
            // PvZ
            if (Players::vZ() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    UnitInfo &unit = *u;
                    if (unit.getRole() == Role::Transport)
                        assignPercentToGoal(unit.getPosition(), Protoss_Corsair, 0.25);
                }
            }

            // Deny enemy expansions
            // PvT
            if (Stations::getMyStations().size() >= 3 && Terrain::getEnemyExpand().isValid() && Terrain::getEnemyExpand() != Buildings::getCurrentExpansion() && BWEB::Map::isUsed(Terrain::getEnemyExpand()) == None) {
                if (Players::vT() || Players::vP())
                    assignPercentToGoal((Position)Terrain::getEnemyExpand(), Protoss_Dragoon, 0.15);
                else
                    assignNumberToGoal((Position)Terrain::getEnemyExpand(), Protoss_Dark_Templar, 1);
            }

            // Defend our natural versus 2Fact
            // PvT
            if (Players::vT() && Strategy::enemyPressure() && !BuildOrder::isPlayPassive() && Broodwar->getFrameCount() < 15000) {
                assignNumberToGoal(BWEB::Map::getNaturalPosition(), Protoss_Dragoon, 2);
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
            Position nextExpand(Buildings::getCurrentExpansion());

            // Send detector to next expansion
            if (nextExpand.isValid()) {
                auto hatchCount = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);
                if (hatchCount >= 2 && BuildOrder::buildCount(Zerg_Hatchery) > vis(Zerg_Hatchery))
                    assignNumberToGoal(nextExpand, Zerg_Overlord, 1);
            }

            // Escort building worker
            if (nextExpand.isValid() && Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0) {
                auto closestBuilder = Util::getClosestUnit(nextExpand, PlayerState::Self, [&](auto &u) {
                    return u.getBuildType().isResourceDepot();
                });

                if (closestBuilder) {
                    if (closestBuilder->hasTarget())
                        assignNumberToGoal(closestBuilder->getTarget().getPosition(), Zerg_Mutalisk, 1, GoalType::Escort);
                    assignNumberToGoal(closestBuilder->getPosition(), Zerg_Mutalisk, 4, GoalType::Escort);
                }
            }

            // Scout chokepoints between army movements
            if (Util::getTime() < Time(4, 0) && Terrain::getEnemyStartingPosition().isValid() && Stations::getMyStations().size() >= 3) {
                set<Position> chokesToGuard;

                for (auto &area : Terrain::getEnemyTerritory()) {
                    for (auto &enemyAreaPair : area->ChokePointsByArea()) {

                        // Territory must be neutral
                        if (Terrain::isInEnemyTerritory(enemyAreaPair.first)
                            || Terrain::isInAllyTerritory(enemyAreaPair.first))
                            continue;

                        // Grab each pair of the neutral territory
                        for (auto &neutralAreaPair : enemyAreaPair.first->ChokePointsByArea()) {

                            // Territory must be neutral
                            if (Terrain::isInEnemyTerritory(neutralAreaPair.first) || Terrain::isInAllyTerritory(neutralAreaPair.first))
                                continue;

                            for (auto &choke : *(neutralAreaPair.second)) {
                                if (!choke.BlockingNeutral() && !choke.Blocked()) {
                                    chokesToGuard.insert(Position(choke.Center()));
                                }
                            }
                        }

                    }
                }

                // Guard each choke with 1 ling
                for (auto &choke : chokesToGuard)
                    assignNumberToGoal(choke, Zerg_Zergling, 1);
            }

            // Aggresively deny enemy expansions
            if (Util::getTime() > Time(4, 0) && !BuildOrder::isRush() && BuildOrder::isTechUnit(Zerg_Mutalisk) && !Players::ZvZ()) {
                multimap<double, BWEB::Station> stationsByDistance;
                for (auto &station : BWEB::Stations::getStations()) {
                    if (Terrain::isInEnemyTerritory(station.getBase()->GetArea()))
                        continue;
                    if (station == Terrain::getEnemyNatural() || station == Terrain::getEnemyMain())
                        continue;
                    if (station == Terrain::getMyNatural() || station == Terrain::getMyMain())
                        continue;
                    if (Stations::ownedBy(&station) == PlayerState::Self || Stations::ownedBy(&station) == PlayerState::Enemy)
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

                    auto dist = station.getBase()->Center().getDistance(Terrain::getEnemyStartingPosition()) * (1 + int(station.getBase()->Geysers().empty()));
                    stationsByDistance.emplace(make_pair(dist, station));
                }

                for (auto &station : stationsByDistance)
                    assignNumberToGoal(station.second.getBase()->Center(), Zerg_Zergling, 1);                
            }

            // Deny enemy expansions
            if (Util::getTime() > Time(8, 0)) {
                if (Stations::getMyStations().size() >= 3 && Stations::getMyStations().size() >= Stations::getEnemyStations().size() && !BuildOrder::isPlayPassive() && Terrain::getEnemyExpand().isValid() && Terrain::getEnemyExpand() != Buildings::getCurrentExpansion() && BWEB::Map::isUsed(Terrain::getEnemyExpand()) == None)
                    assignNumberToGoal((Position)Terrain::getEnemyExpand(), Zerg_Zergling, 4);

                // Attack enemy expansions with a small force            
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getResourceCentroid();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }
                assignPercentToGoal(posBest, Zerg_Zergling, 0.50);
            }

            // Defend my expansions
            if (Util::getTime() > Time(6, 0) && enemyStrength.groundToGround > 0) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = *s.second;

                    auto closestEnemy = Util::getClosestUnit(station.getBase()->Center(), PlayerState::Enemy, [&](auto &u) {
                        return u.canAttackGround() && !u.getType().isWorker();
                    });

                    auto closestWall = BWEB::Walls::getClosestWall(station.getBase()->Location());
                    if (closestWall && closestWall->getArea() == station.getBase()->GetArea() && closestWall->getGroundDefenseCount() > 0)
                        continue;

                    if (!station.isMain() && station.getGroundDefenseCount() == 0)
                        assignNumberToGoal(station.getResourceCentroid(), Zerg_Zergling, 6, GoalType::Contain);
                    if (closestEnemy && closestEnemy->getPosition().getDistance(station.getBase()->Center()) < 480.0)
                        assignNumberToGoal(station.getResourceCentroid(), Zerg_Mutalisk, 6, GoalType::Contain);
                }
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
        updateProtossGoals();
        updateTerranGoals();
        updateZergGoals();
    }
}