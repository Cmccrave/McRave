#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Command
{
    bool special(UnitInfo& unit)
    {
        Position p(unit.getEngagePosition());

        auto canAffordMorph = [&](UnitType type) {
            if (Broodwar->self()->minerals() > type.mineralPrice() && Broodwar->self()->gas() > type.gasPrice() && Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed() > type.supplyRequired())
                return true;
            return false;
        };

        // Battlecruisers - TODO, check for overlap commands on targets instead
        if (unit.getType() == UnitTypes::Terran_Battlecruiser) {
            if ((unit.unit()->getOrder() == Orders::FireYamatoGun || (Broodwar->self()->hasResearched(TechTypes::Yamato_Gun) && unit.unit()->getEnergy() >= TechTypes::Yamato_Gun.energyCost()) && unit.getTarget().unit()->getHitPoints() >= 80) && unit.hasTarget() && unit.getTarget().unit()->exists()) {
                if ((unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()))
                    unit.unit()->useTech(TechTypes::Yamato_Gun, unit.getTarget().unit());

                addCommand(unit.unit(), unit.getTarget().getPosition(), TechTypes::Yamato_Gun);
                return true;
            }
        }

        // Ghosts
        else if (unit.getType() == UnitTypes::Terran_Ghost) {
            if (!unit.unit()->isCloaked() && unit.unit()->getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Command::overlapsEnemyDetection(p))
                unit.unit()->useTech(TechTypes::Personnel_Cloaking);

            if (Buildings::getNukesAvailable() > 0 && unit.hasTarget() && unit.getTarget().getWalkPosition().isValid() && unit.unit()->isCloaked() && Grids::getEAirCluster(unit.getTarget().getWalkPosition()) + Grids::getEGroundCluster(unit.getTarget().getWalkPosition()) > 5.0 && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 320 && unit.getPosition().getDistance(unit.getTarget().getPosition()) > 200) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Unit || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()) {
                    unit.unit()->useTech(TechTypes::Nuclear_Strike, unit.getTarget().unit());
                    addCommand(unit.unit(), unit.getTarget().getPosition(), TechTypes::Nuclear_Strike);
                    return true;
                }
            }
            if (unit.unit()->getOrder() == Orders::NukePaint || unit.unit()->getOrder() == Orders::NukeTrack || unit.unit()->getOrder() == Orders::CastNuclearStrike) {
                addCommand(unit.unit(), unit.unit()->getOrderTargetPosition(), TechTypes::Nuclear_Strike);
                return true;
            }
        }

        // Marine/Firebat
        else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && (unit.getType() == UnitTypes::Terran_Marine || unit.getType() == UnitTypes::Terran_Firebat) && !unit.unit()->isStimmed() && unit.hasTarget() && unit.getTarget().getPosition().isValid() && unit.unit()->getDistance(unit.getTarget().getPosition()) <= unit.getGroundRange())
            unit.unit()->useTech(TechTypes::Stim_Packs);

        // SCV
        else if (unit.getType() == UnitTypes::Terran_SCV) {
            //UnitInfo* mech = Util::getClosestUnit(unit, Filter::IsMechanical && Filter::HP_Percent < 100);
            //if (!Strategy::enemyRush() && mech && mech->unit() && unit.getPosition().getDistance(mech->getPosition()) <= 320 && Grids::getMobility(mech->getWalkPosition()) > 0) {
            //	if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != mech->unit())
            //		unit.unit()->repair(mech->unit());
            //	return true;
            //}

            //UnitInfo* building = Util::getClosestUnit(unit, Filter::GetPlayer == Broodwar->self() && Filter::IsCompleted && Filter::HP_Percent < 100);
            //if (building && building->unit() && (!Strategy::enemyRush() || building->getType() == UnitTypes::Terran_Bunker)) {
            //	if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != building->unit())
            //		unit.unit()->repair(building->unit());
            //	return true;
            //}
        }

        // Siege Tanks
        else if (unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) {
            if (unit.getPosition().getDistance(unit.getEngagePosition()) < 32.0 && unit.getLocalState() == LocalState::Engaging)
                unit.unit()->siege();
            if (unit.getGlobalState() == GlobalState::Retreating && unit.getPosition().getDistance(Terrain::getDefendPosition()) < 320)
                unit.unit()->siege();
        }
        else if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode) {
            if (unit.getPosition().getDistance(unit.getEngagePosition()) > 128.0 || unit.getLocalState() == LocalState::Retreating) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Unsiege)
                    unit.unit()->unsiege();
                return true;
            }
        }

        // Vultures
        else if (unit.getType() == UnitTypes::Terran_Vulture && Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && unit.unit()->getSpiderMineCount() > 0 && unit.getPosition().getDistance(unit.getSimPosition()) <= 400 && Broodwar->getUnitsInRadius(unit.getPosition(), 4, Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine).size() <= 0) {
            if (unit.unit()->getLastCommand().getTechType() != TechTypes::Spider_Mines || unit.unit()->getLastCommand().getTargetPosition().getDistance(unit.getPosition()) > 8)
                unit.unit()->useTech(TechTypes::Spider_Mines, unit.getPosition());
            return true;
        }

        // Wraiths	
        else if (unit.getType() == UnitTypes::Terran_Wraith) {
            if (unit.getHealth() >= 120 && !unit.unit()->isCloaked() && unit.unit()->getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Command::overlapsEnemyDetection(p))
                unit.unit()->useTech(TechTypes::Cloaking_Field);
            else if (unit.getHealth() <= 90 && unit.unit()->isCloaked())
                unit.unit()->useTech(TechTypes::Cloaking_Field);
        }

        // Carriers
        else if (unit.getType() == UnitTypes::Protoss_Carrier && unit.unit()->getInterceptorCount() < MAX_INTERCEPTOR && !unit.unit()->isTraining()) {
            unit.unit()->train(UnitTypes::Protoss_Interceptor);
            return false;
        }

        // Corsairs
        else if (unit.getType() == UnitTypes::Protoss_Corsair && unit.unit()->getEnergy() >= TechTypes::Disruption_Web.energyCost() && Broodwar->self()->hasResearched(TechTypes::Disruption_Web)) {
            auto distBest = DBL_MAX;
            auto posBest = Positions::Invalid;

            // Find an enemy that is attacking and is slow
            for (auto &e : Units::getEnemyUnits()) {
                auto &enemy = e.second;
                auto dist = enemy.getPosition().getDistance(unit.getPosition());

                if (dist < distBest && dist < 256.0 && !overlapsCommands(unit.unit(), TechTypes::Disruption_Web, enemy.getPosition(), 96) && enemy.unit()->isAttacking() && enemy.getSpeed() <= UnitTypes::Protoss_Reaver.topSpeed()) {
                    distBest = dist;
                    posBest = enemy.getPosition();
                }
            }
            if (posBest.isValid()) {
                addCommand(unit.unit(), posBest, TechTypes::Disruption_Web);
                unit.unit()->useTech(TechTypes::Disruption_Web, posBest);
                return true;
            }
        }

        // High Templars
        else if (unit.getType() == UnitTypes::Protoss_High_Templar) {

            // TEST - Check if the order of the HT is storming to add as a command
            if (unit.unit()->getOrder() == Orders::CastPsionicStorm && unit.hasTarget())
                addCommand(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm);

            // If close to target and can cast a storm
            if (unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && !Command::overlapsCommands(unit.unit(), TechTypes::Psionic_Storm, unit.getTarget().getPosition(), 96) && unit.unit()->getEnergy() >= 75 && (Grids::getEGroundCluster(unit.getTarget().getWalkPosition()) + Grids::getEAirCluster(unit.getTarget().getWalkPosition())) >= STORM_LIMIT) {
                unit.unit()->useTech(TechTypes::Psionic_Storm, unit.getTarget().unit());
                addCommand(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm);
                return true;
            }

            // If unit has low energy and is threatened or we want more archons
            else {
                auto lowEnergyThreat = unit.getEnergy() < TechTypes::Psionic_Storm.energyCost() && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0;
                auto wantArchons = Strategy::getUnitScore(UnitTypes::Protoss_Archon) > Strategy::getUnitScore(UnitTypes::Protoss_High_Templar);

                if (!Players::vT() && (lowEnergyThreat || wantArchons)) {

                    // Try to find a friendly templar who is low energy and is threatened
                    auto templar = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u.getType() == UnitTypes::Protoss_High_Templar && (wantArchons || (u.getEnergy() < 75 && Grids::getEGroundThreat(templar->getWalkPosition()) > 0.0));
                    });

                    if (templar) {
                        if (templar->unit()->getLastCommand().getTechType() != TechTypes::Archon_Warp && unit.unit()->getLastCommand().getTechType() != TechTypes::Archon_Warp)
                            unit.unit()->useTech(TechTypes::Archon_Warp, templar->unit());
                        return true;
                    }
                }
            }
        }

        // Reavers
        else if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && unit.unit()->getScarabCount() < MAX_SCARAB && !unit.unit()->isTraining()) {
            unit.unit()->train(UnitTypes::Protoss_Scarab);
            unit.setLastAttackFrame(Broodwar->getFrameCount());	/// Use this to fudge whether a Reaver has actually shot when using shuttles due to cooldown reset
            return false;
        }

        // Hydralisks
        else if (unit.getType() == UnitTypes::Zerg_Hydralisk && Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect)) {
            if (Strategy::getUnitScore(UnitTypes::Zerg_Lurker) > Strategy::getUnitScore(UnitTypes::Zerg_Hydralisk) && canAffordMorph(UnitTypes::Zerg_Lurker) && unit.getPosition().getDistance(unit.getSimPosition()) > 640.0) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Morph)
                    unit.unit()->morph(UnitTypes::Zerg_Lurker);
                return true;
            }
        }

        // Lurkers
        else if (unit.getType() == UnitTypes::Zerg_Lurker) {
            if (unit.getDestination().isValid()) {
                if (!unit.unit()->isBurrowed() && unit.getPosition().getDistance(unit.getDestination()) < 64.0) {
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

        // Mutalisks
        else if (unit.getType() == UnitTypes::Zerg_Mutalisk && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Greater_Spire) > 0) {
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

        // General: Shield Battery Use
        if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shield_Battery) > 0 && (unit.unit()->getGroundWeaponCooldown() > Broodwar->getRemainingLatencyFrames() || unit.unit()->getAirWeaponCooldown() > Broodwar->getRemainingLatencyFrames()) && unit.getType().maxShields() > 0 && (unit.unit()->getShields() <= 10 || (unit.unit()->getShields() < unit.getType().maxShields() && unit.unit()->getOrderTarget() && unit.unit()->getOrderTarget()->exists() && unit.unit()->getOrderTarget()->getType() == UnitTypes::Protoss_Shield_Battery && unit.unit()->getOrderTarget()->getEnergy() >= 10))) {
            auto battery = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return u.getType() == UnitTypes::Protoss_Shield_Battery && u.unit()->isCompleted() && u.getEnergy() > 10;
            });

            if (battery && ((unit.getType().isFlyer() && (!unit.hasTarget() || (unit.getTarget().getPosition().getDistance(unit.getPosition()) >= 320))) || unit.unit()->getDistance(battery->getPosition()) < 320)) {
                if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit || unit.unit()->getLastCommand().getTarget() != battery->unit())
                    unit.unit()->rightClick(battery->unit());
                return true;
            }
        }

        // General: Bunker Loading/Unloading
        //if (unit.getType() == UnitTypes::Terran_Marine && unit.getGlobalStrategy() == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) > 0) {
        //	//UnitInfo* bunker = Util::getClosestAllyBuilding(unit, Filter::GetType == UnitTypes::Terran_Bunker && Filter::SpaceRemaining > 0);
        //	//if (bunker && bunker->unit() && unit.hasTarget()) {
        //	//	if (unit.getTarget().unit()->exists() && unit.getTarget().getPosition().getDistance(unit.getPosition()) <= 320) {
        //	//		unit.unit()->rightClick(bunker->unit());
        //	//		return true;
        //	//	}
        //	//	if (unit.unit()->isLoaded() && unit.getTarget().getPosition().getDistance(unit.getPosition()) > 320)
        //	//		bunker->unit()->unloadAll();
        //	//}
        //}

        // Science Vessels
        if (unit.getType() == UnitTypes::Terran_Science_Vessel && unit.unit()->getEnergy() >= TechTypes::Defensive_Matrix) {
            //UnitInfo* ally = Util::getClosestAllyUnit(unit, Filter::IsUnderAttack);
            //if (ally && ally->getPosition().getDistance(unit.getPosition()) < 640)
            //	unit.unit()->useTech(TechTypes::Defensive_Matrix, ally->unit());
            return true;
        }

        return false;
    }
}