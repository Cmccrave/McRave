#include "McRave.h"

namespace McRave
{
	void TargetTrackerClass::getTarget(UnitInfo& unit)
	{
		if (unit.getType() == UnitTypes::Terran_Medic) allyTarget(unit);		
		else enemyTarget(unit);
	}

	void TargetTrackerClass::enemyTarget(UnitInfo& unit)
	{
		if (Units().getEnemyUnits().size() == 0)
		{
			unit.setTarget(nullptr);
			return;
		}

		auto& bestTarget = Units().getEnemyUnits().begin();
		double highest = 0.0;
		for (auto e = Units().getEnemyUnits().begin(); e != Units().getEnemyUnits().end(); ++e)
		{
			UnitInfo &enemy = e->second;
			if (!enemy.unit()) continue;

			bool enemyCanAttack = ((unit.getType().isFlyer() && enemy.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && enemy.getGroundDamage() > 0.0));
			bool unitCanAttack = ((enemy.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!enemy.getType().isFlyer() && unit.getGroundDamage() > 0.0));

			if (enemy.getType() == UnitTypes::Zerg_Egg || enemy.getType() == UnitTypes::Zerg_Larva) continue; // If it's an egg or larva, ignore it		
			if (!unit.getType().isDetector() && ((enemy.getType().isFlyer() && unit.getAirRange() <= 0.0) || (!enemy.getType().isFlyer() && unit.getGroundRange() <= 0.0))) continue; // If unit is dead or unattackable, ignore it		
			if (enemy.unit()->exists() && enemy.unit()->isStasised()) continue; // If the enemy is stasised, ignore it		
			if (enemy.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getGroundRange() < 32 && unit.getType() != UnitTypes::Protoss_Dark_Templar) continue; // If the enemy is a mine and this is a melee unit (except DT), ignore it
			if (unit.getTransport() && enemy.getType().isBuilding()) continue; // If unit is loaded, don't target buildings		
			if ((enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected() && !enemyCanAttack) continue;

			double thisUnit = 0.0;
			double groundDist = double(Grids().getDistanceHome(enemy.getWalkPosition()) - Grids().getDistanceHome(unit.getWalkPosition()));
			double airDist = unit.getPosition().getDistance(enemy.getPosition());
			double widths = unit.getType().tileWidth() * 16.0 + enemy.getType().tileWidth() * 16.0;
			double distance = widths + (unit.getType().isFlyer() ? airDist : max(groundDist, airDist));
			double allyRange = widths + (enemy.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
			double aqRange = pow(max(allyRange, distance), 10.0);
			double health = 1.0 + (0.25 * unit.getPercentHealth());

			// Allow tanks to target units at their maximum sieged tank
			if (unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) aqRange = pow(max(384.0, distance), 10.0);

			// Detectors only target invis units
			if (unit.getType().isDetector() && !unit.getType().isBuilding())
			{
				if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && ((!enemy.getType().isFlyer() && Grids().getAGroundThreat(enemy.getWalkPosition()) > 0.0) || (enemy.getType().isFlyer() && Grids().getAAirThreat(enemy.getWalkPosition()) > 0.0) || Terrain().isInAllyTerritory(enemy.getTilePosition())) && Grids().getEDetectorGrid(enemy.getWalkPosition()) == 0)
				{
					thisUnit = (health * enemy.getPriority()) / aqRange;
				}
			}
			// Arbiters only target tanks - Testing no regard for distance
			if (unit.getType() == UnitTypes::Protoss_Arbiter)
			{
				if (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || enemy.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || enemy.getType() == UnitTypes::Terran_Science_Vessel)
				{
					thisUnit = (enemy.getPriority() * Grids().getStasisCluster(enemy.getWalkPosition()));
					thisUnit = (health * enemy.getPriority()) / aqRange;
				}
			}

			// High Templars target the highest priority with the largest cluster
			else if (unit.getType() == UnitTypes::Protoss_High_Templar)
			{
				int eGrid = Grids().getEGroundCluster(enemy.getWalkPosition()) + Grids().getEAirCluster(enemy.getWalkPosition());
				int aGrid = Grids().getAGroundCluster(enemy.getWalkPosition()) + Grids().getAAirCluster(enemy.getWalkPosition());
				double cluster = double(eGrid) / double(1 + aGrid);

				if (Grids().getPsiStormGrid(enemy.getWalkPosition()) == 0 && !enemy.getType().isBuilding())
					if (thisUnit > highest)
					{
						thisUnit = (enemy.getPriority() * cluster) / aqRange;
						highest = thisUnit;
						bestTarget = e;
						unit.setEngagePosition(getEngagePosition(unit, enemy));
						unit.setSimPosition(enemy.getPosition());
					}
			}

			else if ((enemy.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!enemy.getType().isFlyer() && unit.getGroundDamage() > 0.0)) thisUnit = (enemy.getPriority() * health) / aqRange;	

			if (thisUnit > highest)
			{
				highest = thisUnit;
				bestTarget = e;
				unit.setEngagePosition(getEngagePosition(unit, enemy));
				unit.setSimPosition(enemy.getPosition());
			}
		}

		unit.setTarget(&bestTarget->second);
	}

	void TargetTrackerClass::allyTarget(UnitInfo& unit)
	{
		if (Units().getAllyUnits().size() == 0)
		{
			unit.setTarget(nullptr);
			return;
		}

		auto& bestTarget = Units().getAllyUnits().begin();
		double highest = 0.0;

		for (auto a = Units().getAllyUnits().begin(); a != Units().getAllyUnits().end(); ++a)
		{
			UnitInfo &ally = a->second;
			if (!ally.unit()) continue;

			if (ally.getType() == UnitTypes::Terran_Medic) continue;
			if (ally.getType() != UnitTypes::Terran_Marine && ally.getType() != UnitTypes::Terran_Firebat) continue;
			double thisUnit = 0.0;
			double groundDist = double(Grids().getDistanceHome(ally.getWalkPosition()) - Grids().getDistanceHome(unit.getWalkPosition()));
			double airDist = unit.getPosition().getDistance(ally.getPosition());
			double widths = unit.getType().tileWidth() * 16.0 + ally.getType().tileWidth() * 16.0;
			double distance = widths + (unit.getType().isFlyer() ? airDist : max(groundDist, airDist));
			double health = 1.0 + (0.50 * unit.getPercentHealth());

			thisUnit = (health * ally.getPriority()) / distance;
			if (thisUnit > highest)
			{
				highest = thisUnit;
				bestTarget = a;
				unit.setEngagePosition(ally.getPosition());
				unit.setSimPosition(ally.getPosition());
			}
		}
		unit.setTarget(&bestTarget->second);
	}

	Position TargetTrackerClass::getEngagePosition(UnitInfo& unit, UnitInfo& enemy)
	{
		if (unit.getPosition().getDistance(enemy.getPosition()) > max(unit.getGroundRange(), unit.getAirRange()))
		{
			return (unit.getPosition() + Position((enemy.getPosition() - unit.getPosition()) * (unit.getPosition().getDistance(enemy.getPosition()) - max(unit.getGroundRange(), unit.getAirRange())) / unit.getPosition().getDistance(enemy.getPosition())));
		}
		return (unit.getPosition());
	}
}