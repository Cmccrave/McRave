#include "UnitInfo.h"
#include "Util.h"
#include "UnitManager.h"
#include "TargetManager.h"
#include "UnitMath.h"

// DEAR GOD PLEASE FUCKING REMOVE THIS WHY DID I DO THIS
#define Targets TargetSingleton::Instance()
#define Units UnitSingleton::Instance()

namespace McRave
{
	UnitInfo::UnitInfo()
	{
		visibleGroundStrength = 0.0;
		visibleAirStrength = 0.0;
		maxGroundStrength = 0.0;
		maxAirStrength = 0.0;
		priority = 0.0;

		percentHealth = 0.0;
		percentShield = 0.0;
		percentTotal = 0.0;
		groundRange = 0.0;
		airRange = 0.0;
		groundDamage = 0.0;
		airDamage = 0.0;
		speed = 0.0;
		engageDist = 0.0;
		simValue = 0.0;
		simBonus = 1.0;

		lastAttackFrame = 0;
		lastVisibleFrame = 0;
		lastMoveFrame = 0;
		resourceHeldFrames = 0;
		remainingTrainFrame = 0;
		shields = 0;
		health = 0;
		minStopFrame = 0;
		energy = 0;

		killCount = 0;


		burrowed = false;

		thisUnit = nullptr;
		transport = nullptr;
		unitType = UnitTypes::None;
		buildingType = UnitTypes::None;
		player = nullptr;
		target = nullptr;
		resource = nullptr;
		role = Role::None;

		// Default to retreating
		tState = TransportState::None;
		lState = LocalState::None;
		gState = GlobalState::None;
		sState = SimState::None;

		position = Positions::Invalid;
		engagePosition = Positions::Invalid;
		destination = Positions::Invalid;
		simPosition = Positions::Invalid;
		lastPos = Positions::Invalid;
		walkPosition = WalkPositions::Invalid;
		lastWalk = WalkPositions::Invalid;
		tilePosition = TilePositions::Invalid;
		buildPosition = TilePositions::Invalid;
		lastTile = TilePositions::Invalid;
	}

	void UnitInfo::setLastPositions()
	{
		lastPos = this->getPosition();
		lastTile = this->getTilePosition();
		lastWalk =  this->getWalkPosition();
	}

	void UnitInfo::updateUnit()
	{
		auto t = this->unit()->getType();
		auto p = this->unit()->getPlayer();

		this->setLastPositions();

		// Update unit positions		
		position				= thisUnit->getPosition();
		destination				= Positions::None;
		tilePosition			= unit()->getTilePosition();
		walkPosition			= Math::getWalkPosition(thisUnit);

		// Update unit stats
		unitType				= t;
		player					= p;
		health					= thisUnit->getHitPoints();
		shields					= thisUnit->getShields();
		energy					= thisUnit->getEnergy();
		percentHealth			= t.maxHitPoints() > 0 ? double(health) / double(t.maxHitPoints()) : 1.0;
		percentShield			= t.maxShields() > 0 ? double(shields) / double(t.maxShields()) : 1.0;
		percentTotal			= t.maxHitPoints() + t.maxShields() > 0 ? double(health + shields) / double(t.maxHitPoints() + t.maxShields()) : 1.0;
		groundRange				= Math::groundRange(*this);
		airRange				= Math::airRange(*this);
		groundDamage			= Math::groundDamage(*this);
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
		simBonus				= 1.0;
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

		this->updateTarget();
		this->updateStuckCheck();
	}

	void UnitInfo::updateTarget()
	{
		// Update my targets
		if (player && player == Broodwar->self()) {
			if (unitType == UnitTypes::Terran_Vulture_Spider_Mine) {
				auto mineTarget = unit()->getOrderTarget();

				if (Units.getEnemyUnits().find(mineTarget) != Units.getEnemyUnits().end())
					target = mineTarget != nullptr ? &Units.getEnemyUnits()[mineTarget] : nullptr;
				else
					target = nullptr;
			}
			else
				Targets.getTarget(*this);
		}

		// Assume enemy targets
		else if (player && player->isEnemy(Broodwar->self())) {

			if (unitType == UnitTypes::Terran_Vulture_Spider_Mine && Units.getMyUnits().find(thisUnit->getOrderTarget()) != Units.getMyUnits().end())
				target = &Units.getMyUnits()[thisUnit->getOrderTarget()];
			else if (unitType != UnitTypes::Terran_Vulture_Spider_Mine && thisUnit->getOrderTarget() && Units.getMyUnits().find(thisUnit->getOrderTarget()) != Units.getMyUnits().end())
				target = &Units.getMyUnits()[thisUnit->getOrderTarget()];
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

	bool UnitInfo::command(UnitCommandType command, Position here)
	{
		// Check if we need to wait a few frames before issuing a command due to stop frames or latency frames
		bool attackCooldown = Broodwar->getFrameCount() - lastAttackFrame <= minStopFrame - Broodwar->getLatencyFrames();
		bool latencyCooldown =	Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0;

		if (attackCooldown || latencyCooldown)
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
		if (command == UnitCommandTypes::Move) {
			double distance = position.getDistance(here);
			double distExtra = max(distance, double(unitType.haltDistance()) / 256.0);
			if (here.getDistance(position) < distExtra) {
				here = position - (position - here) * (distExtra / distance);
				here = Util().clipPosition(position, here);
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

	bool UnitInfo::command(UnitCommandType command, UnitInfo* targetUnit)
	{
		// Check if we need to wait a few frames before issuing a command due to stop frames or latency frames
		bool attackCooldown = Broodwar->getFrameCount() - lastAttackFrame <= minStopFrame - Broodwar->getLatencyFrames();
		bool latencyCooldown =	Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0;

		if (attackCooldown || latencyCooldown)
			return false;

		// Check if this is a new order
		const auto newOrder = [&]() {
			auto canIssue = Broodwar->getFrameCount() - thisUnit->getLastCommandFrame() > Broodwar->getRemainingLatencyFrames();
			auto newOrderTarget = thisUnit->getOrderTarget() != targetUnit->unit();
			return canIssue && newOrderTarget;
		};

		// Check if this is a new command
		const auto newCommand = [&]() {
			auto newCommandTarget = (thisUnit->getLastCommand().getType() != command || thisUnit->getLastCommand().getTarget() != targetUnit->unit());
			return newCommandTarget;
		};

		// If this is a new order or new command than what we're requesting, we can issue it
		if (newOrder() || newCommand()) {
			if (command == UnitCommandTypes::Attack_Unit)
				thisUnit->attack(targetUnit->unit());
			else if (command == UnitCommandTypes::Right_Click_Unit)
				thisUnit->rightClick(targetUnit->unit());
			return true;
		}
		return false;
	}
}