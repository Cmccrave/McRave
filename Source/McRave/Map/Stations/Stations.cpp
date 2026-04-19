#include "Stations.h"

#include "BWEB.h"
#include "Builds/All/BuildOrder.h"
#include "Info/Resource/Resources.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Macro/Planning/Planning.h"
#include "Macro/Planning/Pylons.h"
#include "Main/Common.h"
#include "Map/Grids/Grids.h"
#include "Map/Terrain/Terrain.h"
#include "Map/Walls/Walls.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Stations {
    namespace {
        map<const BWEB::Station *const, PlayerState> stations;
        vector<const BWEB::Station *> selfStations;
        vector<const BWEB::Station *> enemyStations;
        vector<const BWEB::Station *> allyStations;

        map<const BWEB::Station *const, Position> defendPositions;
        multimap<double, const BWEB::Station *const> stationsBySaturation;
        multimap<double, const BWEB::Station *const> stationsByProduction;
        map<const BWEB::Station *const, int> airDefenseCount, groundDefenseCount, remainingMinerals, remainingGas, initialMinerals, initialGas;
        int miningStations = 0, gasingStations = 0;
        map<const BWEB::Station *const, map<const BWEB::Station *, BWEB::Path>> stationNetwork;

        void updateSaturation()
        {
            // Sort stations by saturation and current larva count
            remainingMinerals.clear();
            remainingGas.clear();
            stationsBySaturation.clear();
            auto mineralsLeftTotal = 0, gasLeftTotal = 0;

            // Update self stations
            for (auto &station : getStations(PlayerState::Self)) {
                auto workerCount           = 0;
                auto resourceCount         = 0;
                remainingGas[station]      = 0;
                remainingMinerals[station] = 0;
                for (auto &mineral : Resources::getMyMinerals()) {
                    if (mineral->getStation() == station && !mineral->isThreatened()) {
                        resourceCount++;
                        remainingMinerals[station] += mineral->getRemainingResources();
                        mineralsLeftTotal += mineral->getRemainingResources();
                        workerCount += mineral->targetedByWhat().size();
                    }
                }
                for (auto &gas : Resources::getMyGas()) {
                    if (gas->getStation() == station && !gas->isThreatened()) {
                        resourceCount++;
                        remainingGas[station] += gas->getRemainingResources();
                        gasLeftTotal += gas->getRemainingResources();
                        workerCount += gas->targetedByWhat().size();
                    }
                }

                // Order by lowest saturation first
                auto saturatedLevel = (workerCount > 0 && resourceCount > 0) ? double(workerCount) / double(resourceCount) : 0.0;
                stationsBySaturation.emplace(saturatedLevel, station);
            }

            // Update neutral stations
            for (auto &station : getStations(PlayerState::None)) {
                remainingGas[station]      = 0;
                remainingMinerals[station] = 0;
                for (auto &mineral : Resources::getMyMinerals()) {
                    if (mineral->getStation() == station)
                        remainingMinerals[station] += mineral->getRemainingResources();
                }
                for (auto &gas : Resources::getMyGas()) {
                    if (gas->getStation() == station)
                        remainingGas[station] += gas->getRemainingResources();
                }
            }

            // Determine how many mining and gasing stations we have
            miningStations = int(ceil(double(mineralsLeftTotal) / 13500.0));
            gasingStations = int(ceil(double(gasLeftTotal) / 5000.0));
        }

        void updateProduction()
        {
            // Sort stations by production capabilities
            stationsByProduction.clear();
            for (auto &station : getStations(PlayerState::Self)) {
                auto production = getStationSaturation(station);
                for (auto &unit : Units::getUnits(PlayerState::Self)) {
                    if (Planning::isProductionType(unit->getType())) {
                        auto closestStation = getClosestStationAir(unit->getPosition(), PlayerState::Self,
                                                                   [&](auto station) { return station->isMain() || Broodwar->self()->getRace() == Races::Zerg; });
                        if (closestStation && closestStation == station)
                            production += 1.0;
                    }
                }

                stationsByProduction.emplace(production, station);
            }
        }

        void updateStationDefenses()
        {
            // Calculate defense counts
            airDefenseCount.clear();
            groundDefenseCount.clear();
            vector<PlayerState> states = {PlayerState::Enemy, PlayerState::Self};

            for (auto &state : states) {
                for (auto &u : Units::getUnits(state)) {
                    auto &unit = *u;
                    if (unit.getType().isBuilding() && (unit.canAttackAir() || unit.canAttackGround())) {
                        auto closestStation = getClosestStationAir(unit.getPosition(), state);
                        if (closestStation && (unit.getPosition().getDistance(closestStation->getBase()->Center()) < 256.0 ||
                                               closestStation->getDefenses().find(unit.getTilePosition()) != closestStation->getDefenses().end())) {
                            if (unit.canAttackAir())
                                airDefenseCount[closestStation]++;
                            if (unit.canAttackGround())
                                groundDefenseCount[closestStation]++;
                        }
                    }
                }
            }
        }

        void updateDefendPositions()
        {
            // Only calculate this once per station
            for (auto &station : BWEB::Stations::getStations()) {
                auto defendPosition                 = station.getBase()->Center();
                const BWEM::ChokePoint *defendChoke = nullptr;
                if (defendPositions.find(&station) != defendPositions.end())
                    continue;

                // One choke, one defend position
                if (station.getChokepoint()) {
                    defendPosition = Position(station.getChokepoint()->Center());
                    defendChoke    = station.getChokepoint();
                }

                // Find chokepoint that is closest to enemy reinforcements
                else if (Terrain::getEnemyStartingPosition().isValid()) {
                    auto distBest      = DBL_MAX;
                    auto defendTowards = Terrain::getEnemyNatural() ? Terrain::getEnemyNatural()->getBase()->Center() : Terrain::getEnemyMain()->getBase()->Center();
                    for (auto &choke : station.getBase()->GetArea()->ChokePoints()) {
                        auto dist = BWEB::Map::getGroundDistance(Position(choke->Center()), defendTowards);
                        if (dist < distBest) {
                            distBest       = dist;
                            defendPosition = Position(choke->Center());
                            defendChoke    = choke;
                        }
                    }
                }
                else
                    continue;

                // If there are multiple chokepoints with the same area pair
                auto pathTowards = Terrain::getEnemyStartingPosition().isValid() ? Terrain::getEnemyStartingPosition() : mapBWEM.Center();
                if (station.getBase()->GetArea()->ChokePoints().size() >= 3) {
                    defendPosition = Position(0, 0);
                    int count      = 0;

                    for (auto &choke : station.getBase()->GetArea()->ChokePoints()) {
                        if (defendChoke && choke->GetAreas() != defendChoke->GetAreas())
                            continue;

                        if (Position(choke->Center()).getDistance(pathTowards) < station.getBase()->Center().getDistance(pathTowards)) {
                            defendPosition += Position(choke->Center());
                            count++;
                        }
                    }
                    if (count > 0)
                        defendPosition /= count;
                    else
                        defendPosition = Position(defendChoke->Center());
                }

                // If defend position isn't walkable, move it towards the closest base
                // vector<WalkPosition> directions ={ {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1} };
                // while (Grids::getMobility(defendPosition) <= 2) {
                //    auto best = DBL_MAX;
                //    auto start = WalkPosition(defendPosition);

                //    for (auto &dir : directions) {
                //        auto center = Position(start + dir);
                //        auto dist = center.getDistance(Terrain::getNaturalPosition());
                //        if (dist < best) {
                //            defendPosition = center;
                //            best = dist;
                //        }
                //    }
                //}
                defendPositions[&station] = defendPosition;
            }

            // Check if any walls are completed that defend this station
            for (auto &station : Stations::getStations(PlayerState::Self)) {
                auto wall = BWEB::Walls::getClosestWall(station->getBase()->Center());
                if (wall && Walls::isComplete(wall) && wall->getStation() == station) {
                    if (wall->getOpenings().size() > 0) {
                        TilePosition avgOpeningPosition = TilePosition(0, 0);
                        for (auto opening : wall->getOpenings()) {
                            avgOpeningPosition += opening;
                        }
                        avgOpeningPosition /= int(wall->getOpenings().size());
                        defendPositions[station] = Position(avgOpeningPosition) + Position(16, 16);
                        Visuals::drawCircle(defendPositions[station], 10, Colors::Cyan, true);
                    }
                }
            }
        }

        int PvPgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain()) {
                if (Spy::enemyInvis())
                    return 1 - groundCount;
            }
            else if (station->isNatural()) {
                if (Spy::enemyInvis())
                    return 2 - groundCount;
            }
            return 0;
        }

        int PvTgroundDef(const BWEB::Station *const station)
        {
            return 0;
        }

        int PvZgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain()) {
                if (Spy::getEnemyTransition().find("Muta") != string::npos)
                    return 3 - groundCount;
            }
            else if (station->isNatural()) {
                if (Spy::getEnemyTransition().find("Muta") != string::npos)
                    return 2 - groundCount;
            }
            return 0;
        }        
        
        int PvFFAgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);
            return 0;
        }

        int ZvPgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain()) {
                // Add 2 sunks if we gave up the natural intentionally
                if (!BuildOrder::takeNatural())
                    return (Util::getTime() > Time(2, 40)) + (Util::getTime() > Time(3, 00)) - groundCount;

                //// Add 1 sunks if we opened greedy against proxy gates
                // if (Spy::getEnemyOpener() == "Proxy9/9" && BuildOrder::getCurrentBuild() == Z_HatchPool)
                //    return (Util::getTime() > Time(3, 30)) - groundCount;

                // Add a sunk in main if we lost the natural, maybe it holds to get a win
                if (Players::getDeadCount(PlayerState::Self, Zerg_Hatchery) > 0) {
                    if (BuildOrder::isOpener() && !BuildOrder::isWallMain() && Stations::ownedBy(Terrain::getMyNatural()) == PlayerState::None && Util::getTime() < Time(4, 00))
                        return (Util::getTime() > Time(3, 00)) - groundCount;
                }

                // Add a sunk if we know they can drop
                if (Spy::getEnemyTransition() == P_DTDrop || Spy::getEnemyTransition() == P_Robo) {
                    return (Util::getTime() > Time(6, 00)) - groundCount;
                }
            }
            return 0;
        }

        int ZvTgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain()) {
                if (Players::getTotalCount(PlayerState::Enemy, Terran_Dropship) > 0)
                    return (Util::getTime() > Time(11, 00)) + (Util::getTime() > Time(15, 00)) - groundCount;
                if (Players::ZvT() && Spy::getEnemyBuild() != T_2Rax && Spy::getEnemyTransition() == U_WorkerRush)
                    return 1 - groundCount;
                if (Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Ion_Thrusters) && Util::getTime() > Time(9, 00))
                    return 1 - groundCount;
            }
            return 0;
        }

        int ZvZgroundDef(const BWEB::Station *const station)
        {
            auto groundCount     = getGroundDefenseCount(station);
            auto desiredDefenses = 0;

            // If enemy adds defenses, we can start to cut defenses too
            if (Util::getTime() > Time(4, 00))
                groundCount += Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Creep_Colony) +
                               Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony);

            // Don't make sunkens if we are within a certain ling count or are expanding
            if (station->isMain() && !Spy::enemyRush()) {
                auto lingsPerSunken = 6 * (1 + groundCount);
                if (vis(Zerg_Zergling) + lingsPerSunken >= Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling))
                    return 0;
                if (BuildOrder::takeNatural() || getStations(PlayerState::Self).size() > 1)
                    return 0;
            }

            if (BuildOrder::getCurrentBuild() == Z_PoolHatch) {
                if (station->isMain()) {

                    auto latePool = BuildOrder::getCurrentOpener() != Z_9Pool && BuildOrder::getCurrentOpener() != Z_Overpool;

                    // 4 Pool or 7 Pool
                    if (Spy::getEnemyOpener() == Z_4Pool)
                        return 1 + latePool - groundCount;
                    if (Spy::getEnemyOpener() == Z_7Pool)
                        return 1 - groundCount;

                    // 3 Hatch
                    if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 && com(Zerg_Spire) > 0)
                        return 4 - groundCount;
                    if (Spy::getEnemyTransition() == Z_3HatchSpeedling && com(Zerg_Spire) > 0)
                        return 4 - groundCount;

                    // Excess lings compared to me
                    if (Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > total(Zerg_Zergling) && Util::getTime() > Time(3, 40) && com(Zerg_Spire) > 0)
                        return 1 - groundCount;
                }
            }

            if (BuildOrder::getCurrentBuild() == Z_PoolLair) {
                if (station->isMain()) {

                    // 4 Pool
                    if (Spy::getEnemyOpener() == Z_4Pool)
                        desiredDefenses = max(desiredDefenses, 1 + (vis(Zerg_Spire) > 0));

                    // 3 Hatch
                    if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 && vis(Zerg_Spire) > 0)
                        return 4 - groundCount;

                    // Speedling all-in
                    if (Spy::getEnemyTransition() == Z_2HatchSpeedling)
                        return (Util::getTime() > Time(3, 00)) + (Util::getTime() > Time(3, 00)) + (vis(Zerg_Spire) * 2) - groundCount;

                    if (total(Zerg_Mutalisk) >= 4) {

                        // 12 Pool
                        if (Spy::getEnemyOpener() == Z_12Pool && Spy::getEnemyTransition() != Z_1HatchMuta)
                            // desiredDefenses = max(desiredDefenses, int(Util::getTime() > Time(4, 00)));
                            return 2 - groundCount;

                        // +1Ling
                        if (Spy::getEnemyTransition() == Z_UpgradeLing)
                            // desiredDefenses = max(desiredDefenses, (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(4, 45)));
                            return 2 - groundCount;

                        // 2 Hatch
                        if (Spy::getEnemyBuild() == Z_PoolHatch)
                            // desiredDefenses = max(desiredDefenses, (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(4, 45)));
                            return 2 - groundCount;

                        // Unknown
                        if ((!Terrain::foundEnemy() && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 24) || Spy::enemyFastExpand() || Spy::getEnemyBuild() == Z_HatchPool ||
                            Spy::getEnemyTransition() == Z_2HatchMuta || (Util::getTime() > Time(5, 00) && Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) > 4 * vis(Zerg_Zergling)))
                            // desiredDefenses = max(desiredDefenses, 1);
                            return 1 - groundCount;
                    }
                }
            }
            return desiredDefenses - groundCount;
        }

        int ZvFFAgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (!station->isMain() && !station->isNatural())
                return 2 - groundCount;
            return 0;
        }

        int TvPgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain() && (Spy::getEnemyOpener() == P_9_9 || Spy::getEnemyOpener() == P_Proxy_9_9))
                return 1 - groundCount;
            return 0;
        }

        int TvZgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain() && (Spy::getEnemyOpener() == Z_4Pool || Spy::getEnemyOpener() == Z_9Pool))
                return 1 - groundCount;
            return 0;
        }
        
        int TvFFAgroundDef(const BWEB::Station *const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain() && com(Terran_Marine) > 0)
                return 1 - groundCount;
            return 0;
        }

        void updateOwners()
        {
            selfStations.clear();
            enemyStations.clear();
            allyStations.clear();
            for (auto &station : BWEB::Stations::getStations()) {
                const auto closestSelf  = Util::getClosestUnit(station.getBase()->Center(), PlayerState::Self, [&](auto &u) { return u->getType().isResourceDepot(); });
                const auto closestEnemy = Util::getClosestUnit(station.getBase()->Center(), PlayerState::Self, [&](auto &u) { return u->getType().isResourceDepot(); });

                if (closestSelf && closestSelf->getPosition().getDistance(station.getBase()->Center()) < 96.0) {
                    selfStations.push_back(&station);
                }
                if (closestEnemy && closestEnemy->getPosition().getDistance(station.getBase()->Center()) < 96.0) {
                    enemyStations.push_back(&station);
                }
            }
        }
    } // namespace

    void onFrame()
    {
        Visuals::startPerfTest();
        // updateOwners();
        updateSaturation();
        updateProduction();
        updateStationDefenses();
        updateDefendPositions();
        Visuals::endPerfTest("Stations");
    }

    void onStart()
    {
        for (auto &station : BWEB::Stations::getStations()) {
            for (auto &mineral : station.getBase()->Minerals())
                initialMinerals[&station] += mineral->InitialAmount();
            for (auto &gas : station.getBase()->Geysers())
                initialGas[&station] += gas->InitialAmount();
            stations[&station] = PlayerState::None;
        }

        // Create a path to each station that only obeys terrain
        for (auto &station : BWEB::Stations::getStations()) {
            for (auto &otherStation : BWEB::Stations::getStations()) {
                if (&station == &otherStation)
                    continue;

                BWEB::Path path(station.getBase()->Center(), otherStation.getBase()->Center(), Protoss_Dragoon, true, false);
                path.generateJPS([&](auto &t) { return path.unitWalkable(t); });
                stationNetwork[&station][&otherStation] = path;
            }
        }
    }

    void storeStation(BWAPI::Point<int> here, PlayerState player)
    {
        auto station    = BWEB::Stations::getClosestStation(TilePosition(here));
        auto stationptr = stations.find(station);

        if (!station || here.getDistance(station->getBase()->Center()) > 96.0 || stationptr == stations.end() || stationptr->second != PlayerState::None)
            return;

        // Store station and set resource states if we own this station
        if (player == PlayerState::Self) {

            // Store minerals that still exist
            for (auto &mineral : station->getBase()->Minerals()) {
                if (!mineral || !mineral->Unit())
                    continue;
                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(mineral->Unit());
            }

            // Store geysers that still exist
            for (auto &geyser : station->getBase()->Geysers()) {
                if (!geyser || !geyser->Unit())
                    continue;
                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(geyser->Unit());
            }
        }

        // Add any territory it is in
        stationptr->second = player;
        Terrain::addTerritory(player, station);
    }

    // TODO: Eventually remove this
    void storeStation(Unit unit) { storeStation(unit->getPosition(), Players::getPlayerState(unit)); }

    void removeStation(BWAPI::Point<int> here, PlayerState player)
    {
        auto station    = BWEB::Stations::getClosestStation(TilePosition(here));
        auto stationptr = stations.find(station);

        if (!station || here.getDistance(station->getBase()->Center()) > 96.0 || stationptr == stations.end() || stationptr->second == PlayerState::None)
            return;

        // Remove workers from any resources on this station
        for (auto &mineral : Resources::getMyMinerals()) {
            if (mineral->getStation() == station)
                for (auto &w : mineral->targetedByWhat()) {
                    if (auto worker = w.lock()) {
                        worker->setResource(nullptr);
                        mineral->removeTargetedBy(worker);
                    }
                }
        }
        for (auto &gas : Resources::getMyGas()) {
            if (gas->getStation() == station)
                for (auto &w : gas->targetedByWhat()) {
                    if (auto worker = w.lock()) {
                        worker->setResource(nullptr);
                        gas->removeTargetedBy(worker);
                    }
                }
        }

        // Remove any territory it was in
        stationptr->second = PlayerState::None;
        Terrain::removeTerritory(player, station);
    }

    void removeStation(Unit unit) { removeStation(unit->getPosition(), Players::getPlayerState(unit)); }

    int getColonyCount(const BWEB::Station *const station)
    {
        auto colonies  = 0;
        auto wallNeeds = station->getChokepoint() && BWEB::Walls::getWall(station->getChokepoint()) &&
                         (Walls::needGroundDefenses(*BWEB::Walls::getWall(station->getChokepoint())) > 0 || Walls::needAirDefenses(*BWEB::Walls::getWall(station->getChokepoint())) > 0);
        for (auto &tile : station->getDefenses()) {
            if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                colonies++;
        }
        if (BWEB::Map::isUsed(station->getPocketDefense()) == Zerg_Creep_Colony)
            colonies++;
        return colonies;
    }

    int needGroundDefenses(const BWEB::Station *const station)
    {
        if (BuildOrder::isRush() || BuildOrder::isPressure() || Spy::getEnemyTransition() == P_Carrier || isPocket(station))
            return 0;

        // We don't want to pull workers to build things if none are nearby
        if (getSaturationRatio(station) == 0.0 && getColonyCount(station) == 0)
            return 0;

        if (Players::PvP())
            return PvPgroundDef(station);
        if (Players::PvT())
            return PvTgroundDef(station);
        if (Players::PvZ())
            return PvZgroundDef(station);
        if (Players::PvFFA())
            return PvFFAgroundDef(station);

        if (Players::ZvP())
            return ZvPgroundDef(station);
        if (Players::ZvT())
            return ZvTgroundDef(station);
        if (Players::ZvZ())
            return ZvZgroundDef(station);
        if (Players::ZvFFA())
            return ZvFFAgroundDef(station);

        if (Players::TvP())
            return TvPgroundDef(station);
        if (Players::TvZ())
            return TvZgroundDef(station);
        if (Players::TvFFA())
            return TvFFAgroundDef(station);
        return 0;
    }

    int needAirDefenses(const BWEB::Station *const station)
    {
        // We don't want to pull workers to build things if none are nearby
        if (getSaturationRatio(station) == 0.0 && getColonyCount(station) == 0)
            return 0;

        auto airCount       = getAirDefenseCount(station);
        const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0 ||
                              Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0 ||
                              Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 ||
                              (Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0 && Util::getTime() > Time(4, 45));

        auto mutaBuild  = BuildOrder::getCurrentTransition().find("Muta") != string::npos;
        auto hydraBuild = BuildOrder::getCurrentTransition().find("Hydra") != string::npos;

        if (Players::ZvP()) {

            // Get a spore after a delay, hydras deflect temporarily
            if (station->isNatural() && hydraBuild && enemyAir && !Combat::State::isStaticRetreat(Zerg_Hydralisk)) {
                if (Spy::getEnemyBuild() == P_FFE)
                    return (Util::getTime() > Time(7, 30)) - airCount;
                return (Util::getTime() > Time(6, 30)) - airCount;
            }

            // 1 Gate Corsair
            if (station->isNatural() && Spy::getEnemyBuild() == P_1GateCore && Spy::getEnemyTransition() == P_Corsair && !hydraBuild)
                return (Util::getTime() > Time(4, 20)) - airCount;
            if (station->isNatural() && Spy::getEnemyBuild() == P_1GateCore && Spy::getEnemyTransition() == P_CorsairGoon && !hydraBuild)
                return (Util::getTime() > Time(4, 35)) - airCount;

            // 2 Gate Corsair
            if (station->isNatural() && Spy::getEnemyBuild() == P_2Gate && Spy::getEnemyOpener() == P_10_15 && Spy::getEnemyTransition() == P_Corsair && !hydraBuild)
                return (Util::getTime() > Time(4, 25)) - airCount;
            if (station->isNatural() && Spy::getEnemyBuild() == P_2Gate && Spy::getEnemyTransition() == P_Corsair && !hydraBuild)
                return (Util::getTime() > Time(4, 45)) - airCount;
            if (station->isNatural() && Spy::getEnemyBuild() == P_2Gate && Spy::getEnemyTransition() == P_CorsairGoon && !hydraBuild)
                return (Util::getTime() > Time(5, 45)) - airCount;

            // Late spores if we're allin
            if (station->isNatural() && enemyAir && BuildOrder::isAllIn() && com(Zerg_Hydralisk) == 0 && com(Zerg_Mutalisk) == 0)
                return (Util::getTime() > Time(5, 00)) - airCount;
            if (station->isNatural() && enemyAir && !hydraBuild && !mutaBuild)
                return (Util::getTime() > Time(5, 00)) - airCount;
            if (enemyAir && !mutaBuild && !hydraBuild)
                return (Util::getTime() > Time(9, 00)) - airCount;

            // Corsair DT exist
            if (!Combat::State::isStaticRetreat(Zerg_Hydralisk) && !station->isMain() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 &&
                Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0)
                return (Util::getTime() > Time(9, 00)) - airCount;

            // All-in and Corsairs exists
            if (station->isNatural() && BuildOrder::isAllIn() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0)
                return 1 + (Util::getTime() > Time(10, 00)) - airCount;
        }

        if (Players::ZvZ()) {

            // Get a spore vs 1h muta if we aren't 1h muta or 2h muta on one base
            if (Spy::getEnemyTransition() == Z_1HatchMuta && BuildOrder::getCurrentTransition() != Z_1HatchMuta)
                return (Util::getTime() > Time(4, 15)) + (Util::getTime() > Time(6, 15)) - airCount;

            // We have more bases
            if (Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 && Stations::getStations(PlayerState::Self).size() > Stations::getStations(PlayerState::Enemy).size() &&
                !Combat::State::isStaticRetreat(Zerg_Mutalisk))
                return (Util::getTime() > Time(6, 15)) - airCount;
        }

        if (Players::ZvT()) {

            // 1 Fact 2 Port Wraith
            if (Spy::getEnemyTransition() == T_2PortWraith)
                return (Util::getTime() > Time(4, 30)) - airCount;

            // Late game we want a spore protecting ovies from stray valks/wraiths
            if (Stations::getStations(PlayerState::Self).size() >= 3 &&
                (Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) >= 4 || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) >= 8))
                return 1 - airCount;
        }

        if (Broodwar->self()->getRace() == Races::Terran) {
            if (Spy::enemyInvis())
                return 2 - airCount;
            if (Players::TvZ() && Util::getTime() > Time(4, 30))
                return 3 - airCount;
        }

        return 0;
    }

    bool needPower(const BWEB::Station *const station)
    {
        auto count = 0;
        for (auto &defense : station->getDefenses()) {
            if (Pylons::hasPowerSoon(defense, UnitTypes::Protoss_Photon_Cannon))
                count++;
        }
        return count < 2;
    }

    bool isBaseExplored(const BWEB::Station *const station)
    {
        auto botRight = station->getBase()->Location() + TilePosition(3, 2);
        return (Broodwar->isExplored(station->getBase()->Location()) && Broodwar->isExplored(botRight));
    }

    bool isGeyserExplored(const BWEB::Station *const station)
    {
        for (auto &geyser : Resources::getMyGas()) {
            if (!Broodwar->isExplored(geyser->getTilePosition()) || !Broodwar->isExplored(geyser->getTilePosition() + TilePosition(3, 1)))
                return false;
        }
        return true;
    }

    bool isCompleted(const BWEB::Station *const station)
    {
        // TODO: This is really slow
        const auto base = Util::getClosestUnit(station->getBase()->Center(), PlayerState::Self, [&](auto &u) { return u->getType().isResourceDepot(); });
        return base && base->unit()->isCompleted();
    }

    bool isBlocked(const BWEB::Station *const station)
    {
        auto stationptr = stations.find(station);
        if (stationptr != stations.end() && stationptr->second != PlayerState::None) {
            if (stationptr->second == PlayerState::Self)
                return false;
            return true;
        }

        for (auto x = 0; x < 4; x++) {
            for (auto y = 0; y < 3; y++) {
                auto tile = station->getBase()->Location() + TilePosition(x, y);
                if (BWEB::Map::isUsed(tile) != UnitTypes::None)
                    return true;
            }
        }
        return false;
    }

    // Return true if this station is adjacent to only our territory
    bool isPocket(const BWEB::Station *const station)
    {
        if (station->isMain() || station->isNatural())
            return false;

        for (auto &area : station->getBase()->GetArea()->AccessibleNeighbours()) {
            if (!Terrain::inTerritory(PlayerState::Self, area))
                return false;
        }
        return true;
    }

    // Returns true if this station is going to be lost
    bool isThreatened(const BWEB::Station *const station)
    {
        // Find self unit with losing sim state that is targeting a threatening unit near this station
        const auto closestSelf = Util::getClosestUnit(station->getBase()->Center(), PlayerState::Self, [&](auto &u) {
            if (u->getRole() != Role::Combat)
                return false;
            return Terrain::inArea(station, u->getPosition());
        });
        if (closestSelf) {
            if (closestSelf->hasTarget(); auto target = closestSelf->getTarget().lock()) {
                if (target->isThreatening() && closestSelf->getSimState() == SimState::Loss)
                    return true;
                return false;
            }
        }

        // There's a threatening unit and nothing to support
        const auto closestEnemy = Util::getClosestUnit(station->getBase()->Center(), PlayerState::Enemy, [&](auto &u) { return u->isThreatening() && Terrain::inArea(station, u->getPosition()); });

        return closestEnemy;
    }

    int lastVisible(const BWEB::Station *const station)
    {
        auto botRight = station->getBase()->Location() + TilePosition(4, 3);
        return min(Grids::getLastVisibleFrame(station->getBase()->Location()), Grids::getLastVisibleFrame(botRight));
    }

    double getSaturationRatio(const BWEB::Station *const station)
    {
        for (auto &[r, s] : stationsBySaturation) {
            if (s == station)
                return r;
        }
        return 0.0;
    }

    PlayerState ownedBy(const BWEB::Station *const station)
    {
        auto stationptr = stations.find(station);
        if (stationptr != stations.end())
            return stationptr->second;
        return PlayerState::None;
    }

    const BWEB::Station *const getClosestRetreatStation(UnitInfo &unit)
    {
        const auto ownForwardBase = [&](auto station) {
            if (isPocket(station))
                return true;
            if (station->isMain() && !Terrain::isPocketNatural()) {
                const auto closestNatural = BWEB::Stations::getClosestNaturalStation(station->getBase()->Location());
                if (Stations::ownedBy(closestNatural) == PlayerState::Self && Combat::isHoldNatural())
                    return true;
            }
            return false;
        };

        auto distBest    = DBL_MAX;
        auto bestStation = Terrain::getMyMain();
        for (auto &station : getStations(PlayerState::Self)) {
            auto defendPosition = station->getBase()->Center();
            auto distDefend     = defendPosition.getDistance(unit.getPosition());
            auto distCenter     = station->getBase()->Center().getDistance(unit.getPosition());

            if (station->isNatural() && !Combat::isHoldNatural())
                continue;

            if (distDefend < distBest && !ownForwardBase(station)) {
                bestStation = station;
                distBest    = distDefend;
            }
        }
        return bestStation;
    }

    std::vector<const BWEB::Station *> getStations(PlayerState player)
    {
        return getStations(player, [](auto) { return true; });
    }

    template <typename F> std::vector<const BWEB::Station *> getStations(PlayerState player, F &&pred)
    {
        vector<const BWEB::Station *> returnVector;
        for (auto &[station, stationPlayer] : stations) {
            if (stationPlayer == player && pred(station))
                returnVector.push_back(station);
        }
        return returnVector;
    }

    double getStationSaturation(const BWEB::Station *station)
    {
        for (auto &[saturation, s] : stationsBySaturation) {
            if (s == station)
                return saturation;
        }
        return 0.0;
    }

    BWEB::Path getPathBetween(const BWEB::Station *station1, const BWEB::Station *station2)
    {
        auto station1ptr = stationNetwork.find(station1);
        if (station1ptr != stationNetwork.end()) {
            auto station2ptr = station1ptr->second.find(station2);
            if (station2ptr != station1ptr->second.end())
                return station2ptr->second;
        }
        return {};
    }

    std::map<const BWEB::Station *, BWEB::Path> &getStationNetwork(const BWEB::Station *station)
    {
        auto stationptr = stationNetwork.find(station);
        return stationptr->second;
    }

    Position getDefendPosition(const BWEB::Station *const station)
    {
        if (defendPositions.find(station) != defendPositions.end())
            return defendPositions[station];
        return Positions::Invalid;
    }
    multimap<double, const BWEB::Station *const> &getStationsBySaturation() { return stationsBySaturation; }
    multimap<double, const BWEB::Station *const> &getStationsByProduction() { return stationsByProduction; }
    int getGasingStationsCount() { return gasingStations; }
    int getMiningStationsCount() { return miningStations; }
    int getGroundDefenseCount(const BWEB::Station *const station) { return groundDefenseCount[station]; }
    int getAirDefenseCount(const BWEB::Station *const station) { return airDefenseCount[station]; }
    int getMineralsRemaining(const BWEB::Station *const station) { return remainingMinerals[station]; }
    int getGasRemaining(const BWEB::Station *const station) { return remainingGas[station]; }
    int getMineralsInitial(const BWEB::Station *const station) { return initialMinerals[station]; }
    int getGasInitial(const BWEB::Station *const station) { return initialGas[station]; }
} // namespace McRave::Stations