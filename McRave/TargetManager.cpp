#include "McRave.h"

Unit TargetTrackerClass::getTarget(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Shuttle)
	{
		return nullptr;
	}
	else if (unit.getType() == UnitTypes::Terran_Medic)
	{
		return allyTarget(unit);
	}
	else
	{
		return enemyTarget(unit);
	}
	return nullptr;
}

Unit TargetTrackerClass::enemyTarget(UnitInfo& unit)
{
	double highest = 0.0, thisUnit = 0.0;
	Unit target = nullptr;
	Position targetPosition;
	WalkPosition targetWalkPosition;
	TilePosition targetTilePosition;

	for (auto &e : Units().getEnemyUnits())
	{
		thisUnit = 0.0;
		UnitInfo &enemy = e.second;
		double distance = distance = max(0.0, unit.getPosition().getDistance(enemy.getPosition()));

		if (!enemy.unit())
		{
			continue;
		}

		// If it's an egg or larva, ignore it
		if (enemy.getType() == UnitTypes::Zerg_Egg || enemy.getType() == UnitTypes::Zerg_Larva)
		{
			continue;
		}

		// If unit is dead or unattackable, ignore it
		if (!unit.getType().isDetector() && (enemy.getDeadFrame() > 0 || (enemy.getType().isFlyer() && unit.getAirRange() == 0.0) || (!enemy.getType().isFlyer() && unit.getGroundRange() == 0.0)))
		{
			continue;
		}

		// If the enemy is stasised, ignore it
		if (enemy.unit()->exists() && enemy.unit()->isStasised())
		{
			continue;
		}

		// If the enemy is a mine and this is a melee unit (except DT), ignore it
		if (enemy.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getGroundRange() < 32 && unit.getType() != UnitTypes::Protoss_Dark_Templar)
		{
			continue;
		}


		// If this is a detector unit, target invisible units only
		if (unit.getType().isDetector() && !unit.getType().isBuilding())
		{
			if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && ((!enemy.getType().isFlyer() && Grids().getAGroundThreat(enemy.getWalkPosition()) > 0) || (enemy.getType().isFlyer() && Grids().getAAirThreat(enemy.getWalkPosition()) > 0) || Terrain().isInAllyTerritory(enemy.unit())) && Grids().getEDetectorGrid(enemy.getWalkPosition()) == 0)
			{
				thisUnit = (enemy.getPriority() * (1.0 + 0.1 *(1.0 - enemy.getPercentHealth()))) / distance;
			}
			else
			{
				continue;
			}
		}

		// Arbiters only target tanks - Testing no regard for distance
		if (unit.getType() == UnitTypes::Protoss_Arbiter)
		{
			if (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || enemy.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || enemy.getType() == UnitTypes::Terran_Science_Vessel)
			{
				thisUnit = (enemy.getPriority() * Grids().getStasisCluster(enemy.getWalkPosition()));
			}
		}

		// High Templars target the highest priority with the largest cluster
		else if (unit.getType() == UnitTypes::Protoss_High_Templar)
		{
			if (Grids().getPsiStormGrid(enemy.getWalkPosition()) == 0 && Grids().getACluster(enemy.getWalkPosition()) < (Grids().getEAirCluster(enemy.getWalkPosition()) + Grids().getEGroundCluster(enemy.getWalkPosition())) && !enemy.getType().isBuilding())
			{
				thisUnit = (enemy.getPriority() * max(Grids().getEGroundCluster(enemy.getWalkPosition()), Grids().getEAirCluster(enemy.getWalkPosition()))) / distance;
			}
		}

		else if (enemy.getType().isFlyer() && unit.getAirDamage() > 0.0)
		{
			thisUnit = (enemy.getPriority() * (1.0 + 0.1 *(1.0 - enemy.getPercentHealth()))) / distance;
		}
		else if (!enemy.getType().isFlyer() && unit.getGroundDamage() > 0.0)
		{
			thisUnit = (enemy.getPriority() * (1.0 + 0.1 *(1.0 - enemy.getPercentHealth()))) / distance;
		}

		// If the unit doesn't exist, it's not a suitable target usually (could be removed?)
		if (!enemy.unit()->exists())
		{
			thisUnit = thisUnit * 0.1;
		}

		// If this is the strongest enemy around, target it
		if (thisUnit > 0.0 && (thisUnit > highest || highest == 0.0))
		{
			target = enemy.unit();
			highest = thisUnit;
			targetPosition = enemy.getPosition();
			targetWalkPosition = enemy.getWalkPosition();
			targetTilePosition = enemy.getTilePosition();
		}
	}
	if (target)
	{
		unit.setTargetPosition(targetPosition);
		unit.setTargetWalkPosition(targetWalkPosition);
		unit.setTargetTilePosition(targetTilePosition);
		unit.setEngagePosition(unit.getPosition() + Position((unit.getTargetPosition() - unit.getPosition()) * (unit.getPosition().getDistance(unit.getTargetPosition()) - max(unit.getGroundRange(), unit.getAirRange())) / unit.getPosition().getDistance(unit.getTargetPosition())));
	}
	return target;
}

Unit TargetTrackerClass::allyTarget(UnitInfo& unit)
{
	double highest = 0.0;
	Unit target = nullptr;
	Position targetPosition;
	WalkPosition targetWalkPosition;
	TilePosition targetTilePosition;

	// Search for an ally target that needs healing for medics
	for (auto &a : Units().getAllyUnits())
	{
		UnitInfo ally = a.second;
		if (!ally.unit() || ally.getDeadFrame() != 0 || !ally.getType().isOrganic())
		{
			continue;
		}

		if (ally.unit()->isBeingHealed() && unit.getTarget() != ally.unit())
		{
			continue;
		}

		double distance = unit.getPosition().getDistance(ally.getPosition());

		if (ally.unit()->exists() && ally.unit()->getHitPoints() < ally.getType().maxHitPoints() && (distance < highest || highest == 0.0))
		{
			highest = distance;
			target = ally.unit();
			targetPosition = ally.getPosition();
			targetWalkPosition = ally.getWalkPosition();
			targetTilePosition = ally.getTilePosition();
		}
	}

	// If we found an ally target, store the position
	if (target)
	{
		unit.setTargetPosition(targetPosition);
		unit.setTargetWalkPosition(targetWalkPosition);
		unit.setTargetTilePosition(targetTilePosition);
	}
	return target;
}