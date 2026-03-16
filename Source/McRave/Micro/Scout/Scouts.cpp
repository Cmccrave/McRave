#include "Scouts.h"

#include "Builds/All/BuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Info/Unit/Units.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Micro/All/Commands.h"
#include "Micro/Worker/Workers.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Scouts {

    namespace {

        bool contained  = false;

        bool reachable(Position p)
        {
            auto area = mapBWEM.GetArea(TilePosition(p));
            auto main = Terrain::getMainArea();
            return area && main && area->AccessibleFrom(main);
        }

        enum class ScoutType { None, Main, Natural, Proxy, Safe, Army, Expansion };

        struct ScoutTarget {
            Position center = Positions::Invalid;
            ScoutType type  = ScoutType::None;
            double dist     = 1.0;
            map<UnitType, int> currentTypeCounts;
            map<UnitType, int> desiredTypeCounts;
            vector<Position> positions;

            ScoutTarget() {}

            bool operator<(const ScoutTarget &r) const { return dist < r.dist; }

            void addTargets(Position here, int radius = 0)
            {
                auto sRadius = int(round(1.5 * radius));
                if (here.isValid() && radius == 0) {
                    positions.push_back(here);
                    return;
                }

                vector<Position> offsets = {here,
                                            here + Position(sRadius, 0),
                                            here + Position(-sRadius, 0),
                                            here + Position(0, sRadius),
                                            here + Position(0, -sRadius),
                                            here + Position(radius, radius),
                                            here + Position(-radius, radius),
                                            here + Position(radius, -radius),
                                            here + Position(-radius, -radius)};
                for_each(offsets.begin(), offsets.end(), [&](auto p) {
                    if (p.isValid())
                        positions.push_back(Util::clipPosition(p));
                });
            }

            bool isExplored()
            {
                for (auto &p : positions) {
                    if (!Broodwar->isExplored(TilePosition(p)))
                        return false;
                }
                return true;
            }
        };

        set<Position> unexploredMains, unexploredNaturals;
        map<ScoutType, ScoutTarget> scoutTargets;
        map<UnitType, int> scoutTypeDeaths;

        bool mainScouted       = false;
        bool natScouted        = false;
        bool fullScout         = false;
        bool info              = false;
        bool sacrifice         = false;
        bool workerScoutDenied = false;
        bool firstOverlord     = false;
        bool secondOverlord    = false;
        vector<const BWEB::Station *> scoutOrder, scoutOrderFirstOverlord, scoutOrderSecondOverlord;
        map<const BWEB::Station *const, Position> safePositions;
        bool resourceWalkPossible[256][256];
        UnitType workerType;

        void drawScouting()
        {
            int i = 0;
            for (auto &[scoutType, target] : scoutTargets) {
                Visuals::drawBox(target.center - Position(64, 64), target.center + Position(64, 64), Colors::White);

                for (auto &p : target.positions)
                    Visuals::drawLine(p, target.center, Colors::White);

                auto y = 62;
                for (auto &[type, count] : target.currentTypeCounts) {
                    Broodwar->drawTextMap(target.center - Position(58, y), "%c%s: %d/%d", Text::White, type.c_str(), count, target.desiredTypeCounts[type]);
                    y -= 16;
                }
                Broodwar->drawTextMap(target.center, "%d", i);
                i++;
            }
        }

        void updateMisc()
        {
            scoutTargets.clear();
            unexploredMains.clear();
            unexploredNaturals.clear();

            // Calculate the number of unexplored bases
            for (auto &station : BWEB::Stations::getStations()) {
                Position center = Position(station.getBase()->Location()) + Position(64, 48);

                if (!station.isMain() && !station.isNatural())
                    continue;

                auto &list = station.isMain() ? unexploredMains : unexploredNaturals;
                if (!Stations::isBaseExplored(&station))
                    list.insert(station.getBase()->Center());
            }

            // Determine a full scout is done or not
            static auto loggedFullScout = false;
            fullScout                   = mainScouted && natScouted;
            if (Players::ZvZ())
                fullScout = mainScouted;

            // Log
            if (mainScouted && !loggedFullScout) {
                LOG("Full scout complete");
                loggedFullScout = true;
            }

            // Determine if we are lightly contained such that a scout cant get out
            auto choke         = Terrain::isPocketNatural() ? Terrain::getMainChoke() : Terrain::getNaturalChoke();
            auto closestRanged = Util::getClosestUnit(Position(choke->Center()), PlayerState::Enemy,
                                                      [&](auto &u) { return u->getGroundRange() >= 64.0 && u->getPosition().getDistance(Position(Terrain::getNaturalChoke()->Center())) < 320.0; });
            contained          = closestRanged != nullptr;
        }

        void checkScoutDenied()
        {
            if (workerScoutDenied || Util::getTime() < Time(3, 30) || !Terrain::getEnemyStartingPosition().isValid())
                return;

            auto closestWorker = Util::getFurthestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) { return u->getType().isWorker() && u->getRole() == Role::Scout; });
            if (closestWorker) {

                auto closestEnemy = Util::getClosestUnit(closestWorker->getPosition(), PlayerState::Self,
                                                         [&](auto &u) { return u->getTilePosition().isValid() && !u->getType().isWorker() && !u->getType().isBuilding(); });

                auto closestMain    = BWEB::Stations::getClosestMainStation(closestWorker->getTilePosition());
                auto closeToChoke   = closestMain && closestMain->getChokepoint() && closestWorker->getPosition().getDistance(Position(closestMain->getChokepoint()->Center())) < 160.0;
                auto deniedFromMain = closestMain && mapBWEM.GetArea(closestWorker->getTilePosition()) != closestMain->getBase()->GetArea() &&
                                      Grids::getGroundThreat(closestWorker->getPosition(), PlayerState::Enemy) > 0.0f;
                auto enemyInMain = closestEnemy && mapBWEM.GetArea(closestEnemy->getTilePosition()) == closestMain->getBase()->GetArea();

                if (closeToChoke && deniedFromMain) {
                    LOG("Worker scout denied");
                    workerScoutDenied = true;
                }
            }
        }

        void updateMainScouting()
        {
            auto &main = scoutTargets[ScoutType::Main];

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {

                main.desiredTypeCounts[Protoss_Probe] = int(BuildOrder::shouldScout()) +
                                                        int(Players::PvZ() && !Terrain::getEnemyStartingPosition().isValid() && mapBWEM.StartingLocations().size() == 4 &&
                                                            unexploredMains.size() == 2) +
                                                        int(Players::PvP() && (Spy::enemyProxy() || Spy::enemyPossibleProxy()) && com(Protoss_Zealot) < 1);

                if ((Players::PvZ() && Spy::enemyRush() && Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 2) || (Players::PvT() && (Spy::enemyPressure() || Spy::enemyWalled())) ||
                    (Util::getTime() > Time(5, 00)))
                    main.desiredTypeCounts[Protoss_Probe] = 0;
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                main.desiredTypeCounts[Terran_SCV] = (BuildOrder::shouldScout() || Spy::enemyPossibleProxy() || Spy::enemyProxy());
                if (Util::getTime() > Time(4, 00) || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    main.desiredTypeCounts[Terran_SCV] = 0;
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {

                auto enemyStrength = Players::getStrength(PlayerState::Enemy);
                auto enemyAir = enemyStrength.groundToAir > 0.0 || enemyStrength.airToAir > 0.0 || enemyStrength.airDefense > 0.0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 ||
                                Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0 ||
                                Players::getTotalCount(PlayerState::Enemy, Protoss_Cybernetics_Core) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0 ||
                                Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) > 0 ||
                                Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Marine) > 0 ||
                                Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) > 0 || (Players::ZvT() && Terrain::getEnemyStartingPosition().isValid());

                // Main drone scouting counts
                main.desiredTypeCounts[Zerg_Drone] = int(BuildOrder::shouldScout()) + int(BuildOrder::shouldScout() && BuildOrder::isProxy());
                if (workerScoutDenied || Spy::enemyProxy() || Spy::enemyRush() || Spy::enemyWalled() || Spy::enemyFastExpand() || (total(Zerg_Zergling) >= 6 && Util::getTime() > Time(3, 30)))
                    main.desiredTypeCounts[Zerg_Drone] = 0;

                // ZvT
                if (Players::ZvT()) {

                    // Drone
                    if (Spy::getEnemyOpener() == T_8Rax || Players::getTotalCount(PlayerState::Enemy, Terran_Marine) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 ||
                        Players::getTotalCount(PlayerState::Enemy, Terran_Bunker) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Factory) > 0 ||
                        (Terrain::getEnemyStartingPosition().isValid() && unexploredNaturals.empty() && Util::getTime() > Time(3, 30)) || Spy::getEnemyTransition() == U_WorkerRush ||
                        Util::getTime() > Time(4, 00))
                        main.desiredTypeCounts[Zerg_Drone] = 0;

                    // Overlord
                    main.desiredTypeCounts[Zerg_Overlord] = 2;
                    if (Terrain::getEnemyStartingPosition().isValid())
                        main.desiredTypeCounts[Zerg_Overlord] = 1;
                    if (enemyAir || Spy::enemyFastExpand() || (Terrain::getEnemyStartingPosition().isValid() && vis(Zerg_Zergling) > 0))
                        main.desiredTypeCounts[Zerg_Overlord] = 0;
                }

                // ZvP
                if (Players::ZvP()) {

                    // Drone
                    auto sawZealotTiming = (Spy::getEnemyBuild() == P_2Gate && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 3) ||
                                           (Spy::getEnemyBuild() == P_1GateCore && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) > 0);
                    if (sawZealotTiming || Spy::getEnemyBuild() == P_FFE || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 ||
                        Players::getCompleteCount(PlayerState::Enemy, Protoss_Cybernetics_Core) > 0 || fullScout || Util::getTime() > Time(3, 30))
                        main.desiredTypeCounts[Zerg_Drone] = 0;

                    // Zergling
                    main.desiredTypeCounts[Zerg_Zergling] = Spy::getEnemyTransition() == "Unknown" && Util::getTime() > Time(5, 00);
                    if (BuildOrder::isRush() || Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0 ||
                        (com(Zerg_Sunken_Colony) > 0 && !Terrain::getEnemyStartingPosition().isValid()))
                        main.desiredTypeCounts[Zerg_Zergling] = 0;

                    // Overlord
                    main.desiredTypeCounts[Zerg_Overlord] = 2;
                    if (Terrain::getEnemyStartingPosition().isValid())
                        main.desiredTypeCounts[Zerg_Overlord] = 1;
                    if (enemyAir || Spy::enemyFastExpand() || (Terrain::getEnemyStartingPosition().isValid() && vis(Zerg_Zergling) > 0))
                        main.desiredTypeCounts[Zerg_Overlord] = 0;
                }

                // ZvZ
                if (Players::ZvZ()) {

                    // Drone
                    main.desiredTypeCounts[Zerg_Drone] = 0;

                    // Zergling
                    main.desiredTypeCounts[Zerg_Zergling] = (!Terrain::foundEnemy() && !Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost) &&
                                                             Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost)) ||
                                                            (!Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost) &&
                                                             Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) <= 10);
                    if (BuildOrder::isRush() || Spy::enemyRush() || Spy::enemyPressure() || Spy::enemyTurtle() || Spy::enemyFortress())
                        main.desiredTypeCounts[Zerg_Zergling] = 0;

                    // Overlord
                    main.desiredTypeCounts[Zerg_Overlord] = 2;
                    if (Terrain::getEnemyStartingPosition().isValid())
                        main.desiredTypeCounts[Zerg_Overlord] = 1;
                    if (enemyAir)
                        main.desiredTypeCounts[Zerg_Overlord] = 0;
                }

                // ZvR
                if (Players::ZvR()) {
                    main.desiredTypeCounts[Zerg_Overlord] = 2;
                }
            }

            // Check for fully scouted
            if (Terrain::getEnemyMain()) {
                main.center = Terrain::getEnemyMain()->getBase()->Center();
                main.addTargets(Position(Terrain::getEnemyMain()->getBase()->Center()), 256);

                // Determine if we scouted the main sufficiently
                if (!mainScouted) {
                    for (auto &[type, target] : scoutTargets) {
                        if (type == ScoutType::Main) {
                            auto exploredCount = count_if(target.positions.begin(), target.positions.end(), [&](auto &p) { return Broodwar->isExplored(TilePosition(p)); });
                            if (exploredCount > (Players::ZvZ() ? 2 : 5) && Broodwar->isExplored(TilePosition(target.center)))
                                mainScouted = true;
                        }
                    }
                }

                // Add enemy geyser as a scout
                if (Terrain::getEnemyMain() && mainScouted && !Stations::isGeyserExplored(Terrain::getEnemyMain()) && Util::getTime() < Time(3, 45)) {
                    for (auto &geyser : Resources::getMyGas()) {
                        if (geyser->getStation() == Terrain::getEnemyMain())
                            main.addTargets(Position(geyser->getPosition()));
                    }
                }

                // Add each enemy building as a target
                for (auto &unit : Units::getUnits(PlayerState::Enemy)) {
                    if (mainScouted && unit->getType().isBuilding() && unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition()) == Terrain::getEnemyMain()->getBase()->GetArea())
                        main.addTargets(unit->getPosition(), 96);
                }

                // Add main choke as a target
                if (Terrain::getEnemyMain() && mainScouted && Terrain::getEnemyMain()->getChokepoint() && Stations::isBaseExplored(Terrain::getEnemyMain()))
                    main.addTargets(Position(Terrain::getEnemyMain()->getChokepoint()->Center()));
            }
        }

        void updateNaturalScouting()
        {
            auto &natural = scoutTargets[ScoutType::Natural];
            auto &main    = scoutTargets[ScoutType::Main];

            // Check if fully scouted
            if (Terrain::getEnemyNatural()) {
                natural.center = Terrain::getEnemyNatural()->getBase()->Center();
                if (Stations::isBaseExplored(Terrain::getEnemyNatural()))
                    natScouted = true;

                // Add natural position as a target
                if (Util::getTime() > Time(4, 00) || Players::TvZ() || Players::PvZ())
                    natural.addTargets(Terrain::getEnemyNatural()->getBase()->Center());

                // If we scouted the main, scout the nat to get a full scout
                if (Broodwar->self()->getRace() == Races::Zerg) {
                    if (mainScouted && !natScouted && main.desiredTypeCounts[Zerg_Drone] > 0) {
                        main.desiredTypeCounts[Zerg_Drone]    = 0;
                        natural.desiredTypeCounts[Zerg_Drone] = 1;
                    }
                }
            }
        }

        void updateProxyScouting()
        {
            auto &main  = scoutTargets[ScoutType::Main];
            auto &proxy = scoutTargets[ScoutType::Proxy];

            const auto closestProxyBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Enemy, [&](auto &u) { return u->getType().isBuilding() && u->isProxy(); });

            // Against known proxies without visible proxy style buildings
            if (Spy::enemyProxy() && !closestProxyBuilding) {
                if (Spy::getEnemyBuild() == P_CannonRush) {
                    proxy.center = mapBWEM.Center();
                    proxy.addTargets(Terrain::getMainPosition(), 200);
                }
                else {
                    proxy.center = mapBWEM.Center();
                    proxy.addTargets(mapBWEM.Center(), 200);
                }
            }

            // Check a small area around a proxy building
            if (closestProxyBuilding) {
                proxy.center = closestProxyBuilding->getPosition();
                proxy.addTargets(closestProxyBuilding->getPosition());
            }

            auto scoutMiddle = (!scoutOrder.empty() && Stations::isBaseExplored(scoutOrder.front())) || Broodwar->getStartLocations().size() >= 4;

            // Scout the popular middle proxy location if it's walkable
            if (scoutMiddle && !Players::vZ() && !Terrain::foundEnemy() && mapBWEM.GetArea(TilePosition(mapBWEM.Center())) && !Terrain::isExplored(mapBWEM.Center()) &&
                mapBWEM.GetArea(TilePosition(mapBWEM.Center()))->AccessibleFrom(Terrain::getMainArea())) {
                proxy.addTargets(mapBWEM.Center(), 64);
                proxy.center = mapBWEM.Center();
            }

            // Only scout if unexplored
            proxy.desiredTypeCounts[Zerg_Drone] = 0;
            if (proxy.center.isValid() && !proxy.isExplored() && BuildOrder::shouldScout()) {
                main.desiredTypeCounts[Zerg_Drone]  = 0;
                proxy.desiredTypeCounts[Zerg_Drone] = 1;
            }
        }

        void updateSafeScouting()
        {
            auto &safe = scoutTargets[ScoutType::Safe];
            auto &main = scoutTargets[ScoutType::Main];

            if (Terrain::getEnemyNatural() && safePositions[Terrain::getEnemyNatural()].isValid()) {
                safe.center = safePositions[Terrain::getEnemyNatural()];

                // Zerg
                if (Broodwar->self()->getRace() == Races::Zerg) {

                    safe.desiredTypeCounts[Zerg_Overlord] = 1;
                    if (total(Zerg_Mutalisk) >= 6 || (Players::ZvP() && Util::getTime() > Time(7, 00)) || (Players::ZvT() && Util::getTime() > Time(7, 00)) ||
                        (Players::ZvZ() && Util::getTime() > Time(5, 00)))
                        safe.desiredTypeCounts[Zerg_Overlord] = 0;

                    // When terran is rushing, don't risk losing an overlord for information
                    if (Players::ZvT() && Spy::enemyRush())
                        safe.desiredTypeCounts[Zerg_Overlord] = 0;
                }

                // Overlord scouting sometimes needed
                auto safeToScout = !Spy::enemyFastExpand();
                safe.addTargets(safePositions[Terrain::getEnemyNatural()]);
                if (safeToScout) {
                    if (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(TilePosition(Terrain::getEnemyNatural()->getBase()->Center())) >= 200 && Util::getTime() > Time(4, 00))
                        safe.addTargets(Terrain::getEnemyNatural()->getBase()->Center());
                }
            }
        }

        void updateArmyScouting()
        {
            if (!Terrain::getEnemyNatural() || !Terrain::getMyNatural() || BuildOrder::isRush())
                return;
            auto &army                            = scoutTargets[ScoutType::Army];
            army.desiredTypeCounts[Zerg_Zergling] = 0;

            // No threat at home, we should use a ling to scout the enemy
            if (Broodwar->self()->getRace() == Races::Zerg) {
                auto time = Time(2, 30);
                if (Spy::enemyRush() || Spy::getEnemyBuild() == P_2Gate || Spy::getEnemyBuild() == P_1GateCore || Spy::enemyProxy() || total(Zerg_Zergling) <= 2)
                    time = Time(3, 45);
                if (Spy::getEnemyTransition() == U_WorkerRush)
                    time = Time(6, 00);

                if (Util::getTime() > time) {
                    if ((Players::ZvT() && Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) == 0) || (Players::ZvP() && Spy::enemyFastExpand() && Util::getTime() < Time(9, 00)) ||
                        (Players::ZvP() && !Spy::enemyFastExpand() && Util::getTime() < Time(8, 00)))
                        army.desiredTypeCounts[Zerg_Zergling] = 1 + (int(Spy::getEnemyTransition() == "Unknown") && Util::getTime() > Time(4, 00));
                }
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                if (Util::getTime() > Time(4, 00))
                    army.desiredTypeCounts[Terran_SCV] = 1;
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Util::getTime() > Time(4, 00))
                    army.desiredTypeCounts[Protoss_Probe] = 1;
            }

            // Path backwards to stop on last threatening tile (Ralph scouting)
            auto station = Terrain::isPocketNatural() ? Terrain::getEnemyMain() : Terrain::getEnemyNatural();
            auto start   = Position(station->getChokepoint()->Center());
            auto end     = Position(Terrain::getNaturalChoke()->Center());

            // Army scouting between my natural and enemy natural
            BWEB::Path newPath(start, end, Zerg_Zergling);
            newPath.generateJPS([&](auto t) { return newPath.terrainWalkable(t); });
            auto primarySafeTarget   = Util::findPointOnPath(newPath, [&](auto &t) { return Grids::getGroundThreat(t, PlayerState::Enemy) <= 0.1; });
            auto secondarySafeTarget = Util::findPointOnPath(newPath,
                                                             [&](auto &t) { return Position(t).getDistance(primarySafeTarget) > 96.0 && Grids::getGroundThreat(t, PlayerState::Enemy) <= 0.1; });

            army.center = primarySafeTarget;
            army.addTargets(secondarySafeTarget, 96);

            // If they haven't expanded, check it occasionally, unless we really need army info
            if (!Spy::enemyFastExpand() && !contained)
                army.addTargets(station->getBase()->Center());
        }

        void updateExpansionScouting()
        {
            auto &army = scoutTargets[ScoutType::Army];
            if (Players::ZvZ() || Util::getTime() < Time(7, 00))
                return;

            auto &expansion                            = scoutTargets[ScoutType::Expansion];
            expansion.desiredTypeCounts[Zerg_Zergling] = 1;

            // If we keep seeing workers outside their territory, we need to look, they might be transferring
            if (Util::getTime() >= Time(7, 00) && Spy::getWorkersPulled() >= 5) {
                expansion.desiredTypeCounts[Zerg_Zergling] = 2;
            }

            for (auto &station : Stations::getStations(PlayerState::None)) {

                // Only scout stations that are closer to the enemy
                auto closestSelf  = Stations::getClosestStationGround(station->getBase()->Center(), PlayerState::Self);
                auto closestEnemy = Stations::getClosestStationGround(station->getBase()->Center(), PlayerState::Enemy);

                if (!closestEnemy || !closestSelf ||
                    station->getBase()->Center().getDistance(closestEnemy->getBase()->Center()) < station->getBase()->Center().getDistance(closestSelf->getBase()->Center())) {
                    if (station != Terrain::getEnemyMain() && station != Terrain::getEnemyNatural()) {
                        expansion.center = station->getResourceCentroid();
                        expansion.addTargets(station->getResourceCentroid());
                    }
                }
            }
        }

        void updateScoutRoles()
        {
            const auto assign = [&](UnitType type) {
                UnitInfo *scout = nullptr;

                auto assignPos = Terrain::getMainPosition();
                if (Terrain::getEnemyStartingPosition().isValid())
                    assignPos = Terrain::getEnemyStartingPosition();
                else if (Terrain::getNaturalChoke())
                    assignPos = Position(Terrain::getNaturalChoke()->Center());

                // Proxy takes furthest from natural choke
                if (type.isWorker() && BuildOrder::isProxy()) {
                    scout = Util::getFurthestUnit(assignPos, PlayerState::Self, [&](auto &u) {
                        return u->getRole() == Role::Worker && u->getType() == type && (!u->hasResource() || !u->getResource().lock()->getType().isRefinery()) && u->getBuildType() == None &&
                               !u->unit()->isCarryingMinerals() && !u->unit()->isCarryingGas();
                    });
                }
                else if (type.isFlyer()) {
                    scout = Util::getClosestUnit(assignPos, PlayerState::Self, [&](auto &u) { return u->getRole() == Role::Support && u->getType() == type; });
                }
                else if (type.isWorker()) {
                    scout = Util::getClosestUnitGround(assignPos, PlayerState::Self, [&](auto &u) {
                        return u->getRole() == Role::Worker && u->getType() == type && (!u->hasResource() || !u->getResource().lock()->getType().isRefinery()) && u->getBuildType() == None &&
                               !u->unit()->isCarryingMinerals() && !u->unit()->isCarryingGas();
                    });
                }
                else {
                    scout = Util::getClosestUnitGround(assignPos, PlayerState::Self, [&](auto &u) { return u->getRole() == Role::Combat && u->getType() == type && u->isHealthy(); });
                }

                if (scout) {
                    scout->setRole(Role::Scout);
                    scout->setBuildingType(None);
                    scout->setBuildPosition(TilePositions::Invalid);

                    if (scout->hasResource())
                        Workers::removeUnit(*scout);
                }
            };

            const auto remove = [&](UnitType type) {
                UnitInfo *scout = nullptr;
                if (type.isFlyer()) {
                    scout = Util::getFurthestUnit(Terrain::getEnemyStartingPosition(), PlayerState::Self, [&](auto &u) { return u->getRole() == Role::Scout && u->getType() == type; });
                }
                else {
                    scout = Util::getClosestUnitGround(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) { return u->getRole() == Role::Scout && u->getType() == type; });
                }

                if (scout)
                    scout->setRole(Role::None);
            };

            map<UnitType, int> totalCurrentScoutTypeCounts;
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Scout)
                    totalCurrentScoutTypeCounts[unit.getType()]++;
            }

            map<UnitType, int> totalDesiredScoutTypeCounts;
            for (auto &[_, target] : scoutTargets) {
                for (auto &[type, count] : target.desiredTypeCounts)
                    totalDesiredScoutTypeCounts[type] += count;
            }

            for (auto &[type, count] : totalDesiredScoutTypeCounts) {
                if (scoutTypeDeaths[type] > 0 && (type != Zerg_Zergling || Players::ZvZ()))
                    continue;
                if (totalCurrentScoutTypeCounts[type] < totalDesiredScoutTypeCounts[type] && !contained)
                    assign(type);
                else if (totalCurrentScoutTypeCounts[type] > totalDesiredScoutTypeCounts[type])
                    remove(type);
            }
        }

        void updateScoutOrder()
        {
            scoutOrder.clear();
            scoutOrderFirstOverlord.clear();
            scoutOrderSecondOverlord.clear();
            vector<const BWEB::Station *> mainStations;

            // Get first natural by air for overlord order
            const BWEB::Station *closestNatural  = nullptr;
            const BWEB::Station *furthestNatural = nullptr;
            auto closest                         = DBL_MAX;
            auto furthest                        = 0.0;
            for (auto &station : BWEB::Stations::getStations()) {

                // Add to main stations
                if (station.isMain() && &station != Terrain::getMyMain())
                    mainStations.push_back(&station);

                if (!station.isNatural() || station == Terrain::getMyNatural())
                    continue;

                auto dist = station.getBase()->Center().getDistance(Terrain::getMyMain()->getBase()->Center());
                if (dist < closest) {
                    closestNatural = &station;
                    closest        = dist;
                }

                if (dist > furthest) {
                    furthestNatural = &station;
                    furthest        = dist;
                }
            }

            if (closestNatural) {

                // Create overlord scouting order
                auto startingPosition = closestNatural->getBase()->Center();
                sort(mainStations.begin(), mainStations.end(),
                     [&](auto &lhs, auto &rhs) { return lhs->getBase()->Center().getDistance(startingPosition) < rhs->getBase()->Center().getDistance(startingPosition); });
                for (auto &station : mainStations)
                    scoutOrderFirstOverlord.push_back(station);

                // Second overlord only goes cross spawn
                if (scoutTargets[ScoutType::Main].desiredTypeCounts[Zerg_Drone] == 0) {
                    scoutOrderSecondOverlord = scoutOrderFirstOverlord;
                    reverse(scoutOrderSecondOverlord.begin(), scoutOrderSecondOverlord.end());
                }
                else
                    scoutOrderSecondOverlord.push_back(furthestNatural);

                // Create worker scouting order
                scoutOrder = scoutOrderFirstOverlord;

                // Sometimes proxies get placed at the 3rd closest Station
                if (Players::ZvP() && !Terrain::getEnemyStartingPosition().isValid() && Broodwar->mapFileName().find("Outsider") != string::npos) {
                    auto thirdStation = Stations::getClosestStationAir(Terrain::getMainPosition(), PlayerState::None,
                                                                       [&](auto &station) { return station != Terrain::getMyMain() && station != Terrain::getMyNatural(); });
                    if (thirdStation)
                        scoutOrder.push_back(thirdStation);
                }

                if (!Players::vFFA()) {
                    reverse(scoutOrder.begin(), scoutOrder.end());
                }
            }
        }

        void updateDestination(UnitInfo &unit)
        {
            auto distBest = DBL_MAX;

            if (unit.sacrifice) {
                unit.setDestination(Terrain::getEnemyStartingPosition());
                return;
            }

            // If we have scout targets, find the closest scout target
            auto best = -1.0;
            for (auto &[type, target] : scoutTargets) {

                if (unit.getDestination().isValid() || !target.center.isValid() || target.currentTypeCounts[unit.getType()] >= target.desiredTypeCounts[unit.getType()])
                    continue;

                // Set to the center by default, increment current counts here
                if (unit.isFlying() || reachable(target.center))
                    unit.setDestination(target.center);
                target.currentTypeCounts[unit.getType()]++;

                // If we're scouting safe locations, stay at the center when injured
                if (unit.isFlying() && target.type == ScoutType::Safe && unit.getPercentTotal() < 0.9) {
                    unit.setDestination(target.center);
                    continue;
                }

                // Find the best position to scout
                for (auto &pos : target.positions) {
                    auto time     = Grids::getLastVisibleFrame(TilePosition(pos));
                    auto timeDiff = Broodwar->getFrameCount() - time;
                    auto score    = double(timeDiff) / (1.0 + target.dist);

                    if (!unit.isFlying() && !reachable(pos))
                        continue;
                    if (!Broodwar->isExplored(TilePosition(pos))) {
                        unit.setDestination(pos);
                        break;
                    }

                    if (score > best) {
                        best = score;
                        unit.setDestination(pos);
                    }
                }

                // Remove the scouted position so we don't duplicate effort
                auto itr = find(target.positions.begin(), target.positions.end(), unit.getDestination());
                if (itr != target.positions.end())
                    target.positions.erase(itr);
            }

            // Use scouting order if we don't know where the enemy is
            if (!Terrain::getEnemyMain() && !unit.getDestination().isValid()) {

                auto list = scoutOrder;
                if (unit.getType() == Zerg_Overlord) {
                    if (firstOverlord) {
                        list          = scoutOrderFirstOverlord;
                        firstOverlord = false;
                    }
                    else if (secondOverlord) {
                        list           = scoutOrderSecondOverlord;
                        secondOverlord = false;
                    }
                }

                for (auto &station : list) {
                    auto closestNatural = BWEB::Stations::getClosestNaturalStation(station->getBase()->Location());
                    if (closestNatural && !Stations::isBaseExplored(closestNatural) && unit.getType() == Zerg_Overlord && !Players::ZvZ()) {
                        unit.setDestination(closestNatural->getBase()->Center());
                        break;
                    }
                    if (!Stations::isBaseExplored(station)) {
                        unit.setDestination(station->getBase()->Center());
                        break;
                    }
                }
            }

            // If a destination is still not assigned, just scout the enemy main
            if (!unit.getDestination().isValid()) {
                unit.setDestination(Terrain::getEnemyStartingPosition());
            }

            unit.setNavigation(unit.getDestination());
            Visuals::drawLine(unit.getPosition(), unit.getDestination(), Colors::Yellow);
        }

        void updateSacrifice(UnitInfo &unit)
        {
            static auto sacrificeCount = 0;

            // Sometimes we need to just sacrifice a zergling to get some info based on timings
            if (!BuildOrder::isRush() && Terrain::getEnemyStartingPosition().isValid() && unit.getType() == Zerg_Zergling) {
                if (Players::ZvP()) {

                    // 3hm needs to know whether to switch to hydras or stay mutas
                    // 6hh needs to know whether to ling flood or stay hydras
                    if (Spy::getEnemyBuild() == P_FFE && Util::getTime() > Time(6, 00) && sacrificeCount == 0) {
                        unit.sacrifice = true;
                        unit.setDestinationPath(BWEB::Path());
                        sacrificeCount++;
                        LOG("Sacrificing a scout (count: %d)", sacrificeCount);
                    }

                    // 3hh needs to know whether to drone up or bust with hydras
                    if (Spy::getEnemyBuild() == P_FFE && BuildOrder::getCurrentTransition() == Z_3HatchHydra && Util::getTime() > Time(4, 30) && sacrificeCount == 0) {
                        unit.sacrifice = true;
                        unit.setDestinationPath(BWEB::Path());
                        sacrificeCount++;
                        LOG("Sacrificing a scout (count: %d)", sacrificeCount);
                    }

                    // Need to try and figure out what the tech off 1 base is
                    if (Spy::getEnemyBuild() != P_FFE && !Spy::enemyFastExpand() && Util::getTime() > Time(3, 30) && sacrificeCount == 0) {
                        unit.sacrifice = true;
                        unit.setDestinationPath(BWEB::Path());
                        sacrificeCount++;
                        LOG("Sacrificing a scout (count: %d)", sacrificeCount);
                    }
                }
                if (Players::ZvT()) {
                    if (!Terrain::foundEnemy() && Util::getTime() > Time(4, 00) && sacrificeCount == 0 && Spy::getEnemyBuild() == "Unknown") {
                        unit.sacrifice = true;
                        unit.setDestinationPath(BWEB::Path());
                        sacrificeCount++;
                        LOG("Sacrificing a scout (count: %d)", sacrificeCount);
                    }
                    if (!Spy::enemyFastExpand() && Util::getTime() > Time(4, 45) && sacrificeCount == 1) {
                        unit.sacrifice = true;
                        unit.setDestinationPath(BWEB::Path());
                        sacrificeCount++;
                        LOG("Sacrificing a scout (count: %d)", sacrificeCount);
                    }
                }
            }
        }

        void updatePath(UnitInfo &unit)
        {
            if (unit.isFlying())
                return;

            auto pathPoint      = unit.getFormation().isValid() ? Util::getPathPoint(unit, unit.getFormation()) : Util::getPathPoint(unit, unit.getDestination());
            auto newPathAllowed = !mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) ||
                                  mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)));

            if (newPathAllowed && !unit.hasSamePath(unit.getPosition(), pathPoint)) {

                BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());

                const auto threat   = [&](const TilePosition &t) { return Grids::getGroundThreat(t, PlayerState::Enemy) * 1000.0; };
                const auto walkable = [&](const TilePosition &t) { return newPath.unitWalkable(t); };

                // Suicidal scouts don't care about threat
                if (unit.sacrifice)
                    newPath.generateJPS(walkable);
                else
                    newPath.generateAS(threat, walkable);
                unit.setDestinationPath(newPath);
            }
            Visuals::drawPath(unit.getDestinationPath());
            Visuals::drawLine(unit.getPosition(), pathPoint, Colors::Orange);
        }

        void updateNavigation(UnitInfo &unit)
        {
            auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) { return p.getDistance(unit.getPosition()) >= 96.0 && BWEB::Map::isUsed(TilePosition(p)) == None; });

            if (newDestination.isValid())
                unit.setNavigation(newDestination);
        }

        void updateDecision(UnitInfo &unit)
        {
            // Visuals::drawLine(unit.getPosition(), unit.getDestination(), Colors::Cyan);
            // Visuals::drawLine(unit.getPosition(), unit.getNavigation(), Colors::Orange);

            if (!Units::commandAllowed(unit))
                return;

            // Iterate commands, if one is executed then don't try to execute other commands
            static const auto commands = {Command::attack, Command::kite, Command::gather, Command::explore, Command::move};
            for (auto cmd : commands) {
                if (cmd(unit))
                    break;
            }
        }

        void updateScouts()
        {
            vector<std::weak_ptr<UnitInfo>> sortedScouts;
            firstOverlord  = true;
            secondOverlord = true;

            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Scout)
                    sortedScouts.push_back(unit.weak_from_this());
            }

            // Sort them by distance early on
            if (Util::getTime() < Time(5, 00)) {
                sort(sortedScouts.begin(), sortedScouts.end(), [&](auto &lhs, auto &rhs) {
                    return Terrain::getEnemyMain()
                               ? lhs.lock()->getPosition().getDistance(Terrain::getEnemyStartingPosition()) < rhs.lock()->getPosition().getDistance(Terrain::getEnemyStartingPosition())
                               : lhs.lock()->getPosition().getDistance(Terrain::getMainPosition()) > rhs.lock()->getPosition().getDistance(Terrain::getMainPosition());
                });
            }

            info = false;
            for (auto &u : sortedScouts) {
                if (auto unit = u.lock()) {
                    updateSacrifice(*unit);
                    updateDestination(*unit);
                    updatePath(*unit);
                    updateNavigation(*unit);
                    updateDecision(*unit);

                    // If we're in their territory or ralphing, we have info
                    if (Terrain::inTerritory(PlayerState::Enemy, unit->getPosition()) || unit->getType() == Zerg_Zergling)
                        info = true;
                }
            }
        }
    } // namespace

    void onFrame()
    {
        Visuals::startPerfTest();
        checkScoutDenied();
        updateMisc();
        updateMainScouting();
        updateNaturalScouting();
        updateSafeScouting();
        updateProxyScouting();
        updateArmyScouting();
        updateExpansionScouting();
        updateScoutOrder();
        updateScoutRoles();
        updateScouts();
        // drawScouting();
        Visuals::endPerfTest("Scouts");

        static bool safeDiscovered = false;
        if (!Terrain::getEnemyMain() || !Terrain::getEnemyNatural() || safeDiscovered) {
            return;
        }

        vector<const BWEB::Station *> enemyStations = {Terrain::getEnemyNatural(), Terrain::getEnemyMain()};
        safeDiscovered                              = true;
        safePositions[Terrain::getEnemyNatural()]   = Positions::Invalid;
        safePositions[Terrain::getEnemyMain()]      = Positions::Invalid;

        auto natWatch  = Stations::getDefendPosition(Terrain::getEnemyNatural());
        auto mainWatch = Stations::getDefendPosition(Terrain::getEnemyMain());
        set<TilePosition> safeTiles;
        set<TilePosition> enemyTiles;

        auto neighborAreas = Terrain::getEnemyNatural()->getBase()->GetArea()->AccessibleNeighbours();

        // Not accessible from "enemy territory"
        const auto tileInEnemyArea = [&](TilePosition tile) {
            for (auto x = 0; x < 4; x++) {
                for (auto y = 0; y < 4; y++) {
                    auto w = WalkPosition(tile) + WalkPosition(x, y);
                    if (Terrain::inTerritory(PlayerState::Enemy, Position(w)) || Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), Position(w)) ||
                        Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), Position(w)) || find(neighborAreas.begin(), neighborAreas.end(), mapBWEM.GetArea(w)) != neighborAreas.end() ||
                        (mapBWEM.GetArea(w) && !mapBWEM.GetArea(w)->AccessibleFrom(Terrain::getEnemyNatural()->getBase()->GetArea())))
                        return true;
                }
            }
            return false;
        };

        // Create a path from natural to natural, tiles around it with equal height or less are considered dangerous
        BWEB::Path path = {Terrain::getEnemyNatural()->getChokepoint()->Center(), Terrain::getMyNatural()->getChokepoint()->Center(), Protoss_Dragoon, true, false};
        path.generateJPS([&](auto t) { return path.unitWalkable(t); });

        for (auto &parent : path.getTiles()) {
            auto parentHeight = Broodwar->getGroundHeight(parent);

            Util::testAllPointOnPath(path, [&](auto &pos) {
                for (auto &t : Util::getTileCircle(6)) {
                    auto tile = TilePosition(pos) + t;
                    if (tile.isValid() && pos.isValid() && Broodwar->getGroundHeight(tile) >= parentHeight) {
                        enemyTiles.insert(tile);
                    }
                }
            });
        }

        // Designate tiles as either safe tiles, enemy tiles, or not useful
        const auto checkAround = [&](auto station) {
            auto watch = Stations::getDefendPosition(station);
            for (auto &t : Util::getTileCircle(20)) {
                auto tile = t + TilePosition(watch);
                auto pos  = Position(tile) + Position(16, 16);

                if (!tile.isValid())
                    continue;

                // Within range of overlord sight sort of
                auto withinChokeSight = pos.getDistance(watch) <= 480.0;
                auto enemyArea        = tileInEnemyArea(tile);

                if (enemyArea)
                    enemyTiles.insert(tile);
                else if (withinChokeSight)
                    safeTiles.insert(tile);
            }
        };

        for (auto &station : enemyStations)
            checkAround(station);

        set<Position> potentialSafePositions;
        for (auto &parent : safeTiles) {
            auto lowestHeight   = 4;
            auto expectedCenter = Position(parent + TilePosition(1, 1));
            auto valid          = true;
            for (int x = 0; x < 2; x++) {
                for (int y = 0; y < 2; y++) {
                    auto tile = parent + TilePosition(x, y);
                    if (tile.isValid())
                        lowestHeight = min(lowestHeight, Broodwar->getGroundHeight(tile));
                }
            }

            // Check a hollow box around for having an adjacent enemy area tile
            for (int x = -1; x < 3; x++) {
                int y = -1;
                for (; y < 3;) {
                    auto tile = parent + TilePosition(x, y);
                    if (tile.isValid() && enemyTiles.find(tile) != enemyTiles.end())
                        valid = false;

                    y += (x == -1 || x == 2) ? 1 : 3;
                }
            }

            if (valid) {
                // potentialSafePositions.insert(Position(parent + TilePosition(1, 1)));
            }

            // Each potential tile needs to look for a >= height tile within 5 range zvt, 6 range zvp
            auto range = Players::ZvT() ? 5 : 6;
            for (auto &w : Util::getWalkCircle(range * 4)) {
                auto pos  = Position(WalkPosition(parent) + w);
                auto tile = TilePosition(pos);

                if (Broodwar->getGroundHeight(tile) >= lowestHeight && pos.getDistance(expectedCenter) < range * 32 && enemyTiles.find(tile) != enemyTiles.end())
                    valid = false;
            }

            if (valid) {
                potentialSafePositions.insert(expectedCenter);
            }
        }

        // Get closest safe position
        auto distBest = DBL_MAX;
        for (auto &pos : potentialSafePositions) {
            auto dist = pos.getDistance(Position(Terrain::getEnemyNatural()->getChokepoint()->Center()));
            if (dist < distBest) {
                safePositions[Terrain::getEnemyNatural()] = pos;
                distBest                                  = dist;
            }
        }
    }

    void onStart()
    {
        //// Attempt to find a position that is hard to reach for enemy ranged units
        // for (auto &station : BWEB::Stations::getStations()) {
        //    auto closestMain = BWEB::Stations::getClosestMainStation(TilePosition(station.getBase()->Center()));
        //    if (!station.getChokepoint()
        //        || !closestMain
        //        || !closestMain->getChokepoint())
        //        continue;

        //    const auto closestWatchable = [&](auto &t, auto &p) {
        //        auto closest = 320.0;
        //        for (int x = -8; x <= 8; x++) {
        //            for (int y = -8; y <= 8; y++) {
        //                const auto tile = t + TilePosition(x, y);
        //                if (!tile.isValid())
        //                    continue;

        //                const auto enemyPos = Position(tile) + Position(16, 16);
        //                const auto boxDist = Util::boxDistance(Zerg_Overlord, p, Protoss_Dragoon, enemyPos);
        //                const auto walkableAndConnected = BWEB::Map::isWalkable(tile, Protoss_Dragoon) && mapBWEM.GetArea(tile) && !mapBWEM.GetArea(tile)->AccessibleNeighbours().empty();

        //                if (boxDist < closest && walkableAndConnected) {
        //                    closest = boxDist;
        //                }
        //            }
        //        }
        //        return closest;
        //    };

        //    auto distBest = 0.0;
        //    auto posBest = Position(station.getChokepoint()->Center());
        //    for (int x = -14; x < 14; x++) {
        //        for (int y = -14; y < 14; y++) {
        //            auto tile = TilePosition(station.getChokepoint()->Center()) + TilePosition(x, y);
        //            auto pos = Position(tile) + Position(16, 16);
        //            auto dist = closestWatchable(tile, pos);

        //            if (!pos.isValid() || !tile.isValid())
        //                continue;

        //            if (dist > distBest) {
        //                distBest = dist;
        //                posBest = pos;
        //            }
        //        }
        //    }

        //    Visuals::drawCircle(posBest, 4, Colors::Cyan, false);
        //    safePositions[&station] = posBest;
        //}

        //// Create a grid of positions that have mineral walking capabilities
        // for (int x = 0; x < 256; x++) {
        //    for (int y = 0; y < 256; y++) {
        //        const TilePosition t = TilePosition(x, y);
        //        const Position p = Position(t) + Position(16, 16);

        //        auto resource = Resources::getClosestResource(p, [&](auto &r) {
        //            return r->getPosition().getDistance(p) < 128.0;
        //        });
        //        if (resource && BWEB::Map::isWalkable(t, Protoss_Dragoon) && !BWEB::Map::isUsed(t).isMineralField() && !BWEB::Map::isUsed(t).isRefinery())
        //            resourceWalkPossible[x][y] = true;
        //    }
        //}
    }

    void removeUnit(UnitInfo &unit) { scoutTypeDeaths[unit.getType()]++; }

    bool gatheringInformation() { return info; }
    bool gotFullScout() { return fullScout; }
    bool isSacrificeScout() { return sacrifice; }
    bool enemyDeniedScout() { return workerScoutDenied; }
    vector<const BWEB::Station *> getScoutOrder(UnitType type)
    {
        if (type == Zerg_Overlord || !Players::ZvZ())
            return scoutOrderFirstOverlord;
        return scoutOrder;
    }
} // namespace McRave::Scouts