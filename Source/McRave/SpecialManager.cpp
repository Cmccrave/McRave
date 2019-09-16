#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Command
{
    namespace {

        bool click(UnitInfo& unit)
        {
            // Shield Battery - Repair Shields
            if (com(UnitTypes::Protoss_Shield_Battery) > 0 && (unit.unit()->getGroundWeaponCooldown() > Broodwar->getLatencyFrames() || unit.unit()->getAirWeaponCooldown() > Broodwar->getLatencyFrames()) && unit.getType().maxShields() > 0 && (unit.unit()->getShields() <= 10 || (unit.unit()->getShields() < unit.getType().maxShields() && unit.unit()->getOrderTarget() && unit.unit()->getOrderTarget()->exists() && unit.unit()->getOrderTarget()->getType() == UnitTypes::Protoss_Shield_Battery && unit.unit()->getOrderTarget()->getEnergy() >= 10))) {
                auto &battery = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType() == UnitTypes::Protoss_Shield_Battery && u.unit()->isCompleted() && u.getEnergy() > 10;
                });

                if (battery && ((unit.getType().isFlyer() && (!unit.hasTarget() || (unit.getTarget().getPosition().getDistance(unit.getPosition()) >= 320))) || unit.unit()->getDistance(battery->getPosition()) < 320)) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit || unit.unit()->getLastCommand().getTarget() != battery->unit())
                        unit.unit()->rightClick(battery->unit());
                    return true;
                }
            }

            // Bunker - Loading / Unloading
            else if (unit.getType() == UnitTypes::Terran_Marine && unit.getGlobalState() == GlobalState::Retreat && com(UnitTypes::Terran_Bunker) > 0) {

                auto &bunker = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return (u.getType() == UnitTypes::Terran_Bunker && u.unit()->getSpaceRemaining() > 0);
                });

                if (bunker && bunker->unit() && unit.hasTarget()) {
                    if (unit.getTarget().unit()->exists() && unit.getTarget().getPosition().getDistance(unit.getPosition()) <= 320) {
                        unit.unit()->rightClick(bunker->unit());
                        return true;
                    }
                    if (unit.unit()->isLoaded() && unit.getTarget().getPosition().getDistance(unit.getPosition()) > 320)
                        bunker->unit()->unloadAll();
                }
            }
            return false;
        }

        bool siege(UnitInfo& unit)
        {
            auto targetDist = unit.hasTarget() ? unit.getPosition().getDistance(unit.getTarget().getPosition()) : 0.0;

            // Siege Tanks - Siege
            if (unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) {
                if (unit.hasTarget() && unit.getTarget().getGroundRange() > 32.0 && targetDist <= 450.0 && targetDist >= 100.0 && unit.getLocalState() == LocalState::Attack)
                    unit.unit()->siege();
                if (unit.getGlobalState() == GlobalState::Retreat && unit.getPosition().getDistance(Terrain::getDefendPosition()) < 320)
                    unit.unit()->siege();
            }

            // Siege Tanks - Unsiege
            else if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode) {
                if (unit.hasTarget() && (unit.getTarget().getGroundRange() <= 32.0 || targetDist < 100.0 || targetDist > 450.0 || unit.getLocalState() == LocalState::Retreat)) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Unsiege)
                        unit.unit()->unsiege();
                    return true;
                }
            }
            return false;
        }

        bool repair(UnitInfo& unit)
        {
            // SCV
            if (unit.getType() == UnitTypes::Terran_SCV) {

                // Repair closest injured mech
                auto &mech = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return (u.getType().isMechanical() && u.getPercentHealth() < 1.0);
                });

                if (!Strategy::enemyRush() && mech && mech->unit() && unit.getPosition().getDistance(mech->getPosition()) <= 320 && Grids::getMobility(mech->getWalkPosition()) > 0) {
                    if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != mech->unit())
                        unit.unit()->repair(mech->unit());
                    return true;
                }

                // Repair closest injured building
                auto &building = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getPercentHealth() < 0.35 || (u.getType() == UnitTypes::Terran_Bunker && u.getPercentHealth() < 1.0);
                });

                if (building && building->unit() && (!Strategy::enemyRush() || building->getType() == UnitTypes::Terran_Bunker)) {
                    if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != building->unit())
                        unit.unit()->repair(building->unit());
                    return true;
                }
            }
            return false;
        }

        bool burrow(UnitInfo& unit)
        {
            // Vulture spider mine burrow / unburrow
            if (unit.getType() == UnitTypes::Terran_Vulture) {
                if (Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && unit.unit()->getSpiderMineCount() > 0 && unit.getPosition().getDistance(unit.getSimPosition()) <= 400 && Broodwar->getUnitsInRadius(unit.getPosition(), 128, Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine).size() <= 3) {
                    if (unit.unit()->getLastCommand().getTechType() != TechTypes::Spider_Mines || unit.unit()->getLastCommand().getTargetPosition().getDistance(unit.getPosition()) > 8)
                        unit.unit()->useTech(TechTypes::Spider_Mines, unit.getPosition());
                    return true;
                }
            }

            // Lurker burrow / unburrow
            else if (unit.getType() == UnitTypes::Zerg_Lurker) {
                if (unit.getDestination().isValid() && unit.getPosition().getDistance(unit.getDestination()) < 64.0) {
                    if (!unit.unit()->isBurrowed()) {
                        if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Burrow)
                            unit.unit()->burrow();
                        return true;
                    }
                }
                else if (!unit.unit()->isBurrowed() && unit.getPosition().getDistance(unit.getEngagePosition()) < 32.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Burrow)
                        unit.unit()->burrow();
                    return true;
                }
                else if (unit.unit()->isBurrowed() && unit.getPosition().getDistance(unit.getEngagePosition()) > 128.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Unburrow)
                        unit.unit()->unburrow();
                    return true;
                }
            }
            return false;
        }

        bool cast(UnitInfo& unit)
        {
            // Battlecruiser - Yamato
            if (unit.getType() == UnitTypes::Terran_Battlecruiser) {
                if ((unit.unit()->getOrder() == Orders::FireYamatoGun || (Broodwar->self()->hasResearched(TechTypes::Yamato_Gun) && unit.unit()->getEnergy() >= TechTypes::Yamato_Gun.energyCost()) && unit.getTarget().unit()->getHitPoints() >= 80) && unit.hasTarget() && unit.getTarget().unit()->exists()) {
                    if ((unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()))
                        unit.unit()->useTech(TechTypes::Yamato_Gun, unit.getTarget().unit());

                    Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Yamato_Gun, PlayerState::Self);
                    return true;
                }
            }

            // Ghost - Cloak / Nuke
            else if (unit.getType() == UnitTypes::Terran_Ghost) {

                if (!unit.unit()->isCloaked() && unit.unit()->getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
                    unit.unit()->useTech(TechTypes::Personnel_Cloaking);

                if (com(Terran_Nuclear_Missile) > 0 && unit.hasTarget() && unit.getTarget().getWalkPosition().isValid() && unit.unit()->isCloaked() && Grids::getEAirCluster(unit.getTarget().getWalkPosition()) + Grids::getEGroundCluster(unit.getTarget().getWalkPosition()) > 5.0 && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 320 && unit.getPosition().getDistance(unit.getTarget().getPosition()) > 200) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Unit || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()) {
                        unit.unit()->useTech(TechTypes::Nuclear_Strike, unit.getTarget().unit());
                        Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Nuclear_Strike, PlayerState::Self);
                        return true;
                    }
                }
                if (unit.unit()->getOrder() == Orders::NukePaint || unit.unit()->getOrder() == Orders::NukeTrack || unit.unit()->getOrder() == Orders::CastNuclearStrike) {
                    Actions::addAction(unit.unit(), unit.unit()->getOrderTargetPosition(), TechTypes::Nuclear_Strike, PlayerState::Self);
                    return true;
                }
            }

            // Marine / Firebat - Stim Packs
            else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && (unit.getType() == UnitTypes::Terran_Marine || unit.getType() == UnitTypes::Terran_Firebat) && !unit.unit()->isStimmed() && unit.hasTarget() && unit.getTarget().getPosition().isValid() && unit.unit()->getDistance(unit.getTarget().getPosition()) <= unit.getGroundRange())
                unit.unit()->useTech(TechTypes::Stim_Packs);

            // Science Vessel - Defensive Matrix
            else if (unit.getType() == UnitTypes::Terran_Science_Vessel && unit.unit()->getEnergy() >= TechTypes::Defensive_Matrix) {
                auto &ally = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return (u.unit()->isUnderAttack());
                });
                if (ally && ally->getPosition().getDistance(unit.getPosition()) < 640)
                    unit.unit()->useTech(TechTypes::Defensive_Matrix, ally->unit());
                return true;
            }

            // Wraith - Cloak
            else if (unit.getType() == UnitTypes::Terran_Wraith) {
                if (unit.getHealth() >= 120 && !unit.unit()->isCloaked() && unit.unit()->getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
                    unit.unit()->useTech(TechTypes::Cloaking_Field);
                else if (unit.getHealth() <= 90 && unit.unit()->isCloaked())
                    unit.unit()->useTech(TechTypes::Cloaking_Field);
            }

            // Corsair - Disruption Web
            else if (unit.getType() == UnitTypes::Protoss_Corsair && unit.unit()->getEnergy() >= TechTypes::Disruption_Web.energyCost() && Broodwar->self()->hasResearched(TechTypes::Disruption_Web)) {
                auto closestEnemy = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getPosition().getDistance(unit.getPosition()) < 256.0 && !Actions::overlapsActions(unit.unit(), u.getPosition(), TechTypes::Disruption_Web, PlayerState::Self, 96) && u.hasAttackedRecently() && u.getSpeed() <= UnitTypes::Protoss_Reaver.topSpeed();
                });

                if (closestEnemy) {
                    Actions::addAction(unit.unit(), closestEnemy->getPosition(), TechTypes::Disruption_Web, PlayerState::Self);
                    unit.unit()->useTech(TechTypes::Disruption_Web, closestEnemy->getPosition());
                    return true;
                }
            }

            // High Templar - Psi Storm
            else if (unit.getType() == UnitTypes::Protoss_High_Templar) {

                // If HT is trying to storm, add action
                if (unit.unit()->getOrder() == Orders::CastPsionicStorm && unit.hasTarget())
                    Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);

                // If close to target and can cast a storm
                if (unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && unit.canStartCast(TechTypes::Psionic_Storm)) {
                    unit.unit()->useTech(TechTypes::Psionic_Storm, unit.getTarget().getPosition());
                    Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);
                    return true;
                }
            }
            return false;
        }

        bool morph(UnitInfo& unit)
        {
            auto canAffordMorph = [&](UnitType type) {
                if (Broodwar->self()->minerals() > type.mineralPrice() && Broodwar->self()->gas() > type.gasPrice() && Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed() > type.supplyRequired())
                    return true;
                return false;
            };

            // High Templar - Archon Morph
            if (unit.getType() == UnitTypes::Protoss_High_Templar) {
                auto lowEnergyThreat = unit.getEnergy() < TechTypes::Psionic_Storm.energyCost() && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0f;
                auto wantArchons = Production::scoreUnit(UnitTypes::Protoss_Archon) > Production::scoreUnit(UnitTypes::Protoss_High_Templar);

                if (!Players::vT() && (lowEnergyThreat || wantArchons)) {

                    // Try to find a friendly templar who is low energy and is threatened
                    auto &templar = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u != unit && u.getType() == UnitTypes::Protoss_High_Templar && u.unit()->isCompleted() && (wantArchons || (u.getEnergy() < 75 && Grids::getEGroundThreat(u.getWalkPosition()) > 0.0));
                    });

                    if (templar) {
                        if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                            unit.unit()->useTech(TechTypes::Archon_Warp, templar->unit());
                        return true;
                    }
                }
            }

            // Hydralisk - Lurker Morph
            else if (unit.getType() == UnitTypes::Zerg_Hydralisk && Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect)) {
                if (Strategy::getUnitScore(UnitTypes::Zerg_Lurker) > Strategy::getUnitScore(UnitTypes::Zerg_Hydralisk) && canAffordMorph(UnitTypes::Zerg_Lurker) && unit.getPosition().getDistance(unit.getSimPosition()) > 640.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(UnitTypes::Zerg_Lurker);
                    return true;
                }
            }

            // Mutalisk - Devourer / Guardian Morph
            else if (unit.getType() == UnitTypes::Zerg_Mutalisk && com(UnitTypes::Zerg_Greater_Spire) > 0) {
                if (Strategy::getUnitScore(UnitTypes::Zerg_Devourer) > Strategy::getUnitScore(UnitTypes::Zerg_Mutalisk) && canAffordMorph(UnitTypes::Zerg_Devourer) && unit.getPosition().getDistance(unit.getSimPosition()) > 640.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(UnitTypes::Zerg_Devourer);
                    return true;
                }
                else if (Strategy::getUnitScore(UnitTypes::Zerg_Guardian) > Strategy::getUnitScore(UnitTypes::Zerg_Mutalisk) && canAffordMorph(UnitTypes::Zerg_Guardian) && unit.getPosition().getDistance(unit.getSimPosition()) > 640.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(UnitTypes::Zerg_Guardian);
                    return true;
                }
            }
            return false;
        }

        bool train(UnitInfo& unit)
        {
            // Carrier - Train Interceptor
            if (unit.getType() == UnitTypes::Protoss_Carrier && unit.unit()->getInterceptorCount() < MAX_INTERCEPTOR && !unit.unit()->isTraining()) {
                unit.unit()->train(UnitTypes::Protoss_Interceptor);
                return false;
            }

            // Reaver - Train Scarab
            else if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && unit.unit()->getScarabCount() + int(unit.unit()->getTrainingQueue().size()) < MAX_SCARAB && (Broodwar->self()->minerals() >= 25 || unit.unit()->getGroundWeaponCooldown() >= 60)) {
                unit.unit()->train(UnitTypes::Protoss_Scarab);
                unit.setLastAttackFrame(Broodwar->getFrameCount());	/// Use this to fudge whether a Reaver has actually shot when using shuttles due to cooldown reset
                return false;
            }
            return false;
        }

        bool returnResource(UnitInfo& unit)
        {
            // Can't return cargo if we aren't carrying a resource or overlapping a building position
            if ((!unit.unit()->isCarryingGas() && !unit.unit()->isCarryingMinerals())
                || Buildings::overlapsQueue(unit, unit.getTilePosition()))
                return false;

            auto checkPath = (unit.hasResource() && unit.getPosition().getDistance(unit.getResource().getPosition()) > 320.0) || (!unit.hasResource() && !Terrain::isInAllyTerritory(unit.getTilePosition()));
            if (checkPath) {
                // TODO: Create a path to the closest station and check if it's safe
            }

            // TODO: Check if we have a building to place first?
            if (unit.unit()->getOrder() != Orders::ReturnMinerals && unit.unit()->getOrder() != Orders::ReturnGas && unit.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
                unit.unit()->returnCargo();
            return true;
        }

        bool clearNeutral(UnitInfo& unit)
        {
            // For now only workers clear neutrals
            auto resourceDepot = Broodwar->self()->getRace().getResourceDepot();
            if (!unit.getType().isWorker()
                || vis(resourceDepot) < 2
                || (BuildOrder::buildCount(resourceDepot) == vis(resourceDepot) && BuildOrder::isOpener())
                || unit.unit()->isCarryingMinerals()
                || unit.unit()->isCarryingGas()
                /*|| boulderWorkers != 0*/)
                return false;

            // Find boulders to clear
            for (auto &b : Resources::getMyBoulders()) {
                ResourceInfo &boulder = *b;
                if (!boulder.unit() || !boulder.unit()->exists())
                    continue;
                if ((unit.getPosition().getDistance(boulder.getPosition()) <= 320.0 && boulder.getGathererCount() == 0) || (unit.unit()->isGatheringMinerals() && unit.unit()->getOrderTarget() == boulder.unit())) {
                    if (unit.unit()->getOrderTarget() != boulder.unit())
                        unit.unit()->gather(boulder.unit());
                    //boulderWorkers = 1;
                    return true;
                }
            }
            return false;
        }

        bool build(UnitInfo& unit)
        {
            if (unit.getRole() != Role::Worker || !unit.getBuildPosition().isValid())
                return false;

            const auto fullyVisible = Broodwar->isVisible(unit.getBuildPosition())
                && Broodwar->isVisible(unit.getBuildPosition() + TilePosition(unit.getBuildType().tileWidth(), 0))
                && Broodwar->isVisible(unit.getBuildPosition() + TilePosition(unit.getBuildType().tileWidth(), unit.getBuildType().tileHeight()))
                && Broodwar->isVisible(unit.getBuildPosition() + TilePosition(0, unit.getBuildType().tileHeight()));

            const auto canAfford = Broodwar->self()->minerals() >= unit.getBuildType().mineralPrice() && Broodwar->self()->gas() >= unit.getBuildType().gasPrice();

            // If build position is fully visible and unit is close to it, start building as soon as possible
            if (fullyVisible && canAfford && unit.isWithinBuildRange()) {
                unit.unit()->build(unit.getBuildType(), unit.getBuildPosition());
                return true;
            }
            else if (Workers::shouldMoveToBuild(unit, unit.getBuildPosition(), unit.getBuildType())) {
                Command::move(unit);
                return true;
            }
            return false;
        }

        bool gather(UnitInfo& unit)
        {
            if (unit.getRole() != Role::Worker)
                return false;

            if (unit.hasResource() && unit.getResource().getResourceState() == ResourceState::Mineable && (unit.isWithinGatherRange() || Grids::getAGroundCluster(unit.getPosition()) > 0.0f)) {
                if (unit.canStartGather())
                    unit.unit()->gather(unit.getResource().unit());
                return true;
            }
            else if (!unit.hasResource() || unit.getResource().getResourceState() != ResourceState::Mineable) {
                auto closest = unit.unit()->getClosestUnit(Filter::IsMineralField);
                if (closest && closest->exists() && unit.unit()->getLastCommand().getTarget() != closest && unit.canStartGather())
                    unit.unit()->gather(closest);
                return true;
            }
            return false;
        }
    }

    bool special(UnitInfo& unit)
    {
        return click(unit)
            || siege(unit)
            || repair(unit)
            || burrow(unit)
            || cast(unit)
            || morph(unit)
            || train(unit)
            || returnResource(unit)
            || clearNeutral(unit)
            || build(unit)
            || gather(unit);
    }
}