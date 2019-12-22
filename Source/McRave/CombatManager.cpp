#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {

        int lastRoleChange = 0;
        set<Position> retreatPositions;
        multimap<double, Position> combatClusters;
        multimap<double, UnitInfo&> combatUnitsByDistance;
        constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::hunt, Command::escort, Command::retreat, Command::move };

        void updateRole(UnitInfo& unit)
        {
            // Can't change role to combat if not a worker or we did one this frame
            if (!unit.getType().isWorker()
                || lastRoleChange == Broodwar->getFrameCount())
                return;

            // Only proactively pull the closest worker to our defend position
            auto closestWorker = Util::getClosestUnit(Terrain::getDefendPosition(), PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Worker && (!unit.hasResource() || !unit.getResource().getType().isRefinery());
            });

            const auto combatCount = Units::getMyRoleCount(Role::Combat) - (unit.getRole() == Role::Combat ? 1 : 0);
            const auto combatWorkersCount =  Units::getMyRoleCount(Role::Combat) - com(Protoss_Zealot) - com(Protoss_Dragoon);
            const auto myGroundStrength = Players::getStrength(PlayerState::Self).groundToGround - (unit.getRole() == Role::Combat ? unit.getVisibleGroundStrength() : 0.0);
            const auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
            const auto arriveAtDefense = Broodwar->getFrameCount() + (unit.getPosition().getDistance(Terrain::getDefendPosition()) / unit.getSpeed());

            const auto proactivePullWorker = [&](UnitInfo& unit) {

                // If this isn't the closest mineral worker to the defend position, don't pull it
                if (unit.getRole() == Role::Worker && unit.shared_from_this() != closestWorker)
                    return false;

                // Don't pull workers too early
                if (arriveAtDefense < Strategy::enemyArrivalFrame() - 300)
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
                if (Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) > 2)
                    return false;

                // If this unit has a target that is threatening mining
                if (unit.hasTarget() && Util::getTime() < Time(5, 0)) {
                    if (unit.getTarget().hasAttackedRecently() && unit.getTarget().getPosition().getDistance(closestStation) < unit.getTarget().getGroundReach() && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0)
                        return true;
                }

                // If we have immediate threats
                if (Units::getImmThreat() > myGroundStrength && Util::getTime() < Time(5, 0))
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

            const auto strength = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
            combatClusters.emplace(strength, unit.getPosition());
        }

        void updateLocalState(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.getGlobalState() == GlobalState::Attack ? unit.setLocalState(LocalState::Attack) : unit.setLocalState(LocalState::Retreat);
                return;
            }

            const auto closeToSim = unit.getEngDist() <= unit.getSimRadius();

            // Regardless of any decision, determine if Unit is in danger and needs to retreat
            if (Actions::isInDanger(unit, unit.getPosition())
                || (Actions::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < unit.getSimRadius()))
                unit.setLocalState(LocalState::Retreat);

            // Regardless of local decision, determine if Unit needs to attack or retreat
            else if (unit.globalEngage())
                unit.setLocalState(LocalState::Attack);
            else if (unit.globalRetreat())
                unit.setLocalState(LocalState::Retreat);

            // If within local decision range, determine if Unit needs to attack or retreat
            else if (closeToSim) {
                unit.circlePurple();
                if (unit.localRetreat())
                    unit.setLocalState(LocalState::Retreat);
                else if (unit.localEngage() || (unit.getSimState() == SimState::Win && unit.getGlobalState() == GlobalState::Attack))
                    unit.setLocalState(LocalState::Attack);
                else
                    unit.setLocalState(LocalState::Retreat);
            }
            else if (unit.getGlobalState() == GlobalState::Attack)
                unit.setLocalState(LocalState::Attack);
            else
                unit.setLocalState(LocalState::Retreat);
        }

        void updateGlobalState(UnitInfo& unit)
        {
            bool testingDefense = true;
            if (testingDefense) {
                unit.setGlobalState(GlobalState::Retreat);
                return;
            }

            if (Broodwar->getGameType() == GameTypes::Use_Map_Settings) {
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

                else if (Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
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
            auto moveToTarget = unit.hasTarget() &&
                (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= unit.getSimRadius()
                    || unit.getType().isFlyer()
                    || Broodwar->getFrameCount() < 10000
                    || int(Stations::getEnemyStations().size()) <= 1
                    || Broodwar->mapFileName().find("BlueStorm") != string::npos
                    || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition()));

            // If we're globally retreating, set defend position as destination
            if (unit.getGlobalState() == GlobalState::Retreat && unit.getLocalState() == LocalState::Retreat && (!unit.hasTarget() || (unit.hasTarget() && (!unit.getTarget().isThreatening() || unit.getGroundRange() > 32.0 || unit.getSpeed() > unit.getTarget().getSpeed()))))
                unit.setDestination(Terrain::getDefendPosition());

            // If attacking and target is close, set as destination
            else if (unit.getLocalState() == LocalState::Attack && unit.getEngagePosition().isValid() && moveToTarget && unit.getTarget().getPosition().isValid() && (Grids::getMobility(unit.getEngagePosition()) > 0 || unit.getType().isFlyer())) {
                auto intercept = Util::getInterceptPosition(unit);
                if (unit.getPosition().getDistance(intercept) > unit.getTarget().getPosition().getDistance(intercept))
                    unit.setDestination(Util::getInterceptPosition(unit));
                else
                    unit.setDestination(unit.getEngagePosition());
            }

            // If retreating, find closest retreat position
            else if (unit.getLocalState() == LocalState::Retreat) {
                auto retreat = getClosestRetreatPosition(unit.getPosition());
                if (retreat.isValid())
                    unit.setDestination(retreat);
            }

            // If unit has a goal
            else if (unit.getGoal().isValid()) {

                // Find a concave if not in enemy territory
                if (!Terrain::isInEnemyTerritory((TilePosition)unit.getGoal()) && unit.getPosition().getDistance(unit.getGoal()) < 256.0) {
                    Position bestPosition = Util::getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getGoal())));
                    if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None))
                        unit.setDestination(bestPosition);
                }

                // Set as destination if it is
                else
                unit.setDestination(unit.getGoal());
            }

            // If this is a light air unit, find least recently visited Station to attack
            else if (unit.isLightAir() && Stations::getEnemyStations().size() > 0) {
                auto best = INT_MAX;
                for (auto &station : Stations::getEnemyStations()) {
                    auto tile = station.second->getBWEMBase()->Location();
                    auto current = Grids::lastVisibleFrame(tile);
                    if (current < best) {
                        best = current;
                        unit.setDestination(Position(tile));
                    }
                }
            }

            // If attack position is valid
            else if (Terrain::getAttackPosition().isValid())
                unit.setDestination(Terrain::getAttackPosition());

            // Resort to going to our target if we have one
            else if (unit.hasTarget() && unit.getTarget().getPosition().isValid())
                unit.setDestination(unit.getTarget().getPosition());

            // If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
            else if (!unit.hasTarget() || unit.unit()->isIdle()) {
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
                    multimap<double, Position> startsByDist;

                    // Sort unexplored starts by distance
                    for (auto &start : mapBWEM.StartingLocations()) {
                        const auto center = Position(start) + Position(64, 48);
                        const auto dist = BWEB::Map::getGroundDistance(center, unit.getPosition());

                        if (!Terrain::isExplored(center))
                            startsByDist.emplace(dist, center);
                    }

                    // Assign closest that isn't assigned
                    for (auto &[_, position] : startsByDist) {
                        if (!Actions::overlapsActions(unit.unit(), position, None, PlayerState::Self, 32) || startsByDist.size() == 1) {
                            unit.setDestination(position);
                            break;
                        }
                    }

                    // Assign closest if no option
                    if (!unit.getDestination().isValid()) {
                        for (auto &[_, position] : startsByDist) {
                            unit.setDestination(position);
                            return;
                        }
                    }
                }
            }

            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);

            // Add Action so other Units dont move to same location
            if (unit.getDestination().isValid())
                Actions::addAction(unit.unit(), unit.getDestination(), None, PlayerState::Self);
        }

        void updatePath(UnitInfo& unit)
        {
            if (unit.canCreatePath(unit.getDestination())) {
                BWEB::Path newPath;
                newPath.jpsPath(unit.getPosition(), unit.getDestination(), BWEB::Pathfinding::terrainWalkable);
                unit.setPath(newPath);
            }

            if (unit.getPath().getTarget() == TilePosition(unit.getDestination())) {
                auto newDestination = Util::findPointOnPath(unit.getPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 160.0;
                });

                if (newDestination.isValid())
                    unit.setDestination(newDestination);
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
            combatClusters.clear();
            combatUnitsByDistance.clear();

            // Sort units by distance to destination
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getType() == Protoss_Interceptor
                    || !unit.unit()->isCompleted())
                    continue;

                if (unit.getRole() == Role::Combat)
                    updateClusters(unit);

                updateGlobalState(unit);
                updateLocalState(unit);
                updateRole(unit);

                if (unit.getRole() == Role::Combat) {
                    auto dist = 1.0 / unit.getPosition().getDistance(unit.getDestination());
                    combatUnitsByDistance.emplace(dist, unit);
                }
            }

            // Execute commands ordered by ascending distance
            for (auto &u : combatUnitsByDistance) {
                auto &unit = u.second;

                if (unit.getRole() == Role::Combat) {
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
            for (auto &[_, station] : Stations::getMyStations()) {
                auto posBest = Positions::Invalid;
                auto distBest = DBL_MAX;
                auto tile = station->getBWEMBase()->Location();

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