#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Stations
{
    namespace {
        map<const BWEB::Station * const, PlayerState> stations;
        vector<const BWEB::Station *> selfStations;
        vector<const BWEB::Station *> enemyStations;
        vector<const BWEB::Station *> allyStations;


        map<const BWEB::Station * const, Position> defendPositions;
        multimap<double, const BWEB::Station * const> stationsBySaturation;
        multimap<double, const BWEB::Station * const> stationsByProduction;
        map<const BWEB::Station * const, int> airDefenseCount, groundDefenseCount, remainingMinerals, remainingGas, initialMinerals, initialGas;
        int miningStations = 0, gasingStations = 0;
        map<const BWEB::Station * const, map<const BWEB::Station *, BWEB::Path>> stationNetwork;

        void updateSaturation() {

            // Sort stations by saturation and current larva count
            remainingMinerals.clear();
            remainingGas.clear();
            stationsBySaturation.clear();
            auto mineralsLeftTotal = 0, gasLeftTotal = 0;

            // Update self stations
            for (auto &station : getStations(PlayerState::Self)) {
                auto workerCount = 0;
                auto resourceCount = 0;
                remainingGas[station] = 0;
                remainingMinerals[station] = 0;
                for (auto &mineral : Resources::getMyMinerals()) {
                    if (mineral->getStation() == station && !mineral->isThreatened()) {
                        resourceCount++;
                        remainingMinerals[station]+=mineral->getRemainingResources();
                        mineralsLeftTotal+=mineral->getRemainingResources();
                        workerCount+=mineral->targetedByWhat().size();
                    }
                }
                for (auto &gas : Resources::getMyGas()) {
                    if (gas->getStation() == station && !gas->isThreatened()) {
                        resourceCount++;
                        remainingGas[station]+=gas->getRemainingResources();
                        gasLeftTotal+=gas->getRemainingResources();
                        workerCount+=gas->targetedByWhat().size();
                    }
                }

                // Order by lowest saturation first
                auto saturatedLevel = (workerCount > 0 && resourceCount > 0) ? double(workerCount) / double(resourceCount) : 0.0;
                stationsBySaturation.emplace(saturatedLevel, station);
            }

            // Update neutral stations
            for (auto &station : getStations(PlayerState::None)) {
                remainingGas[station] = 0;
                remainingMinerals[station] = 0;
                for (auto &mineral : Resources::getMyMinerals()) {
                    if (mineral->getStation() == station)
                        remainingMinerals[station]+=mineral->getRemainingResources();
                }
                for (auto &gas : Resources::getMyGas()) {
                    if (gas->getStation() == station)
                        remainingGas[station]+=gas->getRemainingResources();
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
                        auto closestStation = getClosestStationAir(unit->getPosition(), PlayerState::Self, [&](auto station) {
                            return station->isMain() || Broodwar->self()->getRace() == Races::Zerg;
                        });
                        if (closestStation == station)
                            production+=1.0;
                    }
                }

                stationsByProduction.emplace(production, station);
            }
        }

        void updateStationDefenses() {

            // Calculate defense counts
            airDefenseCount.clear();
            groundDefenseCount.clear();
            vector<PlayerState> states ={ PlayerState::Enemy, PlayerState::Self };

            for (auto &state : states) {
                for (auto &u : Units::getUnits(state)) {
                    auto &unit = *u;
                    if (unit.getType().isBuilding() && (unit.canAttackAir() || unit.canAttackGround())) {
                        auto closestStation = getClosestStationAir(unit.getPosition(), state);
                        if (closestStation && (unit.getPosition().getDistance(closestStation->getBase()->Center()) < 256.0 || closestStation->getDefenses().find(unit.getTilePosition()) != closestStation->getDefenses().end())) {
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
            for (auto &station : BWEB::Stations::getStations()) {
                auto defendPosition = station.getBase()->Center();
                const BWEM::ChokePoint * defendChoke = nullptr;

                if (station.getChokepoint()) {
                    defendPosition = Position(station.getChokepoint()->Center());
                    defendChoke = station.getChokepoint();
                }
                else if (Terrain::getEnemyStartingPosition().isValid()) {
                    auto path = mapBWEM.GetPath(station.getBase()->Center(), Terrain::getEnemyStartingPosition());
                    if (!path.empty()) {
                        defendPosition = Position(path.front()->Center());
                        defendChoke = path.front();
                    }
                }

                // If there are multiple chokepoints with the same area pair
                auto pathTowards = Terrain::getEnemyStartingPosition().isValid() ? Terrain::getEnemyStartingPosition() : mapBWEM.Center();
                if (!defendChoke && station.getBase()->GetArea()->ChokePoints().size() >= 3) {
                    defendPosition = Position(0, 0);
                    int count = 0;

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
                        defendPosition = Position(Terrain::getNaturalChoke()->Center());
                }

                // If defend position isn't walkable, move it towards the closest base
                vector<WalkPosition> directions ={ {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1} };
                while (Grids::getMobility(defendPosition) <= 2) {
                    auto best = DBL_MAX;
                    auto start = WalkPosition(defendPosition);

                    for (auto &dir : directions) {
                        auto center = Position(start + dir);
                        auto dist = center.getDistance(Terrain::getNaturalPosition());
                        if (dist < best) {
                            defendPosition = center;
                            best = dist;
                        }
                    }
                }
                defendPositions[&station] = defendPosition;
            }
        }

        int PvPgroundDef(const BWEB::Station * const station)
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
            else {
            }
            return 0;
        }

        int PvTgroundDef(const BWEB::Station * const station)
        {
            if (station->isMain()) {
            }
            else if (station->isNatural()) {
            }
            else {
            }
            return 0;
        }

        int PvZgroundDef(const BWEB::Station * const station)
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
            else {
            }
            return 0;
        }

        int ZvPgroundDef(const BWEB::Station * const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain()) {
                if (BuildOrder::isProxy() && BuildOrder::getCurrentTransition() == "2HatchLurker")
                    return (Util::getTime() > Time(2, 45)) + (Util::getTime() > Time(3, 00)) + (Util::getTime() > Time(3, 30)) + (Util::getTime() > Time(4, 15)) - groundCount;
                if (BuildOrder::isOpener() && Stations::ownedBy(BWEB::Stations::getStartingNatural()) == PlayerState::None)
                    return (Util::getTime() > Time(3, 00)) - groundCount;
            }
            else if (station->isNatural()) {
            }
            else {
                if (Spy::getEnemyTransition() == "ZealotRush")
                    return (Util::getTime() > Time(6, 00)) - groundCount;
                if (Spy::getEnemyBuild() == "1GateCore" || Spy::getEnemyBuild() == "2Gate")
                    return (Util::getTime() > Time(7, 00)) - groundCount;
            }
            return 0;
        }

        int ZvTgroundDef(const BWEB::Station * const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (station->isMain()) {
                if (Players::getTotalCount(PlayerState::Enemy, Terran_Dropship) > 0)
                    return (Util::getTime() > Time(11, 00)) + (Util::getTime() > Time(15, 00)) - groundCount;
                if (BuildOrder::isOpener() && Stations::ownedBy(BWEB::Stations::getStartingNatural()) == PlayerState::None)
                    return (Util::getTime() > Time(3, 00)) - groundCount;
                if (Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Ion_Thrusters) && Util::getTime() > Time(7, 00))
                    return 1 - groundCount;
            }
            else if (station->isNatural()) {
            }
            else {
                return (Util::getTime() > Time(4, 30)) - groundCount;
            }
            return 0;
        }

        int ZvZgroundDef(const BWEB::Station * const station)
        {
            auto groundCount = getGroundDefenseCount(station);
            auto desiredDefenses = 0;

            // If enemy adds defenses, we can start to cut defenses too
            if (Util::getTime() > Time(4, 00))
                groundCount += Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Creep_Colony)
                + Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony);

            if (station->isMain()) {
                if (!Spy::enemyRush() && (BuildOrder::takeNatural() || getStations(PlayerState::Self).size() > 1))
                    return 0;
            }

            if (BuildOrder::getCurrentBuild() == "PoolHatch") {
                if (station->isMain()) {

                    // 4 Pool or 7 Pool
                    if (Spy::getEnemyOpener() == "4Pool" || Spy::getEnemyOpener() == "7Pool")
                        return 1 - groundCount;

                    // 3 Hatch
                    if (Util::getTime() < Time(6, 30) && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 && com(Zerg_Spire) > 0)
                        return 4 - groundCount;
                    if (Spy::getEnemyTransition() == "3HatchSpeedling" && com(Zerg_Spire) > 0)
                        return 4 - groundCount;

                    // Excess lings compared to me
                    if (Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > total(Zerg_Zergling) && Util::getTime() > Time(3, 40) && com(Zerg_Spire) > 0)
                        return 1 - groundCount;
                }
            }

            if (BuildOrder::getCurrentBuild() == "PoolLair") {
                if (station->isMain() && vis(Zerg_Drone) >= 8) {

                    // 4 Pool
                    if (Spy::getEnemyOpener() == "4Pool")
                        desiredDefenses = max(desiredDefenses, 1 + (vis(Zerg_Spire) > 0));

                    // 3 Hatch
                    if (Util::getTime() < Time(6, 30) && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 && vis(Zerg_Spire) > 0)
                        return 4 - groundCount;

                    if (total(Zerg_Mutalisk) >= 4) {

                        // 12 Pool
                        if (Spy::getEnemyOpener() == "12Pool" && Spy::getEnemyTransition() != "1HatchMuta")
                            //desiredDefenses = max(desiredDefenses, int(Util::getTime() > Time(4, 00)));
                            return 2 - groundCount;

                        // Speedling all-in
                        if (Spy::getEnemyTransition() == "2HatchSpeedling" && vis(Zerg_Spire) > 0)
                            //desiredDefenses = max(desiredDefenses, (Util::getTime() > Time(3, 30)) + (Util::getTime() > Time(3, 45)) + (Util::getTime() > Time(4, 15)) + (Util::getTime() > Time(4, 30)));
                            return 2 - groundCount;

                        // +1Ling
                        if (Spy::getEnemyTransition() == "+1Ling")
                            //desiredDefenses = max(desiredDefenses, (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(4, 45)));
                            return 2 - groundCount;

                        // 2 Hatch
                        if (Spy::getEnemyBuild() == "PoolHatch")
                            //desiredDefenses = max(desiredDefenses, (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(4, 45)));
                            return 2 - groundCount;

                        // Unknown
                        if ((!Terrain::foundEnemy() && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                            || Spy::enemyFastExpand()
                            || Spy::getEnemyBuild() == "HatchPool"
                            || Spy::getEnemyTransition() == "2HatchMuta"
                            || (Util::getTime() > Time(5, 00) && Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) > 4 * vis(Zerg_Zergling)))
                            //desiredDefenses = max(desiredDefenses, 1);
                            return 1 - groundCount;
                    }
                }
            }
            return desiredDefenses - groundCount;
        }

        int ZvFFAgroundDef(const BWEB::Station * const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (Players::ZvFFA() && !station->isMain() && !station->isNatural())
                return 2 - groundCount;
            return 0;
        }

        int TvZgroundDef(const BWEB::Station * const station)
        {
            auto groundCount = getGroundDefenseCount(station);

            if (Players::TvFFA() && station->isMain() && com(Terran_Marine) > 0)
                return 1 - groundCount;
            if (Players::TvZ() && station->isMain() && (Spy::getEnemyOpener() == "4Pool" || Spy::getEnemyOpener() == "9Pool"))
                return 1 - groundCount;
            return 0;
        }

        void updateOwners()
        {
            selfStations.clear();
            enemyStations.clear();
            allyStations.clear();
            for (auto &station : BWEB::Stations::getStations()) {
                const auto closestSelf = Util::getClosestUnit(station.getBase()->Center(), PlayerState::Self, [&](auto &u) {
                    return u->getType().isResourceDepot();
                });
                const auto closestEnemy = Util::getClosestUnit(station.getBase()->Center(), PlayerState::Self, [&](auto &u) {
                    return u->getType().isResourceDepot();
                });

                if (closestSelf && closestSelf->getPosition().getDistance(station.getBase()->Center()) < 96.0) {
                    selfStations.push_back(&station);
                }
                if (closestEnemy && closestEnemy->getPosition().getDistance(station.getBase()->Center()) < 96.0) {
                    enemyStations.push_back(&station);
                }
            }
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        //updateOwners();
        updateSaturation();
        updateProduction();
        updateStationDefenses();
        updateDefendPositions();
        Visuals::endPerfTest("Stations");
    }

    void onStart() {
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
        auto station = BWEB::Stations::getClosestStation(TilePosition(here));
        auto stationptr = stations.find(station);

        if (!station
            || here.getDistance(station->getBase()->Center()) > 96.0
            || stationptr == stations.end()
            || stationptr->second != PlayerState::None)
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
    void storeStation(Unit unit) {
        storeStation(unit->getPosition(), Players::getPlayerState(unit));
    }

    void removeStation(BWAPI::Point<int> here, PlayerState player)
    {
        auto station = BWEB::Stations::getClosestStation(TilePosition(here));
        auto stationptr = stations.find(station);

        if (!station
            || here.getDistance(station->getBase()->Center()) > 96.0
            || stationptr == stations.end()
            || stationptr->second == PlayerState::None)
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

    void removeStation(Unit unit) {
        removeStation(unit->getPosition(), Players::getPlayerState(unit));
    }

    int getColonyCount(const BWEB::Station *  const station)
    {
        auto colonies = 0;
        auto wallNeeds = station->getChokepoint() && BWEB::Walls::getWall(station->getChokepoint()) && (Walls::needGroundDefenses(*BWEB::Walls::getWall(station->getChokepoint())) > 0 || Walls::needAirDefenses(*BWEB::Walls::getWall(station->getChokepoint())) > 0);
        for (auto& tile : station->getDefenses()) {
            if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                colonies++;
            //if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony && wallNeeds && BWEB::Walls::getWall(station->getChokepoint())->getDefenses().find(tile) != BWEB::Walls::getWall(station->getChokepoint())->getDefenses().end())
            //    colonies--;
        }
        if (BWEB::Map::isUsed(station->getPocketDefense()) == Zerg_Creep_Colony)
            colonies++;
        return colonies;
    }

    int needGroundDefenses(const BWEB::Station * const station) {

        if (BuildOrder::isRush()
            || BuildOrder::isPressure()
            || Spy::getEnemyTransition() == "Carriers")
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
        if (Players::ZvP())
            return ZvPgroundDef(station);
        if (Players::ZvT())
            return ZvTgroundDef(station);
        if (Players::ZvZ())
            return ZvZgroundDef(station);
        if (Players::TvZ())
            return TvZgroundDef(station);
        return 0;
    }

    int needAirDefenses(const BWEB::Station * const station) {

        // We don't want to pull workers to build things if none are nearby
        if (getSaturationRatio(station) == 0.0 && getColonyCount(station) == 0)
            return 0;

        auto airCount = getAirDefenseCount(station);
        const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
            || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0
            || (Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0 && Util::getTime() > Time(4, 45));

        if (Broodwar->self()->getRace() == Races::Zerg) {
            auto mutaBuild = BuildOrder::getCurrentTransition().find("Muta") != string::npos;
            auto hydraBuild = BuildOrder::getCurrentTransition().find("Hydra") != string::npos;

            if (Players::ZvP()) {

                // Get a spore after a delay, hydras deflect temporarily
                if (station->isNatural() && hydraBuild && enemyAir) {
                    if (Spy::getEnemyBuild() == "FFE")
                        return (Util::getTime() > Time(7, 30)) - airCount;
                    return (Util::getTime() > Time(6, 30)) - airCount;
                }

                // Early spore vs 1gc corsair timing
                if (station->isNatural() && Spy::getEnemyBuild() == "1GateCore" && Spy::getEnemyTransition() == "Corsair")
                    return (Util::getTime() > Time(4, 35)) - airCount;

                // Slightly later spore vs delayed 2gates
                if (station->isNatural() && Spy::getEnemyBuild() == "2Gate" && Spy::getEnemyOpener() == "10/15" && !hydraBuild)
                    return (Util::getTime() > Time(4, 45)) - airCount;

                // Much later spore vs more aggresive 2gates
                if (station->isNatural() && Spy::getEnemyBuild() == "2Gate" && Spy::getEnemyTransition() == "Corsair" && BuildOrder::getCurrentTransition() != "2HatchMuta" && !hydraBuild)
                    return (Util::getTime() > Time(5, 00)) - airCount;
            }

            if (Players::ZvZ()) {

                // Get a spore vs 1h muta if we aren't 1h muta or 2h muta on one base
                if (Spy::getEnemyTransition() == "1HatchMuta" && BuildOrder::getCurrentTransition() != "1HatchMuta")
                    return (Util::getTime() > Time(4, 15)) + (Util::getTime() > Time(6, 15)) - airCount;

                // We have more bases
                if (Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 && Stations::getStations(PlayerState::Self).size() > Stations::getStations(PlayerState::Enemy).size() && com(Zerg_Extractor) >= 2)
                    return (Util::getTime() > Time(6, 15)) - airCount;
            }

            if (Players::ZvT()) {

                // Must get a spore if they have cloak
                if (Spy::getEnemyTransition() == "2PortWraith" && Spy::enemyInvis())
                    return 1 - airCount;

                // Need a spore with later mutas
                if (Spy::getEnemyTransition() == "2PortWraith" && BuildOrder::getCurrentTransition() != "2HatchMuta")
                    return (Util::getTime() > Time(5, 30)) - airCount;

                // Late game we want a spore protecting ovies from stray valks/wraiths
                if (Stations::getStations(PlayerState::Self).size() >= 3 && (Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) >= 4 || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) >= 8))
                    return 1 - airCount;
            }
        }

        if (Broodwar->self()->getRace() == Races::Terran) {
            if (Spy::enemyInvis())
                return 2 - airCount;
            if (Players::TvZ() && Util::getTime() > Time(4, 30))
                return 3 - airCount;
        }

        return 0;
    }

    bool needPower(const BWEB::Station * const station) {
        auto count = 0;
        for (auto &defense : station->getDefenses()) {
            if (Pylons::hasPowerSoon(defense, UnitTypes::Protoss_Photon_Cannon))
                count++;
        }
        return count < 2;
    }

    bool isBaseExplored(const BWEB::Station * const station) {
        auto botRight = station->getBase()->Location() + TilePosition(3, 2);
        return (Broodwar->isExplored(station->getBase()->Location()) && Broodwar->isExplored(botRight));
    }

    bool isGeyserExplored(const BWEB::Station * const station) {
        for (auto &geyser : Resources::getMyGas()) {
            if (!Broodwar->isExplored(geyser->getTilePosition()) || !Broodwar->isExplored(geyser->getTilePosition() + TilePosition(3, 1)))
                return false;
        }
        return true;
    }

    bool isCompleted(const BWEB::Station * const station)
    {
        // TODO: This is really slow
        const auto base = Util::getClosestUnit(station->getBase()->Center(), PlayerState::Self, [&](auto &u) {
            return u->getType().isResourceDepot();
        });
        return base && base->unit()->isCompleted();
    }

    bool isBlocked(const BWEB::Station * const station)
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

    int lastVisible(const BWEB::Station * const station) {
        auto botRight = station->getBase()->Location() + TilePosition(4, 3);
        return min(Grids::getLastVisibleFrame(station->getBase()->Location()), Grids::getLastVisibleFrame(botRight));
    }

    double getSaturationRatio(const BWEB::Station * const station)
    {
        for (auto &[r, s] : stationsBySaturation) {
            if (s == station)
                return r;
        }
        return 0.0;
    }

    PlayerState ownedBy(const BWEB::Station * const station)
    {
        auto stationptr = stations.find(station);
        if (stationptr != stations.end())
            return stationptr->second;
        return PlayerState::None;
    }

    const BWEB::Station * const getClosestRetreatStation(UnitInfo& unit)
    {
        const auto closerThanSim = [&](auto &defendPosition) {
            if (!unit.hasSimTarget())
                return true;
            auto sim = unit.getSimTarget().lock();
            auto eitherFlyer = (sim->isFlying() || unit.isFlying());

            return !unit.hasSimTarget()
                || eitherFlyer
                || BWEB::Map::getGroundDistance(unit.getPosition(), defendPosition) < BWEB::Map::getGroundDistance(unit.getSimTarget().lock()->getPosition(), defendPosition);
        };

        const auto alreadyInArea = [&](auto station) {
            if (Terrain::inArea(station->getBase()->GetArea(), unit.getPosition()))
                return true;

            // If this is a main, check if we own a natural that isn't under attack
            if (station->isNatural()) {
                const auto closestMain = BWEB::Stations::getClosestMainStation(station->getBase()->Location());
                if (closestMain && Stations::ownedBy(closestMain) == PlayerState::Self) {
                    if (Terrain::inArea(closestMain->getBase()->GetArea(), unit.getPosition()))
                        return true;
                }
            }
            return false;
        };

        const auto ownForwardBase = [&](auto station) {
            if (station->isMain()) {
                const auto closestNatural = BWEB::Stations::getClosestNaturalStation(station->getBase()->Location());
                if (Stations::ownedBy(closestNatural) == PlayerState::Self)
                    return true;
            }
            return false;
        };

        auto here = unit.getPosition();
        if (unit.getGoal().isValid() && unit.getGoalType() == GoalType::Defend) {
            here = unit.getGoal();
        }
        else {
            const auto lowGroundCount = Broodwar->self()->getRace() == Races::Zerg && vis(Zerg_Zergling) < 12 && vis(Zerg_Hydralisk) < 6;
            if (Util::getTime() < Time(5, 00) || Spy::enemyRush() || lowGroundCount)
                return Combat::isDefendNatural() ? Terrain::getMyNatural() : Terrain::getMyMain();
        }

        auto distBest = DBL_MAX;
        auto bestStation = Terrain::getMyMain();
        for (auto &station : getStations(PlayerState::Self)) {
            auto defendPosition = Stations::getDefendPosition(station);
            auto distDefend = defendPosition.getDistance(here);
            auto distCenter = station->getBase()->Center().getDistance(here);

            if (unit.hasTarget()) {
                auto target = unit.getTarget().lock();
                if (Terrain::inTerritory(PlayerState::Self, target->getPosition()) && target->getPosition().getDistance(station->getBase()->Center()) < distCenter)
                    continue;
            }

            if (distDefend < distBest && !ownForwardBase(station) && (closerThanSim(defendPosition) || alreadyInArea(station))) {
                bestStation = station;
                distBest = distDefend;
            }
        }
        return bestStation;
    }

    std::vector<const BWEB::Station *> getStations(PlayerState player) {
        return getStations(player, [](auto) {
            return true;
        });
    }

    template<typename F>
    std::vector<const BWEB::Station *> getStations(PlayerState player, F &&pred) {
        vector<const BWEB::Station*> returnVector;
        for (auto &[station, stationPlayer] : stations) {
            if (stationPlayer == player && pred(station))
                returnVector.push_back(station);
        }
        return returnVector;
    }

    double getStationSaturation(const BWEB::Station * station) {
        for (auto &[saturation, s] : stationsBySaturation) {
            if (s == station)
                return saturation;
        }
        return 0.0;
    }

    BWEB::Path getPathBetween(const BWEB::Station * station1, const BWEB::Station * station2)
    {
        auto station1ptr = stationNetwork.find(station1);
        if (station1ptr != stationNetwork.end()) {
            auto station2ptr = station1ptr->second.find(station2);
            if (station2ptr != station1ptr->second.end())
                return station2ptr->second;
        }
        return {};
    }

    std::map<const BWEB::Station *, BWEB::Path>& getStationNetwork(const BWEB::Station * station)
    {
        auto stationptr = stationNetwork.find(station);
        return stationptr->second;
    }

    Position getDefendPosition(const BWEB::Station * const station) { return defendPositions[station]; }
    multimap<double, const BWEB::Station * const>& getStationsBySaturation() { return stationsBySaturation; }
    multimap<double, const BWEB::Station * const>& getStationsByProduction() { return stationsByProduction; }
    int getGasingStationsCount() { return gasingStations; }
    int getMiningStationsCount() { return miningStations; }
    int getGroundDefenseCount(const BWEB::Station * const station) { return groundDefenseCount[station]; }
    int getAirDefenseCount(const BWEB::Station * const station) { return airDefenseCount[station]; }
    int getMineralsRemaining(const BWEB::Station * const station) { return remainingMinerals[station]; }
    int getGasRemaining(const BWEB::Station * const station) { return remainingGas[station]; }
    int getMineralsInitial(const BWEB::Station * const station) { return initialMinerals[station]; }
    int getGasInitial(const BWEB::Station * const station) { return initialGas[station]; }
}