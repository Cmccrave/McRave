#include "UnitInfo.h"
#include "McRave.h"

namespace McRave
{
	CombatUnit::CombatUnit(UnitInfo * newUnit) {
		// Store UnitInfo pointer
		unitInfo = newUnit;

		// Initialize the rest
		visibleGroundStrength = 0.0;
		visibleAirStrength = 0.0;
		maxGroundStrength = 0.0;
		maxAirStrength = 0.0;
		priority = 0.0;
		simBonus = 1.0;

		assignedTarget = nullptr;
		assignedTransport = nullptr;

		visibleGroundStrength	= Util().getVisibleGroundStrength(*unitInfo);
		maxGroundStrength		= Util().getMaxGroundStrength(*unitInfo);
		visibleAirStrength		= Util().getVisibleAirStrength(*unitInfo);
		maxAirStrength			= Util().getMaxAirStrength(*unitInfo);
		priority				= Util().getPriority(*unitInfo);
	}

	void CombatUnit::updateTarget()
	{
		// Update my targets
		if (info()->getPlayer() && info()->getPlayer() == Broodwar->self()) {
			if (info()->getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
				auto mineTarget = info()->unit()->getOrderTarget();

				if (Units().getEnemyUnits().find(mineTarget) != Units().getEnemyUnits().end())
					assignedTarget = mineTarget != nullptr ? &Units().getEnemyUnits()[mineTarget] : nullptr;
				else
					assignedTarget = nullptr;
			}
			else
				Targets().getTarget(*unitInfo);
		}

		// Assume enemy targets
		else if (info()->getPlayer() && info()->getPlayer()->isEnemy(Broodwar->self())) {

			if (info()->getType() == UnitTypes::Terran_Vulture_Spider_Mine && unitInfo->unit()->getOrderTarget() && unitInfo->unit()->getOrderTarget()->getPlayer() == Broodwar->self())
				assignedTarget = &Units().getMyUnits()[unitInfo->unit()->getOrderTarget()];
			else if (info()->getType() != UnitTypes::Terran_Vulture_Spider_Mine && unitInfo->unit()->getOrderTarget() && Units().getMyUnits().find(unitInfo->unit()->getOrderTarget()) != Units().getMyUnits().end())
				assignedTarget = &Units().getMyUnits()[unitInfo->unit()->getOrderTarget()];
			else
				assignedTarget = nullptr;
		}
	}

	UnitInfo::UnitInfo()
	{
		percentHealth = 0.0;
		groundRange = 0.0;
		airRange = 0.0;
		groundDamage = 0.0;
		airDamage = 0.0;
		speed = 0.0;

		localStrategy = 0;
		globalStrategy = 0;
		lastAttackFrame = 0;
		lastVisibleFrame = 0;
		lastMoveFrame = 0;
		shields = 0;
		health = 0;
		minStopFrame = 0;
		killCount = 0;		

		burrowed = false;
		thisUnit = nullptr;	
		player = nullptr;		
		unitType = UnitTypes::None;

		position = Positions::Invalid;
		engagePosition = Positions::Invalid;
		destination = Positions::Invalid;
		simPosition = Positions::Invalid;
		walkPosition = WalkPositions::Invalid;
		tilePosition = TilePositions::Invalid;
	}

	void UnitInfo::setLastPositions()
	{
		if (!this->unit() || !this->unit()->exists())
			return;

		lastPos = this->getPosition();
		lastTile = this->getTilePosition();
		//lastWalk =  this->unit()->getWalkPosition();
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
		walkPosition			= Util().getWalkPosition(thisUnit);

		// Update unit stats
		unitType				= t;
		player					= p;
		health					= thisUnit->getHitPoints();
		shields					= thisUnit->getShields();
		percentHealth			= Util().getPercentHealth(*this);
		groundRange				= Util().groundRange(*this);
		airRange				= Util().airRange(*this);
		groundDamage			= Util().groundDamage(*this);
		airDamage				= Util().airDamage(*this);
		speed 					= Util().speed(*this);
		minStopFrame			= Util().getMinStopFrame(t);
		burrowed				= (thisUnit->isBurrowed() || thisUnit->getOrder() == Orders::Burrowing || thisUnit->getOrder() == Orders::VultureMine);

		// Update McRave stats
		lastAttackFrame			= (t != UnitTypes::Protoss_Reaver && (thisUnit->isStartingAttack() || thisUnit->isRepairing())) ? Broodwar->getFrameCount() : lastAttackFrame;
		killCount				= unit()->getKillCount();
		
		this->resetForces();		
		this->updateStuckCheck();
	}

	void UnitInfo::updateStuckCheck() {
		if (player != Broodwar->self() || lastPos != position || !thisUnit->isMoving() || thisUnit->getLastCommand().getType() == UnitCommandTypes::Stop || lastAttackFrame == Broodwar->getFrameCount())
			lastMoveFrame = Broodwar->getFrameCount();
	}

	void UnitInfo::createDummy(UnitType t) {
		unitType				= t;
		player					= Broodwar->self();
		groundRange				= Util().groundRange(*this);
		airRange				= Util().airRange(*this);
		groundDamage			= Util().groundDamage(*this);
		airDamage				= Util().airDamage(*this);
		speed 					= Util().speed(*this);
	}
}