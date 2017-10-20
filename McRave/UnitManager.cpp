#include "McRave.h"

void UnitTrackerClass::update()
{
	Display().startClock();
	updateAliveUnits();
	updateDeadUnits();
	updateGlobalCalculations();
	Display().performanceTest(__FUNCTION__);
	return;
}

void UnitTrackerClass::onUnitDiscover(Unit unit)
{
	if (unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		Units().storeEnemy(unit);
	}
	return;
}

void UnitTrackerClass::onUnitCreate(Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		// Store supply if it costs supply
		if (unit->getType().supplyRequired() > 0)
		{
			supply += unit->getType().supplyRequired();
		}

		// Store Buildings, Bases, Pylons
		if (unit->getType().isResourceDepot())
		{
			Bases().storeBase(unit);
			Buildings().storeBuilding(unit);
		}
		else if (unit->getType() == UnitTypes::Protoss_Pylon)
		{
			Pylons().storePylon(unit);
			Buildings().storeBuilding(unit);
		}
		else if (unit->getType() == UnitTypes::Protoss_Photon_Cannon)
		{
			storeAlly(unit);
		}
		else if (unit->getType().isBuilding())
		{
			Buildings().storeBuilding(unit);
		}
	}
	return;
}

void UnitTrackerClass::onUnitDestroy(Unit unit)
{
	if (allyUnits.find(unit) != allyUnits.end())
	{
		allyUnits[unit].setDeadFrame(Broodwar->getFrameCount());
		allySizes[unit->getType().size()] -= 1;

		if (allyUnits[unit].getTransport())
		{
			Transport().removeUnit(unit);
		}
	}
	else if (allyDefenses.find(unit) != allyDefenses.end())
	{
		Grids().updateDefenseGrid(allyDefenses[unit]);
		allyDefenses.erase(unit);
	}
	else if (enemyUnits.find(unit) != enemyUnits.end())
	{
		enemyUnits[unit].setDeadFrame(Broodwar->getFrameCount());
		enemySizes[unit->getType().size()] -= 1;
	}

	if (unit->getPlayer() == Broodwar->self())
	{
		// Store supply if it costs supply
		if (unit->getType().supplyRequired() > 0)
		{
			supply -= unit->getType().supplyRequired();
		}
		if (unit->getType().isResourceDepot())
		{
			Bases().removeBase(unit);
			Buildings().removeBuilding(unit);
		}
		else if (unit->getType().isBuilding())
		{
			Buildings().removeBuilding(unit);
		}
		else if (unit->getType().isWorker())
		{
			Workers().removeWorker(unit);
		}
		else if (unit->getType() == UnitTypes::Protoss_Shuttle)
		{
			Transport().removeUnit(unit);
		}
	}
	if (unit->getType().isResourceContainer() && unit->getType().isMineralField())
	{
		Resources().removeResource(unit);
	}
	return;
}

void UnitTrackerClass::onUnitMorph(Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		// Zerg morphing
		if (unit->getType().getRace() == Races::Zerg)
		{
			if (unit->getType() == UnitTypes::Zerg_Egg || unit->getType() == UnitTypes::Zerg_Lurker_Egg)
			{
				// Figure out how morphing works
			}
			else if (unit->getType().isBuilding() && Workers().getMyWorkers().find(unit) != Workers().getMyWorkers().end())
			{
				Workers().removeWorker(unit);
				Buildings().storeBuilding(unit);
				supply -= 2;
			}
			else if (unit->getType().isWorker())
			{
				Workers().storeWorker(unit);
			}
			else if (!unit->getType().isWorker() && !unit->getType().isBuilding())
			{
				storeAlly(unit);
			}
		}

		// Protoss morphing
		if (unit->getType() == UnitTypes::Protoss_Archon)
		{
			// Remove the two HTs and their respective supply
			// whatBuilds returns previous units size
		}
	}
	else if (unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		if (enemyUnits.find(unit) != enemyUnits.end())
		{
			enemyUnits[unit].setType(unit->getType());
		}
		else
		{
			storeEnemy(unit);
		}
	}
	else if (unit->getType().isResourceContainer())
	{
		Resources().storeResource(unit);
	}
}

void UnitTrackerClass::onUnitComplete(Unit unit)
{
	// Don't store useless things
	if (unit->getType() == UnitTypes::Protoss_Scarab || unit->getType() == UnitTypes::Zerg_Larva)
	{
		return;
	}

	if (unit->getPlayer() == Broodwar->self())
	{
		allySizes[unit->getType().size()] += 1;

		// Store Workers, Units, Bases(update base grid?)
		if (unit->getType().isWorker())
		{
			Workers().storeWorker(unit);
		}

		if (unit->getType() == UnitTypes::Protoss_Shuttle || unit->getType() == UnitTypes::Terran_Dropship) //|| unit->getType() == UnitTypes::Zerg_Overlord
		{
			Transport().storeUnit(unit);
		}
		else if (!unit->getType().isWorker() && !unit->getType().isBuilding())
		{
			storeAlly(unit);
		}
	}
	else if (unit->getType().isResourceContainer())
	{
		Resources().storeResource(unit);
	}
}

void UnitTrackerClass::updateAliveUnits()
{
	// Reset global strengths
	globalEnemyStrength = 0.0;
	globalAllyStrength = 0.0;
	allyDefense = 0.0;
	enemyDefense = 0.0;

	// Update Enemy Units
	for (auto &e : enemyUnits)
	{
		UnitInfo &enemy = e.second;

		if (!enemy.unit())
		{
			continue;
		}

		// If deadframe is 0, unit is alive still
		if (enemy.getDeadFrame() == 0)
		{
			if (enemy.unit()->exists())
			{
				updateEnemy(enemy);
			}
			enemyComposition[enemy.getType()] += 1;

			// If tile is visible but unit is not, remove position
			if (!enemy.unit()->exists() && enemy.getPosition() != Positions::None && Broodwar->isVisible(TilePosition(enemy.getPosition())))
			{
				enemy.setPosition(Positions::None);
			}

			// Strength based calculations ignore workers and buildings
			if (!enemy.getType().isWorker() && !enemy.getType().isBuilding())
			{
				globalEnemyStrength += max(enemy.getVisibleGroundStrength(), enemy.getVisibleAirStrength());
			}
			if (enemy.getType().isBuilding() && enemy.getGroundDamage() > 0 && enemy.unit()->isCompleted())
			{
				enemyDefense += enemy.getVisibleGroundStrength();
			}

			// Set last command frame
			if (enemy.unit()->isStartingAttack())
			{
				enemy.setLastAttackFrame(Broodwar->getFrameCount());
			}
		}

		// If unit is dead
		else if (enemy.getDeadFrame() != 0)
		{
			// Add a portion of the strength to ally strength
			//globalAllyStrength += max(enemy.getMaxGroundStrength(), enemy.getMaxAirStrength()) * 1 / (1.0 + 0.001*(double(Broodwar->getFrameCount()) - double(enemy.getDeadFrame())));
		}
	}

	// Update Ally Defenses
	for (auto &a : allyDefenses)
	{
		UnitInfo &ally = a.second;
		if (ally.getDeadFrame() == 0)
		{
			updateAlly(ally);
			allyDefense += ally.getMaxGroundStrength();
		}
	}

	// Update Ally Units
	for (auto &a : allyUnits)
	{
		UnitInfo &ally = a.second;

		if (!ally.unit())
		{
			continue;
		}

		// If deadframe is 0, unit is alive still
		if (ally.getDeadFrame() == 0)
		{
			updateAlly(ally);

			if (ally.getType().isWorker() && (Grids().getResourceGrid(ally.getTilePosition()) == 0 || Grids().getEGroundThreat(ally.getWalkPosition()) == 0.0))
			{
				Workers().storeWorker(ally.unit());
				ally.setDeadFrame(Broodwar->getFrameCount());
				continue;
			}

			if (ally.getStrategy() == 1)
			{
				fudgeWin++;
			}
			else if (ally.getStrategy() == 0)
			{
				fudgeLoss++;
			}

			globalAllyStrength += max(ally.getVisibleGroundStrength(), ally.getVisibleAirStrength());

			// Set last command frame
			if (ally.unit()->isStartingAttack())
			{
				ally.setLastAttackFrame(Broodwar->getFrameCount());
			}
		}
		else
		{
			//globalEnemyStrength += max(ally.getMaxGroundStrength(), ally.getMaxAirStrength()) * 0.5 / (1.0 + 0.001*(double(Broodwar->getFrameCount()) - double(ally.getDeadFrame())));
		}
	}
	return;
}

void UnitTrackerClass::updateDeadUnits()
{
	// Check for decayed ally units
	for (map<Unit, UnitInfo>::iterator itr = allyUnits.begin(); itr != allyUnits.end();)
	{
		if (itr->second.getDeadFrame() != 0 && (itr->second.getDeadFrame() + 500 < Broodwar->getFrameCount() || itr->second.getType().isWorker()))
		{
			itr = allyUnits.erase(itr);
		}
		else
		{
			++itr;
		}
	}
	// Check for decayed enemy units
	for (map<Unit, UnitInfo>::iterator itr = enemyUnits.begin(); itr != enemyUnits.end();)
	{
		if ((*itr).second.getDeadFrame() != 0 && (*itr).second.getDeadFrame() + 500 < Broodwar->getFrameCount())
		{
			itr = enemyUnits.erase(itr);
		}
		else
		{
			++itr;
		}
	}
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
	
	// Update calculations
	unit.setTarget(Targets().getTarget(unit));
	getLocalCalculation(unit);
	return;
}

void UnitTrackerClass::getLocalCalculation(UnitInfo& unit)
{
	// Variables for calculating local strengths
	double enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
	double enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
	double simulationTime = 5.0;
	double simRatio = 0.0;
	double unitToEngage = (unit.getPosition().getDistance(unit.getEngagePosition())) / unit.getSpeed();
	double enemyRange, allyRange, unitRange;
	UnitInfo &target = Units().getEnemyUnit(unit.getTarget());

	// Check every enemy unit being in range of the target
	for (auto &e : enemyUnits) 
	{
		UnitInfo &enemy = e.second;
		double enemyToEngage = 0.0;

		// Ignore workers and stasised units
		if (!enemy.unit() || enemy.getType().isWorker() || (enemy.unit() && enemy.unit()->exists() && enemy.unit()->isStasised())) continue;

		unit.getType().isFlyer() ? enemyRange = enemy.getAirRange() : enemyRange = enemy.getGroundRange();
		enemy.getType().isFlyer() ? unitRange = unit.getAirRange() : unitRange = unit.getGroundRange();

		if (enemy.getSpeed() > 0.0)
		{			
			enemyToEngage = max(0.0, (enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange) / enemy.getSpeed());
			simRatio = max(0.0, simulationTime - (enemyToEngage - unitToEngage));
		}
		else if (enemyRange > unitRange)
		{
			enemy.getPosition().getDistance(unit.getEngagePosition()) <= enemyRange ? simRatio = simulationTime : simRatio = 0.0;
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

		if (8.0 * abs(Grids().getDistanceHome(ally.getWalkPosition()) - Grids().getDistanceHome(unit.getWalkPosition())) / unit.getSpeed() > 5.0) continue;
		
		target.getType().isFlyer() ? allyRange = ally.getAirRange() : allyRange = ally.getGroundRange();
		allyToEngage = max(0.0, (ally.getPosition().getDistance(unit.getTargetPosition()) - allyRange) / ally.getSpeed());
		simRatio = max(0.0, simulationTime - (allyToEngage - unitToEngage));

		if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(ally.getEngagePosition())) == 0) simRatio = simRatio * 5.0;

		allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
		allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;
	}

	// Store the difference of strengths
	unit.setGroundLocal(allyLocalGroundStrength - enemyLocalGroundStrength);
	unit.setAirLocal(allyLocalAirStrength - enemyLocalAirStrength);

	// If a unit is clearly out of range, set as "no local" and skip calculating
	if (unit.getPosition().getDistance(unit.getTargetPosition()) > 640.0)
	{
		unit.setStrategy(3);
		return;
	}

	// Specific passive strategy
	if (Strategy().isPlayPassive())
	{
		// Specific passive melee strategy
		if (max(unit.getGroundRange(), unit.getAirRange()) <= 32)
		{
			// If against rush and not ready to wall up, fight in mineral line
			if (Strategy().isRush() || !Strategy().isHoldChoke())
			{
				if (unit.getTarget() && unit.getTarget()->exists() && ((Grids().getResourceGrid(unit.getTarget()->getTilePosition()) > 0 && Grids().getResourceGrid(unit.getTilePosition()) > 0)))
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
				if (unit.getTarget() && unit.getTarget()->exists() && (unit.getPosition().getDistance(unit.getTargetPosition()) < 16 || unit.getType().isWorker()) && Terrain().isInAllyTerritory(unit.getTarget()) && Terrain().isInAllyTerritory(unit.unit()))
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
		// Specific passive ranged strategy
		else if (max(unit.getGroundRange(), unit.getAirRange()) > 32)
		{
			// If against rush and not ready to wall up, fight in mineral line
			if (Strategy().isRush() && !Strategy().isHoldChoke())
			{
				if (unit.getTarget() && unit.getTarget()->exists() && ((Grids().getResourceGrid(unit.getTarget()->getTilePosition()) > 0 || Grids().getResourceGrid(unit.getTilePosition()) > 0)))
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
				if (unit.getTarget() && unit.getTarget()->exists() && ((Terrain().isInAllyTerritory(unit.unit()) && unit.getPosition().getDistance(unit.getTargetPosition()) <= max(unit.getGroundRange(), unit.getAirRange())) || (Terrain().isInAllyTerritory(unit.getTarget()) && !unit.getTarget()->getType().isWorker())))
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

	// Specific High Templar strategy
	if (unit.getType() == UnitTypes::Protoss_High_Templar)
	{
		if (unit.unit()->getEnergy() < 75)
		{
			unit.setStrategy(0);
			return;
		}
	}

	// Specific Medic strategy
	if (unit.getType() == UnitTypes::Terran_Medic)
	{
		if (unit.unit()->getEnergy() <= 0)
		{
			unit.setStrategy(0);
			return;
		}
	}

	// Specific ground unit strategy
	if (!unit.getType().isFlyer())
	{
		// Specific melee strategy
		if (max(unit.getGroundRange(), unit.getAirRange()) <= 32)
		{
			if (unit.getTarget() && unit.getTarget()->exists())
			{
				// Force to stay on tanks
				if ((unit.getTarget()->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getTarget()->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && unit.getPosition().getDistance(unit.getTargetPosition()) < 128)
				{
					unit.setStrategy(1);
					return;
				}

				// Avoid attacking mines
				if (unit.getTarget()->getType() == UnitTypes::Terran_Vulture_Spider_Mine && !unit.getType().isWorker() && unit.getType() != UnitTypes::Terran_Vulture && unit.getType() != UnitTypes::Protoss_Archon && unit.getType() != UnitTypes::Protoss_Dark_Archon)
				{
					unit.setStrategy(0);
					return;
				}
			}
		}

		// If an enemy is within ally territory, engage
		if (unit.getTarget() && unit.getTarget()->exists() && theMap.GetArea(unit.getTarget()->getTilePosition()) && Terrain().isInAllyTerritory(unit.getTarget()))
		{
			unit.setStrategy(1);
			return;
		}

		// If a Reaver is in range of something, engage it
		if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && unit.getGroundRange() >= unit.getPosition().getDistance(unit.getTargetPosition()))
		{
			unit.setStrategy(1);
			return;
		}
	}

	int offset = 1.0;
	if (Broodwar->self()->getRace() == Races::Protoss) offset = 0.2;
	else if (Broodwar->self()->getRace() == Races::Terran) offset = 0.5;

	// If last command was engage
	if (unit.getStrategy() == 1)
	{
		// Latch based system for at least 80% disadvantage to disengage
		if ((!unit.getType().isFlyer() && allyLocalGroundStrength < enemyLocalGroundStrength*(1.0 - offset)) || (unit.getType().isFlyer() && allyLocalAirStrength < enemyLocalAirStrength*(1.0 - offset)))
		{
			unit.setStrategy(0);
			return;
		}
		else
		{
			return;
		}
	}

	// If last command was disengage/no command
	else
	{
		// Latch based system for at least 120% advantage to engage
		if ((!unit.getType().isFlyer() && allyLocalGroundStrength > enemyLocalGroundStrength*(1.0 + offset)) || (unit.getType().isFlyer() && allyLocalAirStrength > enemyLocalAirStrength*(1.0 + offset)))
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

void UnitTrackerClass::updateGlobalCalculations()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		// If we're playing passive, don't attempt to move out
		if (Strategy().isPlayPassive())
		{
			globalStrategy = 2;
			return;
		}

		// If we're ahead, move out
		if (globalAllyStrength > globalEnemyStrength)
		{
			globalStrategy = 1;
			return;
		}
		// If there's only one enemy left and it's a Terran, contain him
		else if (Players().getPlayers().size() <= 1 && Players().getNumberTerran() > 0)
		{
			globalStrategy = 1;
			return;
		}
		// Else, retreat, useful to check number of enemy players left for multiplayer games
		else
		{
			globalStrategy = 0;
			return;
		}
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
	if (allyUnits.find(unit) != allyUnits.end())
	{
		return allyUnits[unit];
	}
	assert();
	return UnitInfo();
}

UnitInfo& UnitTrackerClass::getEnemyUnit(Unit unit)
{
	if (enemyUnits.find(unit) != enemyUnits.end())
	{
		return enemyUnits[unit];
	}
	assert();
	return UnitInfo();
}

set<Unit> UnitTrackerClass::getAllyUnitsFilter(UnitType type)
{
	returnValues.clear();
	for (auto &u : allyUnits)
	{
		UnitInfo &unit = u.second;
		if (unit.getType() == type)
		{
			returnValues.insert(unit.unit());
		}
	}
	return returnValues;
}