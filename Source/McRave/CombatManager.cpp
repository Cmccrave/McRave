#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {
        int lastRoleChange = 0;
        set<Position> retreatPositions;
        pair<double, Position> airCluster;
        multimap<double, Position> combatClusters;
        multimap<double, UnitInfo&> combatUnitsByDistance;
        map<const BWEM::ChokePoint*, vector<WalkPosition>> concaveCache;
        map<TilePosition, int> testFIFO;
        BWEB::Path airClusterPath;
        constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::hunt, Command::escort, Command::retreat, Command::move };

        function flyerWalkable = [](const TilePosition &t) {
            return (((Broodwar->getFrameCount() - Grids::lastVisibleFrame(t)) > 1000 || Grids::getEAirThreat(Position(t)) <= Grids::getAAirCluster(Position(t))) && (Broodwar->getFrameCount() - testFIFO[t] > 240 || testFIFO[t] == 0));
        };

        Position findConcavePosition(UnitInfo& unit, BWEM::Area const * area, BWEM::ChokePoint const * choke)
        {
            // Force ranged concaves if enemy has ranged units (defending only)
            const auto enemyRangeExists = Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Dragoon) > 0
                || Players::getTotalCount(PlayerState::Enemy, UnitTypes::Zerg_Hydralisk) > 0
                || Players::vT();

            // Don't try concaves without chokepoints for now (can use lines in future)
            if (!choke)
                return unit.getPosition();

            auto chokeCount = area->ChokePoints().size();
            auto chokeCenter = Position(choke->Center());
            auto isMelee = unit.getGroundDamage() > 0 && unit.getGroundRange() <= 32.0;
            auto meleeCount = com(Protoss_Zealot) + com(Zerg_Zergling) + com(Terran_Firebat);
            auto rangedCount = com(Protoss_Dragoon) + com(Protoss_Reaver) + com(Protoss_High_Templar) + com(Zerg_Hydralisk) + com(Terran_Marine) + com(Terran_Medic) + com(Terran_Siege_Tank_Siege_Mode) + com(Terran_Siege_Tank_Tank_Mode) + com(Terran_Vulture);
            auto base = area->Bases().empty() ? nullptr : &area->Bases().front();
            auto scoreBest = 0.0;
            auto posBest = unit.getPosition();
            auto line = BWEB::Map::lineOfBestFit(choke);

            auto useMeleeRadius = isMelee && !enemyRangeExists && Players::getSupply(PlayerState::Self) < 80 && !Players::ZvT();
            auto radius = useMeleeRadius && !Terrain::isDefendNatural() ? 64.0 : (choke->Width() / 2.0);
            auto alreadyValid = false;

            //// Check if a wall exists at this choke
            //auto wall = BWEB::Walls::getWall(choke);
            //if (wall) {
            //    auto minDistance = DBL_MAX;
            //    for (auto &piece : wall->getLargeTiles()) {
            //        auto center = Position(piece) + Position(64, 48);
            //        auto closestGeo = BWEB::Map::getClosestChokeTile(choke, center);
            //        if (center.getDistance(closestGeo) < minDistance)
            //            minDistance = center.getDistance(closestGeo);
            //    }

            //    for (auto &piece : wall->getMediumTiles()) {
            //        auto center = Position(piece) + Position(48, 32);
            //        auto closestGeo = BWEB::Map::getClosestChokeTile(choke, center);
            //        if (center.getDistance(closestGeo) < minDistance)
            //            minDistance = center.getDistance(closestGeo);
            //    }

            //    for (auto &piece : wall->getSmallTiles()) {
            //        auto center = Position(piece) + Position(32, 32);
            //        auto closestGeo = BWEB::Map::getClosestChokeTile(choke, center);
            //        if (center.getDistance(closestGeo) < minDistance)
            //            minDistance = center.getDistance(closestGeo);
            //    }
            //    radius = wall->getOpening().isValid() ? minDistance + 32.0 : minDistance;
            //}

            const auto isValid = [&](WalkPosition w, Position projection) {
                const auto t = TilePosition(w);
                const auto p = Position(w);

                // Find a vector projection of this point
                auto projDist = projection.getDistance(chokeCenter);

                // Choke end nodes and distance to choke center
                auto pt1 = Position(choke->Pos(choke->end1));
                auto pt2 = Position(choke->Pos(choke->end2));
                auto pt1Dist = pt1.getDistance(chokeCenter);
                auto pt2Dist = pt2.getDistance(chokeCenter);

                // Determine if we should lineup at projection or wrap around choke end nodes
                if (chokeCount < 3 && (pt1Dist < projDist || pt2Dist < projDist))
                    projection = pt1.getDistance(projection) < pt2.getDistance(projection) ? pt1 : pt2;

                if ((alreadyValid && p.getDistance(unit.getPosition()) > 160.0)
                    || p.getDistance(projection) < radius
                    || p.getDistance(projection) >= 640.0
                    || Buildings::overlapsQueue(unit, p)
                    || !Broodwar->isWalkable(w)
                    || !Util::isTightWalkable(unit, p))
                    return false;
                return true;
            };

            const auto scorePosition = [&](WalkPosition w, Position projection) {
                const auto p = Position(w);

                const auto distProj = exp(p.getDistance(projection));
                const auto distCenter = p.getDistance(chokeCenter);
                const auto distUnit = p.getDistance(unit.getPosition());
                const auto distAreaBase = base ? base->Center().getDistance(p) : 1.0;
                return 1.0 / (distCenter * distAreaBase * distUnit * distProj);
            };

            // Testing something
            //alreadyValid = scorePosition(unit.getWalkPosition()) > 0.0 && find(concaveCache[choke].begin(), concaveCache[choke].end(), unit.getWalkPosition()) != concaveCache[choke].end();

            // Find a position around the center that is suitable        
            auto &tiles = concaveCache[choke];
            for (auto &w : tiles) {
                const auto projection = Util::vectorProjection(line, Position(w));
                const auto score = scorePosition(w, projection);

                if (score > scoreBest && isValid(w, projection)) {
                    posBest = Position(w);
                    scoreBest = score;
                }
            }
            return posBest;
        }

        void updateRole(UnitInfo& unit)
        {
            // Can't change role to combat if not a worker or we did one this frame
            if (!unit.getType().isWorker()
                || lastRoleChange == Broodwar->getFrameCount())
                return;

            // Only proactively pull the closest worker to our defend position
            auto closestWorker = Util::getClosestUnit(Terrain::getDefendPosition(), PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Worker && (!unit.hasResource() || !unit.getResource().getType().isRefinery()) && !unit.getBuildPosition().isValid();
            });

            const auto combatCount = Units::getMyRoleCount(Role::Combat) - (unit.getRole() == Role::Combat ? 1 : 0);
            const auto combatWorkersCount =  Units::getMyRoleCount(Role::Combat) - com(Protoss_Zealot) - com(Protoss_Dragoon);
            const auto myGroundStrength = Players::getStrength(PlayerState::Self).groundToGround - (unit.getRole() == Role::Combat ? unit.getVisibleGroundStrength() : 0.0);
            const auto myDefenseStrength = Players::getStrength(PlayerState::Self).groundDefense;
            const auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
            auto arriveAtDefense = unit.timeArrivesWhen();

            const auto proactivePullWorker = [&](UnitInfo& unit) {

                // If this isn't the closest mineral worker to the defend position, don't pull it
                if (unit.getRole() == Role::Worker && unit.shared_from_this() != closestWorker)
                    return false;

                // Don't pull workers too early
                if (arriveAtDefense < Strategy::enemyArrivalTime() - Time(0, 15))
                    return false;

                if (Broodwar->self()->getRace() == Races::Protoss) {
                    int completedDefenders = com(Protoss_Photon_Cannon) + com(Protoss_Zealot);
                    int visibleDefenders = vis(Protoss_Photon_Cannon) + vis(Protoss_Zealot);

                    // Don't pull low shield probes
                    if (unit.getType() == Protoss_Probe && unit.getShields() < 16)
                        return false;

                    // If trying to hide tech, pull 1 probe with a Zealot
                    if (!BuildOrder::isRush() && BuildOrder::isHideTech() && combatCount < 2 && completedDefenders > 0)
                        return true;

                    // If trying to FFE, pull based on Cannon/Zealot numbers, or lack of scouting information
                    if (BuildOrder::getCurrentBuild() == "FFE") {
                        if (Strategy::enemyRush() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 1)
                            return true;
                        if (Strategy::enemyPressure() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 2)
                            return true;
                        if (!Terrain::getEnemyStartingPosition().isValid() && Strategy::getEnemyBuild() == "Unknown" && myGroundStrength < 2.00 && completedDefenders < 1 && visibleDefenders > 0)
                            return true;
                    }

                    // If trying to 2Gate at our natural, pull based on Zealot numbers
                    else if (BuildOrder::getCurrentBuild() == "2Gate" && BuildOrder::getCurrentOpener() == "Natural") {
                        if (Strategy::enemyRush() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 1)
                            return true;
                        if (Strategy::enemyPressure() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 2)
                            return true;
                    }

                    // If trying to 1GateCore and scouted 2Gate late, pull workers to block choke when we are ready
                    else if (BuildOrder::getCurrentBuild() == "1GateCore" && Strategy::getEnemyBuild() == "2Gate" && BuildOrder::getCurrentTransition() != "Defensive" && Strategy::defendChoke()) {
                        if (Util::getTime() < Time(3, 30) && combatWorkersCount < 2)
                            return true;
                    }
                }
                return false;
            };

            const auto reactivePullWorker = [&](UnitInfo& unit) {

                // HACK: Don't pull workers reactively versus vultures
                if (Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) > 0)
                    return false;

                // If this unit has a target that is threatening mining
                if (unit.hasTarget() && Util::getTime() < Time(10, 0) && closestStation) {
                    if (unit.isThreatening() && unit.getTarget().getPosition().getDistance(unit.getPosition()) < unit.getTarget().getGroundReach())
                        return true;
                }

                // If we have immediate threats
                if ((!Strategy::enemyRush() || Strategy::getEnemyTransition() == "WorkerRush") && Units::getImmThreat() > myGroundStrength + myDefenseStrength && Util::getTime() < Time(10, 0))
                    return true;
                return false;
            };

            // Check if workers should fight or work
            if (unit.getType().isWorker()) {
                if (unit.getRole() == Role::Worker && !unit.unit()->isCarryingMinerals() && !unit.unit()->isCarryingGas() && (reactivePullWorker(unit) || proactivePullWorker(unit))) {
                    unit.setRole(Role::Combat);
                    unit.setBuildingType(None);
                    unit.setBuildPosition(TilePositions::Invalid);
                    lastRoleChange = Broodwar->getFrameCount();
                }
                else if (unit.getRole() == Role::Combat && !reactivePullWorker(unit) && !proactivePullWorker(unit)) {
                    unit.setRole(Role::Worker);
                    lastRoleChange = Broodwar->getFrameCount();
                }
            }
        }

        void updateClusters(UnitInfo& unit)
        {
            // Don't update clusters for fragile combat units
            if (unit.getType() == Protoss_High_Templar
                || unit.getType() == Protoss_Dark_Archon
                || unit.getType() == Protoss_Reaver
                || unit.getType() == Protoss_Interceptor
                || unit.getType() == Zerg_Defiler)
                return;

            // Figure out what type to make the center of our cluster around
            auto clusterAround = UnitTypes::None;
            if (Broodwar->self()->getRace() == Races::Protoss)
                clusterAround = vis(Protoss_Carrier) > 0 ? Protoss_Carrier : Protoss_Corsair;
            else if (Broodwar->self()->getRace() == Races::Zerg)
                clusterAround = vis(Zerg_Guardian) > 0 ? Zerg_Guardian : Zerg_Mutalisk;
            else if (Broodwar->self()->getRace() == Races::Terran)
                clusterAround = vis(Terran_Battlecruiser) > 0 ? Terran_Battlecruiser : Terran_Wraith;

            if (unit.isFlying() && unit.getType() == clusterAround) {
                if (Grids::getAAirCluster(unit.getWalkPosition()) > airCluster.first)
                    airCluster = make_pair(Grids::getAAirCluster(unit.getWalkPosition()), unit.getPosition());
            }
            else if (!unit.isFlying()) {
                const auto strength = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
                combatClusters.emplace(strength, unit.getPosition());
            }
        }

        void updateLocalState(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.setLocalState(LocalState::None);
                return;
            }

            const auto closeToSim = unit.getPosition().getDistance(unit.getSimPosition()) <= unit.getSimRadius() || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition());

            // Regardless of any decision, determine if Unit is in danger and needs to retreat
            if (Actions::isInDanger(unit, unit.getPosition())
                || (Actions::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < unit.getSimRadius()))
                unit.setLocalState(LocalState::Retreat);

            // Regardless of local decision, determine if Unit needs to attack or retreat
            else if (unit.globalEngage())
                unit.setLocalState(LocalState::Attack);
            else if (unit.globalRetreat() && unit.getSimState() == SimState::Loss)
                unit.setLocalState(LocalState::Retreat);

            // If within local decision range, determine if Unit needs to attack or retreat
            else if (closeToSim) {
                if (unit.localRetreat() || unit.getSimState() == SimState::Loss)
                    unit.setLocalState(LocalState::Retreat);
                else if (unit.localEngage() || unit.getSimState() == SimState::Win)
                    unit.setLocalState(LocalState::Attack);
            }

            // Default state
            else
                unit.setLocalState(LocalState::None);
        }

        void updateGlobalState(UnitInfo& unit)
        {
            bool testingDefense = false;
            if (testingDefense) {
                unit.setGlobalState(GlobalState::Retreat);
                return;
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if ((!BuildOrder::isFastExpand() && Strategy::enemyFastExpand())
                    || (Strategy::enemyProxy() && !Strategy::enemyRush())
                    || BuildOrder::isRush()
                    || unit.getType() == Protoss_Dark_Templar
                    || (Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 && com(Protoss_Observer) == 0 && Broodwar->getFrameCount() < 15000))
                    unit.setGlobalState(GlobalState::Attack);

                else if (unit.getType().isWorker()
                    || (Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (unit.getType() == Protoss_Corsair && !BuildOrder::firstReady() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0)
                    || (unit.getType() == Protoss_Carrier && com(Protoss_Interceptor) < 16 && !Strategy::enemyPressure()))
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }

            // Zerg
            else if (Broodwar->self()->getRace() == Races::Zerg) {

                if (BuildOrder::isRush())
                    unit.setGlobalState(GlobalState::Attack);

                else if ((Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (Players::ZvT() && unit.getType() == Zerg_Zergling && Strategy::enemyPressure() && Util::getTime() < Time(8, 0)))
                    unit.setGlobalState(GlobalState::Retreat);

                else
                    unit.setGlobalState(GlobalState::Attack);
            }

            // Terran
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (BuildOrder::isPlayPassive() || !BuildOrder::firstReady())
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }
        }

        void updateDestination(UnitInfo& unit)
        {
            // If attacking and target is close, set as destination
            if (unit.getLocalState() == LocalState::Attack) {
                const auto &intercept = Util::getInterceptPosition(unit);
                if (intercept.getDistance(unit.getTarget().getPosition()) < intercept.getDistance(unit.getPosition()) && (Grids::getMobility(intercept) > 0 || unit.getType().isFlyer()))
                    unit.setDestination(intercept);
                else
                    unit.setDestination(unit.getEngagePosition());
            }

            // If we're globally retreating, set defend position as destination
            else if (unit.getGlobalState() == GlobalState::Retreat && Strategy::defendChoke() && (!unit.hasTarget() || (unit.hasTarget() && (!unit.getTarget().isThreatening() || unit.getGroundRange() > 32.0 || unit.getSpeed() > unit.getTarget().getSpeed())))) {
                const BWEM::Area * defendArea = nullptr;
                const BWEM::ChokePoint * defendChoke = Terrain::isDefendNatural() && !Players::ZvT() ? BWEB::Map::getNaturalChoke() : BWEB::Map::getMainChoke();

                // Decide which area is within my territory, useful for maps with small adjoining areas like Andromeda
                if (!defendArea) {
                    auto &[a1, a2] = defendChoke->GetAreas();
                    if (a1 && Terrain::isInAllyTerritory(a1))
                        defendArea = a1;
                    if (a2 && Terrain::isInAllyTerritory(a2))
                        defendArea = a2;
                }

                if (defendArea)
                    unit.setDestination(findConcavePosition(unit, defendArea, defendChoke));
            }

            // If retreating, find closest retreat position
            else if (unit.getLocalState() == LocalState::Retreat || unit.getGlobalState() == GlobalState::Retreat) {
                const auto &retreat = getClosestRetreatPosition(unit.getPosition());
                if (retreat.isValid() && (!unit.isLightAir() || Players::getStrength(PlayerState::Enemy).airToAir > 0.0))
                    unit.setDestination(retreat);
                else
                    unit.setDestination(BWEB::Map::getMainPosition());
            }

            // If unit has a goal
            else if (unit.getGoal().isValid())
                unit.setDestination(unit.getGoal());

            // If this is a light air unit, go to the air cluster first if far away
            else if (unit.isFlying() && airCluster.second.isValid() && (unit.getPosition().getDistance(airCluster.second) > 160.0 || unit.isSuicidal()))
                unit.setDestination(airCluster.second);

            // If this is a light air unit and we can harass
            else if (unit.isLightAir() && Stations::getEnemyStations().size() > 0 && unit.getPosition().getDistance(unit.getSimPosition()) > unit.getSimRadius() + 160.0 && (!unit.hasTarget() || unit.getTarget().timeArrivesWhen() - Time(0, 10) > unit.timeArrivesWhen())) {
                unit.setDestination(Terrain::getHarassPosition());
                unit.setDestinationPath(airClusterPath);
            }

            // Resort to going to our target if we have one
            else if (unit.hasTarget() && unit.getEngagePosition().isValid())
                unit.setDestination(unit.getEngagePosition());

            // If attack position is valid
            else if (Terrain::getAttackPosition().isValid())
                unit.setDestination(Terrain::getAttackPosition());

            // If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
            else if (!unit.hasTarget() || !unit.getTarget().getPosition().isValid() || unit.unit()->isIdle()) {
                if (Terrain::getEnemyStartingPosition().isValid()) {
                    auto best = DBL_MAX;

                    for (auto &area : mapBWEM.Areas()) {
                        for (auto &base : area.Bases()) {
                            if (area.AccessibleNeighbours().size() == 0
                                || Terrain::isInAllyTerritory(base.Location()))
                                continue;

                            int time = Grids::lastVisibleFrame(base.Location());
                            if (time < best) {
                                best = time;
                                unit.setDestination(Position(base.Location()));
                            }
                        }
                    }
                }
                else {

                    // Sort unexplored starts by distance
                    multimap<double, Position> startsByDist;
                    for (auto &start : mapBWEM.StartingLocations()) {
                        const auto center = Position(start) + Position(64, 48);
                        const auto dist = BWEB::Map::getGroundDistance(center, BWEB::Map::getMainPosition());

                        if (!Terrain::isExplored(center))
                            startsByDist.emplace(dist, center);
                    }

                    // Assign closest that isn't assigned
                    int test = INT_MAX;
                    for (auto &[_, position] : startsByDist) {
                        if (!Actions::overlapsActions(unit.unit(), position, Broodwar->self()->getRace().getWorker(), PlayerState::Self) && !Actions::overlapsActions(unit.unit(), position, Zerg_Overlord, PlayerState::Self)) {
                            unit.setDestination(position);
                            break;
                        }
                    }

                    // Assigned furthest
                    if (!unit.getDestination().isValid() && !startsByDist.empty())
                        unit.setDestination(startsByDist.rbegin()->second);

                }
            }

            // Add Action so other Units dont move to same location
            if (unit.getDestination().isValid())
                Actions::addAction(unit.unit(), unit.getDestination(), None, PlayerState::Self);
        }

        void updatePath(UnitInfo& unit)
        {
            testFIFO = unit.getLastTilesVisited();

            // Generate a new path that obeys collision of terrain and buildings
            if (unit.getDestination().isValid() && !unit.isFlying()) {
                BWEB::Path newPath;
                newPath.jpsPath(unit.getPosition(), unit.getDestination(), BWEB::Pathfinding::unitWalkable);
                unit.setDestinationPath(newPath);
            }

            // If path is reachable, find a point n pixels away to set as new destination
            if (unit.getDestinationPath().isReachable()) {
                auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 160.0;
                });

                if (newDestination.isValid())
                    unit.setDestination(newDestination);
            }
            else if (!unit.getQuickPath().empty()) {
                unit.setDestination(Position(unit.getQuickPath().front()->Center()));
            }
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()                                                                                            // Prevent crashes            
                || unit.unit()->isLoaded()
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())    // If the unit is locked down, maelstrommed, stassised, or not completed
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Special"),
                make_pair(2, "Attack"),
                make_pair(3, "Approach"),
                make_pair(4, "Kite"),
                make_pair(5, "Defend"),
                make_pair(6, "Hunt"),
                make_pair(7, "Escort"),
                make_pair(8, "Retreat"),
                make_pair(9, "Move")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateUnits() {
            //BWEB::Pathfinding::clearCache(flyerWalkable);
            combatClusters.clear();
            combatUnitsByDistance.clear();
            airCluster.first = 0.0;
            airCluster.second = Positions::Invalid;

            // Sort units by distance to destination
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                // Don't update if
                if (!unit.unit()->isCompleted()
                    || unit.getType() == Terran_Vulture_Spider_Mine
                    || unit.getType() == Protoss_Scarab
                    || unit.getType() == Protoss_Interceptor
                    || unit.getType().isSpell())
                    continue;

                // Check if we need to pull/push workers to/from combat role
                if (unit.getType().isWorker())
                    updateRole(unit);

                // Update combat role units states and sort by distance to destination
                if (unit.getRole() == Role::Combat) {
                    updateClusters(unit);
                    updateGlobalState(unit);
                    updateLocalState(unit);
                    auto dist = unit.hasTarget() ? 1.0 / unit.getPosition().getDistance(unit.getTarget().getPosition()) : unit.getPosition().getDistance(BWEB::Map::getMainPosition());
                    combatUnitsByDistance.emplace(dist, unit);
                }
            }

            // Setup air cluster harassing path
            if (TilePosition(airCluster.second) != airClusterPath.getSource()) {
                BWEB::Path newPath;

                if (airCluster.second.getDistance(Terrain::getHarassPosition()) > 640.0)
                    newPath.jpsPath(airCluster.second, Terrain::getHarassPosition(), flyerWalkable);

                airClusterPath = newPath;
            }

            // Execute commands ordered by ascending distance
            for (auto &u : combatUnitsByDistance) {
                auto &unit = u.second;

                if (unit.getRole() == Role::Combat) {
                     
                    if (unit.isNearSplash())
                        unit.circleBlue();

                    Horizon::simulate(unit);
                    updateDestination(unit);
                    updatePath(unit);
                    updateDecision(unit);
                }
            }
        }

        void updateRetreatPositions()
        {
            retreatPositions.clear();
            for (auto &[unit, station] : Stations::getMyStations()) {
                auto posBest = Positions::Invalid;
                auto distBest = DBL_MAX;
                auto tile = station->getBWEMBase()->Location();

                if (!unit->isCompleted() && !unit->isMorphing())
                    continue;

                // Find a TilePosition around it that is suitable to path to
                for (int x = tile.x - 6; x < tile.x + 10; x++) {
                    for (int y = tile.y - 6; y < tile.y + 10; y++) {
                        TilePosition t(x, y);
                        Position center = Position(t) + Position(16, 16);
                        auto dist = center.getDistance(station->getResourceCentroid());
                        if (t.isValid() && dist < distBest && BWEB::Map::isUsed(t) == None) {
                            posBest = center;
                            distBest = dist;
                        }
                    }
                }

                // If valid, add to set of retreat positions
                if (posBest.isValid())
                    retreatPositions.insert(posBest);
            }
        }
    }

    void onStart()
    {
        const auto createCache = [&](const BWEM::ChokePoint * chokePoint, const BWEM::Area * area) {
            auto center = chokePoint->Center();
            for (int x = center.x - 60; x <= center.x + 60; x++) {
                for (int y = center.y - 60; y <= center.y + 60; y++) {
                    WalkPosition w(x, y);
                    const auto p = Position(w) + Position(4, 4);

                    if (!p.isValid()
                        || (area && mapBWEM.GetArea(w) != area)
                        || Grids::getMobility(w) < 6)
                        continue;

                    auto closest = Util::getClosestChokepoint(p);
                    if (closest != chokePoint && p.getDistance(Position(closest->Center())) < 160.0 && (closest == BWEB::Map::getMainChoke() || closest == BWEB::Map::getNaturalChoke()))
                        continue;

                    concaveCache[chokePoint].push_back(w);
                }
            }
        };

        // Main area for defending sometimes is wrong like Andromeda
        const BWEM::Area * defendArea = nullptr;
        auto &[a1, a2] = BWEB::Map::getMainChoke()->GetAreas();
        if (a1 && Terrain::isInAllyTerritory(a1))
            defendArea = a1;
        if (a2 && Terrain::isInAllyTerritory(a2))
            defendArea = a2;

        createCache(BWEB::Map::getMainChoke(), defendArea);
        createCache(BWEB::Map::getMainChoke(), BWEB::Map::getMainArea());

        // Natural area should always be correct
        createCache(BWEB::Map::getNaturalChoke(), BWEB::Map::getNaturalArea());
    }

    void onFrame() {
        Visuals::startPerfTest();
        updateUnits();
        updateRetreatPositions();
        Visuals::endPerfTest("Combat");
    }

    Position getClosestRetreatPosition(Position here)
    {
        auto distBest = DBL_MAX;
        auto posBest = Positions::Invalid;
        for (auto &position : retreatPositions) {
            auto dist = position.getDistance(here);
            if (dist < distBest) {
                posBest = position;
                distBest = dist;
            }
        }
        return posBest;
    }

    multimap<double, Position>& getCombatClusters() { return combatClusters; }
}