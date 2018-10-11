#include "McRave.h"

namespace McRave
{
	void TargetManager::getTarget(UnitInfo& unit)
	{
		if (unit.getType() == UnitTypes::Terran_Medic)
			allyTarget(unit);
		else
			enemyTarget(unit);

		getEngagePosition(unit);
		getPathToTarget(unit);
	}

	void TargetManager::enemyTarget(UnitInfo& unit)
	{
		UnitInfo* bestTarget = nullptr;
		double closest = DBL_MAX, highest = 0.0;

		const auto shouldTarget = [&](UnitInfo& enemy, bool unitCanAttack, bool enemyCanAttack) {
			if ((unit.getType() == UnitTypes::Protoss_Zealot && enemy.getType().isWorker() && !Units().isThreatening(enemy) && Terrain().isInAllyTerritory(enemy.getTilePosition()))
				|| (enemy.getType() == UnitTypes::Zerg_Egg || enemy.getType() == UnitTypes::Zerg_Larva || enemy.getType() == UnitTypes::Protoss_Scarab) // If enemy is an egg or larva
				|| (!unit.getType().isDetector() && !unitCanAttack) 													// If unit can't attack and unit is not a detector
				|| (enemy.unit()->exists() && enemy.unit()->isStasised()) 												// If enemy is stasised
				|| (enemy.getType().isBuilding() && enemy.getAirDamage() == 0.0 && enemy.getGroundDamage() == 0.0 && unit.getType() == UnitTypes::Zerg_Mutalisk)
				|| (enemy.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)				// If enemy is a mine and my unit is melee
				|| (unit.getTransport() && enemy.getType().isBuilding() && Broodwar->getFrameCount() < 15000) 													// If unit has an assigned transport, don't target buildings
				|| ((enemy.isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected() && !enemyCanAttack && !unit.getType().isDetector())		// If enemy is invisible and can't attack this unit
				|| (!enemy.getType().isFlyer() && enemy.unit()->isUnderDisruptionWeb() && unit.getGroundRange() <= 64)		// If enemy is under DWEB and my unit is melee 
				|| (unit.getType() == UnitTypes::Protoss_Dark_Templar && enemy.getType() == UnitTypes::Terran_Vulture)
				|| (unit.getType().isFlyer() && enemy.getType() == UnitTypes::Protoss_Interceptor)
				|| (unit.getType() == UnitTypes::Protoss_Zealot && BuildOrder().isRush() && !enemy.getType().isWorker() && Broodwar->getFrameCount() < 10000))
				return false;
			return true;
		};

		const auto checkBest = [&](UnitInfo& enemy, double thisUnit, double health, double distance) {

			auto priority = (enemy.getType().isBuilding() && enemy.getGroundDamage() == 0.0 && enemy.getAirDamage() == 0.0) ? enemy.getPriority() / 2.0 : enemy.getPriority();

			// Detector targeting
			if (unit.getType().isDetector() && !unit.getType().isBuilding()) {
				if (enemy.isBurrowed() || enemy.unit()->isCloaked())
					thisUnit = (priority * health) / distance;
			}

			// Cluster targeting for AoE units
			else if (unit.getType() == UnitTypes::Protoss_High_Templar || unit.getType() == UnitTypes::Protoss_Arbiter) {
				if (!enemy.getType().isBuilding() && enemy.getType() != UnitTypes::Terran_Vulture_Spider_Mine) {

					double eGrid = Grids().getEGroundCluster(enemy.getWalkPosition()) + Grids().getEAirCluster(enemy.getWalkPosition());
					double aGrid = Grids().getAGroundCluster(enemy.getWalkPosition()) + Grids().getAAirCluster(enemy.getWalkPosition());
					double score = eGrid;

					if (eGrid > aGrid)
						thisUnit = (priority * score) / distance;
				}
			}

			// Proximity targeting
			else if (unit.getType() == UnitTypes::Protoss_Reaver) {
				if (enemy.getType().isBuilding() && (enemy.getGroundDamage() == 0.0 || enemy.getAirDamage() == 0.0))
					thisUnit = 0.1 / distance;
				else
					thisUnit = health / distance;
			}

			// Priority targeting
			else
				thisUnit = (priority * health) / distance;

			// If this enemy is more important to target, set as current target
			if (thisUnit > highest) {
				highest = thisUnit;
				bestTarget = &enemy;
			}
			return;
		};

		for (auto &e : Units().getEnemyUnits()) {
			UnitInfo &enemy = e.second;

			// Valid check;
			if (!enemy.unit()
				|| !enemy.getWalkPosition().isValid()
				|| !unit.getWalkPosition().isValid()
				|| (enemy.getType().isBuilding() && !Units().isThreatening(enemy) && enemy.getGroundDamage() == 0.0 && Terrain().isInAllyTerritory(enemy.getTilePosition()) && Broodwar->getFrameCount() < 10000))
				continue;

			bool enemyCanAttack = ((unit.getType().isFlyer() && enemy.getAirDamage() > 0.0) || (!unit.getType().isFlyer() && enemy.getGroundDamage() > 0.0) || (!unit.getType().isFlyer() && enemy.getType() == UnitTypes::Terran_Vulture_Spider_Mine));
			bool unitCanAttack = ((enemy.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!enemy.getType().isFlyer() && unit.getGroundDamage() > 0.0) || (unit.getType() == UnitTypes::Protoss_Carrier));

			double allyRange = enemy.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
			double airDist = unit.getPosition().getDistance(enemy.getPosition());
			double widths = unit.getType().tileWidth() * 16.0 + enemy.getType().tileWidth() * 16.0;
			double distance = widths + max(allyRange, airDist);
			double health = enemyCanAttack ? 1.0 + (0.2*(1.0 - unit.getPercentHealth())) : 1.0;
			double thisUnit = 0.0;

			// Set sim position
			if ((unitCanAttack || enemyCanAttack) && distance < closest) {
				unit.setSimPosition(enemy.getPosition());
				closest = distance;
			}

			// If should target, check if it's best
			if (shouldTarget(enemy, unitCanAttack, enemyCanAttack))
				checkBest(enemy, thisUnit, health, distance);
		}

		for (auto &n : Units().getNeutralUnits()) {
			UnitInfo &neutral = n.second;
			if (!neutral.unit())
				continue;

			double health = 1.0 + (0.15 / unit.getPercentHealth());
			double airDist = unit.getPosition().getDistance(neutral.getPosition());
			double widths = unit.getType().tileWidth() * 16.0 + neutral.getType().tileWidth() * 16.0;
			double distance = widths + airDist;
			double thisUnit = 0.0;
			bool unitCanAttack = ((neutral.getType().isFlyer() && unit.getAirDamage() > 0.0) || (!neutral.getType().isFlyer() && unit.getGroundDamage() > 0.0));

			if (!unitCanAttack)
				continue;

			if (unit.getTransport() && neutral.getType().isBuilding())
				continue;

			thisUnit = neutral.getPriority() / distance;

			// If this neutral is more important to target, set as current target
			if (thisUnit > highest) {
				highest = thisUnit;
				bestTarget = &neutral;
			}
		}
		unit.setTarget(bestTarget);
	}

	void TargetManager::allyTarget(UnitInfo& unit)
	{
		if (Units().getMyUnits().size() == 0) {
			unit.setTarget(nullptr);
			return;
		}

		UnitInfo* bestTarget = nullptr;
		double highest = 0.0;

		for (auto &a : Units().getMyUnits())
		{
			UnitInfo &ally = a.second;
			if (!ally.unit())
				continue;

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
				bestTarget = &ally;
				unit.setEngagePosition(ally.getPosition());
				unit.setSimPosition(ally.getPosition());
			}
		}
		unit.setTarget(bestTarget);
	}

	void TargetManager::getEngagePosition(UnitInfo& unit)
	{
		if (!unit.hasTarget()) {
			unit.setEngagePosition(Positions::None);
			return;
		}

		double distance = unit.getDistance(unit.getTarget());
		double range = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
		double leftover = distance - range;
		Position direction = (unit.getPosition() - unit.getTarget().getPosition()) * int(leftover / distance);

		if (distance > range)
			unit.setEngagePosition(unit.getPosition() - direction);
		else
			unit.setEngagePosition(unit.getPosition());


		// See if we pass any narrow chokes while trying to fight
		if (unit.getEngagePosition().isValid() && !unit.getType().isFlyer() && !unit.getTransport() && Units().getSupply() >= 80 && mapBWEM.GetArea(unit.getTilePosition()) && mapBWEM.GetArea((TilePosition)unit.getEngagePosition())) {
			auto unitWidth = min(unit.getType().width(), unit.getType().height());
			auto chokeWidth = 0;

			for (auto choke : mapBWEM.GetPath(unit.getPosition(), unit.getEngagePosition())) {
				chokeWidth = Util().chokeWidth(choke);				

				if (unitWidth > 0.0 && unitWidth <= 5.0) {
					auto squeeze = max(1, chokeWidth / unitWidth); /// How many of this unit can fit through at once
					auto ratio = 1.0 - (max(1.0, double(squeeze)) / 10.0);

					if (ratio <= 1.0 && ratio > 0.0)
						unit.setSimBonus(ratio);
				}
			}

			Broodwar->drawTextMap(unit.getPosition(), "%.2f", unit.getSimBonus());
		}
	}

	void TargetManager::getPathToTarget(UnitInfo& unit)
	{
		auto const shouldCreatePath = [&]() {
			if (!unit.getTargetPath().getTiles().empty() && unit.samePath())												// If both units have the same tile
				return false;

			if (!unit.getTransport()																						// If unit has no transport
				&& unit.getTilePosition().isValid() && unit.getTarget().getTilePosition().isValid()							// If both units have valid tiles
				&& mapBWEB.getUsedTiles().find(unit.getTarget().getTilePosition()) == mapBWEB.getUsedTiles().end()			// Doesn't overlap buildings
				&& !unit.getType().isFlyer() && !unit.getTarget().getType().isFlyer()										// Doesn't include flyers
				&& unit.getPosition().getDistance(unit.getTarget().getPosition()) < 480.0									// Isn't too far from engaging
				&& mapBWEB.getGroundDistance(unit.getPosition(), unit.getTarget().getPosition()) < 480.0					// TEMP: Check ground distance too
				&& mapBWEB.isWalkable(unit.getTilePosition()) && mapBWEB.isWalkable(unit.getTarget().getTilePosition()))	// Walkable tiles
				return true;
			return false;
		};

		// If no target, no distance/path available
		if (!unit.hasTarget()) {
			unit.setEngDist(0.0);
			unit.setTargetPath(BWEB::Path());
			return;
		}

		// Set distance as estimate when targeting a building/flying unit or far away
		if (unit.getTarget().getType().isBuilding() || unit.getTarget().getType().isFlyer() || unit.getPosition().getDistance(unit.getTarget().getPosition()) >= 800.0 || unit.getTilePosition() == unit.getTarget().getTilePosition()) {
			unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
			unit.setTargetPath(BWEB::Path());
			return;
		}

		// If should create path, grab one from BWEB
		if (shouldCreatePath()) {
			Path newPath;
			newPath.createUnitPath(mapBWEB, mapBWEM, unit.getPosition(), unit.getTarget().getPosition());
			unit.setTargetPath(newPath);
		}

		// Measure distance minus range
		if (!unit.getTargetPath().getTiles().empty()) {
			double range = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
			unit.setEngDist(max(0.0, unit.getTargetPath().getDistance() - range));
		}

		// Otherwise approximate
		else {
			auto dist = unit.getPosition().getDistance(unit.getEngagePosition());
			unit.setEngDist(dist);
		}

		// Display path
		if (unit.getTarget().unit()->exists())
			Display().displayPath(unit, unit.getTargetPath().getTiles());
	}
}