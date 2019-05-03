#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Combat {

    namespace {

        multimap<double, Position> combatClusters;
        multimap<double, UnitInfo&> combatUnitsByDistance;
        constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::hunt, Command::escort, Command::retreat, Command::move };

        void updateClusters(UnitInfo& unit)
        {
            if (unit.getType() == UnitTypes::Protoss_High_Templar
                || unit.getType() == UnitTypes::Zerg_Defiler
                || unit.getType() == UnitTypes::Protoss_Dark_Archon)
                return;

            double strength = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
            combatClusters.emplace(strength, unit.getPosition());
        }

        void updateLocalState(UnitInfo& unit)
        {
            if (!unit.hasTarget()) {
                if (unit.getGlobalState() == GlobalState::Attack)
                    unit.setLocalState(LocalState::Attack);
                else
                    unit.setLocalState(LocalState::Retreat);
                return;
            }

            auto fightingAtHome = ((Terrain::isInAllyTerritory(unit.getTilePosition()) && Util::unitInRange(unit)) || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition()));
            auto invisTarget = unit.getTarget().isHidden();
            auto enemyReach = unit.getType().isFlyer() ? unit.getTarget().getAirReach() : unit.getTarget().getGroundReach();
            auto enemyThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());
            auto destinationThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getDestination()) : Grids::getEGroundThreat(unit.getDestination());

            const auto inDanger = [&]() {
                if (Command::isInDanger(unit, unit.getPosition()) || (Command::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < SIM_RADIUS))
                    return true;
                return false;
            };

            if (unit.getTarget().isHidden())
                unit.getTarget().circleBlue();

            const auto forceRetreat = [&]() {
                if (/*(unit.getType().isMechanical() && unit.getPercentTotal() < LOW_MECH_PERCENT_LIMIT)
                    || */(unit.getType().getRace() == Races::Zerg && unit.getPercentTotal() < LOW_BIO_PERCENT_LIMIT)
                    || (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75)
                    || Grids::getESplash(unit.getWalkPosition()) > 0
                    || (invisTarget && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= enemyReach)
                    || unit.getGlobalState() == GlobalState::Retreat)
                    return true;
                return false;
            };

            const auto forceEngage = [&]() {
                if (invisTarget)
                    return false;

                // If enemy needs to be killed
                if (unit.getTarget().isThreatening())
                    return true;

                // If fighting in our territory
                if (fightingAtHome) {
                    if ((!unit.getType().isFlyer() || !unit.getTarget().getType().isFlyer()) && (Strategy::defendChoke() || unit.getGroundRange() > 64.0))
                        return true;
                }
                return false;
            };

            const auto localRetreat = [&]() {
                if ((unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && !BuildOrder::isProxy() && unit.getTarget().getType() == UnitTypes::Terran_Vulture && Grids::getMobility(unit.getTarget().getWalkPosition()) > 6 && Grids::getCollision(unit.getTarget().getWalkPosition()) < 4)
                    || ((unit.getType() == UnitTypes::Protoss_Scout || unit.getType() == UnitTypes::Protoss_Corsair) && unit.getTarget().getType() == UnitTypes::Zerg_Overlord && Grids::getEAirThreat((WalkPosition)unit.getEngagePosition()) * 5.0 > (double)unit.getShields())
                    || (unit.getType() == UnitTypes::Protoss_Corsair && unit.getTarget().getType() == UnitTypes::Zerg_Scourge && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Corsair) < 6)
                    || (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= TechTypes::Healing.energyCost())
                    || (unit.getType() == UnitTypes::Zerg_Mutalisk && Grids::getEAirThreat((WalkPosition)unit.getEngagePosition()) > 0.0 && unit.getHealth() <= 30)
                    || (unit.getType().maxShields() > 0 && unit.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && Broodwar->getFrameCount() < 8000 && !Terrain::isInAllyTerritory(unit.getTilePosition()))
                    || (unit.getType() == UnitTypes::Terran_SCV && Broodwar->getFrameCount() > 12000)
                    || (unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && !unit.getTarget().isBurrowed()))
                    return true;
                return false;
            };

            const auto localEngage = [&]() {
                if ((Broodwar->getFrameCount() > 10000 && !unit.getType().isFlyer() && (unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 96.0 || Util::unitInRange(unit)))
                    || ((unit.unit()->isCloaked() || unit.isBurrowed()) && !Command::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
                    || (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util::unitInRange(unit))
                    || (unit.getSimState() == SimState::Win && unit.getGlobalState() == GlobalState::Attack))
                    return true;
                return false;
            };

            if (unit.hasTarget()) {

                if (inDanger())
                    unit.setLocalState(LocalState::Retreat);
                else if (forceEngage())
                    unit.setLocalState(LocalState::Attack);
                else if (forceRetreat())
                    unit.setLocalState(LocalState::Retreat);

                else if (unit.getPosition().getDistance(unit.getSimPosition()) <= SIM_RADIUS) {
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
                    || (unit.getType() == UnitTypes::Protoss_Corsair && !BuildOrder::firstReady() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0))
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

            // If target is close, set as destination
            if (unit.getEngagePosition().isValid() && moveToTarget && unit.getTarget().getPosition().isValid() && Grids::getMobility(unit.getEngagePosition()) > 0) {
                if (unit.getTarget().unit()->exists())
                    unit.setDestination(Util::getInterceptPosition(unit));
                else
                    unit.setDestination(unit.getTarget().getPosition());
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
                if (unit.getRole() == Role::Combat) {
                    updateDestination(unit);
                    auto dist = unit.getPosition().getDistance(unit.getDestination());
                    combatUnitsByDistance.emplace(dist, unit);
                }
            }

            // Execute commands ordered by ascending distance
            for (auto &u : combatUnitsByDistance) {
                auto &unit = u.second;
                if (unit.getRole() == Role::Combat) {
                    Horizon::simulate(unit);

                    updateClusters(unit);
                    updateGlobalState(unit);
                    updateLocalState(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        updateUnits();
        Visuals::endPerfTest("Combat");
    }

    multimap<double, Position>& getCombatClusters() { return combatClusters; }
}