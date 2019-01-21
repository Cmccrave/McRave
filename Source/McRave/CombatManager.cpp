#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Combat {

    namespace {
        multimap<double, Position> combatClusters;
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
            if (unit.hasTarget()) {

                auto fightingAtHome = ((Terrain::isInAllyTerritory(unit.getTilePosition()) && Util::unitInRange(unit)) || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition()));
                auto invisTarget = unit.getTarget().unit()->exists() && (unit.getTarget().unit()->isCloaked() || unit.getTarget().isBurrowed()) && !unit.getTarget().unit()->isDetected();
                auto enemyReach = unit.getType().isFlyer() ? unit.getTarget().getAirReach() : unit.getTarget().getGroundReach();
                auto enemyThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());

                // Testing
                if (Command::isInDanger(unit, unit.getPosition()) || (Command::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < SIM_RADIUS))
                    unit.setLocalState(LocalState::Retreat);

                // Force engaging
                else if (!invisTarget && (unit.getTarget().isThreatening()
                    || (fightingAtHome && (!unit.getType().isFlyer() || !unit.getTarget().getType().isFlyer()) && (Strategy::defendChoke() || unit.getGroundRange() > 64.0))))
                    unit.setLocalState(LocalState::Attack);

                // Force retreating
                else if ((unit.getType().isMechanical() && unit.getPercentTotal() < LOW_MECH_PERCENT_LIMIT)
                    || (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75)
                    || Grids::getESplash(unit.getWalkPosition()) > 0
                    || (invisTarget && (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= enemyReach || enemyThreat))
                    || unit.getGlobalState() == GlobalState::Retreat)
                    unit.setLocalState(LocalState::Retreat);

                // Close enough to make a decision
                else if (unit.getPosition().getDistance(unit.getSimPosition()) <= SIM_RADIUS) {

                    // Retreat
                    if ((unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && !BuildOrder::isProxy() && unit.getTarget().getType() == UnitTypes::Terran_Vulture && Grids::getMobility(unit.getTarget().getWalkPosition()) > 6 && Grids::getCollision(unit.getTarget().getWalkPosition()) < 4)
                        || ((unit.getType() == UnitTypes::Protoss_Scout || unit.getType() == UnitTypes::Protoss_Corsair) && unit.getTarget().getType() == UnitTypes::Zerg_Overlord && Grids::getEAirThreat((WalkPosition)unit.getEngagePosition()) * 5.0 > (double)unit.getShields())
                        || (unit.getType() == UnitTypes::Protoss_Corsair && unit.getTarget().getType() == UnitTypes::Zerg_Scourge && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Corsair) < 6)
                        || (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= TechTypes::Healing.energyCost())
                        || (unit.getType() == UnitTypes::Zerg_Mutalisk && Grids::getEAirThreat((WalkPosition)unit.getEngagePosition()) > 0.0 && unit.getHealth() <= 30)
                        || (unit.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && Broodwar->getFrameCount() < 8000)
                        || (unit.getType() == UnitTypes::Terran_SCV && Broodwar->getFrameCount() > 12000)
                        || (invisTarget && !unit.getTarget().isThreatening() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) == 0))
                        unit.setLocalState(LocalState::Retreat);

                    // Engage
                    else if ((Broodwar->getFrameCount() > 10000 && !unit.getType().isFlyer() && (unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 96.0 || Util::unitInRange(unit)))
                        || ((unit.unit()->isCloaked() || unit.isBurrowed()) && !Command::overlapsEnemyDetection(unit.getEngagePosition()))
                        || (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util::unitInRange(unit))
                        || (unit.getSimState() == SimState::Win && unit.getGlobalState() == GlobalState::Attack))
                        unit.setLocalState(LocalState::Attack);
                    else
                        unit.setLocalState(LocalState::Retreat);
                }
                else if (unit.getGlobalState() == GlobalState::Attack) {
                    unit.setLocalState(LocalState::Attack);
                }
                else {
                    unit.setLocalState(LocalState::Retreat);
                }
            }
            else if (unit.getGlobalState() == GlobalState::Attack) {
                unit.setLocalState(LocalState::Attack);
            }
            else {
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
                    || (Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (unit.getType() == UnitTypes::Protoss_Corsair && !BuildOrder::firstReady() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0))
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }
            else
                unit.setGlobalState(GlobalState::Attack);
        }

        void updateDestination(UnitInfo& unit)
        {
            auto moveToTarget = unit.hasTarget() && (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= SIM_RADIUS || unit.getType().isFlyer());

            // If unit has a goal
            if (unit.getGoal().isValid()) {

                // Find a concave if not in enemy territory
                if (!Terrain::isInEnemyTerritory((TilePosition)unit.getDestination())) {
                    Position bestPosition = Util::getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));
                    if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None))
                        unit.setDestination(bestPosition);
                }

                // Set as destination if it is
                else if (unit.unit()->getLastCommand().getTargetPosition() != unit.getDestination() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
                    unit.setDestination(unit.getGoal());
            }

            // If target is close, set as destination
            else if (moveToTarget && unit.getTarget().getPosition().isValid() && Grids::getMobility(unit.getEngagePosition()) > 0)
                unit.setDestination(unit.getTarget().getPosition());

            else if (Terrain::getAttackPosition().isValid())
                unit.setDestination(Terrain::getAttackPosition());

            // TODO: Check if a scout is moving here too
            // If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
            else if (unit.unit()->isIdle()) {
                if (Terrain::getEnemyStartingPosition().isValid()) {
                    unit.setDestination(Terrain::randomBasePosition());
                }
                else {
                    for (auto &start : Broodwar->getStartLocations()) {
                        if (start.isValid() && !Broodwar->isExplored(start) && !Command::overlapsActions(unit.unit(), unit.getType(), Position(start), 32)) {
                            unit.setDestination(Position(start));
                        }
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
    }

    void onFrame() {

        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;

            if (unit.getRole() == Role::Combat) {
                Horizon::simulate(unit);

                updateClusters(unit);
                updateGlobalState(unit);
                updateLocalState(unit);
                updateDestination(unit);
                updateDecision(unit);
            }
        }
    }

    multimap<double, Position>& getCombatClusters() { return combatClusters; }
}