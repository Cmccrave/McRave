#include "Builds/All/BuildOrder.h"
#include "Commands.h"
#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Macro/Planning/Planning.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Micro/Combat/Combat.h"
#include "Strategy/Actions/Actions.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace UnitCommandTypes;
using namespace TechTypes;

namespace McRave::Command {
    namespace {
        set<Position> testPositions;
        int lastMorphFrame = -999;
    } // namespace

    bool lift(UnitInfo &unit)
    {
        // If we need to lift
        if (!unit.isFlying()) {
            if (unit.getType() == Terran_Engineering_Bay || unit.getType() == Terran_Science_Facility)
                unit.unit()->lift();
        }

        // If we don't need barracks, lift it, TODO: Check ramp type?
        if (unit.getType() == Terran_Barracks) {

            auto wallPiece = false;
            for (auto &[choke, wall] : BWEB::Walls::getWalls()) {
                for (auto &tile : wall.getLargeTiles()) {
                    if (unit.getTilePosition() == tile)
                        wallPiece = true;
                }
            }

            // Wall off if we need to
            if (unit.getGoal().isValid() && unit.getPosition().getDistance(unit.getGoal()) < 96.0) {
                if (unit.isFlying() && unit.unit()->getLastCommand().getType() != UnitCommandTypes::Land)
                    unit.unit()->land(TilePosition(unit.getGoal()));
                return true;
            }

            // Lift if we need to
            else if (!unit.isFlying())
                unit.unit()->lift();
        }
        return false;
    }

    bool click(UnitInfo &unit)
    {
        // Shield Battery - Repair Shields
        if (com(Protoss_Shield_Battery) > 0 && (unit.unit()->getGroundWeaponCooldown() > Broodwar->getLatencyFrames() || unit.unit()->getAirWeaponCooldown() > Broodwar->getLatencyFrames()) &&
            unit.getType().maxShields() > 0 &&
            (unit.unit()->getShields() <= 10 || (unit.unit()->getShields() < unit.getType().maxShields() && unit.unit()->getOrderTarget() && unit.unit()->getOrderTarget()->exists() &&
                                                 unit.unit()->getOrderTarget()->getType() == Protoss_Shield_Battery && unit.unit()->getOrderTarget()->getEnergy() >= 10))) {
            auto battery = Util::getClosestUnit(unit.getPosition(), PlayerState::Self,
                                                [&](auto &u) { return u->getType() == Protoss_Shield_Battery && u->unit()->isCompleted() && u->getEnergy() > 10; });

            if (battery && ((unit.getType().isFlyer() && (!unit.hasTarget() || (unit.getTarget().lock()->getPosition().getDistance(unit.getPosition()) >= 320))) ||
                            unit.unit()->getDistance(battery->getPosition()) < 320)) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit || unit.unit()->getLastCommand().getTarget() != battery->unit()) {
                    unit.setCommand(Right_Click_Unit, *battery);
                    unit.commandText = "Regen";
                }
                return true;
            }
        }

        // Bunker - Loading / Unloading
        else if (unit.getType() == Terran_Marine && vis(Terran_Bunker) > 0) {

            auto bunker = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return (u->getType() == Terran_Bunker && u->unit()->getSpaceRemaining() > 0); });

            auto loadBunker   = false;
            auto unloadBunker = false;
            auto alwaysLoad   = unit.getGlobalState() == GlobalState::Retreat;

            if (bunker) {
                if (unit.hasTarget()) {
                    auto &target = *unit.getTarget().lock();
                    auto close   = target.isWithinReach(unit) && target.isWithinReach(*bunker);
                    loadBunker   = alwaysLoad || close;
                    unloadBunker = !alwaysLoad && !close;
                }
                else {
                    loadBunker   = alwaysLoad;
                    unloadBunker = !alwaysLoad;
                }
                if (!unit.unit()->isLoaded() && loadBunker) {
                    unit.setCommand(Right_Click_Unit, *bunker);
                    unit.commandText = "LoadBunker";
                    return true;
                }
                if (unit.unit()->isLoaded() && unloadBunker)
                    bunker->unit()->unloadAll();
            }
        }

        // Drone - Nydus travelling
        else if (unit.getType() == Zerg_Drone && com(Zerg_Nydus_Canal) > 0 && unit.hasResource() && !unit.isWithinGatherRange()) {
            auto destinationNydus = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                if (u->getType() == Zerg_Nydus_Canal) {
                    auto stationD = unit.getResource().lock()->getStation()->getDefenses();
                    return stationD.find(u->getTilePosition()) != stationD.end();
                }
                return false;
            });

            if (destinationNydus && destinationNydus->unit()->getNydusExit()) {
                if (auto info = Units::getUnitInfo(destinationNydus->unit()->getNydusExit())) {
                    if (unit.getPosition().getDistance(info->getPosition()) < 160.0) {
                        unit.setCommand(UnitCommandTypes::Right_Click_Unit, *info);
                        unit.commandText = "NydusTravel";
                        return true;
                    }
                }
            }
        }

        if (!unit.getType().isWorker())
            return false;

        // If unit has a resource and wants to gather but a unit is close by, mineral walk past it
        if (unit.hasResource()) {
            auto resource = unit.getResource().lock();
            auto boxDist  = Util::boxDistance(unit.getType(), unit.getPosition(), resource->getType(), resource->getPosition());

            if (unit.getDestination() == resource->getPosition() && boxDist >= 128.0) {
                auto closestUnit = Util::getClosestUnit(unit.getPosition(), PlayerState::None, [&](auto &u) {
                    return u->isCompleted() && *u != unit && !u->getType().isBuilding() && !u->isFlying() && u->getPosition().getDistance(unit.getPosition()) < 32.0;
                });

                if (closestUnit) {
                    unit.unit()->gather(resource->unit());
                    unit.commandText = "MineralWalk";
                    return true;
                }
            }
        }

        return false;
    }

    bool siege(UnitInfo &unit)
    {
        auto targetDist = unit.hasTarget() ? unit.getPosition().getDistance(unit.getTarget().lock()->getPosition()) : 0.0;

        // Don't siege next to a tank that is already sieged
        if (!unit.unit()->isSieged()) {
            auto nearestSiegedFriend = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return unit != *u && u->getType() == Terran_Siege_Tank_Siege_Mode; });

            if (nearestSiegedFriend && nearestSiegedFriend->getPosition().getDistance(unit.getPosition()) < 96.0)
                return false;
        }

        auto siege = (unit.getGlobalState() == GlobalState::Retreat && unit.getPosition().getDistance(Combat::getDefendPosition()) < 280.0) ||
                     (unit.hasTarget() && targetDist <= 520.0 && unit.getLocalState() != LocalState::Retreat);
        auto unsiege = unit.hasTarget() && targetDist > 320.0;

        // Siege Tanks - Siege
        if (unit.getType() == Terran_Siege_Tank_Tank_Mode || unit.getType() == Terran_Siege_Tank_Siege_Mode) {
            if (siege) {
                unit.unit()->siege();
                unit.commandText = "Siege";
            }
            else if (unsiege) {
                unit.unit()->unsiege();
                unit.commandText = "Unsiege";
            }
        }
        return false;
    }

    bool repair(UnitInfo &unit)
    {
        // SCV
        if (unit.getType() == Terran_SCV) {

            // Repair closest injured mech
            auto mech = Util::getClosestUnit(unit.getPosition(), PlayerState::Self,
                                             [&](auto &u) { return *u != unit && u->isCompleted() && (u->getType().isMechanical() && u->getPercentTotal() < 1.0); });

            if (mech && mech->unit()) {
                if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != mech->unit())
                    unit.unit()->repair(mech->unit());
                unit.commandText = "Repair";
                return true;
            }

            // Repair closest injured building
            auto building = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return *u != unit && u->isCompleted() &&
                       (u->getPercentTotal() < 0.35 || (u->getType() == Terran_Bunker && u->getPercentTotal() < 1.0) || (u->getType() == Terran_Missile_Turret && u->getPercentTotal() < 1.0));
            });

            if (building && building->unit()) {
                if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != building->unit())
                    unit.unit()->repair(building->unit());
                unit.commandText = "Repair";
                return true;
            }
        }
        return false;
    }

    bool burrow(UnitInfo &unit)
    {
        // Vulture spider mine burrowing
        if (unit.getType() == Terran_Vulture) {
            if (Broodwar->self()->hasResearched(Spider_Mines) && unit.unit()->getSpiderMineCount() > 0 && unit.hasSimTarget() &&
                unit.getPosition().getDistance(unit.getSimTarget().lock()->getPosition()) <= 400 &&
                Broodwar->getUnitsInRadius(unit.getPosition(), 128, Filter::GetType == Terran_Vulture_Spider_Mine).size() <= 3) {
                if (unit.unit()->getLastCommand().getTechType() != Spider_Mines || unit.unit()->getLastCommand().getTargetPosition().getDistance(unit.getPosition()) > 8) {
                    unit.setCommand(Spider_Mines, unit.getPosition());
                    unit.commandText = "Planting";
                }
                return true;
            }
        }

        // Lurker burrowing
        else if (unit.getType() == Zerg_Lurker) {
            auto targetDist = unit.hasTarget() ? unit.getPosition().getDistance(unit.getTarget().lock()->getPosition()) : 0.0;
            auto burrow     = (unit.getGlobalState() == GlobalState::Retreat && unit.getPosition().getDistance(Combat::getDefendPosition()) < 280.0) ||
                          (unit.hasTarget() && unit.isWithinRange(*unit.getTarget().lock()) && unit.getLocalState() == LocalState::Attack) ||
                          (unit.hasTarget() && targetDist <= 275.0 && unit.getLocalState() == LocalState::Hold);
            auto unburrow = (unit.hasTarget() && targetDist > 320.0);

            if (!unit.isBurrowed() && burrow) {
                unit.setCommand(Burrow, unit.getPosition());
                unit.commandText = "Burrowing";
                return true;
            }
            else if (unit.isBurrowed() && unburrow) {
                unit.setCommand(Unburrow, unit.getPosition());
                unit.commandText = "Unburrowing";
                return true;
            }
        }

        // Drone
        else if (unit.getType().isWorker() && Broodwar->self()->hasResearched(Burrowing) && unit.getRole() == Role::Worker) {

            // Check if station is protected
            auto grdDef = false;
            auto airDef = false;
            if (unit.hasResource() && unit.isWithinGatherRange()) {
                auto resource = unit.getResource().lock();
                if (Stations::getGroundDefenseCount(resource->getStation()) > 0)
                    grdDef = true;
                if (Stations::getAirDefenseCount(resource->getStation()) > 0)
                    airDef = true;
            }

            // Determine if we need to burrow or not
            auto threatened = false;
            auto &list      = unit.getUnitsInReachOfThis();
            auto damage     = 5;
            for (auto &t : list) {
                if (auto enemy = t.lock()) {
                    if (enemy->getType() == Protoss_Reaver || enemy->isThreatening()) {
                        threatened = enemy->hasTarget() && enemy->getTarget().lock()->getType().isWorker() && enemy->hasAttackedRecently();
                        damage += enemy->getGroundDamage();
                    }
                }
            }

            auto hideFromDamage = unit.isBurrowed() ? damage >= unit.getHealth() / 2 : damage >= unit.getHealth();
            auto burrowUnit     = !unit.getBuildPosition().isValid() && (hideFromDamage || threatened) && !Planning::overlapsPlan(unit, unit.getPosition());

            // Burrow/unburrow as needed
            if (!unit.isBurrowed() && burrowUnit) {
                unit.setCommand(Burrow, unit.getPosition());
                unit.commandText = "Burrowing";
                return true;
            }
            else if (unit.isBurrowed() && !burrowUnit) {
                unit.setCommand(Unburrow, unit.getPosition());
                unit.commandText = "Unburrowing";
                return true;
            }
        }

        // Zergling burrowing
        else if (unit.getType() == Zerg_Zergling && Broodwar->self()->hasResearched(Burrowing)) {
            auto fullHealth = unit.getHealth() == unit.getType().maxHitPoints();

            if (!unit.isBurrowed() && unit.getGoalType() == GoalType::Contain && !Planning::overlapsPlan(unit, unit.getPosition()) && unit.getGoal().getDistance(unit.getPosition()) < 16.0 &&
                !Actions::overlapsDetection(unit.unit(), unit.getPosition(), PlayerState::Enemy)) {
                unit.setCommand(Burrow, unit.getPosition());
                unit.commandText = "Burrowing";
                return true;
            }
            if (unit.isBurrowed() && (unit.getGoalType() != GoalType::Contain || Planning::overlapsPlan(unit, unit.getPosition()) || unit.getGoal().getDistance(unit.getPosition()) > 16.0 ||
                                      Actions::overlapsDetection(unit.unit(), unit.getPosition(), PlayerState::Enemy))) {
                unit.setCommand(Unburrow, unit.getPosition());
                unit.commandText = "Burrowing";
                return true;
            }
        }

        return false;
    }

    bool castSelf(UnitInfo &unit)
    {
        // Ghost - Cloak
        if (unit.getType() == Terran_Ghost) {
            if (!unit.unit()->isCloaked() && unit.getEnergy() >= 50 &&
                (Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()) || unit.getPosition().getDistance(unit.getEngagePosition()) < unit.getEngageRadius())) {
                unit.setCommand(Personnel_Cloaking);
                unit.commandText = "Cloak";
            }
        }

        // Science Vessel - Defensive Matrix
        else if (unit.getType() == Terran_Science_Vessel && unit.getEnergy() >= Defensive_Matrix.energyCost()) {
            auto ally = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return (u->unit()->isUnderAttack()); });
            if (ally && ally->getPosition().getDistance(unit.getPosition()) < 640) {
                unit.setCommand(Defensive_Matrix, *ally);
                unit.commandText = "DefenseMatrix";
                return true;
            }
        }

        // Wraith - Cloak
        else if (unit.getType() == Terran_Wraith) {
            if (unit.getHealth() >= 120 && !unit.unit()->isCloaked() && unit.getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 &&
                !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy)) {
                unit.setCommand(Cloaking_Field);
                unit.commandText = "Cloak";
            }
            else if (unit.getHealth() <= 90 && unit.unit()->isCloaked()) {
                unit.setCommand(Cloaking_Field);
                unit.commandText = "Cloak";
            }
        }

        // Corsair - Disruption Web
        else if (unit.getType() == Protoss_Corsair) {

            // If close to a slow enemy and can cast disruption web
            if (unit.getEnergy() >= Disruption_Web.energyCost() && Broodwar->self()->hasResearched(Disruption_Web)) {
                auto slowEnemy = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u->getPosition().getDistance(unit.getPosition()) < 256.0 && !Actions::overlapsActions(unit.unit(), u->getPosition(), Disruption_Web, PlayerState::Self, 96) &&
                           u->hasAttackedRecently() && u->getSpeed() <= Protoss_Reaver.topSpeed();
                });

                if (slowEnemy) {
                    Actions::addAction(unit.unit(), slowEnemy->getPosition(), Disruption_Web, PlayerState::Self, Util::getCastRadius(TechTypes::Disruption_Web));
                    unit.setCommand(Disruption_Web, slowEnemy->getPosition());
                    unit.commandText = "DWeb";
                    return true;
                }
            }
        }
        return false;
    }

    bool castTarget(UnitInfo &unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        // Battlecruiser - Yamato
        if (unit.getType() == Terran_Battlecruiser && Broodwar->self()->hasResearched(Yamato_Gun)) {
            if ((unit.unit()->getOrder() == Orders::FireYamatoGun || (unit.getEnergy() >= Yamato_Gun.energyCost()) && target.getHealth() >= 80)) {
                if ((unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech || unit.unit()->getLastCommand().getTarget() != target.unit())) {
                    unit.setCommand(Yamato_Gun, target);
                    unit.commandText = "Yamato";
                }
                Actions::addAction(unit.unit(), target.getPosition(), Yamato_Gun, PlayerState::Neutral);
                return true;
            }
        }

        // Ghost - Nuke / Lockdown
        else if (unit.getType() == Terran_Ghost) {

            if (com(Terran_Nuclear_Missile) > 0 && unit.unit()->isCloaked() && unit.getPosition().getDistance(target.getPosition()) > 200) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Position || unit.unit()->getLastCommand().getTargetPosition() != target.getPosition()) {
                    unit.setCommand(Nuclear_Strike, target.getPosition());
                    unit.commandText = "Nuke";
                    Actions::addAction(unit.unit(), target.getPosition(), Nuclear_Strike, PlayerState::Neutral, Util::getCastRadius(Nuclear_Strike));
                    return true;
                }
            }

            // Move to actions
            if (unit.unit()->getOrder() == Orders::NukePaint || unit.unit()->getOrder() == Orders::NukeTrack || unit.unit()->getOrder() == Orders::CastNuclearStrike) {
                unit.commandText = "Nuke";
                Actions::addAction(unit.unit(), unit.unit()->getOrderTargetPosition(), Nuclear_Strike, PlayerState::Neutral, Util::getCastRadius(Nuclear_Strike));
                return true;
            }
        }

        // Marine / Firebat - Stim Packs
        else if ((unit.getType() == Terran_Marine || unit.getType() == Terran_Firebat) && Broodwar->self()->hasResearched(Stim_Packs) && !unit.isStimmed() && unit.isWithinRange(target)) {
            unit.setCommand(Stim_Packs);
            unit.commandText = "Stim";
            return true;
        }

        // Comsat scans
        else if (unit.getType() == Terran_Comsat_Station) {
            if (target.unit()->exists() && !Actions::overlapsDetection(unit.unit(), target.getPosition(), PlayerState::Self)) {
                unit.setCommand(Scanner_Sweep, target.getPosition());
                unit.commandText = "Scan";
                Actions::addAction(unit.unit(), target.getPosition(), Spell_Scanner_Sweep, PlayerState::Self, Util::getCastRadius(Scanner_Sweep));
                return true;
            }
        }

        // Arbiters - Stasis Field
        else if (unit.getType() == Protoss_Arbiter) {

            // If close to target and can cast Stasis Field
            if (unit.canStartCast(Stasis_Field, target.getPosition())) {
                unit.setCommand(Stasis_Field, target);
                unit.commandText = "Stasis";
                Actions::addAction(unit.unit(), target.getPosition(), Stasis_Field, PlayerState::Self, Util::getCastRadius(Stasis_Field));
                return true;
            }
        }

        // High Templar - Psi Storm
        else if (unit.getType() == Protoss_High_Templar) {

            // If close to target and can cast Psi Storm
            if (unit.getPosition().getDistance(target.getPosition()) <= 320 && unit.canStartCast(Psionic_Storm, target.getPosition())) {
                unit.setCommand(Psionic_Storm, target);
                unit.commandText = "Storm";
                Actions::addAction(unit.unit(), target.getPosition(), Psionic_Storm, PlayerState::Neutral, Util::getCastRadius(Psionic_Storm));
                return true;
            }
        }

        // Defiler - Dark Swarm / Plague
        else if (unit.getType() == Zerg_Defiler && !unit.isBurrowed()) {

            auto selfMoreMelee  = vis(Zerg_Zergling) + vis(Zerg_Ultralisk) > vis(Zerg_Hydralisk);
            auto enemyLessMelee = (Players::ZvP() && Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) > Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot)) || (Players::ZvT());

            auto castSwarm = selfMoreMelee && enemyLessMelee && (target.getType() == Terran_Marine || target.getType() == Terran_Medic || target.isSiegeTank());

            // If close to target and can cast Plague
            if (!unit.targetsFriendly() && !castSwarm && unit.getLocalState() != LocalState::None && unit.canStartCast(Plague, target.getPosition())) {
                unit.setCommand(Plague, target.getPosition());
                unit.commandText = "Plague";
                Actions::addAction(unit.unit(), target.getPosition(), Plague, PlayerState::Neutral, Util::getCastRadius(Plague));
                return true;
            }

            // If close to target and can cast Dark Swarm
            if (!unit.targetsFriendly() && castSwarm && unit.getPosition().getDistance(target.getPosition()) <= 400 && unit.canStartCast(Dark_Swarm, target.getPosition())) {
                unit.setCommand(Dark_Swarm, target.getPosition());
                unit.commandText = "DarkSwarm";
                Actions::addAction(unit.unit(), target.getPosition(), Dark_Swarm, PlayerState::Neutral, Util::getCastRadius(Dark_Swarm));
                return true;
            }

            // If within range of an intermediate point within engaging distance of a tank
            if (!unit.targetsFriendly() && vis(Zerg_Zergling) + vis(Zerg_Ultralisk) > vis(Zerg_Hydralisk) && target.isSiegeTank()) {
                for (auto &tile : unit.getMarchPath().getTiles()) {
                    auto center = Position(tile) + Position(16, 16);

                    auto distMin = BuildOrder::getCompositionPercentage(Zerg_Hydralisk) > 0.0 ? 200.0 : 0.0;
                    auto dist    = center.getDistance(target.getPosition());

                    if (dist <= 400 && dist > distMin && unit.canStartCast(Dark_Swarm, center)) {
                        if (unit.unit()->getLastCommand().getTechType() != Dark_Swarm)
                            unit.setCommand(Dark_Swarm, center);
                        Actions::addAction(unit.unit(), center, Dark_Swarm, PlayerState::Neutral, Util::getCastRadius(Dark_Swarm));
                        unit.commandText = "DarkSwarm";
                        return true;
                    }
                }
            }

            // If close to target and need to Consume
            if (unit.targetsFriendly() && unit.getEnergy() < 150 && unit.getPosition().getDistance(target.getPosition()) <= 160.0 && unit.canStartCast(Consume, target.getPosition())) {
                unit.setCommand(Consume, target);
                unit.commandText = "Consume";
                return true;
            }
        }

        // Queen - Broodlings / Ensnare
        else if (unit.getType() == Zerg_Queen) {
            if (unit.canStartCast(Spawn_Broodlings, target)) {
                unit.setCommand(Spawn_Broodlings, target);
                unit.commandText = "Broodlings";
                Actions::addAction(unit.unit(), target.unit(), Spawn_Broodlings, PlayerState::Neutral);
                return true;
            }

            if (unit.canStartCast(Ensnare, target.getPosition())) {
                unit.setCommand(Ensnare, target.getPosition());
                unit.commandText = "Ensnare";
                Actions::addAction(unit.unit(), target.getPosition(), Ensnare, PlayerState::Neutral);
                return true;
            }
        }

        return false;
    }

    bool morph(UnitInfo &unit)
    {
        if (lastMorphFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4)
            return false;

        auto canAffordMorph = [&](UnitType type) {
            if (Broodwar->self()->minerals() >= type.mineralPrice() && Broodwar->self()->gas() >= type.gasPrice() &&
                Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed() > type.supplyRequired())
                return true;
            return false;
        };

        // High Templar - Archon Morph
        if (unit.getType() == Protoss_High_Templar) {
            auto lowEnergyThreat = unit.getEnergy() < Psionic_Storm.energyCost() && Grids::getGroundThreat(unit.getPosition(), PlayerState::Enemy) > 0.0f;
            auto wantArchons     = vis(Protoss_Archon) / BuildOrder::getCompositionPercentage(Protoss_Archon) < vis(Protoss_High_Templar) / BuildOrder::getCompositionPercentage(Protoss_High_Templar);

            if (!Players::vT() && (lowEnergyThreat || wantArchons)) {

                // Try to find a friendly templar who is low energy and is threatened
                auto templar = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return *u != unit && u->getType() == Protoss_High_Templar && u->unit()->isCompleted() &&
                           (wantArchons || (u->getEnergy() < 75 && Grids::getGroundThreat(u->getPosition(), PlayerState::Enemy) > 0.0));
                });

                if (templar) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->useTech(Archon_Warp, templar->unit());
                    lastMorphFrame   = Broodwar->getFrameCount();
                    unit.commandText = "Archonification";
                    return true;
                }
            }
        }

        // Hydralisk - Lurker Morph
        else if (unit.getType() == Zerg_Hydralisk && Broodwar->self()->hasResearched(Lurker_Aspect)) {
            const auto wantLurkers = BuildOrder::getCompositionPercentage(Zerg_Lurker) >= 1.00;

            if (wantLurkers) {
                const auto furthestHydra = Util::getFurthestUnit(Terrain::getEnemyStartingPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == UnitTypes::Zerg_Hydralisk; });
                if (canAffordMorph(Zerg_Lurker) && furthestHydra && unit == *furthestHydra) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(Zerg_Lurker);
                    lastMorphFrame   = Broodwar->getFrameCount();
                    unit.commandText = "Lurkification";
                    return true;
                }
            }
        }

        // Mutalisk - Devourer / Guardian Morph
        else if (unit.getType() == Zerg_Mutalisk && com(Zerg_Greater_Spire) > 0) {
            const auto wantDevo  = BuildOrder::getCompositionPercentage(Zerg_Devourer) >= 1.00;
            const auto wantGuard = BuildOrder::getCompositionPercentage(Zerg_Guardian) >= 1.00;

            if (wantGuard) {
                if (canAffordMorph(Zerg_Guardian) && unit.getPosition().getDistance(unit.getSimTarget().lock()->getPosition()) >= unit.getEngageRadius() &&
                    unit.getPosition().getDistance(unit.getSimTarget().lock()->getPosition()) < unit.getEngageRadius() + 160.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(Zerg_Guardian);
                    lastMorphFrame   = Broodwar->getFrameCount();
                    unit.commandText = "Guardianification";
                    return true;
                }
            }
            else if (wantDevo) {
                if (canAffordMorph(Zerg_Devourer) && unit.getPosition().getDistance(unit.getSimTarget().lock()->getPosition()) >= unit.getEngageRadius()) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(Zerg_Devourer);
                    lastMorphFrame   = Broodwar->getFrameCount();
                    unit.commandText = "Devourification";
                    return true;
                }
            }
        }
        return false;
    }

    bool train(UnitInfo &unit) { return false; }

    bool returnResource(UnitInfo &unit)
    {
        // Can't return cargo if we aren't carrying a resource or overlapping a building position
        if ((!unit.unit()->isCarryingGas() && !unit.unit()->isCarryingMinerals()) || unit.getRole() != Role::Worker)
            return false;

        auto checkPath = (unit.hasResource() && unit.getPosition().getDistance(unit.getResource().lock()->getPosition()) > 320.0) ||
                         (!unit.hasResource() && !Terrain::inTerritory(PlayerState::Self, unit.getPosition()));
        if (checkPath) {
            // TODO: Create a path to the closest station and check if it's safe
        }

        // TODO: Check if we have a building to place first?
        if ((unit.unit()->isCarryingMinerals() || unit.unit()->isCarryingGas())) {
            if (unit.framesHoldingResource() >= 24 && (unit.unit()->isIdle() || (unit.unit()->getOrder() != Orders::ReturnMinerals && unit.unit()->getOrder() != Orders::ReturnGas)) &&
                unit.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo) {
                unit.unit()->returnCargo();
            }
            unit.commandText = "Return";
            return true;
        }

        return false;
    }

    bool clearNeutral(UnitInfo &unit)
    {
        // For now only workers clear neutrals
        auto resourceDepot = Broodwar->self()->getRace().getResourceDepot();
        if (!unit.getType().isWorker() || Util::getTime() < Time(8, 00) || Stations::getStations(PlayerState::Self).size() <= 2 ||
            (BuildOrder::buildCount(resourceDepot) == vis(resourceDepot) && BuildOrder::isOpener()) || unit.unit()->isCarryingMinerals() || unit.unit()->isCarryingGas())
            return false;

        // Find boulders to clear
        for (auto &b : Resources::getMyBoulders()) {
            ResourceInfo &boulder = *b;
            if (!boulder.unit() || !boulder.unit()->exists())
                continue;
            if ((unit.getPosition().getDistance(boulder.getPosition()) <= 320.0 && boulder.getGathererCount() == 0) ||
                (unit.unit()->isGatheringMinerals() && unit.unit()->getOrderTarget() == boulder.unit())) {

                auto closestWorker = Util::getClosestUnit(boulder.getPosition(), PlayerState::Self, [&](auto &u) { return u->getRole() == Role::Worker; });

                if (closestWorker && *closestWorker != unit && closestWorker->unit()->getOrderTarget() == boulder.unit())
                    continue;

                if (unit.unit()->getOrderTarget() != boulder.unit())
                    unit.unit()->gather(boulder.unit());
                unit.commandText = "Bouldering";
                return true;
            }
        }
        return false;
    }

    bool build(UnitInfo &unit)
    {
        if (unit.getRole() != Role::Worker || !unit.getBuildPosition().isValid())
            return false;

        const auto topLeft  = Position(unit.getBuildPosition());
        const auto botRight = Position(unit.getBuildPosition() + TilePosition(unit.getBuildType().tileWidth(), unit.getBuildType().tileHeight()));
        const auto center   = (topLeft + botRight) / 2;

        const auto fullyVisible = Broodwar->isVisible(unit.getBuildPosition()) && Broodwar->isVisible(unit.getBuildPosition() + TilePosition(unit.getBuildType().tileWidth(), 0)) &&
                                  Broodwar->isVisible(unit.getBuildPosition() + TilePosition(unit.getBuildType().tileWidth(), unit.getBuildType().tileHeight())) &&
                                  Broodwar->isVisible(unit.getBuildPosition() + TilePosition(0, unit.getBuildType().tileHeight()));

        const auto canAfford = Broodwar->self()->minerals() >= unit.getBuildType().mineralPrice() && Broodwar->self()->gas() >= unit.getBuildType().gasPrice();

        // Check for spidermines blocking it that can be killed
        if (Players::getVisibleCount(PlayerState::Self, Terran_Vulture_Spider_Mine) > 0) {
            auto closestMine = Util::getClosestUnit(center, PlayerState::Self,
                                                    [&](auto &u) { return u->getType() == Terran_Vulture_Spider_Mine && Util::rectangleIntersect(u->getPosition(), topLeft, botRight); });
            if (closestMine) {
                unit.setCommand(Attack_Unit, *closestMine);
                unit.commandText = "Attack Mine";
                return true;
            }
        }

        // If build position is fully visible and unit is close to it, start building as soon as possible
        if (fullyVisible && canAfford && unit.isWithinBuildRange()) {
            if (unit.unit()->getLastCommandFrame() < Broodwar->getFrameCount() - 10 && unit.unit()->getOrder() != Orders::PlaceBuilding) {
                unit.unit()->build(unit.getBuildType(), unit.getBuildPosition());
            }
            unit.commandText = "Build";
            return true;
        }
        return false;
    }

    bool gather(UnitInfo &unit)
    {
        if (unit.getRole() != Role::Worker)
            return false;

        const auto hasMineableResource = unit.hasResource() && (unit.getResource().lock()->getResourceState() == ResourceState::Mineable || Util::getTime() < Time(4, 00)) &&
                                         unit.getResource().lock()->unit()->exists();
        auto resource = unit.hasResource() ? &*unit.getResource().lock() : Resources::getClosestMineral(unit.getPosition(), [&](auto &m) { return true; });
        if (!resource)
            return false;

        auto boxDist              = Util::boxDistance(unit.getType(), unit.getPosition(), resource->getType(), resource->getPosition());
        auto allowOptimisations   = Players::getSupply(PlayerState::Self, Races::None) <= 200;

        const auto nearNonBlockingChoke = [&]() {
            auto closest = Util::getClosestChokepoint(unit.getPosition());
            return closest && !closest->Blocked() && unit.getPosition().getDistance(Position(closest->Center())) < 160.0;
        };

        const auto nearNonWorker = [&]() {
            auto closest = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return *u != unit && !u->getType().isWorker(); });
            return closest && Util::boxDistance(unit, *closest) < 32.0;
        };

        const auto canGather = [&](ResourceInfo *resource) {
            if (allowOptimisations) {
                if (boxDist > 0 && unit.unit()->getOrder() == Orders::MoveToMinerals && resource->getGatherOrderPositions().find(unit.getPosition()) != resource->getGatherOrderPositions().end())
                    return true;
                // Force mineral lock
                if (unit.unit()->getTarget() == resource->unit() && unit.unit()->getLastCommand().getType() == UnitCommandTypes::Gather)
                    return false;
            }
            else {
                // Don't try to mineral lock
                if (unit.unit()->getTarget(); auto info = Resources::getResourceInfo(unit.unit()->getTarget())) {
                    if (unit.hasResource() && info->getStation() == unit.getResource().lock()->getStation())
                        return false;
                }
            }
            if (Util::getTime() < Time(4, 00))
                return true;
            if (Grids::getGroundThreat(unit.getPosition(), PlayerState::Enemy) > 0.0f)
                return true;
            if (hasMineableResource && (unit.isWithinGatherRange() || nearNonWorker() || nearNonBlockingChoke()))
                return true;
            if (!hasMineableResource) // What
                return true;
            if (Planning::overlapsPlan(unit, unit.getPosition()))
                return true;

            return false;
        };

        // These worker order timers are based off Stardust: https://github.com/bmnielsen/Stardust/blob/master/src/Workers/WorkerOrderTimer.cpp#L153
        // From what Bruce found, you can prevent what seems like ~7 frames of a worker "waiting" to mine
        if (unit.hasResource() && Util::getTime() < Time(1, 30)) {
            auto boxDist = Util::boxDistance(unit.getType(), unit.getPosition(), unit.getResource().lock()->getType(), unit.getResource().lock()->getPosition());
            if (boxDist == 0) {
                auto frame   = Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 11;
                auto optimal = unit.getPositionHistory().find(frame);
                if (optimal != unit.getPositionHistory().end()) {

                    // Must check if we found a position to close, otherwise sometimes we send gather commands every frame or issue the command too early, not benefting from our optimization
                    for (auto &[f, position] : unit.getPositionHistory()) {
                        if (f <= (frame + 2))
                            continue;
                        unit.getResource().lock()->getGatherOrderPositions().erase(position);
                    }
                    unit.getResource().lock()->getGatherOrderPositions().insert(optimal->second);
                }
                unit.getPositionHistory().clear();
            }
        }

        // Gather from resource
        auto station = Stations::getClosestStationGround(unit.getPosition(), PlayerState::Self);
        if (resource->unit()->exists()) {
            if (canGather(resource)) {
                unit.unit()->gather(resource->unit());
                unit.commandText = "Gather";
                return true;
            }
        }
        return false;
    }

    bool special(UnitInfo &unit)
    {
        return castSelf(unit) || castTarget(unit) || burrow(unit) || click(unit) || siege(unit) || repair(unit) || morph(unit) || train(unit) || returnResource(unit) || clearNeutral(unit) ||
               build(unit) || lift(unit) || gather(unit);
    }
} // namespace McRave::Command