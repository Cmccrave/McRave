#include "UnitInfo.h"

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
		groundRange = 0.0;
		airRange = 0.0;
		groundDamage = 0.0;
		airDamage = 0.0;
		speed = 0.0;

		localStrategy = 0;
		globalStrategy = 0;
		lastAttackFrame = 0;
		lastCommandFrame = 0;
		lastVisibleFrame = 0;
		shields = 0;
		health = 0;
		minStopFrame = 0;

		killCount = 0;

		burrowed = false;

		thisUnit = nullptr;
		transport = nullptr;
		unitType = UnitTypes::None;
		who = nullptr;
		target = nullptr;

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
}