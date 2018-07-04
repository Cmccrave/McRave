#include "McRave.h"

namespace McRave
{
	void TargetManager::getTarget(UnitInfo& unit)
	{
		if (unit.getType() == UnitTypes::Terran_Medic)
			allyTarget(unit);
		else
			enemyTarget(unit);
	}

	void TargetManager::enemyTarget(UnitInfo& u)
	{
		UnitInfo* bestTarget = nullptr;
		double closest = DBL_MAX, highest = 0.0;
		for (auto &enemy : Units().getEnemyUnits()) {

			UnitInfo &e = enemy.second;
			if (!e.unit()) continue;
			if (!e.getWalkPosition().isValid() || !u.getWalkPosition().isValid()) continue;
			if (e.getType().isBuilding() && !Units().isThreatening(e) && Broodwar->getFrameCount() < 6000) continue;

			bool enemyCanAttack = ((u.getType().isFlyer() && e.getAirDamage() > 0.0) || (!u.getType().isFlyer() && e.getGroundDamage() > 0.0) || (!u.getType().isFlyer() && e.getType() == UnitTypes::Terran_Vulture_Spider_Mine));
			bool unitCanAttack = ((e.getType().isFlyer() && u.getAirDamage() > 0.0) || (!e.getType().isFlyer() && u.getGroundDamage() > 0.0));

			if (u.getType() == UnitTypes::Protoss_Carrier)
				unitCanAttack = true;

			double allyRange = e.getType().isFlyer() ? u.getAirRange() : u.getGroundRange();
			double airDist = u.getPosition().getDistance(e.getPosition());
			double widths = u.getType().tileWidth() * 16.0 + e.getType().tileWidth() * 16.0;
			double distance = widths + max(allyRange, airDist);
			double health = 1.0 + (0.15 / u.getPercentHealth());
			double thisUnit = 0.0;


			if ((unitCanAttack || enemyCanAttack) && distance < closest) {
				u.setSimPosition(e.getPosition());
				closest = distance;
			}

			if (!unitCanAttack) continue;
			if (e.getType() == UnitTypes::Zerg_Egg || e.getType() == UnitTypes::Zerg_Larva) continue;					// If enemy is an egg or larva
			if (!u.getType().isDetector() && !unitCanAttack) continue;													// If unit can't attack and unit is not a detector
			if (e.unit()->exists() && e.unit()->isStasised()) continue;													// If enemy is stasised
			if (e.getType() == UnitTypes::Terran_Vulture_Spider_Mine && u.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Air_Weapons) < 2) continue;				// If enemy is a mine and my unit is melee
			//if (u.getTransport() && e.getType().isBuilding()) continue;							// If unit has an assigned transport, don't target buildings
			if ((e.isBurrowed() || e.unit()->isCloaked()) && !e.unit()->isDetected() && !enemyCanAttack && !u.getType().isDetector()) continue;		// If enemy is invisible and can't attack this unit
			if (!e.getType().isFlyer() && e.unit()->isUnderDisruptionWeb() && u.getGroundRange() <= 64) continue;		// If enemy is under DWEB and my unit is melee 
			if (u.getType() == UnitTypes::Protoss_Dark_Templar && e.getType() == UnitTypes::Terran_Vulture) continue;
			//if (u.getType() == UnitTypes::Protoss_Arbiter && e.getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode && e.getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode) continue;

			// Detectors only target invis units
			else if (u.getType().isDetector() && !u.getType().isBuilding()) {
				//Broodwar->drawCircleMap(u.getPosition(), 8, Colors::Green);
				if (e.isBurrowed() || e.unit()->isCloaked())
					thisUnit = (e.getPriority() * health) / distance;
			}

			// High Templars target the highest priority with the largest cluster
			else if (u.getType() == UnitTypes::Protoss_High_Templar || u.getType() == UnitTypes::Protoss_Arbiter) {
				if (!e.getType().isBuilding() && e.getType() != UnitTypes::Terran_Vulture_Spider_Mine) {

					double eGrid = Grids().getEGroundCluster(e.getWalkPosition()) + Grids().getEAirCluster(e.getWalkPosition());
					double aGrid = Grids().getAGroundCluster(e.getWalkPosition()) + Grids().getAAirCluster(e.getWalkPosition());
					double score = eGrid / (1.0 + aGrid);

					if (eGrid > aGrid)
						thisUnit = (e.getPriority() * score) / distance;
				}
			}

			// All other units target based on priority
			else
				thisUnit = (e.getPriority() * health) / distance;

			// If this enemy is more important to target, set as current target
			if (thisUnit > highest) {
				highest = thisUnit;
				bestTarget = &e;
			}
		}

		for (auto &n : Units().getNeutralUnits()) {
			UnitInfo &neutral = n.second;
			if (!neutral.unit())
				continue;

			double health = 1.0 + (0.15 / u.getPercentHealth());
			double airDist = u.getPosition().getDistance(neutral.getPosition());
			double widths = u.getType().tileWidth() * 16.0 + neutral.getType().tileWidth() * 16.0;
			double distance = widths + airDist;
			double thisUnit = 0.0;
			bool unitCanAttack = ((neutral.getType().isFlyer() && u.getAirDamage() > 0.0) || (!neutral.getType().isFlyer() && u.getGroundDamage() > 0.0));

			if (!unitCanAttack)
				continue;

			if (u.getTransport() && neutral.getType().isBuilding()) continue;

			thisUnit = neutral.getPriority() / distance;

			// If this neutral is more important to target, set as current target
			if (thisUnit > highest) {
				highest = thisUnit;
				bestTarget = &neutral;
			}
		}

		u.setTarget(bestTarget);
		u.setEngagePosition(getEngagePosition(u));
		getPathToTarget(u);
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

	Position TargetManager::getEngagePosition(UnitInfo& unit)
	{
		// TODO: Check this
		double distance = unit.getDistance(unit.getTarget());
		double range = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
		Position direction = (unit.getPosition() - unit.getTarget().getPosition()) / distance;

		if (distance > range)		
			return (unit.getPosition() + Position(direction*range));		
		return (unit.getPosition());
	}

	void TargetManager::getPathToTarget(UnitInfo& unit)
	{
		if (!unit.hasTarget()) {
			unit.setEngDist(0.0);
			unit.setTargetPath(vector<TilePosition>{});
			return;
		}

		if (unit.getTarget().getType().isBuilding()) {
			unit.setEngDist(unit.getPosition().getDistance(unit.getEngagePosition()));
			unit.setTargetPath(vector<TilePosition>{});
			return;
		}

		auto const shouldCreatePath = [&]() {			
			if (!unit.getTargetPath().empty() && unit.samePath())															// If both units have the same tile
				return false;

			if (!unit.getTransport()																						// If unit has no transport
				&& mapBWEM.GetArea(unit.getTilePosition()) && mapBWEM.GetArea(unit.getTarget().getTilePosition())			// Valid areas exist
				&& mapBWEB.getUsedTiles().find(unit.getTarget().getTilePosition()) == mapBWEB.getUsedTiles().end()			// Doesn't overlap buildings
				&& !unit.getType().isFlyer() && !unit.getTarget().getType().isFlyer()										// Doesn't include flyers
				&& unit.getPosition().getDistance(unit.getTarget().getPosition()) < 800.0									// Isn't too far from engaging
				&& mapBWEB.isWalkable(unit.getTilePosition()) && mapBWEB.isWalkable(unit.getTarget().getTilePosition()))	// Walkable tiles
				return true;
			return false;
		};		

		// If should create path, grab one from BWEB
		if (shouldCreatePath()) {			
			auto path = mapBWEB.findPath(mapBWEB, unit.getTilePosition(), unit.getTarget().getTilePosition(), true, true, false, false);
			unit.setTargetPath(path);
		}

		// If empty, assume unreachable
		if (unit.getTargetPath().empty())
			unit.setEngDist(DBL_MAX);

		// If path is longer than 2 tiles, resize path to fufill position level resolution of both units
		else if (unit.getTargetPath().size() >= 3) {
			auto distOffset = unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
			auto posCorrection = unit.getPosition().getDistance((Position)*(unit.getTargetPath().end() - 1)) + unit.getTarget().getPosition().getDistance((Position)*(unit.getTargetPath().begin() + 1));
			auto dist = max(0.0, ((double)unit.getTargetPath().size() * 32.0) - distOffset - 64.0 + posCorrection);

			unit.setEngDist(dist);
		}

		// Otherwise approximate
		else {
			auto dist = unit.getPosition().getDistance(unit.getEngagePosition());
			unit.setEngDist(dist);
		}

		// Display path
		if (unit.getTarget().unit()->exists())
			Display().displayPath(unit, unit.getTargetPath());

		if (unit.getEngDist() != DBL_MAX)
			Broodwar->drawTextMap(unit.getPosition(), "%.2f", unit.getEngDist());

	}
}