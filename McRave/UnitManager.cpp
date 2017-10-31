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
	globalEnemyGroundStrength = 0.0;
	globalAllyGroundStrength = 0.0;
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
		if (!unit.getType().isWorker() && !unit.getType().isBuilding()) unit.getType().isFlyer() ? globalEnemyAirStrength += unit.getVisibleAirStrength() : globalEnemyGroundStrength += unit.getVisibleGroundStrength(); // If unit is not a worker or building, add it to global strength	
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

		if (!unit.getType().isBuilding()) unit.getType().isFlyer() ? globalAllyAirStrength += unit.getVisibleAirStrength() : globalAllyGroundStrength += unit.getVisibleGroundStrength();
		if (unit.getType().isWorker() && Workers().getMyWorkers().find(unit.unit()) != Workers().getMyWorkers().end()) Workers().removeWorker(unit.unit()); // Remove the worker role if needed			
		if ((unit.getType().isWorker() && (Grids().getResourceGrid(unit.getTilePosition()) == 0 || Grids().getEGroundThreat(unit.getWalkPosition()) == 0.0)) || (BuildOrder().getCurrentBuild() == "Sparks" && Units().getGlobalGroundStrategy() != 1)) Workers().storeWorker(unit.unit()); // If this is a worker and is ready to go back to being a worker
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
	double unitToEngage = (8.0 * abs(Grids().getDistanceHome(unit.getWalkPosition()) - Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())))) / unit.getSpeed();
	double enemyRange, unitRange;
	UnitInfo &target = Units().getEnemyUnit(unit.getTarget());
	
	// Check every enemy unit being in range of the target
	for (auto &e : enemyUnits)
	{
		UnitInfo &enemy = e.second;

		// Ignore workers and stasised units
		if (!enemy.unit() || enemy.getType().isWorker() || (enemy.unit() && enemy.unit()->exists() && enemy.unit()->isStasised())) continue;

		unit.getType().isFlyer() ? enemyRange = enemy.getAirRange() + (enemy.getType().width() / 2.0) : enemyRange = enemy.getGroundRange() + (enemy.getType().width() / 2.0);
		enemy.getType().isFlyer() ? unitRange = unit.getAirRange() + (unit.getType().width() / 2.0) : unitRange = unit.getGroundRange() + (unit.getType().width() / 2.0);

		double enemyToEngage = 0.0;
		double distance = enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange;

		if (Grids().getDistanceHome(enemy.getWalkPosition()) > 0 && Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())) > 0)
		{
			distance = (8.0 * abs(Grids().getDistanceHome(enemy.getWalkPosition()) - Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())))) - enemyRange;
		}

		if (enemy.getSpeed() > 0.0)
		{
			enemyToEngage = max(0.0, distance / enemy.getSpeed());
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else
		{
			enemyToEngage = 0.0;
			enemy.getPosition().getDistance(unit.getEngagePosition()) <= enemyRange ? simRatio = simulationTime : simRatio = 0.0;
		}

		// Situations where an enemy should be treated as stronger than it actually is
		if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected()) simRatio = simRatio * 5.0;
		if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))	simRatio = simRatio * 2.0;
		//if (!enemy.unit()->exists()) simRatio = simRatio * (1.0 + double(Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) / 5000);

		enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
		enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
	}

	// Check every ally being in range of the target
	for (auto &a : allyUnits)
	{
		UnitInfo &ally = a.second;
		double allyToEngage = 0.0;
		double allyRange = (ally.getType().width() / 2.0) + target.getType().isFlyer() ? ally.getAirRange() : ally.getGroundRange();
		double distanceA = ally.getPosition().getDistance(ally.getEngagePosition());
		double distanceB = ally.getPosition().getDistance(unit.getEngagePosition());

		if (Grids().getDistanceHome(ally.getWalkPosition()) > 0 && Grids().getDistanceHome(WalkPosition(ally.getEngagePosition())) > 0)
		{
			distanceA = (8.0 * abs(Grids().getDistanceHome(ally.getWalkPosition()) - Grids().getDistanceHome(WalkPosition(ally.getEngagePosition()))));
		}
		if (Grids().getDistanceHome(ally.getWalkPosition()) > 0 && Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())) > 0)
		{
			distanceB = (8.0 * abs(Grids().getDistanceHome(ally.getWalkPosition()) - Grids().getDistanceHome(WalkPosition(unit.getEngagePosition())))) - allyRange;
		}

		if (distanceB / ally.getSpeed() > simulationTime) continue;

		//if (distance / ally.getSpeed() > simulationTime) continue;
		allyToEngage = max(0.0, distanceB / ally.getSpeed());
		simRatio = max(0.0, simulationTime - (allyToEngage - unitToEngage));

		if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(ally.getEngagePosition())) == 0) simRatio = simRatio * 5.0;
		if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTargetPosition())))	simRatio = simRatio * 2.0;

		allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
		allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;
	}

	// Store the difference of strengths
	enemyLocalGroundStrength > 0.0 ? unit.setGroundLocal(allyLocalGroundStrength / enemyLocalGroundStrength) : unit.setGroundLocal(2.0);
	enemyLocalAirStrength > 0.0 ? unit.setAirLocal(allyLocalAirStrength / enemyLocalAirStrength) : unit.setAirLocal(2.0);
	return;
}

void UnitTrackerClass::updateStrategy(UnitInfo& unit)
{
	// Latch based engagement decision making based on what race we are playing
	double minThreshold = 1.0, maxThreshold = 1.0;
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Players().getNumberZerg() > 0) minThreshold = 0.8, maxThreshold = 1.2;
		if (Players().getNumberTerran() > 0) minThreshold = 0.6, maxThreshold = 1.0;
	}
	else
	{
		if (Players().getNumberZerg() > 0) minThreshold = 1.1, maxThreshold = 1.3;
		if (Players().getNumberProtoss() > 0) minThreshold = 1.1, maxThreshold = 1.3;
	}

	UnitInfo &target = Units().getEnemyUnit(unit.getTarget());
	double decisionLocal = unit.getType().isFlyer() ? unit.getAirLocal() : unit.getGroundLocal();
	double decisionGlobal = unit.getType().isFlyer() ? globalAirStrategy : globalGroundStrategy;

	// If a unit is clearly out of range, set as "no local" and skip calculating
	if (!unit.getTarget())
	{
		unit.setStrategy(3);
		return;
	}

	// If unit is a worker, always fight
	if (unit.getType().isWorker() && unit.getTarget()->exists())
	{
		unit.setStrategy(1);
		return;
	}

	// Stupid way to hardcode only aggresion from sparks build
	if (BuildOrder().getCurrentBuild() == "Sparks" && !BuildOrder().isOpener())
	{
		if (unit.getTarget()->exists())
		{
			unit.setStrategy(1);
			return;
		}
		else
		{
			unit.setStrategy(3);
			return;
		}
	}	

	// If unit is in ally territory
	if (Terrain().isInAllyTerritory(unit.unit()) && decisionGlobal == 0)
	{
		if (!unit.getTarget()->exists())
		{
			unit.setStrategy(2);
			return;
		}
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
			if (Terrain().isInAllyTerritory(target.unit()) || unit.getPosition().getDistance(unit.getTargetPosition()) < unit.getGroundRange())
			{
				unit.setStrategy(1);
				return;
			}
			else if (unit.getPosition().getDistance(unit.getTargetPosition()) > 640)
			{
				unit.setStrategy(2);
				return;
			}
			else
			{
				unit.setStrategy(0);
				return;
			}
		}
	}

	if (unit.getPosition().getDistance(unit.getTargetPosition()) > 640.0)
	{
		unit.setStrategy(3);
		return;
	}

	// If unit is a High Templar and low energy, retreat	
	if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.unit()->getEnergy() < 75)
	{
		unit.setStrategy(0);
		return;
	}

	// If unit is a Medic and no energy, retreat	
	if (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= 0)
	{
		unit.setStrategy(0);
		return;
	}

	// If a unit is a Reaver and within range of an enemy
	if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && unit.getGroundRange() >= unit.getPosition().getDistance(unit.getTargetPosition()))
	{
		unit.setStrategy(1);
		return;
	}

	// If unit is a melee ground unit
	if (!unit.getType().isFlyer() && max(unit.getGroundRange(), unit.getAirRange()) <= 32 && (target.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || target.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && unit.getPosition().getDistance(unit.getTargetPosition()) < 128)
	{
		unit.setStrategy(1);
		return;
	}

	// Avoid attacking mines unless it is a Dark Templar or a floating unit
	if (!unit.getType().isFlyer() && max(unit.getGroundRange(), unit.getAirRange()) <= 32 && target.getType() == UnitTypes::Terran_Vulture_Spider_Mine && !unit.getType().isWorker() && unit.getType() != UnitTypes::Terran_Vulture && unit.getType() != UnitTypes::Protoss_Archon && unit.getType() != UnitTypes::Protoss_Dark_Archon && unit.getType() != UnitTypes::Protoss_Dark_Templar)
	{
		unit.setStrategy(0);
		return;
	}

	// If last command was engage
	if (unit.getStrategy() == 1)
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() < minThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() < minThreshold))
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
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() > maxThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() > maxThreshold))
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


	//else if (unit.getStrategy() == 1 && ((!unit.getType().isFlyer() && unit.getGroundLocal() < 1.0 - offset) || (unit.getType().isFlyer() && unit.getAirLocal() < 1.0 - offset))) unit.setStrategy(0); // If last command was engage
	//else if ((!unit.getType().isFlyer() && unit.getGroundLocal() > 1.0 + offset) || (unit.getType().isFlyer() && unit.getAirLocal() > 1.0 + offset)) unit.setStrategy(1); // If last command was disengage	
}

void UnitTrackerClass::updateGlobalSimulation()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		double offset = 1.0;
		if (Players().getNumberZerg() > 0) offset = 1.1;
		if (Players().getNumberTerran() > 0) offset = 0.8;

		if (Strategy().isPlayPassive())	globalGroundStrategy = 0;
		else if (globalAllyGroundStrength >= globalEnemyGroundStrength * offset) globalGroundStrategy = 1;
		else if (Players().getPlayers().size() <= 1 && Players().getNumberTerran() > 0)	globalGroundStrategy = 1;
		else globalGroundStrategy = 0;

		if (Strategy().isPlayPassive())	globalAirStrategy = 0;
		else if (globalAllyAirStrength >= globalEnemyAirStrength * offset) globalAirStrategy = 1;
		else if (Players().getPlayers().size() <= 1 && Players().getNumberTerran() > 0)	globalAirStrategy = 1;
		else globalAirStrategy = 0;
		return;
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) > 0)
		{
			if (globalAllyGroundStrength > globalEnemyGroundStrength)
			{
				globalGroundStrategy = 1;
				return;
			}
			else
			{
				globalGroundStrategy = 0;
				return;
			}
		}
		else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && BuildOrder().getCurrentBuild() == "Sparks")
		{
			globalGroundStrategy = 1;
			return;
		}
		else
		{
			globalGroundStrategy = 0;
			return;
		}
	}
	globalGroundStrategy = 1;
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