#include "McRave.h"

using namespace std;
using namespace BWAPI;

namespace McRave
{
    UnitInfo::UnitInfo() {}

    void UnitInfo::setLastPositions()
    {
        lastPos = position;
        lastTile = tilePosition;
        lastWalk =  walkPosition;
    }

    void UnitInfo::verifyPaths()
    {
        if (lastTile != this->unit()->getTilePosition()) {
            BWEB::Path emptyPath;
            this->setRetreatPath(emptyPath);
            this->setAttackPath(emptyPath);
        }
    }

    void UnitInfo::update()
    {
        auto t = thisUnit->getType();
        auto p = thisUnit->getPlayer();

        if (thisUnit->exists()) {

            setLastPositions();
            verifyPaths();

            // Update unit positions		
            position				= thisUnit->getPosition();
            destination				= Positions::Invalid;
            goal                    = Positions::Invalid;
            tilePosition			= unit()->getTilePosition();
            walkPosition			= Math::getWalkPosition(thisUnit);

            // Update unit stats
            unitType				= t;
            player					= p;
            health					= thisUnit->getHitPoints() > 0 ? thisUnit->getHitPoints() : health;
            shields					= thisUnit->getShields() > 0 ? thisUnit->getShields() : shields;
            energy					= thisUnit->getEnergy();
            percentHealth			= t.maxHitPoints() > 0 ? double(health) / double(t.maxHitPoints()) : 0.0;
            percentShield			= t.maxShields() > 0 ? double(shields) / double(t.maxShields()) : 0.0;
            percentTotal			= t.maxHitPoints() + t.maxShields() > 0 ? double(health + shields) / double(t.maxHitPoints() + t.maxShields()) : 0.0;
            groundRange				= Math::groundRange(*this);
            groundDamage			= Math::groundDamage(*this);
            groundReach				= groundRange + (speed * 32.0) + double(unitType.width() / 2) + 64.0;
            airRange				= Math::airRange(*this);
            airReach				= airRange + (speed * 32.0) + double(unitType.width() / 2) + 64.0;
            airDamage				= Math::airDamage(*this);
            speed 					= Math::speed(*this);
            minStopFrame			= Math::stopAnimationFrames(t);
            burrowed				= (unitType != UnitTypes::Terran_Vulture_Spider_Mine && thisUnit->isBurrowed()) || thisUnit->getOrder() == Orders::Burrowing;
            flying					= thisUnit->isFlying() || thisUnit->getType().isFlyer() || thisUnit->getOrder() == Orders::LiftingOff || thisUnit->getOrder() == Orders::BuildingLiftOff;

            // Update McRave stats
            visibleGroundStrength	= Math::getVisibleGroundStrength(*this);
            maxGroundStrength		= Math::getMaxGroundStrength(*this);
            visibleAirStrength		= Math::getVisibleAirStrength(*this);
            maxAirStrength			= Math::getMaxAirStrength(*this);
            priority				= Math::getPriority(*this);
            lastAttackFrame			= (t != UnitTypes::Protoss_Reaver && (thisUnit->isStartingAttack() || thisUnit->isRepairing())) ? Broodwar->getFrameCount() : lastAttackFrame;

            // Reset states
            lState					= LocalState::None;
            gState					= GlobalState::None;
            tState					= TransportState::None;

            // Resource held frame
            if (thisUnit->isCarryingGas() || thisUnit->isCarryingMinerals())
                resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
            else if (thisUnit->isGatheringGas() || thisUnit->isGatheringMinerals())
                resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
            else
                resourceHeldFrames = 0;

            // Remaining train frame
            remainingTrainFrame = max(0, remainingTrainFrame - 1);

            // BWAPI won't reveal isStartingAttack when hold position is executed if the unit can't use hold position, XIMP uses this on workers
            if (player != Broodwar->self() && unitType.isWorker()) {
                if (thisUnit->getGroundWeaponCooldown() == 1 || thisUnit->getAirWeaponCooldown() == 1)
                    lastAttackFrame = Broodwar->getFrameCount();
            }

            target = std::weak_ptr<UnitInfo>();
            updateTarget();
            updateStuckCheck();
        }

        // If this is a spider mine and doesn't have a target, it's still considered burrowed
        if (unitType == UnitTypes::Terran_Vulture_Spider_Mine && (!thisUnit->exists() || (!target.lock() && thisUnit->getSecondaryOrder() == Orders::Cloak)))
            burrowed = true;
    }

    void UnitInfo::updateTarget()
    {
        // Update my target
        if (player && player == Broodwar->self()) {
            if (unitType == UnitTypes::Terran_Vulture_Spider_Mine) {
                if (thisUnit->getOrderTarget())
                    target = Units::getUnitInfo(thisUnit->getOrderTarget());
            }
            else
                Targets::getTarget(*this);
        }

        // Assume enemy target
        else if (player && player->isEnemy(Broodwar->self())) {
            if (thisUnit->getOrderTarget()) {
                auto &targetInfo = Units::getUnitInfo(thisUnit->getOrderTarget());
                if (targetInfo) {
                    target = targetInfo;
                    targetInfo->getTargetedBy().push_back(make_shared<UnitInfo>(*this));
                }
            }
            else
                target = weak_ptr<UnitInfo>();
        }
    }

    void UnitInfo::updateStuckCheck() {
        if (player != Broodwar->self() || lastPos != position || !thisUnit->isMoving() || thisUnit->getLastCommand().getType() == UnitCommandTypes::Stop || lastAttackFrame == Broodwar->getFrameCount())
            lastMoveFrame = Broodwar->getFrameCount();
    }

    void UnitInfo::createDummy(UnitType t) {
        unitType				= t;
        player					= Broodwar->self();
        groundRange				= Math::groundRange(*this);
        airRange				= Math::airRange(*this);
        groundDamage			= Math::groundDamage(*this);
        airDamage				= Math::airDamage(*this);
        speed 					= Math::speed(*this);
    }

    bool UnitInfo::command(UnitCommandType command, Position here, bool overshoot)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames();

        if (position.getDistance(here) < 64 && command == UnitCommandTypes::Move && role == Role::Combat)
            Command::addAction(thisUnit, here, unitType, PlayerState::Self);

        if (cancelAttackRisk)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto canIssue = Broodwar->getFrameCount() - thisUnit->getLastCommandFrame() > Broodwar->getLatencyFrames();
            auto newOrderPosition = thisUnit->getOrderTargetPosition().getDistance(here) > 96;
            return canIssue && newOrderPosition;
        };

        // Check if this is a new command
        const auto newCommand = [&]() {
            auto newCommandPosition = (thisUnit->getLastCommand().getType() != command || thisUnit->getLastCommand().getTargetPosition() != here);
            return newCommandPosition;
        };

        // Check if we should overshoot for halting distance
        if (overshoot) {
            auto distance = position.getApproxDistance(here);
            auto haltDistance = max({ distance, 32, unitType.haltDistance() / 256 });

            if (distance > 0) {
                here = position - ((position - here) * haltDistance / distance);
                here = Util::clipLine(position, here);
            }
        }

        // If this is a new order or new command than what we're requesting, we can issue it
        if (newOrder() || newCommand()) {
            if (command == UnitCommandTypes::Move)
                thisUnit->move(here);
            return true;
        }
        return false;
    }

    bool UnitInfo::command(UnitCommandType command, UnitInfo& targetUnit)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames();
        Command::addAction(thisUnit, targetUnit.getPosition(), unitType, PlayerState::Self);

        if (cancelAttackRisk)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto canIssue = Broodwar->getFrameCount() - thisUnit->getLastCommandFrame() > Broodwar->getLatencyFrames();
            auto newOrderTarget = thisUnit->getOrderTarget() != targetUnit.unit();
            return canIssue && newOrderTarget;
        };

        // Check if this is a new command
        const auto newCommand = [&]() {
            auto newCommandTarget = (thisUnit->getLastCommand().getType() != command || thisUnit->getLastCommand().getTarget() != targetUnit.unit());
            return newCommandTarget;
        };

        // If this is a new order or new command than what we're requesting, we can issue it
        if (newOrder() || newCommand()) {
            if (command == UnitCommandTypes::Attack_Unit)
                thisUnit->attack(targetUnit.unit());
            else if (command == UnitCommandTypes::Right_Click_Unit)
                thisUnit->rightClick(targetUnit.unit());
            return true;
        }
        return false;
    }

    bool UnitInfo::isThreatening()
    {
        if ((burrowed || (thisUnit && thisUnit->exists() && thisUnit->isCloaked())) && !Command::overlapsDetection(thisUnit, position, PlayerState::Self) || Stations::getMyStations().size() > 2)
            return false;

        auto temp = Terrain::isInAllyTerritory(tilePosition) || groundRange > 32.0 ? groundReach / 1.5 : groundReach / 5;

        // Define "close" - TODO: define better
        auto close = position.getDistance(Terrain::getDefendPosition()) < temp;
        auto atHome = Terrain::isInAllyTerritory(tilePosition);
        auto manner = position.getDistance(Terrain::getMineralHoldPosition()) < 256.0;
        auto exists = thisUnit && thisUnit->exists();
        auto attacked = exists && hasAttackedRecently() && target.lock() && (target.lock()->getType().isBuilding() || target.lock()->getType().isWorker()) && (close || atHome);
        auto constructingClose = exists && (position.getDistance(Terrain::getDefendPosition()) < 320.0 || close) && (thisUnit->isConstructing() || thisUnit->getOrder() == Orders::ConstructingBuilding || thisUnit->getOrder() == Orders::PlaceBuilding);
        auto inRangePieces = Terrain::inRangeOfWallPieces(*this);
        auto inRangeDefenses = Terrain::inRangeOfWallDefenses(*this);

        const auto threatening = [&] {
            // Building: blocking any buildings, is close or at home and can attack or is a battery, is a manner building
            if (unitType.isBuilding()) {
                if (Buildings::overlapsQueue(*this, tilePosition))
                    return true;
                if ((close || atHome) && (airDamage > 0.0 || groundDamage > 0.0 || unitType == UnitTypes::Protoss_Shield_Battery || unitType.isRefinery()))
                    return true;
                if (manner)
                    return true;
            }
            // Worker: constructing close, has attacked and close
            else if (unitType.isWorker()) {
                if (constructingClose)
                    return true;
                if (close || attacked)
                    return true;
            }
            // Unit: close, close to shield battery, in territory while defending choke
            else {
                if (Terrain::isDefendNatural()) {
                    if (BuildOrder::isWallNat() && inRangeDefenses)
                        return true;
                    if (!BuildOrder::isWallNat() && (close || attacked))
                        return true;
                }
                else {
                    if (close || attacked)
                        return true;
                }
                if (attacked && inRangePieces)
                    return true;
                if (atHome && Strategy::defendChoke())
                    return true;
                if (com(UnitTypes::Protoss_Shield_Battery) > 0) {
                    auto &battery = Util::getClosestUnit(position, PlayerState::Self, [&](auto &u) {
                        return u.getType() == UnitTypes::Protoss_Shield_Battery && u.unit()->isCompleted();
                    });
                    if (battery && position.getDistance(battery->getPosition()) <= 128.0 && Terrain::isInAllyTerritory(tilePosition))
                        return true;
                }
            }
            return false;
        };

        if (threatening()) {
            lastThreateningFrame = Broodwar->getFrameCount();
            return true;
        }
        return Broodwar->getFrameCount() - lastThreateningFrame < 50;
    }

    bool UnitInfo::canStartAttack()
    {
        if (!target.lock()
            || (groundDamage == 0 && airDamage == 0)
            || isSpellcaster())
            return false;        

        auto attackAnimation = 0;
        
        if (!unitType.isFlyer() && !isHovering())
            attackAnimation = lastPos != position ? Math::firstAttackAnimationFrames(unitType) : Math::contAttackAnimationFrames(unitType);

        auto cooldown = (target.lock()->getType().isFlyer() ? thisUnit->getAirWeaponCooldown() : thisUnit->getGroundWeaponCooldown()) - Broodwar->getLatencyFrames() - attackAnimation;

        if (unitType == UnitTypes::Protoss_Reaver)
            cooldown = lastAttackFrame - Broodwar->getFrameCount() + 60;

        auto cooldownReady = cooldown <= engageDist / (transport.lock() ? transport.lock()->getSpeed() : speed);
        return cooldownReady;
    }

    bool UnitInfo::canStartCast(TechType tech)
    {
        if (!target.lock()
            || Command::overlapsActions(thisUnit, target.lock()->getPosition(), tech, PlayerState::Self, Util::getCastRadius(tech)))
            return false;

        auto energyNeeded = tech.energyCost() - energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady = energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize < engageDist / (transport.lock() ? transport.lock()->getSpeed() : speed);


        if (!spellReady && !spellWillBeReady)
            return false;

        if (engageDist >= 360.0)
            return true;

        if (auto currentTarget = target.lock()) {
            auto ground = Grids::getEGroundCluster(currentTarget->getPosition());
            auto air = Grids::getEAirCluster(currentTarget->getPosition());

            if (ground + air >= Util::getCastLimit(tech) || currentTarget->isHidden() || (currentTarget->hasTarget() && currentTarget->getTarget() == *this))
                return true;
        }
        return false;
    }

    bool UnitInfo::canAttackGround()
    {
        if (groundDamage > 0.0)
            return true;

        return unitType == UnitTypes::Protoss_High_Templar
            || unitType == UnitTypes::Protoss_Dark_Archon
            || unitType == UnitTypes::Protoss_Carrier
            || unitType == UnitTypes::Terran_Medic
            || unitType == UnitTypes::Terran_Science_Vessel
            || unitType == UnitTypes::Zerg_Defiler
            || unitType == UnitTypes::Zerg_Queen;
    }

    bool UnitInfo::canAttackAir()
    {
        if (airDamage > 0.0)
            return true;

        return unitType == UnitTypes::Protoss_High_Templar
            || unitType == UnitTypes::Protoss_Dark_Archon
            || unitType == UnitTypes::Protoss_Carrier
            || unitType == UnitTypes::Terran_Science_Vessel
            || unitType == UnitTypes::Zerg_Defiler
            || unitType == UnitTypes::Zerg_Queen;
    }

    bool UnitInfo::isWithinReach(UnitInfo& unit)
    {
        auto sizes = (max(unit.getType().width(), unit.getType().height()) + max(unitType.width(), unitType.height())) / 2;
        auto dist = position.getDistance(unit.getPosition()) - sizes - 32.0;
        return unit.getType().isFlyer() ? airReach >= dist : groundReach >= dist;
    }

    bool UnitInfo::isWithinRange(UnitInfo& unit)
    {
        auto sizes = (max(unit.getType().width(), unit.getType().height()) + max(unitType.width(), unitType.height())) / 2;
        auto dist = position.getDistance(unit.getPosition()) - sizes - 32.0;
        return unit.getType().isFlyer() ? airRange >= dist : groundRange >= dist;
    }
}