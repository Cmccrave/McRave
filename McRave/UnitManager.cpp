#include "McRave.h"

void UnitTrackerClass::update()
{
	Display().startClock();
	updateUnits();
	updateGlobalSimulation();
	Display().performanceTest(__FUNCTION__);
}

void UnitTrackerClass::updateUnits()
{
	// Reset global strengths
	globalEnemyGroundStrength = 0.0;
	globalAllyGroundStrength = 0.0;
	immThreat = 0.0;
	proxThreat = 0.0;
	allyDefense = 0.0;
	enemyDefense = 0.0;

	// Latch based engagement decision making based on what race we are playing	
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Players().getNumberZerg() > 0 || Players().getNumberProtoss() > 0) minThreshold = 0.8, maxThreshold = 1.2;
		if (Players().getNumberTerran() > 0) minThreshold = 0.6, maxThreshold = 1.4;
		if (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() == Races::Terran) minThreshold = 0.6, maxThreshold = 1.0;
		if (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() != Races::Terran) minThreshold = 0.8, maxThreshold = 1.2;
	}
	else
	{
		if (Players().getNumberTerran() > 0) minThreshold = 0.8, maxThreshold = 1.2;
		if (Players().getNumberZerg() > 0 || Players().getNumberProtoss() > 0) minThreshold = 1.0, maxThreshold = 1.4;
	}

	// Update Enemy Units
	for (auto &u : enemyUnits)
	{
		UnitInfo &unit = u.second;
		if (!unit.unit()) continue; // Ignore improper storage if it happens		
		if (unit.unit()->exists() && unit.unit()->isVisible())	updateEnemy(unit); // If unit is visible, update it
		if (!unit.unit()->exists() && unit.getPosition().isValid() && Broodwar->isVisible(unit.getTilePosition())) unit.setPosition(Positions::None); // If unit is not visible but his position is, move it
		if (unit.getType().isValid()) enemyComposition[unit.getType()] += 1; // If unit has a valid type, update enemy composition tracking
		if (!unit.getType().isBuilding() && !unit.getType().isWorker()) unit.getType().isFlyer() ? globalEnemyAirStrength += unit.getVisibleAirStrength() : globalEnemyGroundStrength += unit.getVisibleGroundStrength(); // If unit is not a worker or building, add it to global strength	
		if ((Terrain().isInAllyTerritory(unit.getTilePosition()) || unit.getPosition().getDistance(Terrain().getDefendPosition()) < 64) && (!unit.getType().isWorker() || unit.getLastAttackFrame() > 0)) proxThreat += unit.getVisibleGroundStrength();
		if (Grids().getBaseGrid(unit.getTilePosition()) > 0 && (!unit.getType().isWorker() || unit.getLastAttackFrame() > 0)) immThreat += unit.getVisibleGroundStrength();
		if (unit.getType().isBuilding() && unit.getGroundDamage() > 0 && unit.unit()->isCompleted()) enemyDefense += unit.getVisibleGroundStrength(); // If unit is a building and deals damage, add it to global defense		
	}

	// Update Ally Defenses
	for (auto &u : allyDefenses)
	{
		UnitInfo &unit = u.second;
		updateAlly(unit);
		if (unit.unit()->isCompleted()) allyDefense += unit.getMaxGroundStrength();
	}

	// Update Ally Units
	for (auto &u : allyUnits)
	{
		UnitInfo &unit = u.second;
		if (!unit.unit()) continue;
		updateAlly(unit);
		updateLocalSimulation(unit);
		updateStrategy(unit);

		if (unit.getType().isWorker() && Workers().getMyWorkers().find(unit.unit()) != Workers().getMyWorkers().end()) Workers().removeWorker(unit.unit()); // Remove the worker role if needed	
		if (unit.getType().isWorker() && !Util().shouldPullWorker(unit.unit())) Workers().storeWorker(unit.unit()); // If this is a worker and is ready to go back to being a worker
		if (!unit.getType().isBuilding()) unit.getType().isFlyer() ? globalAllyAirStrength += unit.getVisibleAirStrength() : globalAllyGroundStrength += unit.getVisibleGroundStrength();
	}
}

void UnitTrackerClass::updateLocalSimulation(UnitInfo& unit)
{
	// Variables for calculating local strengths
	double enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
	double enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
	double unitToEngage = max(0.0, unit.getPosition().getDistance(unit.getEngagePosition()) / unit.getSpeed());
	double simulationTime = unitToEngage + 5.0;
	double offset = 0.0, maxRange = 0.0;
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());

	// Check every enemy unit being in range of the target
	for (auto &e : enemyUnits)
	{
		UnitInfo &enemy = e.second;
		// Ignore workers and stasised units
		if (!enemy.unit() || enemy.getType().isWorker() || (enemy.unit() && enemy.unit()->exists() && enemy.unit()->isStasised())) continue;

		double widths = enemy.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
		double enemyRange = widths + (unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange());
		double unitRange = widths + (enemy.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
		double enemyToEngage = 0.0;
		double distance = max(0.0, enemy.getPosition().getDistance(unit.getPosition()) - enemyRange);
		double simRatio = 0.0;

		if (enemy.getSpeed() > 0.0)
		{
			enemyToEngage = max(0.0, (distance - widths) / enemy.getSpeed());
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else if (enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths <= 0.0)
		{
			enemyToEngage = max(0.0, (unit.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths) / unit.getSpeed());
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else
		{
			continue;
		}

		// Situations where an enemy should be treated as stronger than it actually is
		if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected()) simRatio = simRatio * 5.0;
		if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition()))) simRatio = simRatio * 2.0;
		if (enemy.getLastVisibleFrame() < Broodwar->getFrameCount()) simRatio = simRatio * (1.0 + min(0.2, (double(Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) / 5000.0)));

		if (simRatio > 0.0 && enemyRange > maxRange) maxRange = enemyRange;

		enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
		enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
	}

	// Check every ally being in range of the target
	for (auto &a : allyUnits)
	{
		UnitInfo &ally = a.second;

		double widths = ally.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
		double allyRange = widths + (unit.getType().isFlyer() ? ally.getAirRange() : ally.getGroundRange());
		double distance = ally.getPosition().getDistance(ally.getEngagePosition());
		double speed = (unit.getTransport() && unit.getTransport()->exists()) ? unit.getTransport()->getType().topSpeed() * 24.0 : ally.getSpeed();

		if (ally.getPosition().getDistance(unit.getEngagePosition()) / speed > simulationTime) continue;

		offset = max(0.0, (distance - maxRange) / speed);
		double allyToEngage = max(0.0, (distance / speed) - offset);
		double simRatio = max(0.0, simulationTime - allyToEngage);

		// Situations where an ally should be treated as stronger than it actually is
		if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(ally.getEngagePosition())) == 0) simRatio = simRatio * 5.0;
		if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTargetPosition())))	simRatio = simRatio * 2.0;

		allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
		allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;
	}

	// Store the difference of strengths
	enemyLocalGroundStrength > 0.0 ? unit.setGroundLocal(allyLocalGroundStrength / enemyLocalGroundStrength) : unit.setGroundLocal(2.0);
	enemyLocalAirStrength > 0.0 ? unit.setAirLocal(allyLocalAirStrength / enemyLocalAirStrength) : unit.setAirLocal(2.0);
}

void UnitTrackerClass::updateStrategy(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());
	double widths = target.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
	double allyRange = widths + (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
	double enemyRange = widths + (unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange());

	// If unit does not have a target or clearly out of range of sim target, set as "no local"
	if (!unit.getTarget() || (unit.getPosition().getDistance(unit.getSimPosition()) > 640.0 && isAhead(unit))) unit.setStrategy(3);

	else if ((Terrain().isInAllyTerritory(unit.getTilePosition()) && isBehind(unit)) || (Terrain().isInAllyTerritory(target.getTilePosition()))) // If unit is in ally territory
	{
		if (!Strategy().isHoldChoke())
		{
			if (Grids().getBaseGrid(target.getTilePosition()) > 0 && isThreatening(target)) unit.setStrategy(1);
			else if (allyRange > enemyRange && Terrain().isInAllyTerritory(target.getTilePosition())) unit.setStrategy(1);
			else unit.setStrategy(2);
		}
		else
		{
			if ((Terrain().isInAllyTerritory(target.getTilePosition()) && (isThreatening(target)) || isAhead(unit)) || Util().unitInRange(unit)) unit.setStrategy(1);
			else if (!Util().targetInRange(unit) || allyRange >= enemyRange) unit.setStrategy(2);
			else unit.setStrategy(0);
		}
	}

	else if (shouldRetreat(unit)) unit.setStrategy(0);
	else if (shouldDefend(unit)) unit.setStrategy(2);
	else if (shouldAttack(unit)) unit.setStrategy(1);

	else if (unit.getStrategy() == 1) // If last command was engage
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() < minThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() < minThreshold)) unit.setStrategy(0);
		else unit.setStrategy(1);
	}
	else // If last command was disengage/no command
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() > maxThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() > maxThreshold)) unit.setStrategy(1);
		else unit.setStrategy(0);
	}
}

bool UnitTrackerClass::shouldAttack(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());

	if (unit.getType().isWorker() && unit.getTarget()->exists()) return true; // If unit is a worker, always fight
	else if (Grids().getBaseGrid(target.getTilePosition()) > 0 && (!target.getType().isWorker() || Broodwar->getFrameCount() - target.getLastAttackFrame() < 500)) return true;
	else if ((unit.unit()->isCloaked() || unit.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(unit.getEngagePosition())) <= 0) return true;
	else if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util().unitInRange(unit)) return true; // If a unit is a Reaver and within range of an enemy	
	else if ((target.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || target.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && unit.getPosition().getDistance(unit.getTargetPosition()) < 128) return true; // If unit is close to a tank, keep attacking it
	return false;
}

bool UnitTrackerClass::shouldRetreat(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());

	if (unit.getType() == UnitTypes::Protoss_Zealot && target.getType() == UnitTypes::Terran_Vulture && Grids().getMobilityGrid(target.getWalkPosition()) > 6 && Grids().getAntiMobilityGrid(target.getWalkPosition()) < 2) return true; // If unit is a Zealot, don't chase Vultures TODO -- check for mobility
	else if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.unit()->getEnergy() < 75) return true; // If unit is a High Templar and low energy, retreat	
	else if (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= 0) return true; // If unit is a Medic and no energy, retreat	
	else if ((target.unit()->isCloaked() || target.unit()->isBurrowed()) && !target.unit()->isDetected()) return true;
	return false;
}

bool UnitTrackerClass::shouldDefend(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());

	if (unit.getType().isFlyer() && Players().getNumberZerg() > 0 && globalEnemyAirStrength > 0.0 && Broodwar->self()->getUpgradeLevel(BuildOrder().getFirstUpgrade()) <= 0) return true;
	else if (!unit.getType().isFlyer() && Players().getNumberZerg() > 0 && BuildOrder().isForgeExpand() && globalEnemyGroundStrength > 0.0 && Broodwar->self()->getUpgradeLevel(BuildOrder().getFirstUpgrade()) <= 0) return true;
	else if (Strategy().isPlayPassive() && !Terrain().isInAllyTerritory(unit.getTilePosition())) return true;
	return false;
}

bool UnitTrackerClass::isBehind(UnitInfo& unit)
{
	double decisionLocal = unit.getType().isFlyer() ? unit.getAirLocal() : unit.getGroundLocal();
	double decisionGlobal = unit.getType().isFlyer() ? globalAirStrategy : globalGroundStrategy;
	return (decisionGlobal == 0 || (!unit.getType().isFlyer() && unit.getGroundLocal() <= minThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() <= minThreshold));
}

bool UnitTrackerClass::isAhead(UnitInfo& unit)
{
	double decisionLocal = unit.getType().isFlyer() ? unit.getAirLocal() : unit.getGroundLocal();
	double decisionGlobal = unit.getType().isFlyer() ? globalAirStrategy : globalGroundStrategy;
	return (decisionGlobal == 1 || (!unit.getType().isFlyer() && unit.getGroundLocal() > minThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() > minThreshold));
}

bool UnitTrackerClass::isThreatening(UnitInfo& unit)
{
	if (!unit.getType().isWorker() || Broodwar->getFrameCount() - unit.getLastAttackFrame() < 500) return true;
	return false;
}

void UnitTrackerClass::updateGlobalSimulation()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Strategy().isPlayPassive())	globalGroundStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(BuildOrder().getFirstUpgrade()) >= 1 || Broodwar->self()->hasResearched(BuildOrder().getFirstTech()))	globalGroundStrategy = 1;
		else if (globalAllyGroundStrength < globalEnemyGroundStrength * maxThreshold) globalGroundStrategy = 0;
		else globalGroundStrategy = 1;

		if (Strategy().isPlayPassive())	globalAirStrategy = 0;
		else if ((Broodwar->self()->getUpgradeLevel(BuildOrder().getFirstUpgrade()) >= 1 || Broodwar->self()->hasResearched(BuildOrder().getFirstTech())))	globalAirStrategy = 1;
		else if (globalAllyAirStrength < globalEnemyAirStrength * maxThreshold) globalAirStrategy = 0;
		else globalAirStrategy = 1;
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		double offset = 1.0;
		if (Players().getNumberZerg() > 0) offset = 1.4;
		if (Players().getNumberProtoss() > 0) offset = 1.4;

		if (Strategy().isPlayPassive())	globalGroundStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)) globalGroundStrategy = 1;
		else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && BuildOrder().getCurrentBuild() == "Sparks") globalGroundStrategy = 1;
		else if (globalAllyGroundStrength < globalEnemyGroundStrength * offset) globalGroundStrategy = 0;
		else globalGroundStrategy = 1;

		if (Strategy().isPlayPassive())	globalAirStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)) globalAirStrategy = 1;
		else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && BuildOrder().getCurrentBuild() == "Sparks") globalGroundStrategy = 1;
		else if (globalAllyAirStrength < globalEnemyAirStrength * offset) globalAirStrategy = 0;
		else globalAirStrategy = 1;
	}
	else if (Broodwar->self()->getRace() == Races::Zerg)
	{
		double offset = 1.0;
		if (Strategy().isPlayPassive())	globalGroundStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost) >= 1)	globalGroundStrategy = 1;
		else if (globalAllyGroundStrength < globalEnemyGroundStrength * offset) globalGroundStrategy = 0;
		else globalGroundStrategy = 1;

		if (Strategy().isPlayPassive())	globalAirStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost) >= 1)	globalAirStrategy = 1;
		else if (globalAllyGroundStrength < globalEnemyGroundStrength * offset) globalAirStrategy = 0;
		else globalAirStrategy = 1;
	}
}

UnitInfo& UnitTrackerClass::getAllyUnit(Unit unit)
{
	if (allyUnits.find(unit) != allyUnits.end()) return allyUnits[unit];
	assert(0);
	return UnitInfo();
}

UnitInfo& UnitTrackerClass::getEnemyUnit(Unit unit)
{
	if (enemyUnits.find(unit) != enemyUnits.end()) return enemyUnits[unit];
	assert(0);
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

set<Unit> UnitTrackerClass::getEnemyUnitsFilter(UnitType type)
{
	returnValues.clear();
	for (auto &u : enemyUnits)
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
	if (unit->getType().isResourceDepot()) Bases().storeBase(unit);
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
	unit.setHealth(unit.unit()->getHitPoints());
	unit.setShields(unit.unit()->getShields());
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
	if (unit.unit()->isStartingAttack() || unit.unit()->isRepairing()) unit.setLastAttackFrame(Broodwar->getFrameCount());
	if (unit.unit()->getTarget() && unit.unit()->getTarget()->exists()) unit.setTarget(unit.unit()->getTarget());
	else if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.unit()->getOrderTarget()) unit.setTarget(unit.unit()->getOrderTarget());
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
	else allyUnits[unit].setUnit(unit);
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
	unit.setLastCommand(unit.unit()->getLastCommand());

	// Update statistics
	unit.setHealth(unit.unit()->getHitPoints());
	unit.setShields(unit.unit()->getShields());
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
}