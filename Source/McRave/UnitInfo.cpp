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

    void UnitInfo::updateUnit()
    {
        auto t = thisUnit->getType();
        auto p = thisUnit->getPlayer();

        setLastPositions();

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
        minStopFrame			= Math::getMinStopFrame(t);
        burrowed				= thisUnit->isBurrowed() || thisUnit->getOrder() == Orders::Burrowing || thisUnit->getOrder() == Orders::VultureMine;
        flying					= thisUnit->isFlying() || thisUnit->getType().isFlyer() || thisUnit->getOrder() == Orders::LiftingOff || thisUnit->getOrder() == Orders::BuildingLiftOff;

        // Update McRave stats
        visibleGroundStrength	= Math::getVisibleGroundStrength(*this);
        maxGroundStrength		= Math::getMaxGroundStrength(*this);
        visibleAirStrength		= Math::getVisibleAirStrength(*this);
        maxAirStrength			= Math::getMaxAirStrength(*this);
        priority				= Math::getPriority(*this);
        lastAttackFrame			= (t != UnitTypes::Protoss_Reaver && (thisUnit->isStartingAttack() || thisUnit->isRepairing())) ? Broodwar->getFrameCount() : lastAttackFrame;
        killCount				= unit()->getKillCount();

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

        // HACK: BWAPI for some reason doesn't like isStartingAttack or isAttackFrame???
        if (player != Broodwar->self()) {

            // These both do nothing
            /*if (thisUnit->isAttackFrame()) {
                lastAttackFrame = Broodwar->getFrameCount();
                circleBlue();
            }
            if (thisUnit->getOrder() == Orders::AttackUnit)
                circleOrange();*/

            if (Util::unitInRange(*this) && thisUnit->getOrder() == Orders::AttackUnit)
                lastAttackFrame = Broodwar->getFrameCount();
        }

        target = nullptr;
        updateTarget();
        updateStuckCheck();
    }

    void UnitInfo::updateTarget()
    {
        // Update my target
        if (player && player == Broodwar->self()) {
            if (unitType == UnitTypes::Terran_Vulture_Spider_Mine) {
                if (thisUnit->getOrderTarget())
                    target = Units::getUnit(thisUnit->getOrderTarget());
            }
            else
                Targets::getTarget(*this);
        }

        // Assume enemy target
        else if (player && player->isEnemy(Broodwar->self())) {            
            if (thisUnit->getOrderTarget()) {
                auto &targetInfo = Units::getUnit(thisUnit->getOrderTarget());
                if (targetInfo) {
                    target = targetInfo;
                    targetInfo->getTargetedBy().insert(make_shared<UnitInfo>(*this));
                }
            }
            else
                target = nullptr;
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

    bool UnitInfo::command(BWAPI::UnitCommandType command, BWAPI::Position here, bool overshoot)
    {
		// Check if we need to wait a few frames before issuing a command due to stop frames
		int frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
		bool attackCooldown = frameSinceAttack <= minStopFrame - Broodwar->getRemainingLatencyFrames();

        if (attackCooldown)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto canIssue = Broodwar->getFrameCount() - thisUnit->getLastCommandFrame() > Broodwar->getLatencyFrames();
            auto newOrderPosition = thisUnit->getOrderTargetPosition() != here;
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
            auto distExtra = max(distance, unitType.haltDistance() / 256);
            if (distance > 0 && here.getDistance(position) < distExtra) {
                here = position - (position - here) * (distExtra / distance);
                here = Util::clipPosition(position, here);
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

    bool UnitInfo::command(BWAPI::UnitCommandType command, UnitInfo& targetUnit)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
		int frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        bool attackCooldown = frameSinceAttack <= minStopFrame - Broodwar->getRemainingLatencyFrames();

        if (attackCooldown)
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
        if ((burrowed || (thisUnit && thisUnit->exists() && thisUnit->isCloaked())) && !Command::overlapsAllyDetection(position) || Stations::getMyStations().size() > 2)
            return false;

        auto temp = groundRange > 32.0 ? groundReach / 2 : groundReach / 5;

        // Define "close" - TODO: define better
        auto close = position.getDistance(Terrain::getDefendPosition()) < temp;
        auto atHome = Terrain::isInAllyTerritory(tilePosition);
        auto manner = position.getDistance(Terrain::getMineralHoldPosition()) < 256.0;
        auto exists = thisUnit && thisUnit->exists();
        auto attacked = exists && hasAttackedRecently() && target && (target->getType().isBuilding() || target->getType().isWorker());
        auto constructingClose = exists && (position.getDistance(Terrain::getDefendPosition()) < 320.0 || close) && (thisUnit->isConstructing() || thisUnit->getOrder() == Orders::ConstructingBuilding || thisUnit->getOrder() == Orders::PlaceBuilding);
        auto inRangePieces = Terrain::inRangeOfWallPieces(*this);
        auto inRangeDefenses = Terrain::inRangeOfWallDefenses(*this);

		if (attacked)
			circlePurple();

        const auto threatening = [&] {
            // Building: blocking any buildings, is close or at home and can attack or is a battery, is a manner building
            if (unitType.isBuilding()) {
                if (Buildings::overlapsQueue(unitType, tilePosition))
                    return true;
                if ((close || atHome) && (airDamage > 0.0 || groundDamage > 0.0 || unitType == UnitTypes::Protoss_Shield_Battery))
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
                if (close || attacked || inRangeDefenses)
                    return true;
                if (attacked && inRangePieces)
                    return true;
                if (atHome && Strategy::defendChoke())
                    return true;
                if (com(UnitTypes::Protoss_Shield_Battery) > 0) {
                    auto battery = Util::getClosestUnit(position, PlayerState::Self, [&](auto &u) {
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

    bool UnitInfo::isHidden()
    {
        return (burrowed || (thisUnit->exists() && thisUnit->isCloaked())) && !thisUnit->isDetected();
    }

	bool UnitInfo::canStartAttack()
	{
		if (!target)
			return false;

		auto cooldown = target->getType().isFlyer() ? thisUnit->getAirWeaponCooldown() : thisUnit->getGroundWeaponCooldown();

        if (unitType == UnitTypes::Protoss_Reaver)
            cooldown = lastAttackFrame - Broodwar->getFrameCount() + 60;

		auto targetExists = target->unit()->exists();
		auto cooldownReady =  cooldown < Broodwar->getLatencyFrames();
		auto cooldownWillBeReady = cooldown < engageDist / (transport ? transport->getSpeed() : speed);
		return (cooldownReady || cooldownWillBeReady);
	}
}