#include "McRave.h"

void UnitTrackerClass::update()
{
	Display().startClock();
	updateUnits();
	updateGlobalSimulation();
	Display().performanceTest(__FUNCTION__);
	return;
}

void UnitTrackerClass::updateUnits()
{
	// Reset global strengths
	globalEnemyStrength = 0.0;
	globalAllyStrength = 0.0;
	allyDefense = 0.0;
	enemyDefense = 0.0;

	// Update Enemy Units
	for (auto &u : enemyUnits)
	{
		UnitInfo &unit = u.second;
		if (!unit.unit()) continue; // Ignore improper storage if it happens
		if (unit.unit()->exists())	updateEnemy(unit); // If unit is visible, update it
		if (!unit.unit()->exists() && unit.getPosition().isValid() && Broodwar->isVisible(TilePosition(unit.getPosition()))) unit.setPosition(Positions::None); // If unit is not visible but his position is, move it
		if (unit.getType().isValid()) enemyComposition[unit.getType()] += 1; // If unit has a valid type, update enemy composition tracking
		if (!unit.getType().isWorker() && !unit.getType().isBuilding()) globalEnemyStrength += max(unit.getVisibleGroundStrength(), unit.getVisibleAirStrength()); // If unit is not a worker or building, add it to global strength	
		if (unit.getType().isBuilding() && unit.getGroundDamage() > 0 && unit.unit()->isCompleted()) enemyDefense += unit.getVisibleGroundStrength(); // If unit is a building and deals damage, add it to global defense
	}

	// Update Ally Defenses
	for (auto &u : allyDefenses)
	{
		UnitInfo &unit = u.second;
		updateAlly(unit);
		allyDefense += unit.getMaxGroundStrength();
	}

	// Update Ally Units
	for (auto &u : allyUnits)
	{
		UnitInfo &unit = u.second;
		if (!unit.unit()) continue;
		updateAlly(unit);
		updateLocalSimulation(unit);
		updateStrategy(unit);

		if (!unit.getType().isBuilding()) globalAllyStrength += max(unit.getVisibleGroundStrength(), unit.getVisibleAirStrength());	
		if (unit.getType().isWorker() && Workers().getMyWorkers().find(unit.unit()) != Workers().getMyWorkers().end()) Workers().removeWorker(unit.unit()); // Remove the worker role if needed			
		if ((unit.getType().isWorker() && (Grids().getResourceGrid(unit.getTilePosition()) == 0 || Grids().getEGroundThreat(unit.getWalkPosition()) == 0.0)) || (BuildOrder().getCurrentBuild() == "Sparks" && Units().getGlobalStrategy() != 1)) Workers().storeWorker(unit.unit()); // If this is a worker and is ready to go back to being a worker
	}
	return;
}

void UnitTrackerClass::updateLocalSimulation(UnitInfo& unit)
{
	// Variables for calculating local strengths
	double enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
	double enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
	double simulationTime = 5.0;
	double simRatio = 0.0;
	double unitToEngage = (unit.getPosition().getDistance(unit.getEngagePosition())) / unit.getSpeed();
	double enemyRange, allyRange, unitRange;
	UnitInfo &target = Units().getEnemyUnit(unit.getTarget());

	// Stupid way to hardcode only aggresion from sparks build
	if (BuildOrder().getCurrentBuild() == "Sparks" && !BuildOrder().isOpener())
	{
		unit.setStrategy(1);
		return;
	}

	// Check every enemy unit being in range of the target
	for (auto &e : enemyUnits)
	{
		UnitInfo &enemy = e.second;
		double enemyToEngage = 0.0, distance = 0.0;

		// Ignore workers and stasised units
		if (!enemy.unit() || enemy.getType().isWorker() || (enemy.unit() && enemy.unit()->exists() && enemy.unit()->isStasised())) continue;

		unit.getType().isFlyer() ? enemyRange = enemy.getAirRange() + (enemy.getType().width() / 2.0) : enemyRange = enemy.getGroundRange() + (enemy.getType().width() / 2.0);
		enemy.getType().isFlyer() ? unitRange = unit.getAirRange() + (unit.getType().width() / 2.0) : unitRange = unit.getGroundRange() + (unit.getType().width() / 2.0);

		distance = (8.0 * abs(Grids().getDistanceHome(enemy.getWalkPosition()) - Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())))) - enemyRange;
		if (distance / enemy.getSpeed() > simulationTime) continue;

		if (enemy.getSpeed() > 0.0)
		{
			enemyToEngage = max(0.0, (enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange) / enemy.getSpeed());
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else if (enemyRange > unitRange)
		{
			enemyToEngage = max(0.0, (enemy.getPosition().getDistance(unit.getPosition()) - enemyRange) / unit.getSpeed());
			enemy.getPosition().getDistance(unit.getEngagePosition()) <= enemyRange ? simRatio = simulationTime - enemyToEngage : simRatio = 0.0;
		}

		// Situations where an enemy should be treated as stronger than it actually is
		if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected()) simRatio = simRatio * 5.0;
		if (Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))	simRatio = simRatio * 2.0;
		if (!enemy.unit()->exists()) simRatio = simRatio * (1.0 + double(Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) / 5000);

		enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
		enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
	}

	// Check every ally being in range of the target
	for (auto &a : allyUnits)
	{
		UnitInfo &ally = a.second;
		double allyToEngage = 0.0;

		target.getType().isFlyer() ? allyRange = ally.getAirRange() + (ally.getType().width() / 2.0) : allyRange = ally.getGroundRange() + (ally.getType().width() / 2.0);

		//double distanceAE = 8.0 * abs(Grids().getDistanceHome(WalkPosition(ally.getPosition())) - Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())));
		double distanceAA = 8.0 * abs(Grids().getDistanceHome(WalkPosition(ally.getEngagePosition())) - Grids().getDistanceHome(WalkPosition(unit.getEngagePosition()))) - allyRange;

		if (/*distanceAE / ally.getSpeed() > simulationTime &&*/ distanceAA / ally.getSpeed() > simulationTime) continue;

		
		allyToEngage = max(0.0, (ally.getPosition().getDistance(unit.getTargetPosition()) - allyRange) / ally.getSpeed());
		simRatio = max(0.0, simulationTime - allyToEngage);

		if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(ally.getEngagePosition())) == 0) simRatio = simRatio * 5.0;

		allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
		allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;
	}

	// Store the difference of strengths
	enemyLocalGroundStrength > 0 ? unit.setGroundLocal(allyLocalGroundStrength / enemyLocalGroundStrength) : unit.setGroundLocal(2.0);
	enemyLocalAirStrength > 0 ? unit.setAirLocal(allyLocalAirStrength / enemyLocalAirStrength) : unit.setAirLocal(2.0);
	return;
}

void UnitTrackerClass::updateStrategy(UnitInfo& unit)
{
	// If a unit is clearly out of range, set as "no local" and skip calculating
	if (!unit.getTarget())
	{
		unit.setStrategy(3);
		return;
	}

	// Latch based engagement decision making based on what race we are playing
	double offset = 0.0;
	if (Broodwar->self()->getRace() == Races::Protoss) offset = 0.2;
	else if (Broodwar->self()->getRace() == Races::Terran) offset = 0.5;

	UnitInfo &target = Units().getEnemyUnit(unit.getTarget());
	if (unit.getType().isWorker()) unit.setStrategy(1);
	double decision;
	unit.getType().isFlyer() ? decision = unit.getAirLocal() : decision = unit.getGroundLocal();

	// If unit is in ally territory
	if (Terrain().isInAllyTerritory(unit.unit()) && decision < 1.0 + offset)
	{
		!unit.getTarget()->exists() ? unit.setStrategy(2) : 0;

		// If unit is melee
		if (max(unit.getGroundRange(), unit.getAirRange()) <= 32)
		{
			// If against rush and not ready to wall up, fight in mineral line
			if (Strategy().isRush() || !Strategy().isHoldChoke())
			{
				if (Grids().getResourceGrid(target.getTilePosition()) > 0 && Grids().getResourceGrid(unit.getTilePosition()) > 0)
				{
					unit.setStrategy(1);
					return;
				}
				else
				{
					unit.setStrategy(2);
					return;
				}
			}

			// Else hold ramp and attack anything within range
			else
			{
				if (Terrain().isInAllyTerritory(target.unit()))
				{
					unit.setStrategy(1);
					return;
				}
				else
				{
					unit.setStrategy(2);
					return;
				}
			}
		}

		// If unit is ranged
		else if (max(unit.getGroundRange(), unit.getAirRange()) > 32)
		{
			// If against rush and not ready to wall up, fight in mineral line
			if (Strategy().isRush() || !Strategy().isHoldChoke())
			{
				if (Grids().getResourceGrid(target.getTilePosition()) > 0 || Grids().getResourceGrid(unit.getTilePosition()) > 0)
				{
					unit.setStrategy(1);
					return;
				}
				else
				{
					unit.setStrategy(2);
					return;
				}
			}
			else if (Strategy().isHoldChoke())
			{
				if ((unit.getPosition().getDistance(unit.getTargetPosition()) <= max(unit.getGroundRange(), unit.getAirRange())) || Terrain().isInAllyTerritory(target.unit()))
				{
					unit.setStrategy(1);
					return;
				}
				else
				{
					unit.setStrategy(2);
					return;
				}
			}
		}
	}

	if (unit.getPosition().getDistance(unit.getTargetPosition()) > 640.0)
	{
		unit.setStrategy(3);
		return;
	}

	// If unit is a High Templar
	if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.unit()->getEnergy() < 75)
	{
		unit.setStrategy(0);
		return;
	}

	// If unit is a Medic
	if (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= 0)
	{
		unit.setStrategy(0);
		return;
	}

	// If a unit is a Reaver
	if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && unit.getGroundRange() >= unit.getPosition().getDistance(unit.getTargetPosition()))
	{
		unit.setStrategy(1);
		return;
	}

	// If unit is a melee ground unit
	if (!unit.getType().isFlyer() && max(unit.getGroundRange(), unit.getAirRange()) <= 32)
	{
		// Force to stay on tanks
		if ((target.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || target.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && unit.getPosition().getDistance(unit.getTargetPosition()) < 128)
		{
			unit.setStrategy(1);
			return;
		}

		// Avoid attacking mines unless it is a Dark Templar or a floating unit
		if (target.getType() == UnitTypes::Terran_Vulture_Spider_Mine && !unit.getType().isWorker() && unit.getType() != UnitTypes::Terran_Vulture && unit.getType() != UnitTypes::Protoss_Archon && unit.getType() != UnitTypes::Protoss_Dark_Archon && unit.getType() != UnitTypes::Protoss_Dark_Templar)
		{
			unit.setStrategy(0);
			return;
		}
	}

	// If last command was engage
	if (unit.getStrategy() == 1)
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() < 1.0 - offset) || (unit.getType().isFlyer() && unit.getAirLocal() < 1.0 - offset))
		{
			unit.setStrategy(0);
			return;
		}
		else
		{
			unit.setStrategy(1);
			return;
		}
	}

	// If last command was disengage/no command
	else
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() > 1.0 + offset) || (unit.getType().isFlyer() && unit.getAirLocal() > 1.0 + offset))
		{
			unit.setStrategy(1);
			return;
		}
		else
		{
			unit.setStrategy(0);
			return;
		}
	}


	// Disregard local if no target, no recent local calculation and not within ally region
	unit.setStrategy(3);
	return;
}

void UnitTrackerClass::updateGlobalSimulation()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Strategy().isPlayPassive())	globalStrategy = 0;
		else if (globalAllyStrength > globalEnemyStrength) globalStrategy = 1;
		else if (Players().getPlayers().size() <= 1 && Players().getNumberTerran() > 0)	globalStrategy = 1;
		else globalStrategy = 0;
		return;
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters))
		{
			if (globalAllyStrength > globalEnemyStrength)
			{
				globalStrategy = 1;
				return;
			}
			else
			{
				globalStrategy = 0;
				return;
			}
		}
		else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && BuildOrder().getCurrentBuild() == "Sparks")
		{
			globalStrategy = 1;
			return;
		}
		else
		{
			globalStrategy = 2;
			return;
		}
	}
	globalStrategy = 1;
	return;
}

UnitInfo& UnitTrackerClass::getAllyUnit(Unit unit)
{
	if (allyUnits.find(unit) != allyUnits.end()) return allyUnits[unit];
	assert();
	return UnitInfo();
}

UnitInfo& UnitTrackerClass::getEnemyUnit(Unit unit)
{
	if (enemyUnits.find(unit) != enemyUnits.end()) return enemyUnits[unit];
	assert();
	return UnitInfo();
}

set<Unit> UnitTrackerClass::getAllyUnitsFilter(UnitType type)
{
	returnValues.clear();
	for (auto &u : allyUnits)
	{
		UnitInfo &unit = u.second;
		if (unit.getType() == type)	returnValues.insert(unit.unit());
	}
	return returnValues;
}

void UnitTrackerClass::storeEnemy(Unit unit)
{
	enemyUnits[unit].setUnit(unit);
	enemySizes[unit->getType().size()] += 1;
	return;
}

void UnitTrackerClass::updateEnemy(UnitInfo& unit)
{
	// Update units
	auto t = unit.unit()->getType();
	auto p = unit.unit()->getPlayer();

	// Update information
	unit.setType(t);
	unit.setPosition(unit.unit()->getPosition());
	unit.setWalkPosition(Util().getWalkPosition(unit.unit()));
	unit.setTilePosition(unit.unit()->getTilePosition());
	unit.setPlayer(unit.unit()->getPlayer());

	// Update statistics
	unit.setPercentHealth(Util().getPercentHealth(unit));
	unit.setGroundRange(Util().getTrueGroundRange(unit));
	unit.setAirRange(Util().getTrueAirRange(unit));
	unit.setGroundDamage(Util().getTrueGroundDamage(unit));
	unit.setAirDamage(Util().getTrueAirDamage(unit));
	unit.setSpeed(Util().getTrueSpeed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));
	unit.setLastVisibleFrame(Broodwar->getFrameCount());

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));

	// Set last command frame
	if (unit.unit()->isStartingAttack()) unit.setLastAttackFrame(Broodwar->getFrameCount());
	return;
}

void UnitTrackerClass::storeAlly(Unit unit)
{
	if (unit->getType().isBuilding())
	{
		allyDefenses[unit].setUnit(unit);
		allyDefenses[unit].setType(unit->getType());
		allyDefenses[unit].setTilePosition(unit->getTilePosition());
		allyDefenses[unit].setPosition(unit->getPosition());
		Grids().updateDefenseGrid(allyDefenses[unit]);
	}
	else
	{
		allyUnits[unit].setUnit(unit);
	}
	return;
}

void UnitTrackerClass::updateAlly(UnitInfo& unit)
{
	auto t = unit.unit()->getType();
	auto p = unit.unit()->getPlayer();

	// Update information
	unit.setType(t);
	unit.setPosition(unit.unit()->getPosition());
	unit.setDestination(Positions::None);
	unit.setTilePosition(unit.unit()->getTilePosition());
	unit.setWalkPosition(Util().getWalkPosition(unit.unit()));
	unit.setPlayer(unit.unit()->getPlayer());

	// Update statistics
	unit.setPercentHealth(Util().getPercentHealth(unit));
	unit.setGroundRange(Util().getTrueGroundRange(unit));
	unit.setAirRange(Util().getTrueAirRange(unit));
	unit.setGroundDamage(Util().getTrueGroundDamage(unit));
	unit.setAirDamage(Util().getTrueAirDamage(unit));
	unit.setSpeed(Util().getTrueSpeed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));

	// Update target
	unit.setTarget(Targets().getTarget(unit));

	// Set last command frame
	if (unit.unit()->isStartingAttack()) unit.setLastAttackFrame(Broodwar->getFrameCount());
	return;
}