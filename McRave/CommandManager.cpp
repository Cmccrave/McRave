#include "McRave.h"

void CommandTrackerClass::update()
{
	Display().startClock();
	updateAlliedUnits();
	Display().performanceTest(__FUNCTION__);
	return;
}

void CommandTrackerClass::updateAlliedUnits()
{
	for (auto &u : Units().getAllyUnits())
	{
		UnitInfo &unit = u.second;

		// Prevent crashes
		if (!unit.unit() || (unit.unit() && !unit.unit()->exists()))
		{
			continue;
		}

		// Support units and transports have their own commands
		if (unit.getType() == UnitTypes::Protoss_Observer || unit.getType() == UnitTypes::Protoss_Arbiter || unit.getType() == UnitTypes::Protoss_Shuttle)
		{
			continue;
		}

		// If the unit received a command already during this frame
		if (unit.getLastCommandFrame() >= Broodwar->getFrameCount())
		{
			continue;
		}

		// Ignore the unit if it is locked down, maelstrommed, stassised, or not completed
		if (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted()))
		{
			continue;
		}

		// If the unit is ready to perform an action after an attack (certain units have minimum frames after an attack before they can receive a new command)
		if (Broodwar->getFrameCount() - unit.getLastAttackFrame() > unit.getMinStopFrame() - Broodwar->getLatencyFrames())
		{
			// If under a storm, dark swarm or EMP
			if (Grids().getPsiStormGrid(unit.getWalkPosition()) > 0 || Grids().getEMPGrid(unit.getWalkPosition()) > 0)
			{
				flee(unit);
				continue;
			}

			// If globally behind
			if (Units().getGlobalStrategy() == 0 || Units().getGlobalStrategy() == 2)
			{
				// Check if we have a target
				if (unit.getTarget())
				{
					// If locally ahead, attack
					if (unit.getStrategy() == 1 && unit.getTarget()->exists())
					{
						attack(unit);
						continue;
					}
					// Else flee
					else if (unit.getStrategy() == 0)
					{
						flee(unit);
						continue;
					}
				}
				// Else defend
				defend(unit);
				continue;
			}

			// If globally ahead
			else if (Units().getGlobalStrategy() == 1)
			{
				// Check if we have a target
				if (unit.getTarget())
				{
					// If locally behind, flee
					if (unit.getStrategy() == 0 || unit.getStrategy() == 2)
					{
						flee(unit);
						continue;
					}
					// If target exists, attack
					else if (unit.getStrategy() == 1 && unit.getTarget()->exists())
					{
						attack(unit);
						continue;
					}
					// Else move unit
					else
					{
						move(unit);
						continue;
					}
				}
			}
			// Else move unit
			move(unit);
			continue;
		}
	}
	return;
}

void CommandTrackerClass::attack(UnitInfo& unit)
{
	// TEMP -- Set to false initially
	bool moveAway = false;
	bool moveTo = false;

	// Specific High Templar behavior
	if (unit.getType() == UnitTypes::Protoss_High_Templar)
	{
		if (unit.getTarget() && unit.getTarget()->exists() && unit.unit()->getEnergy() >= 75)
		{
			unit.unit()->useTech(TechTypes::Psionic_Storm, unit.getTarget());
			Grids().updatePsiStorm(unit.getTargetWalkPosition());
			return;
		}
	}

	// Specific Marine and Firebat behavior	
	if ((unit.getType() == UnitTypes::Terran_Marine || unit.getType() == UnitTypes::Terran_Firebat) && !unit.unit()->isStimmed() && unit.getTargetPosition().isValid() && unit.unit()->getDistance(unit.getTargetPosition()) <= unit.getGroundRange())
	{
		unit.unit()->useTech(TechTypes::Stim_Packs);
	}

	// Specific Medic behavior -- if removed, nulls get stored further down
	if (unit.getType() == UnitTypes::Terran_Medic)
	{
		if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Unit || unit.unit()->getLastCommand().getTarget() != unit.getTarget())
		{
			unit.unit()->useTech(TechTypes::Healing, unit.getTarget());
		}
		return;
	}

	// Specific Tank behavior
	if (unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
	{
		if (unit.unit()->getDistance(unit.getTargetPosition()) <= 400 && unit.unit()->getDistance(unit.getTargetPosition()) > 128)
		{
			unit.unit()->siege();
		}
	}
	if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
	{
		if (unit.unit()->getDistance(unit.getTargetPosition()) > 400 || unit.unit()->getDistance(unit.getTargetPosition()) < 128)
		{
			unit.unit()->unsiege();
		}
	}

	// If we can use a Shield Battery
	if (Grids().getBatteryGrid(unit.getTilePosition()) > 0 && ((unit.unit()->getLastCommand().getType() == UnitCommandTypes::Right_Click_Unit && unit.unit()->getShields() < 40) || unit.unit()->getShields() < 10))
	{
		if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit)
		{
			for (auto& b : Buildings().getMyBuildings())
			{
				BuildingInfo &building = b.second;
				if (building.getType() == UnitTypes::Protoss_Shield_Battery && building.getEnergy() >= 10 && unit.unit()->getDistance(building.getPosition()) < 320)
				{
					unit.unit()->rightClick(building.unit());
					continue;
				}
			}
		}
		return;
	}

	// If we can use a bunker
	if (Grids().getBunkerGrid(unit.getTilePosition()) > 0)
	{
		Unit bunker = unit.unit()->getClosestUnit(Filter::GetType == UnitTypes::Terran_Bunker && Filter::SpaceRemaining > 0);
		if (bunker)
		{
			unit.unit()->rightClick(bunker);
			return;
		}
	}

	// If kiting unnecessary, disable
	if (unit.getTarget()->getType().isBuilding() || unit.getType() == UnitTypes::Protoss_Corsair || unit.getType().isWorker())
	{
		moveAway = false;
	}

	// Reavers should always kite away from their target if it has lower range
	else if (unit.getType() == UnitTypes::Protoss_Reaver && Units().getEnemyUnits()[unit.getTarget()].getGroundRange() < unit.getGroundRange())
	{
		moveAway = true;
	}

	// If kiting is a good idea, enable
	else if ((unit.getGroundRange() > 32 && unit.unit()->isUnderAttack()) || (Units().getEnemyUnits()[unit.getTarget()].getGroundRange() <= unit.getGroundRange() && (unit.unit()->getDistance(unit.getTargetPosition()) <= unit.getGroundRange() - Units().getEnemyUnits()[unit.getTarget()].getGroundRange() && Units().getEnemyUnits()[unit.getTarget()].getGroundRange() > 0 && unit.getGroundRange() > 32 || unit.unit()->getHitPoints() < 40)))
	{
		moveAway = true;
	}

	// If approaching is necessary
	if (unit.getGroundRange() > 32 && unit.getGroundRange() < Units().getEnemyUnits()[unit.getTarget()].getGroundRange() && !Units().getEnemyUnits()[unit.getTarget()].getType().isBuilding() && unit.getSpeed() > Units().getEnemyUnits()[unit.getTarget()].getSpeed())
	{
		moveTo = true;
	}

	// If kite is true and weapon on cooldown, move
	if (!unit.getTarget()->getType().isFlyer() && unit.unit()->getGroundWeaponCooldown() > 0 || unit.getTarget()->getType().isFlyer() && unit.unit()->getAirWeaponCooldown() > 0 || (unit.getTarget()->exists() && (unit.getTarget()->isCloaked() || unit.getTarget()->isBurrowed()) && !unit.getTarget()->isDetected()))
	{
		if (moveTo)
		{
			approach(unit);
			return;
		}
		if (moveAway)
		{
			flee(unit);
			return;
		}
	}
	else if ((!unit.getTarget()->getType().isFlyer() && unit.unit()->getGroundWeaponCooldown() <= 0) || (unit.getTarget()->getType().isFlyer() && unit.unit()->getAirWeaponCooldown() <= 0))
	{
		// If unit receieved an attack command on the target already, don't give another order
		if (unit.unit()->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && unit.unit()->getLastCommand().getTarget() == unit.getTarget())
		{
			return;
		}
		else
		{
			unit.unit()->attack(unit.getTarget());
		}
		unit.setTargetPosition(Units().getEnemyUnits()[unit.getTarget()].getPosition());
	}
	return;
}

void CommandTrackerClass::move(UnitInfo& unit)
{
	// If it's a tank, make sure we're unsieged before moving - TODO: Check that target has velocity and > 512 or no velocity and < tank range
	if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
	{
		if (unit.unit()->getDistance(unit.getTargetPosition()) > 512 || unit.unit()->getDistance(unit.getTargetPosition()) < 128)
		{
			unit.unit()->unsiege();
			return;
		}
	}

	// If unit has a transport, move to it or load into it
	if (unit.getTransport() && unit.getTransport()->exists())
	{
		unit.unit()->rightClick(unit.getTransport());
		return;
	}

	// If target doesn't exist, move towards it
	if (unit.getTarget() && unit.getTargetPosition().isValid() && unit.getStrategy() != 3)
	{
		if (unit.unit()->getLastCommand().getTargetPosition() != unit.getTargetPosition())
		{
			unit.unit()->move(unit.getTargetPosition());
		}
		return;
	}

	// If no target, move to closest enemy base if there is any
	else if (Terrain().getEnemyBasePositions().size() > 0 && !unit.getType().isFlyer())
	{
		double closestD = 0.0;
		Position closestP;
		for (auto &base : Terrain().getEnemyBasePositions())
		{
			if (Grids().getEnemyArmyCenter().getDistance(base) > closestD)
			{
				closestD = Grids().getEnemyArmyCenter().getDistance(base);
				closestP = base;
			}
		}
		unit.unit()->move(closestP);
	}

	// If no target and no enemy bases, move to a random base
	else
	{
		int random = rand() % (Terrain().getAllBaseLocations().size() - 1);
		int i = 0;
		if (unit.unit()->isIdle())
		{
			for (auto &base : Terrain().getAllBaseLocations())
			{
				if (i == random)
				{
					unit.unit()->move(Position(base));
					return;
				}
				else
				{
					i++;
				}
			}
		}
	}
	return;
}

void CommandTrackerClass::defend(UnitInfo& unit)
{
	// Early on, defend mineral line - TODO: Use ordered bases to check which one to hold
	if (!Strategy().isHoldChoke())
	{
		for (auto &base : Bases().getMyBases())
		{
			if (unit.unit()->getLastCommand().getTargetPosition() != Position(base.second.getResourcesPosition()) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
			{
				unit.unit()->move(Position(base.second.getResourcesPosition()));
			}
			return;
		}
	}

	// Defend chokepoint with concave
	int min = unit.getGroundRange();
	int max = unit.getGroundRange() + 128;
	double closestD = 0.0;
	WalkPosition start = unit.getWalkPosition();
	WalkPosition bestPosition = start;
	/*if (unit.getGroundRange() <= 32)
	{
	min = 64;
	max = 128;
	}*/

	// Find closest chokepoint
	WalkPosition choke = WalkPosition(Terrain().getFirstChoke());
	if (BuildOrder().getBuildingDesired()[UnitTypes::Protoss_Nexus] >= 2 || Strategy().isAllyFastExpand())
	{
		choke = WalkPosition(Terrain().getSecondChoke());
	}

	// Find suitable position to hold at chokepoint
	closestD = unit.getPosition().getDistance(Position(choke));
	for (int x = choke.x - 25; x <= choke.x + 25; x++)
	{
		for (int y = choke.y - 25; y <= choke.y + 25; y++)
		{
			if (WalkPosition(x, y).isValid() && Grids().getMobilityGrid(x, y) > 0 && theMap.GetArea(WalkPosition(x, y)) && Terrain().getAllyTerritory().find(theMap.GetArea(WalkPosition(x, y))->Id()) != Terrain().getAllyTerritory().end() && Position(WalkPosition(x, y)).getDistance(Position(choke)) > min && Position(WalkPosition(x, y)).getDistance(Position(choke)) < max && Position(WalkPosition(x, y)).getDistance(Position(choke)) < closestD)
			{
				if (Util().isMobile(start, WalkPosition(x, y), unit.getType()))
				{
					bestPosition = WalkPosition(x, y);
					closestD = Position(WalkPosition(x, y)).getDistance(Position(choke));
				}
			}
		}
	}
	if (bestPosition.isValid() && bestPosition != start)
	{
		if ((unit.unit()->getOrderTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move))
		{
			unit.unit()->move(Position(bestPosition));
		}
		unit.setTargetPosition(Position(bestPosition));
		Grids().updateAllyMovement(unit.unit(), bestPosition);
	}
	return;
}

void CommandTrackerClass::flee(UnitInfo& unit)
{
	WalkPosition start = unit.getWalkPosition();
	WalkPosition bestPosition = start;
	double best = 0.0, closest = 0.0;

	// If it's a tank, make sure we're unsieged before moving -  TODO: Check that target has velocity and > 512 or no velocity and < tank range
	if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
	{
		if (unit.unit()->getDistance(unit.getTargetPosition()) > 512 || unit.unit()->getDistance(unit.getTargetPosition()) < 128)
		{
			unit.unit()->unsiege();
			return;
		}
	}

	// If it's a High Templar, merge into an Archon if low energy
	if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.unit()->getEnergy() < 75 && Grids().getEGroundThreat(unit.getWalkPosition()) > 0.0)
	{
		for (auto &t : Units().getAllyUnitsFilter(UnitTypes::Protoss_High_Templar))
		{
			UnitInfo &templar = Units().getAllyUnit(t);
			if (templar.unit() && templar.unit()->exists() && templar.unit()->getEnergy() < 75 && Grids().getEGroundThreat(templar.getWalkPosition()) > 0.0)
			{
				if (templar.unit()->getLastCommand().getTechType() != TechTypes::Archon_Warp && unit.unit()->getLastCommand().getTechType() != TechTypes::Archon_Warp)
				{
					unit.unit()->useTech(TechTypes::Archon_Warp, templar.unit());
				}
				return;
			}
		}
	}

	// Search a 16x16 grid around the unit
	for (int x = start.x - 16; x <= start.x + 16 + (unit.getType().tileWidth() * 4); x++)
	{
		for (int y = start.y - 16; y <= start.y + 16 + (unit.getType().tileHeight() * 4); y++)
		{
			if (!WalkPosition(x, y).isValid())
			{
				continue;
			}

			double mobility = double(Grids().getMobilityGrid(x, y));
			double distance = 0.0;
			double threat = Grids().getEGroundThreat(x, y);

			if (Terrain().isInAllyTerritory(unit.unit()))
			{
				distance = 1.0 / max(1.0, unit.getPosition().getDistance(unit.getTargetPosition()));
			}
			else
			{
				distance = double(Grids().getDistanceHome(x, y));
			}

			if ((threat <= best || best == 0.0) && (distance / mobility < closest || closest == 0.0) && Util().isMobile(start, WalkPosition(x, y), unit.getType()))
			{
				best = threat;
				closest = distance / mobility;
				bestPosition = WalkPosition(x, y);
			}
		}
	}

	// If a valid position was found that isn't the starting position
	if (bestPosition.isValid() && bestPosition != start)
	{
		Grids().updateAllyMovement(unit.unit(), bestPosition);
		unit.setTargetPosition(Position(bestPosition));
		if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition))
		{
			unit.unit()->move(Position(bestPosition));
		}
	}
	return;
}

void CommandTrackerClass::approach(UnitInfo& unit)
{
	// TODO, use linear interpolation to approach closer, so that units can approach a cliff to snipe units on higher terrain (carriers, tanks)
	if (unit.getTargetPosition().isValid())
	{
		unit.unit()->move(unit.getTargetPosition());
	}
	return;
}

void CommandTrackerClass::exploreArea(UnitInfo& unit)
{
	// Given a region, explore a random portion of it based on random metrics like:
	// Distance to enemy
	// Distance to home
	// Last explored
	// Unit deaths
	// Untouched resources
}