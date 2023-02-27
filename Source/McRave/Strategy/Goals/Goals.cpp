#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Goals {

    namespace {

        multimap<double, BWEB::Station> stationsByDistance;

        void assignNumberToGoal(Position here, UnitType type, int count, GoalType gType = GoalType::None)
        {
            if (!here.isValid())
                return;

            for (int current = 0; current < count; current++) {

                if (type.isFlyer()) {
                    const auto closest = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                        if (gType == GoalType::Attack && u->globalRetreat())
                            return false;
                        return u->getType() == type && !u->getGoal().isValid();
                    });

                    if (closest) {
                        closest->setGoal(here);
                        closest->setGoalType(gType);
                    }
                }

                else {
                    const auto closest = Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &u) {
                        if (gType == GoalType::Attack && u->globalRetreat())
                            return false;
                        return u->getType() == type && !u->getGoal().isValid();
                    });

                    if (closest) {
                        closest->setGoal(here);
                        closest->setGoalType(gType);
                    }
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

        // HACK: Need a pred function integrated above 
        void assignWorker(Position here) {
            auto worker = Util::getClosestUnitGround(here, PlayerState::Self, [&](auto &u) {
                return Workers::canAssignToBuild(*u) || u->getPosition().getDistance(here) < 64.0;
            });
            if (worker)
                worker->setGoal(here);
        }

        void clearGoals()
        {
            for (auto &unit : Units::getUnits(PlayerState::Self)) {
                unit->setGoal(Positions::Invalid);
                unit->setGoalType(GoalType::None);
            }
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

            // Before Hydras have upgrades, defend vulnerable bases
            if (BuildOrder::isTechUnit(Zerg_Hydralisk) && Combat::State::isStaticRetreat(Zerg_Hydralisk)) {
                auto &stations = Stations::getStations(PlayerState::Self);
                if (!stations.empty()) {
                    auto mainStations = int(count_if(stations.begin(), stations.end(), [&](auto& s) { return s->isMain(); }));
                    auto percentPer = 1.0 / double(stations.size() - mainStations);
                    for (auto &station : stations) {
                        if (station->isMain())
                            continue;
                        assignPercentToGoal(Stations::getDefendPosition(station), rangedType, percentPer, GoalType::None);
                    }
                }
            }

            // Send a worker early when we want to
            if (BuildOrder::isPlanEarly() && Planning::getCurrentExpansion() && !Planning::whatPlannedHere(Planning::getCurrentExpansion()->getBase()->Location()).isResourceDepot()) {
                if (int(Stations::getStations(PlayerState::Self).size() < 2))
                    assignWorker(Terrain::getMyNatural()->getBase()->Center());
                else if (Planning::getCurrentExpansion())
                    assignWorker(Planning::getCurrentExpansion()->getBase()->Center());
            }

            // Send detector to next expansion
            if (Planning::getCurrentExpansion()) {
                auto nextExpand = Planning::getCurrentExpansion()->getBase()->Center();
                auto needDetector = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 || Players::vZ();
                if (nextExpand.isValid() && needDetector && BWEB::Map::isUsed(Planning::getCurrentExpansion()->getBase()->Location()) == UnitTypes::None) {
                    if (Stations::getStations(PlayerState::Self).size() >= 2 && BuildOrder::buildCount(base) > vis(base))
                        assignNumberToGoal(nextExpand, detector, 1);
                }

                // Escort expanders
                if (nextExpand.isValid() && (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 2 || Stations::getStations(PlayerState::Self).size() <= 1 || Spy::getEnemyTransition() == "4Gate")) {
                    auto closestBuilder = Util::getClosestUnit(nextExpand, PlayerState::Self, [&](auto &u) {
                        return u->getBuildType().isResourceDepot();
                    });
                    auto type = (vis(airType) > 0 && Broodwar->self()->getRace() == Races::Zerg) ? airType : rangedType;

                    if (closestBuilder) {
                        assignNumberToGoal(closestBuilder->getPosition(), type, 1, GoalType::Escort);
                        for (auto &t : closestBuilder->getUnitsTargetingThis()) {
                            if (auto targeter = t.lock())
                                assignNumberToGoal(targeter->getPosition(), type, 1, GoalType::Escort);
                        }
                    }
                }
            }

            // Aggresively deny enemy expansions
            stationsByDistance.clear();
            if (Terrain::getEnemyStartingPosition().isValid() && !BuildOrder::isRush() && !Players::ZvZ() && !Players::vFFA()) {

                for (auto &station : BWEB::Stations::getStations()) {

                    auto closestSelfStation = Stations::getClosestStationGround(station.getBase()->Center(), PlayerState::Self);
                    auto closestEnemyStation = Stations::getClosestStationGround(station.getBase()->Center(), PlayerState::Enemy);

                    if (!closestSelfStation || !closestEnemyStation)
                        continue;

                    if (Terrain::inTerritory(PlayerState::Enemy, station.getBase()->GetArea()))
                        continue;
                    if (station == Terrain::getEnemyNatural() || station == Terrain::getEnemyMain())
                        continue;
                    if (station == Terrain::getMyNatural() || station == Terrain::getMyMain() || (Planning::getCurrentExpansion() && station == Planning::getCurrentExpansion()))
                        continue;
                    if (Stations::ownedBy(&station) == PlayerState::Self || Stations::ownedBy(&station) == PlayerState::Enemy)
                        continue;
                    if (station.getBase()->Center().getDistance(closestSelfStation->getBase()->Center()) < station.getBase()->Center().getDistance(closestEnemyStation->getBase()->Center())
                        && BWEB::Map::getGroundDistance(station.getBase()->Center(), closestSelfStation->getBase()->Center()) < BWEB::Map::getGroundDistance(station.getBase()->Center(), closestEnemyStation->getBase()->Center()))
                        continue;

                    // Create a path and check if areas are already enemy owned
                    auto badStation = false;
                    auto path = mapBWEM.GetPath(station.getBase()->Center(), Terrain::getMainPosition());
                    for (auto &choke : path) {
                        if (Terrain::inTerritory(PlayerState::Enemy, choke->GetAreas().first) || Terrain::inTerritory(PlayerState::Enemy, choke->GetAreas().second))
                            badStation = true;
                    }
                    if (badStation)
                        continue;

                    auto dist = station.getBase()->Center().getDistance(Terrain::getEnemyStartingPosition());
                    stationsByDistance.emplace(make_pair(dist, station));
                }

                // Protoss denies closest base
                if (Players::PvT()) {
                    for (auto &station : stationsByDistance) {
                        auto type = (Players::PvT() || Players::PvP()) ? rangedType : meleeType;
                        assignNumberToGoal(station.second.getBase()->Center(), type, 2);
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
                for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                    auto pos = station->getBase()->Center();
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
            if (Stations::getStations(PlayerState::Self).size() >= 3) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                    auto pos = station->getBase()->Center();
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

            // Clear out base early game
            if (Util::getTime() < Time(4, 00) && !Spy::enemyRush() && !Spy::enemyPressure() && !Players::ZvZ() && Players::getVisibleCount(PlayerState::Enemy, Terran_Factory) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) == 0) {
                auto oldestTile = Terrain::getOldestPosition(Terrain::getMainArea());

                if (oldestTile.isValid())
                    assignNumberToGoal(oldestTile, Zerg_Zergling, 1, GoalType::Explore);
            }

            // Assign an Overlord to watch our Choke early on
            if (Terrain::getNaturalChoke() && !Spy::enemyRush()) {
                if ((Util::getTime() < Time(3, 00) && !Spy::enemyProxy() && Broodwar->getStartLocations().size() <= 2) || (Util::getTime() < Time(2, 15) && Spy::enemyProxy()) || (Players::ZvZ() && enemyStrength.airToAir <= 0.0))
                    assignNumberToGoal(Position(Terrain::getNaturalChoke()->Center()), Zerg_Overlord, 1, GoalType::Escort);
            }

            // Assign an Overlord to each Station
            for (auto &station : Stations::getStations(PlayerState::Self)) {
                auto closestSunk = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                    return u->getType() == Zerg_Sunken_Colony && Terrain::inArea(station->getBase()->GetArea(), u->getPosition());
                });
                auto closestSpore = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                    return u->getType() == Zerg_Spore_Colony && Terrain::inArea(station->getBase()->GetArea(), u->getPosition());
                });

                if (closestSpore)
                    assignNumberToGoal(closestSpore->getPosition(), Zerg_Overlord, 1, GoalType::Escort);
                else if (closestSunk)
                    assignNumberToGoal(closestSunk->getPosition(), Zerg_Overlord, 1, GoalType::Escort);
                else
                    assignNumberToGoal(station->getBase()->Center(), Zerg_Overlord, 1, GoalType::Escort);
            }

            // Assign an Overlord to each main choke
            for (auto &station : Stations::getStations(PlayerState::Self)) {
                if (station->isMain() && station->getChokepoint() && ((Players::ZvZ() && Players::getStrength(PlayerState::Enemy).airToAir == 0.0) || Stations::getStations(PlayerState::Self).size() >= 2 || Util::getTime() < Time(4, 00)))
                    assignNumberToGoal(station->getChokepoint()->Center(), Zerg_Overlord, 1, GoalType::Escort);
            }

            // Assign an Overlord to each possible expansion the enemy can take
            if (Broodwar->self()->getRace() == Races::Zerg && Util::getTime() > Time(6, 00) && vis(Zerg_Overlord) >= 8) {
                int i = vis(Zerg_Overlord) - 8;
                for (auto &[dist, station] : stationsByDistance) {
                    auto notScoutedRecently = (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(TilePosition(station.getBase()->Center())) > 1440);
                    assignNumberToGoal(station.getResourceCentroid(), Zerg_Overlord, 1, GoalType::Explore);
                    i--;
                    if (i <= 0)
                        break;
                }
            }

            // Attack enemy expansions with a small force
            if (Util::getTime() > Time(6, 00) || Spy::enemyProxy()) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &station : Stations::getStations(PlayerState::Enemy)) {

                    if (!Spy::enemyProxy()) {
                        if (station == Terrain::getEnemyNatural()
                            || station == Terrain::getEnemyMain())
                            continue;
                    }

                    if (Terrain::getEnemyMain() && station != Terrain::getEnemyMain() && Terrain::getEnemyNatural() && station != Terrain::getEnemyNatural()) {
                        if (Stations::getAirDefenseCount(station) == 0 && Players::ZvP())
                            assignNumberToGoal(station->getBase()->Center(), Zerg_Mutalisk, 1, GoalType::Attack);
                    }

                    auto pos = station->getBase()->Center();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }
                if (posBest.isValid()) {
                    assignPercentToGoal(posBest, Zerg_Zergling, 1.00, GoalType::Attack);
                    if (Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Grooved_Spines) && Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Muscular_Augments))
                        assignPercentToGoal(posBest, Zerg_Hydralisk, 0.50, GoalType::Attack);
                }
            }

            // Send a Zergling to a low energy Defiler
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getType() == Zerg_Defiler && unit.getEnergy() < 150)
                    assignNumberToGoal(unit.getTilePosition(), Zerg_Zergling, 1, GoalType::Attack);
            }
        }

        void updateCampaignGoals()
        {
            // Future stuff
            if (Broodwar->getGameType() == GameTypes::Use_Map_Settings && !Broodwar->isMultiplayer()) {

            }
        }
    }

    void onFrame()
    {
        clearGoals();
        updateGenericGoals();
        updateProtossGoals();
        updateTerranGoals();
        updateZergGoals();
        updateCampaignGoals();
    }
}