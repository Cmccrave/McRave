#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Command
{
    bool click(UnitInfo& unit)
    {
        // Shield Battery - Repair Shields
        if (com(Protoss_Shield_Battery) > 0 && (unit.unit()->getGroundWeaponCooldown() > Broodwar->getLatencyFrames() || unit.unit()->getAirWeaponCooldown() > Broodwar->getLatencyFrames()) && unit.getType().maxShields() > 0 && (unit.unit()->getShields() <= 10 || (unit.unit()->getShields() < unit.getType().maxShields() && unit.unit()->getOrderTarget() && unit.unit()->getOrderTarget()->exists() && unit.unit()->getOrderTarget()->getType() == Protoss_Shield_Battery && unit.unit()->getOrderTarget()->getEnergy() >= 10))) {
            auto &battery = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return u.getType() == Protoss_Shield_Battery && u.unit()->isCompleted() && u.getEnergy() > 10;
            });

            if (battery && ((unit.getType().isFlyer() && (!unit.hasTarget() || (unit.getTarget().getPosition().getDistance(unit.getPosition()) >= 320))) || unit.unit()->getDistance(battery->getPosition()) < 320)) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit || unit.unit()->getLastCommand().getTarget() != battery->unit())
                    unit.unit()->rightClick(battery->unit());
                return true;
            }
        }


        // Bunker - Loading / Unloading
        else if (unit.getType() == Terran_Marine && com(Terran_Bunker) > 0) {

            auto &bunker = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return (u.getType() == Terran_Bunker && u.unit()->getSpaceRemaining() > 0);
            });

            if (bunker) {
                unit.unit()->rightClick(bunker->unit());
                return true;
            }

            /*if (bunker && bunker->unit() && unit.hasTarget()) {
                if (unit.getTarget().unit()->exists() && unit.getTarget().getPosition().getDistance(unit.getPosition()) <= 320) {
                    unit.unit()->rightClick(bunker->unit());
                    return true;
                }
                if (unit.unit()->isLoaded() && unit.getTarget().getPosition().getDistance(unit.getPosition()) > 320)
                    bunker->unit()->unloadAll();
            }*/
        }
        return false;
    }

    bool siege(UnitInfo& unit)
    {
        auto targetDist = unit.hasTarget() ? unit.getPosition().getDistance(unit.getTarget().getPosition()) : 0.0;

        // Siege Tanks - Siege
        if (unit.getType() == Terran_Siege_Tank_Tank_Mode) {
            if (unit.hasTarget() && unit.getTarget().getGroundRange() > 32.0 && targetDist <= 450.0 && targetDist >= 100.0 && unit.getLocalState() == LocalState::Attack)
                unit.unit()->siege();
            if (unit.getGlobalState() == GlobalState::Retreat && unit.getPosition().getDistance(Terrain::getDefendPosition()) < 320)
                unit.unit()->siege();
        }

        // Siege Tanks - Unsiege
        else if (unit.getType() == Terran_Siege_Tank_Siege_Mode) {
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
        if (unit.getType() == Terran_SCV) {

            //// Repair closest injured mech
            //auto &mech = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
            //    return (u.getType().isMechanical() && u.getPercentHealth() < 1.0);
            //});

            //if (!Strategy::enemyRush() && mech && mech->unit() && unit.getPosition().getDistance(mech->getPosition()) <= 320 && Grids::getMobility(mech->getWalkPosition()) > 0) {
            //    if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != mech->unit())
            //        unit.unit()->repair(mech->unit());
            //    return true;
            //}

            //// Repair closest injured building
            //auto &building = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
            //    return u.getPercentHealth() < 0.35 || (u.getType() == Terran_Bunker && u.getPercentHealth() < 1.0);
            //});

            //if (building && building->unit() && (!Strategy::enemyRush() || building->getType() == Terran_Bunker)) {
            //    if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != building->unit())
            //        unit.unit()->repair(building->unit());
            //    return true;
            //}
        }
        return false;
    }

    bool burrow(UnitInfo& unit)
    {
        // Vulture spider mine burrowing
        if (unit.getType() == Terran_Vulture) {
            if (Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && unit.unit()->getSpiderMineCount() > 0 && unit.getPosition().getDistance(unit.getSimPosition()) <= 400 && Broodwar->getUnitsInRadius(unit.getPosition(), 128, Filter::GetType == Terran_Vulture_Spider_Mine).size() <= 3) {
                if (unit.unit()->getLastCommand().getTechType() != TechTypes::Spider_Mines || unit.unit()->getLastCommand().getTargetPosition().getDistance(unit.getPosition()) > 8)
                    unit.unit()->useTech(TechTypes::Spider_Mines, unit.getPosition());
                return true;
            }
        }

        // Lurker burrowing
        else if (unit.getType() == Zerg_Lurker) {
            if (unit.getLocalState() == LocalState::Attack) {
                if (unit.getDestination().isValid() && unit.getPosition().getDistance(unit.getDestination()) < 16.0) {
                    if (!unit.unit()->isBurrowed()) {
                        if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Burrow) {
                            if (!unit.getTarget().isThreatening())
                                easyWrite(to_string(unit.getSimValue()));
                            unit.unit()->burrow();
                        }
                        return true;
                    }
                }
                else if (!unit.unit()->isBurrowed() && unit.getPosition().getDistance(unit.getEngagePosition()) < 32.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Burrow) {
                        if (!unit.getTarget().isThreatening())
                            easyWrite(to_string(unit.getSimValue()));
                        unit.unit()->burrow();
                    }
                    return true;
                }
            }
            else if (unit.unit()->isBurrowed() && unit.getPosition().getDistance(unit.getEngagePosition()) > 128.0) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Unburrow)
                    unit.unit()->unburrow();
                return true;
            }
        }

        // Drone / Defiler burrowing
        else if ((unit.getType().isWorker() || unit.getType() == Zerg_Defiler) && Broodwar->self()->hasResearched(TechTypes::Burrowing)) {
            if (!unit.isBurrowed() && unit.unit()->canBurrow()) {
                auto threatened = (unit.getType().isWorker() || unit.getType() == Zerg_Defiler) && unit.unit()->isUnderAttack();
                if (threatened && Grids::getEGroundThreat(unit.getPosition()) > 0.0f) {
                    unit.unit()->burrow();
                    return true;
                }
            }
            else if (unit.isBurrowed()) {
                if (unit.isHealthy() && Grids::getEGroundThreat(unit.getPosition()) == 0.0f) {
                    unit.unit()->unburrow();
                    return true;
                }
            }
        }

        // Zergling burrowing
        else if (unit.getType() == Zerg_Zergling && Broodwar->self()->hasResearched(TechTypes::Burrowing)) {
            if (!unit.isBurrowed() && !Buildings::overlapsQueue(unit, unit.getPosition()) && unit.getGoal().getDistance(unit.getPosition()) < 16.0)
                unit.unit()->burrow();
            if (unit.isBurrowed() && (Buildings::overlapsQueue(unit, unit.getPosition()) || unit.getGoal().getDistance(unit.getPosition()) > 16.0))
                unit.unit()->unburrow();
        }

        return false;
    }

    bool cast(UnitInfo& unit)
    {
        // Battlecruiser - Yamato
        if (unit.getType() == Terran_Battlecruiser) {
            if ((unit.unit()->getOrder() == Orders::FireYamatoGun || (Broodwar->self()->hasResearched(TechTypes::Yamato_Gun) && unit.getEnergy() >= TechTypes::Yamato_Gun.energyCost()) && unit.getTarget().unit()->getHitPoints() >= 80) && unit.hasTarget() && unit.getTarget().unit()->exists()) {
                if ((unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()))
                    unit.unit()->useTech(TechTypes::Yamato_Gun, unit.getTarget().unit());

                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Yamato_Gun, PlayerState::Self);
                return true;
            }
        }

        // Ghost - Cloak / Nuke
        else if (unit.getType() == Terran_Ghost) {

            if (!unit.unit()->isCloaked() && unit.getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
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
        else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && (unit.getType() == Terran_Marine || unit.getType() == Terran_Firebat) && !unit.unit()->isStimmed() && unit.hasTarget() && unit.getTarget().getPosition().isValid() && unit.unit()->getDistance(unit.getTarget().getPosition()) <= unit.getGroundRange())
            unit.unit()->useTech(TechTypes::Stim_Packs);

        // Science Vessel - Defensive Matrix
        else if (unit.getType() == Terran_Science_Vessel && unit.getEnergy() >= TechTypes::Defensive_Matrix) {
            auto &ally = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return (u.unit()->isUnderAttack());
            });
            if (ally && ally->getPosition().getDistance(unit.getPosition()) < 640)
                unit.unit()->useTech(TechTypes::Defensive_Matrix, ally->unit());
            return true;
        }

        // Wraith - Cloak
        else if (unit.getType() == Terran_Wraith) {
            if (unit.getHealth() >= 120 && !unit.unit()->isCloaked() && unit.getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
                unit.unit()->useTech(TechTypes::Cloaking_Field);
            else if (unit.getHealth() <= 90 && unit.unit()->isCloaked())
                unit.unit()->useTech(TechTypes::Cloaking_Field);
        }

        // Corsair - Disruption Web
        else if (unit.getType() == Protoss_Corsair && unit.getEnergy() >= TechTypes::Disruption_Web.energyCost() && Broodwar->self()->hasResearched(TechTypes::Disruption_Web)) {
            auto closestEnemy = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u.getPosition().getDistance(unit.getPosition()) < 256.0 && !Actions::overlapsActions(unit.unit(), u.getPosition(), TechTypes::Disruption_Web, PlayerState::Self, 96) && u.hasAttackedRecently() && u.getSpeed() <= Protoss_Reaver.topSpeed();
            });

            if (closestEnemy) {
                Actions::addAction(unit.unit(), closestEnemy->getPosition(), TechTypes::Disruption_Web, PlayerState::Self);
                unit.unit()->useTech(TechTypes::Disruption_Web, closestEnemy->getPosition());
                return true;
            }
        }

        // High Templar - Psi Storm
        else if (unit.getType() == Protoss_High_Templar) {

            // If trying to cast Psi Storm, add action
            if (unit.unit()->getOrder() == Orders::CastPsionicStorm && unit.hasTarget())
                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);

            // If close to target and can cast a storm
            if (unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && unit.canStartCast(TechTypes::Psionic_Storm)) {
                unit.unit()->useTech(TechTypes::Psionic_Storm, unit.getTarget().getPosition());
                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);
                return true;
            }
        }

        // Defiler - Consume / Dark Swarm / Plague
        else if (unit.getType() == Zerg_Defiler) {

            // If trying to cast Dark Swarm, add action
            if (unit.unit()->getOrder() == Orders::CastDarkSwarm)
                Actions::addAction(unit.unit(), unit.unit()->getOrderTargetPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);

            // If trying to cast Plague, add action
            if (unit.unit()->getOrder() == Orders::CastPlague)
                Actions::addAction(unit.unit(), unit.unit()->getOrderTargetPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral);

            // If close to target and can cast Dark Swarm
            if (unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && unit.canStartCast(TechTypes::Dark_Swarm)) {
                unit.unit()->useTech(TechTypes::Dark_Swarm, unit.getTarget().getPosition());
                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral);
                return true;
            }

            // If close to target and can cast Plague
            if (unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && unit.canStartCast(TechTypes::Plague)) {
                unit.unit()->useTech(TechTypes::Plague, unit.getTarget().getPosition());
                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Plague, PlayerState::Neutral);
                return true;
            }

            // If close to target and need to Consume
            if (unit.hasTarget() && unit.getEnergy() < 100 && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && unit.canStartCast(TechTypes::Consume)) {
                unit.unit()->useTech(TechTypes::Consume, unit.getTarget().unit());
                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Consume, PlayerState::Neutral);
                return true;
            }
        }
        return false;
    }

    bool morph(UnitInfo& unit)
    {
        auto canAffordMorph = [&](UnitType type) {
            if (Broodwar->self()->minerals() >= type.mineralPrice() && Broodwar->self()->gas() >= type.gasPrice() && Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed() > type.supplyRequired())
                return true;
            return false;
        };

        // High Templar - Archon Morph
        if (unit.getType() == Protoss_High_Templar) {
            auto lowEnergyThreat = unit.getEnergy() < TechTypes::Psionic_Storm.energyCost() && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0f;
            auto wantArchons = vis(Protoss_Archon) / BuildOrder::getCompositionPercentage(Protoss_Archon) < vis(Protoss_High_Templar) / BuildOrder::getCompositionPercentage(Protoss_High_Templar);

            if (!Players::vT() && (lowEnergyThreat || wantArchons)) {

                // Try to find a friendly templar who is low energy and is threatened
                auto &templar = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u != unit && u.getType() == Protoss_High_Templar && u.unit()->isCompleted() && (wantArchons || (u.getEnergy() < 75 && Grids::getEGroundThreat(u.getWalkPosition()) > 0.0));
                });

                if (templar) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->useTech(TechTypes::Archon_Warp, templar->unit());
                    return true;
                }
            }
        }

        // Hydralisk - Lurker Morph
        else if (unit.getType() == Zerg_Hydralisk && Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect)) {
            const auto wantLurkers = vis(Zerg_Lurker) / BuildOrder::getCompositionPercentage(Zerg_Lurker) < vis(Zerg_Hydralisk) / BuildOrder::getCompositionPercentage(Zerg_Hydralisk);
            const auto onlyLurkers = BuildOrder::getCompositionPercentage(Zerg_Lurker) >= 1.00;

            if ((wantLurkers || onlyLurkers)) {
                if (canAffordMorph(Zerg_Lurker) && unit.getPosition().getDistance(unit.getSimPosition()) > unit.getSimRadius()) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(Zerg_Lurker);
                    return true;
                }
            }
        }

        // Mutalisk - Devourer / Guardian Morph
        else if (unit.getType() == Zerg_Mutalisk && com(Zerg_Greater_Spire) > 0) {
            const auto wantDevo = vis(Zerg_Devourer) / BuildOrder::getCompositionPercentage(Zerg_Devourer) < (vis(Zerg_Mutalisk) / BuildOrder::getCompositionPercentage(Zerg_Mutalisk)) + (vis(Zerg_Guardian) / BuildOrder::getCompositionPercentage(Zerg_Guardian));
            const auto wantGuard = vis(Zerg_Guardian) / BuildOrder::getCompositionPercentage(Zerg_Guardian) < (vis(Zerg_Mutalisk) / BuildOrder::getCompositionPercentage(Zerg_Mutalisk)) + (vis(Zerg_Devourer) / BuildOrder::getCompositionPercentage(Zerg_Devourer));

            const auto onlyDevo = BuildOrder::getCompositionPercentage(Zerg_Devourer) >= 1.00;
            const auto onlyGuard = BuildOrder::getCompositionPercentage(Zerg_Guardian) >= 1.00;

            if ((onlyGuard || wantGuard)) {
                if (canAffordMorph(Zerg_Guardian) && unit.getPosition().getDistance(unit.getSimPosition()) >= unit.getSimRadius() && unit.getPosition().getDistance(unit.getSimPosition()) < unit.getSimRadius() + 160.0) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(Zerg_Guardian);
                    return true;
                }
            }
            else if ((onlyDevo || wantDevo)) {
                if (canAffordMorph(Zerg_Devourer) && unit.getPosition().getDistance(unit.getSimPosition()) >= unit.getSimRadius()) {
                    if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                        unit.unit()->morph(Zerg_Devourer);
                    return true;
                }
            }
        }
        return false;
    }

    bool train(UnitInfo& unit)
    {
        // Carrier - Train Interceptor
        if (unit.getType() == Protoss_Carrier && unit.unit()->getInterceptorCount() < MAX_INTERCEPTOR && !unit.unit()->isTraining()) {
            unit.unit()->train(Protoss_Interceptor);
            return false;
        }

        // Reaver - Train Scarab
        else if (unit.getType() == Protoss_Reaver && !unit.unit()->isLoaded() && unit.unit()->getScarabCount() + int(unit.unit()->getTrainingQueue().size()) < MAX_SCARAB && (Broodwar->self()->minerals() >= 25 || unit.unit()->getGroundWeaponCooldown() >= 60)) {
            unit.unit()->train(Protoss_Scarab);
            unit.setLastAttackFrame(Broodwar->getFrameCount());    /// Use this to fudge whether a Reaver has actually shot when using shuttles due to cooldown reset
            return false;
        }
        return false;
    }

    bool returnResource(UnitInfo& unit)
    {
        // Can't return cargo if we aren't carrying a resource or overlapping a building position
        if ((!unit.unit()->isCarryingGas() && !unit.unit()->isCarryingMinerals()))
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
            || Util::getTime() < Time(6, 0)
            || vis(resourceDepot) < 2
            || (BuildOrder::buildCount(resourceDepot) == vis(resourceDepot) && BuildOrder::isOpener())
            || unit.unit()->isCarryingMinerals()
            || unit.unit()->isCarryingGas())
            return false;

        // Find boulders to clear
        for (auto &b : Resources::getMyBoulders()) {
            ResourceInfo &boulder = *b;
            if (!boulder.unit() || !boulder.unit()->exists())
                continue;
            if ((unit.getPosition().getDistance(boulder.getPosition()) <= 320.0 && boulder.getGathererCount() == 0) || (unit.unit()->isGatheringMinerals() && unit.unit()->getOrderTarget() == boulder.unit())) {

                auto closestWorker = Util::getClosestUnit(boulder.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Worker;
                });

                if (closestWorker != unit.shared_from_this())
                    continue;

                if (unit.unit()->getOrderTarget() != boulder.unit())
                    unit.unit()->gather(boulder.unit());
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
        // HACK: Just spam a move command
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

        const auto hasMineableResource = unit.hasResource() && unit.getResource().getResourceState() == ResourceState::Mineable && unit.getResource().unit()->exists();

        const auto canGather = [&]() {
            auto closestChokepoint = Util::getClosestChokepoint(unit.getPosition());
            auto nearNonBlockingChoke = closestChokepoint && !closestChokepoint->Blocked();

            if (hasMineableResource && (unit.isWithinGatherRange() || Grids::getAGroundCluster(unit.getPosition()) > 0.0f || nearNonBlockingChoke))
                return true;
            if (!hasMineableResource && (unit.unit()->isIdle() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Gather || (unit.unit()->getTarget() && !unit.unit()->getTarget()->getType().isMineralField())))
                return true;
            if (Buildings::overlapsQueue(unit, unit.getPosition()))
                return true;
            return false;
        };

        // Check if we're trying to build a defense near this worker
        if (unit.hasResource()) {
            auto station = unit.getResource().getStation();

            if (station && (Stations::needGroundDefenses(*station) > 0 || Stations::needAirDefenses(*station) > 0)) {
                auto builder = Util::getClosestUnit(unit.getResource().getPosition(), PlayerState::Self, [&](auto &u) {
                    return u != unit && u.getBuildType() != UnitTypes::None;
                });

                // Builder is close and may need space opened up
                if (builder) {
                    auto center = Position(builder->getBuildPosition()) + Position(32, 32);
                    if (builder->getPosition().getDistance(center) < 64.0 && unit.getResource().getPosition().getDistance(center) < 128.0 && Broodwar->self()->minerals() >= builder->getBuildType().mineralPrice()) {
                        unit.circlePurple();

                        // Get furthest Mineral
                        BWEM::Mineral * furthest = nullptr;
                        auto furthestDist = 0.0;
                        for (auto &resource : unit.getResource().getStation()->getBase()->Minerals()) {
                            if (resource && resource->Unit()->exists()) {
                                auto dist = resource->Pos().getDistance(center);
                                if (dist > furthestDist) {
                                    furthestDist = dist;
                                    furthest = resource;
                                }
                            }
                        }

                        // Spam gather it to move out of the way
                        if (furthest) {
                            unit.unit()->gather(furthest->Unit());
                            return true;
                        }
                    }
                }

            }
        }

        // Gather from resource
        auto station = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
        auto target = hasMineableResource ? unit.getResource().unit() : Broodwar->getClosestUnit(station ? station->getResourceCentroid() : BWEB::Map::getMainPosition(), Filter::IsMineralField);
        if (target && canGather()) {            
            if (unit.unit()->getTarget() != target)
                unit.unit()->gather(target);
            return true;
        }
        return false;
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