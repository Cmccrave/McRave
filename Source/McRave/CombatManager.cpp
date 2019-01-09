#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Combat {

    namespace {      

        void updateLocalState(UnitInfo& unit)
        {
            if (unit.hasTarget()) {

                auto fightingAtHome = ((Terrain::isInAllyTerritory(unit.getTilePosition()) && Util::unitInRange(unit)) || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition()));
                auto invisTarget = unit.getTarget().unit()->exists() && (unit.getTarget().unit()->isCloaked() || unit.getTarget().isBurrowed()) && !unit.getTarget().unit()->isDetected();
                auto enemyReach = unit.getType().isFlyer() ? unit.getTarget().getAirReach() : unit.getTarget().getGroundReach();
                auto enemyThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());

                // Testing
                if (Command::isInDanger(unit, unit.getPosition()) || (Command::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < SIM_RADIUS))
                    unit.setLocalState(LocalState::Retreating);

                // Force engaging
                else if (!invisTarget && (unit.getTarget().isThreatening()
                    || (fightingAtHome && (!unit.getType().isFlyer() || !unit.getTarget().getType().isFlyer()) && (Strategy::defendChoke() || unit.getGroundRange() > 64.0))))
                    unit.setLocalState(LocalState::Engaging);

                // Force retreating
                else if ((unit.getType().isMechanical() && unit.getPercentTotal() < LOW_MECH_PERCENT_LIMIT)
                    || (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75)
                    || Grids::getESplash(unit.getWalkPosition()) > 0
                    || (invisTarget && (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= enemyReach || enemyThreat))
                    || unit.getGlobalState() == GlobalState::Retreating)
                    unit.setLocalState(LocalState::Retreating);

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
                        unit.setLocalState(LocalState::Retreating);

                    // Engage
                    else if ((Broodwar->getFrameCount() > 10000 && (unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 96.0 || Util::unitInRange(unit)))
                        || ((unit.unit()->isCloaked() || unit.isBurrowed()) && !Command::overlapsEnemyDetection(unit.getEngagePosition()))
                        || (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util::unitInRange(unit))
                        || (unit.getSimState() == SimState::Win && unit.getGlobalState() == GlobalState::Engaging))
                        unit.setLocalState(LocalState::Engaging);
                    else
                        unit.setLocalState(LocalState::Retreating);
                }
                else if (unit.getGlobalState() == GlobalState::Retreating) {
                    unit.setLocalState(LocalState::Retreating);
                }
                else {
                    unit.setLocalState(LocalState::None);
                }
            }
            else if (unit.getGlobalState() == GlobalState::Retreating) {
                unit.setLocalState(LocalState::Retreating);
            }
            else {
                unit.setLocalState(LocalState::None);
            }
        }

        void updateGlobalState(UnitInfo& unit)
        {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if ((!BuildOrder::isFastExpand() && Strategy::enemyFastExpand())
                    || (Strategy::enemyProxy() && !Strategy::enemyRush())
                    || BuildOrder::isRush())
                    unit.setGlobalState(GlobalState::Engaging);

                else if ((Strategy::enemyRush() && !Players::vT())
                    || (!Strategy::enemyRush() && BuildOrder::isHideTech() && BuildOrder::isOpener())
                    || unit.getType().isWorker()
                    || (Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (unit.getType() == UnitTypes::Protoss_Corsair && !BuildOrder::firstReady() && Units::getGlobalEnemyAirStrength() > 0.0))
                    unit.setGlobalState(GlobalState::Retreating);
                else
                    unit.setGlobalState(GlobalState::Engaging);
            }
            else
                unit.setGlobalState(GlobalState::Engaging);
        }
    }

    void onFrame() {
        for (auto &u : Units::getMyUnits()) {
            auto &unit = u.second;

            if (unit.getRole() == Role::Combat) {
                Horizon::simulate(unit);
                updateGlobalState(unit);
                updateLocalState(unit);                
            }
        }
    }
}