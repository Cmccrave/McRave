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
        health					= thisUnit->getHitPoints();
        shields					= thisUnit->getShields();
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
        beingAttackedCount		= 0;

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
            if (thisUnit->getOrderTarget())
                target = Units::getUnit(thisUnit->getOrderTarget());
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
        bool attackCooldown = Broodwar->getFrameCount() - lastAttackFrame <= minStopFrame - Broodwar->getRemainingLatencyFrames();

        if (attackCooldown)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto canIssue = Broodwar->getFrameCount() - thisUnit->getLastCommandFrame() > Broodwar->getRemainingLatencyFrames();
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
        bool attackCooldown = Broodwar->getFrameCount() - lastAttackFrame <= minStopFrame - Broodwar->getRemainingLatencyFrames();

        if (attackCooldown)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto canIssue = Broodwar->getFrameCount() - thisUnit->getLastCommandFrame() > Broodwar->getRemainingLatencyFrames();
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
        if ((burrowed || (thisUnit && thisUnit->exists() && thisUnit->isCloaked())) && !Command::overlapsAllyDetection(position))
            return false;

        // Define "close" - TODO: define better
        auto close = position.getDistance(Terrain::getDefendPosition()) < groundRange || position.getDistance(Terrain::getDefendPosition()) < airRange;
        auto atHome = Terrain::isInAllyTerritory(tilePosition);
        auto manner = position.getDistance(Terrain::getMineralHoldPosition()) < 256.0;
        auto exists = thisUnit && thisUnit->exists();
        auto attacked = exists && hasAttackedRecently() && target && (target->getType().isBuilding() || target->getType().isWorker());
        auto constructingClose = exists && (position.getDistance(Terrain::getDefendPosition()) < 320.0 || close) && (thisUnit->isConstructing() || thisUnit->getOrder() == Orders::ConstructingBuilding || thisUnit->getOrder() == Orders::PlaceBuilding);
        auto inRange = Terrain::inRangeOfWall(*this);

        // Situations where a unit should be attacked:
        // 1) Building
        //	- Blocking any of my building spots or expansions
        //	- Has damage and is "close" or "atHome"
        //	- Shield battery and is "close" or "atHome"
        //	- Manner pylon

        if (unitType.isBuilding()) {
            if (Buildings::overlapsQueue(unitType, tilePosition))
                return true;
            if ((close || atHome) && (airDamage > 0.0 || groundDamage > 0.0 || unitType == UnitTypes::Protoss_Shield_Battery))
                return true;
            if (manner)
                return true;
        }

        // 2) Worker
        // - SCV building or repairing something "close"
        // - In my territory

        else if (unitType.isWorker()) {
            if (constructingClose)
                return true;
            if (close || attacked)
                return true;
            if (atHome && Strategy::defendChoke())
                return true;
        }

        // 3) Unit
        // - "close"
        // - Near my shield battery

        else {
            if (close || attacked || inRange)
                return true;
            if (atHome && Strategy::defendChoke())
                return true;
            if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shield_Battery) > 0) {
                auto battery = Util::getClosestUnit(position, PlayerState::Self, [&](auto &u) {
                    return u.getType() == UnitTypes::Protoss_Shield_Battery && u.unit()->isCompleted();
                });
                if (battery && position.getDistance(battery->getPosition()) <= 128.0)
                    return true;
            }
        }
        return false;
    }
}