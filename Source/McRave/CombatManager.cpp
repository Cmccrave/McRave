#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {

        set<Position> retreatPositions;
        multimap<double, Position> combatClusters;
        multimap<double, UnitInfo&> combatUnitsByDistance;
        constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::hunt, Command::escort, Command::retreat, Command::move };

        void updateRole(UnitInfo& unit)
        {
            if (!unit.getType().isWorker())
                return;

            auto closestWorker = Util::getClosestUnit(Terrain::getDefendPosition(), PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Worker && (!unit.hasResource() || !unit.getResource().getType().isRefinery());
            });

            auto combatCount = Units::getMyRoleCount(Role::Combat) - (unit.getRole() == Role::Combat ? 1 : 0);
            auto myGroundStrength = Players::getStrength(PlayerState::Self).groundToGround - (unit.getRole() == Role::Combat ? unit.getVisibleGroundStrength() : 0.0);
            auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
            auto arriveAtDefense = Broodwar->getFrameCount() + (unit.getPosition().getDistance(Terrain::getDefendPosition()) / unit.getSpeed());

            const auto proactivePullWorker = [&](UnitInfo& unit) {

                // If this isn't the closest mineral worker to the defend position, don't pull it
                if (unit.getRole() == Role::Worker && unit.shared_from_this() != closestWorker)
                    return false;

                // Don't pull workers too early
                if (arriveAtDefense < Strategy::enemyArrivalFrame() - 150)
                    return false;

                if (Broodwar->self()->getRace() == Races::Protoss) {
                    int completedDefenders = Broodwar->self()->completedUnitCount(Protoss_Photon_Cannon) + Broodwar->self()->completedUnitCount(Protoss_Zealot);
                    int visibleDefenders = Broodwar->self()->visibleUnitCount(Protoss_Photon_Cannon) + Broodwar->self()->visibleUnitCount(Protoss_Zealot);

                    // Don't pull low shield probes
                    if (unit.getType() == Protoss_Probe && unit.getShields() < 8)
                        return false;

                    // If trying to hide tech, pull 1 probe with a Zealot
                    if (BuildOrder::isHideTech() && combatCount < 2 && completedDefenders > 0)
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
                        if (combatCount < 4)
                            return true;
                    }
                }
                return false;
            };

            const auto reactivePullWorker = [&](UnitInfo& unit) {
                if (Units::getEnemyCount(Terran_Vulture) > 2)
                    return false;

                if (unit.getType() == Protoss_Probe) {
                    if (unit.getShields() < 8)
                        return false;
                }
                if (unit.getType() == Terran_SCV) {
                    if (unit.getHealth() < 20)
                        return false;
                }

                if (unit.hasTarget()) {
                    if (unit.getTarget().hasAttackedRecently() && unit.getTarget().getPosition().getDistance(closestStation) < unit.getTarget().getGroundReach() && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0 && Broodwar->getFrameCount() < 10000)
                        return true;
                }

                // If we have immediate threats
                if (Units::getImmThreat() > myGroundStrength && Broodwar->getFrameCount() < 10000)
                    return true;
                return false;
            };

            // Check if workers should fight or work
            if (unit.getType().isWorker()) {
                if (unit.getRole() == Role::Worker && !unit.unit()->isCarryingMinerals() && !unit.unit()->isCarryingGas() && (reactivePullWorker(unit) || proactivePullWorker(unit))) {
                    unit.setRole(Role::Combat);
                    unit.setBuildingType(None);
                    unit.setBuildPosition(TilePositions::Invalid);

                    // Adjust counters
                    Players::adjustStrength(unit, false);
                    Units::adjustRoleCount(unit.getRole(), 1);
                }
                else if (unit.getRole() == Role::Combat && !reactivePullWorker(unit) && !proactivePullWorker(unit)) {
                    unit.setRole(Role::Worker);

                    // Adjust counters
                    Players::adjustStrength(unit, true);
                    Units::adjustRoleCount(unit.getRole(), -1);
                }

                if (reactivePullWorker(unit))
                    unit.circleBlack();
                if (proactivePullWorker(unit))
                    unit.circleBlue();
            }
        }

        void updateClusters(UnitInfo& unit)
        {
            if (unit.getType() == Protoss_High_Templar
                || unit.getType() == Zerg_Defiler
                || unit.getType() == Protoss_Dark_Archon
                || unit.getType() == Protoss_Reaver)
                return;

            double strength = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
            combatClusters.emplace(strength, unit.getPosition());
        }

        void updateLocalState(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                unit.getGlobalState() == GlobalState::Attack ? unit.setLocalState(LocalState::Attack) : unit.setLocalState(LocalState::Retreat);
                return;
            }

            const auto fightingAtHome = ((Terrain::isInAllyTerritory(unit.getTilePosition()) && unit.isWithinRange(unit.getTarget())) || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition()));
            const auto enemyReach = unit.getType().isFlyer() ? unit.getTarget().getAirReach() : unit.getTarget().getGroundReach();
            const auto enemyThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());
            const auto destinationThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getDestination()) : Grids::getEGroundThreat(unit.getDestination());
            const auto distToSim = unit.getPosition().getDistance(unit.getSimPosition());

            const auto inDanger = [&]() {
                if (Command::isInDanger(unit, unit.getPosition()) || (Command::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < SIM_RADIUS))
                    return true;
                return false;
            };

            const auto globalRetreat = [&]() {
                if ((unit.getType().getRace() == Races::Zerg && unit.getPercentTotal() < LOW_BIO_PERCENT_LIMIT)                             // ...unit is Zerg and low HP percent
                    || Grids::getESplash(unit.getWalkPosition()) > 0                                                                        // ...unit is within splash of a Spider Mine or Scarab
                    || (unit.getTarget().isHidden() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= enemyReach)        // ...target is hidden and unit is within reach
                    || unit.getGlobalState() == GlobalState::Retreat)                                                                       // ...global state is retreating
                    return true;
                return false;
            };

            const auto globalEngage = [&]() {
                if (unit.getTarget().isHidden())                                                                                            // ...target is hidden
                    return false;
                if (unit.getTarget().isThreatening() && !unit.getType().isFlyer())                                                          // ...target needs to be killed
                    return true;
                if (fightingAtHome) {                                                                                                       // ...unit is fighting target in our territory
                    if ((!unit.getType().isFlyer() || !unit.getTarget().getType().isFlyer())                                                // ...unit and target are not flying units
                        && (Strategy::defendChoke() || unit.getGroundRange() > 64.0))                                                       // ...we're defending our choke or unit is ranged
                        return true;
                }
                return false;
            };

            const auto localRetreat = [&]() {
                if ((unit.getType() == Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && !BuildOrder::isProxy() && unit.getTarget().getType() == Terran_Vulture && Grids::getMobility(unit.getTarget().getWalkPosition()) > 6 && Grids::getCollision(unit.getTarget().getWalkPosition()) < 4)     // ...unit is a slow Zealot attacking a Vulture
                    || (unit.isLightAir() && unit.getType().maxShields() > 0 && unit.getTarget().getType() == Zerg_Overlord && Grids::getEAirThreat(unit.getEngagePosition()) * 5.0 > (double)unit.getShields())                                                                                                                            // ...unit is a low shield light air attacking a Overlord under threat greater than our shields
                    || (unit.getType() == Protoss_Corsair && unit.getTarget().getType() == Zerg_Scourge && com(Protoss_Corsair) < 6)                                                                                                                                                                                                        // ...unit is a Corsair attacking Scourge with less than 6 completed Corsairs
                    || (unit.getType() == Terran_Medic && unit.unit()->getEnergy() <= TechTypes::Healing.energyCost())                                                                                                                                                                                                                      // ...unit is a Medic with no energy
                    || (unit.getType() == Zerg_Mutalisk && Grids::getEAirThreat(unit.getEngagePosition()) > 0.0 && unit.getHealth() <= 30)                                                                                                                                                                                                  // ...unit is a low HP Mutalisk attacking a target under air threat
                    //|| (unit.getType().maxShields() > 0 && unit.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && Broodwar->getFrameCount() < 8000)                                                                                                                                                                                        // ...unit is a low shield unit in the early stages of the game - DISABLED
                    || (unit.getType() == Terran_SCV && Broodwar->getFrameCount() > 12000)                                                                                                                                                                                                                                                  // ...unit is an SCV outside of early game
                    || (unit.hasTransport() && !unit.unit()->isLoaded() && unit.getType() == Protoss_High_Templar && unit.canStartCast(TechTypes::Psionic_Storm))
                    || (unit.hasTransport() && !unit.unit()->isLoaded() && unit.getType() == Protoss_Reaver && unit.canStartAttack()))
                    return true;
                return false;
            };

            const auto localEngage = [&]() {
                if ((!unit.getType().isFlyer() && unit.getTarget().isSiegeTank() && unit.getTarget().getTargetedBy().size() >= 4 && ((unit.isWithinRange(unit.getTarget()) && unit.getGroundRange() > 32.0) || (unit.isWithinReach(unit.getTarget()) && unit.getGroundRange() <= 32.0)))
                    || (unit.isHidden() && !Command::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
                    || (unit.getType() == Protoss_Reaver && !unit.unit()->isLoaded() && unit.isWithinRange(unit.getTarget()))
                    || (unit.getSimState() == SimState::Win && unit.getGlobalState() == GlobalState::Attack)
                    || (unit.getTarget().getType() == Terran_Vulture_Spider_Mine && !unit.getTarget().isBurrowed()))
                    return true;
                return false;
            };

            if (unit.hasTarget()) {

                if (inDanger())
                    unit.setLocalState(LocalState::Retreat);
                else if (globalEngage())
                    unit.setLocalState(LocalState::Attack);
                else if (globalRetreat())
                    unit.setLocalState(LocalState::Retreat);

                else if (distToSim <= SIM_RADIUS) {
                    if (localRetreat())
                        unit.setLocalState(LocalState::Retreat);
                    else if (localEngage())
                        unit.setLocalState(LocalState::Attack);
                    else
                        unit.setLocalState(LocalState::Retreat);
                }

                else if (unit.getGlobalState() == GlobalState::Attack)
                    unit.setLocalState(LocalState::Attack);
                else
                    unit.setLocalState(LocalState::Retreat);
            }
        }

        void updateGlobalState(UnitInfo& unit)
        {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if ((!BuildOrder::isFastExpand() && Strategy::enemyFastExpand())
                    || (Strategy::enemyProxy() && !Strategy::enemyRush())
                    || BuildOrder::isRush())
                    unit.setGlobalState(GlobalState::Attack);

                else if ((Strategy::enemyRush() && !Players::vT())
                    || (!Strategy::enemyRush() && BuildOrder::isHideTech() && BuildOrder::isOpener())
                    || unit.getType().isWorker()
                    || (Broodwar->getFrameCount() < 13000 && BuildOrder::isPlayPassive())
                    || (unit.getType() == Protoss_Corsair && !BuildOrder::firstReady() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0)
                    || unit.getType() == Protoss_Carrier && com(Protoss_Interceptor) < 16)
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }
            else if (Broodwar->self()->getRace() == Races::Zerg)
                unit.setGlobalState(GlobalState::Attack);
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (BuildOrder::isPlayPassive() || !BuildOrder::firstReady())
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }
        }

        void updateDestination(UnitInfo& unit)
        {
            auto moveToTarget = unit.hasTarget() && (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= SIM_RADIUS || unit.getType().isFlyer() || Broodwar->getFrameCount() < 15000);

            // If we're globally retreating, set defend position as destination
            if (unit.getGlobalState() == GlobalState::Retreat && (!unit.hasTarget() || (unit.hasTarget() && (!unit.getTarget().isThreatening() || unit.getGroundRange() > 32.0 || unit.getSpeed() > unit.getTarget().getSpeed()))))
                unit.setDestination(Terrain::getDefendPosition());

            // If target is close, set as destination
            if (unit.getLocalState() == LocalState::Attack && unit.getEngagePosition().isValid() && moveToTarget && unit.getTarget().getPosition().isValid() && Grids::getMobility(unit.getEngagePosition()) > 0) {
                auto intercept = Util::getInterceptPosition(unit);
                if (intercept.getDistance(unit.getTarget().getPosition()) < intercept.getDistance(unit.getPosition()))
                    unit.setDestination(Util::getInterceptPosition(unit));
                else
                    unit.setDestination(unit.getEngagePosition());
            }

            else if (unit.getLocalState() == LocalState::Retreat) {
                auto retreat = getClosestRetreatPosition(unit.getPosition());
                if (retreat.isValid())
                    unit.setDestination(retreat);
                unit.circleRed();
                Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Yellow);
            }

            // If unit has a goal
            else if (unit.getGoal().isValid()) {

                // Find a concave if not in enemy territory
                if (!Terrain::isInEnemyTerritory((TilePosition)unit.getGoal())) {
                    Position bestPosition = Util::getConcavePosition(unit, unit.getGroundRange(), mapBWEM.GetArea(TilePosition(unit.getGoal())));
                    if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None))
                        unit.setDestination(bestPosition);
                }

                // Set as destination if it is
                else if (unit.unit()->getLastCommand().getTargetPosition() != unit.getGoal() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
                    unit.setDestination(unit.getGoal());
            }

            // If this is a light air unit
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

            // TODO: Check if a scout is moving here too
            // If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
            else if (unit.unit()->isIdle()) {
                if (Terrain::getEnemyStartingPosition().isValid())
                    unit.setDestination(Terrain::randomBasePosition());
                else {
                    for (auto &start : Broodwar->getStartLocations()) {
                        if (start.isValid() && !Broodwar->isExplored(start) && !Command::overlapsActions(unit.unit(), Position(start), unit.getType(), PlayerState::Self, 32))
                            unit.setDestination(Position(start));
                    }
                }
            }

            if (unit.canCreatePath(unit.getDestination())) {
                BWEB::Path newPath;
                newPath.createUnitPath(unit.getPosition(), unit.getDestination());
                unit.setPath(newPath);
            }

            auto newDestination = Util::findPointOnPath(unit.getPath(), [&](Position p) {
                return p.getDistance(unit.getPosition()) >= 64.0;
            });

            if (newDestination.isValid())
                unit.setDestination(newDestination);

            //Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Yellow);
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes			
                || unit.unit()->isLoaded()
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())	// If the unit is locked down, maelstrommed, stassised, or not completed
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

                if (unit.getType() == UnitTypes::Protoss_Interceptor)
                    continue;

                updateClusters(unit);
                updateGlobalState(unit);
                updateLocalState(unit);
                updateRole(unit);

                if (unit.getRole() == Role::Combat) {
                    auto dist = unit.getPosition().getDistance(unit.getDestination());
                    combatUnitsByDistance.emplace(dist, unit);
                }
            }

            // Execute commands ordered by ascending distance
            for (auto &u : combatUnitsByDistance) {
                auto &unit = u.second;

                if (unit.getRole() == Role::Combat) {
                    Horizon::simulate(unit);
                    updateDestination(unit);
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
                if (posBest.isValid()) {
                    retreatPositions.insert(posBest);
                    Broodwar->drawCircleMap(posBest, 6, Colors::Purple, true);
                }
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