#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Goals {

    namespace {

        multimap<double, BWEB::Station> stationsByDistance;
        UnitType rangedType = UnitTypes::None;
        UnitType meleeType = UnitTypes::None;
        UnitType airType = UnitTypes::None;
        UnitType detector = UnitTypes::None;
        UnitType base = UnitTypes::None;

        map<UnitInfo*, pair<Position, GoalType>> oldGoals;

        struct GoalTarget {
            Position center = Positions::Invalid;
            GoalType gtype;
            map<UnitType, int> currentTypeCounts;
            map<UnitType, int> desiredTypeCounts;

            GoalTarget() {};
        };

        template <class T>
        void assignNumberToGoal(T t, UnitType type, int count, GoalType gType = GoalType::None)
        {
            const auto here = Position(t);
            if (!here.isValid())
                return;

            // Check for a cached assignment
            int countCached = 0;
            for (int current = 0; current < count; current++) {
                for (auto &[unit, goalPair] : oldGoals) {
                    if (!unit->getGoal().isValid() && unit->getRole() != Role::Scout && unit->getType() == type && goalPair.first == here && goalPair.second == gType) {
                        unit->setGoal(here);
                        unit->setGoalType(gType);
                        countCached++;
                        break;
                    }
                }
            }

            // Get new assignment
            count -= countCached;
            for (int current = 0; current < count; current++) {
                if (type.isFlyer()) {
                    const auto closest = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                        if (gType == GoalType::Attack && Combat::State::isStaticRetreat(type))
                            return false;
                        if (u->getRole() == Role::Scout)
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
                        if (gType == GoalType::Attack && Combat::State::isStaticRetreat(type))
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

        template <class T>
        void assignPercentToGoal(T t, UnitType type, double percent, GoalType gType = GoalType::None)
        {
            const auto here = Position(t);
            const auto count = int(percent * double(com(type)));
            assignNumberToGoal(here, type, count, gType);
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
            oldGoals.clear();
            for (auto &unit : Units::getUnits(PlayerState::Self)) {
                oldGoals[&*unit] = make_pair(unit->getGoal(), unit->getGoalType());
                if (unit->getGoalType() != GoalType::Harass) {
                    unit->setGoal(Positions::Invalid);
                    unit->setGoalType(GoalType::None);
                }
            }
        }

        void updateExpandDenial()
        {
            // Deny enemy expansions from being built by sending a small force ahead of time
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
                        auto type = (Players::PvT() || Players::PvP()) ? Protoss_Dragoon : Protoss_Zealot;
                        assignNumberToGoal(station.second.getBase()->Center(), type, 2);
                        break;
                    }
                }

                // Terran a vulture/marine?
            }
        }

        void updateGenericGoals()
        {
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
                if (nextExpand.isValid() && BuildOrder::shouldExpand()) {
                    auto closestBuilder = Util::getClosestUnit(nextExpand, PlayerState::Self, [&](auto &u) {
                        return u->getBuildType().isResourceDepot();
                    });
                    auto type = (vis(airType) > 0 && Broodwar->self()->getRace() == Races::Zerg) ? airType : rangedType;

                    if (closestBuilder && !closestBuilder->isWithinBuildRange()) {
                        for (auto &unit : Units::getUnits(PlayerState::Enemy)) {
                            if (!unit->isSuicidal() && unit->getPosition().getDistance(closestBuilder->getPosition()) < 640.0)
                                assignNumberToGoal(unit->getPosition(), type, 1, GoalType::Escort);
                        }
                        assignPercentToGoal(closestBuilder->getPosition(), type, 1.0, GoalType::Escort);
                    }
                    else {
                        for (auto &unit : Units::getUnits(PlayerState::Enemy)) {
                            if (unit->getPosition().getDistance(nextExpand) < 640.0)
                                assignNumberToGoal(unit->getPosition(), type, 1, GoalType::Escort);
                        }
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

            // Zealots need to block the holes in walls in PvZ
            if (Players::PvZ() && Combat::State::isStaticRetreat(Protoss_Zealot)) {
                if (Walls::getNaturalWall()) {
                    for (auto &opening : Walls::getNaturalWall()->getOpenings()) {
                        assignNumberToGoal(Position(opening) + Position(16, 16), Protoss_Zealot, 1, GoalType::Block);
                    }
                }
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
            if (Broodwar->self()->getRace() != Races::Terran)
                return;

            // Wall off natural if we have one
            if (Combat::State::isStaticRetreat(Terran_Siege_Tank_Tank_Mode)) {
                auto naturalWall = Walls::getNaturalWall();
                if (naturalWall) {
                    for (auto &tile : naturalWall->getLargeTiles())
                        assignNumberToGoal(tile, Terran_Barracks, 1, GoalType::Defend);
                }
            }
        }

        void updateOverlordGoals()
        {
            auto enemyAir = Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) + Players::getVisibleCount(PlayerState::Enemy, Terran_Wraith) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk);

            // Assign an Overlord to watch our Choke early on
            if (Terrain::getNaturalChoke() && !Spy::enemyRush()) {
                const auto natSpot = Position(Terrain::getNaturalChoke()->Center());
                if ((Util::getTime() < Time(3, 00) && !Spy::enemyProxy()) || (Util::getTime() < Time(2, 15) && Spy::enemyProxy()) || (Players::ZvZ() && !enemyAir)) {
                    assignNumberToGoal(natSpot, Zerg_Overlord, 1, GoalType::Escort);
                    return;
                }
            }

            // Assign an Overlord to each main choke
            if (!enemyAir) {
                for (auto &station : Stations::getStations(PlayerState::Self)) {
                    if (station->isMain()) {
                        auto closestNatural = BWEB::Stations::getClosestNaturalStation(station->getBase()->Location());
                        if (Players::ZvP() || Players::ZvT()) {
                            if (closestNatural && Stations::ownedBy(closestNatural) == PlayerState::Self && station->getChokepoint())
                                assignNumberToGoal(station->getChokepoint()->Center(), Zerg_Overlord, 1, GoalType::Escort);
                        }
                        if (Players::ZvZ() && Players::getStrength(PlayerState::Enemy).airToAir == 0.0)
                            assignNumberToGoal(station->getChokepoint()->Center(), Zerg_Overlord, 1, GoalType::Escort);
                    }
                }
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
        }

        void updateZergGoals()
        {
            if (Broodwar->self()->getRace() != Races::Zerg)
                return;

            auto enemyStrength = Players::getStrength(PlayerState::Enemy);

            // Clear out base early game
            if (Util::getTime() < Time(4, 00) && !Spy::enemyProxy() && !Spy::enemyRush() && !Spy::enemyPressure() && !Players::ZvZ() && Players::getVisibleCount(PlayerState::Enemy, Terran_Factory) == 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Gateway) == 0) {
                auto oldestTile = Terrain::getOldestPosition(Terrain::getMainArea());

                if (oldestTile.isValid())
                    assignNumberToGoal(oldestTile, Zerg_Zergling, 1, GoalType::Explore);
            }

            // Before we have a spore, we need to defend overlords            
            auto &stations = Stations::getStations(PlayerState::Self);
            auto mainStations = int(count_if(stations.begin(), stations.end(), [&](auto& s) { return s->isMain(); }));

            // Before Hydras have upgrades, defend vulnerable bases, put lings on defense too
            if (Combat::State::isStaticRetreat(Zerg_Hydralisk) && !BuildOrder::isAllIn() && com(Zerg_Hydralisk_Den) > 0) {
                auto evenSplit = (1.0 / double(stations.size() - mainStations));

                if (!stations.empty()) {
                    for (auto &station : stations) {
                        if ((station->isNatural() && Terrain::isPocketNatural()) || (station->isMain() && !Terrain::isPocketNatural()))
                            continue;
                        assignPercentToGoal(Stations::getDefendPosition(station), Zerg_Hydralisk, evenSplit, GoalType::Defend);
                        assignPercentToGoal(Stations::getDefendPosition(station), Zerg_Zergling, evenSplit, GoalType::Defend);
                    }
                }
            }

            // Always keep 2 hydras at home to protect from sairs if we dont have a spore
            if (!Combat::State::isStaticRetreat(Zerg_Hydralisk) && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) >= 2 && Players::getCompleteCount(PlayerState::Self, Zerg_Spore_Colony) == 0) {
                assignNumberToGoal(Terrain::getNaturalPosition(), Zerg_Hydralisk, 2, GoalType::Defend);
            }

            // Always leave 2 lings at home in ZvZ
            if (!Combat::State::isStaticRetreat(Zerg_Zergling) && Players::ZvZ() && int(stations.size()) >= 2 && !Spy::enemyRush())
                assignNumberToGoal(Terrain::getMyNatural()->getResourceCentroid(), Zerg_Zergling, 2, GoalType::Defend);

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

                    auto pos = station->getBase()->Center();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }
                if (posBest.isValid()) {
                    assignPercentToGoal(posBest, Zerg_Zergling, 1.00, GoalType::Explore);
                    if (Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Grooved_Spines) && Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Muscular_Augments))
                        assignPercentToGoal(posBest, Zerg_Hydralisk, 0.50, GoalType::Explore);
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

    void onStart()
    {
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
    }

    void onFrame()
    {
        clearGoals();
        updateGenericGoals();
        updateProtossGoals();
        updateTerranGoals();
        updateOverlordGoals();
        updateZergGoals();
        updateCampaignGoals();
    }
}