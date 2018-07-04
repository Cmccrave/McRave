#include "McRave.h"

void UnitManager::onFrame()
{
	Display().startClock();
	updateUnits();
	Display().performanceTest(__FUNCTION__);
}

void UnitManager::updateUnits()
{
	// Reset calculations and caches
	globalEnemyGroundStrength = 0.0;
	globalEnemyAirStrength = 0.0;
	globalAllyGroundStrength = 0.0;
	globalAllyAirStrength = 0.0;
	immThreat = 0.0;
	proxThreat = 0.0;
	allyDefense = 0.0;
	splashTargets.clear();
	enemyComposition.clear();

	// Latch based engagement decision making based on what race we are playing	
	if (Broodwar->self()->getRace() == Races::Protoss) {
		if (Players().getNumberZerg() > 0 || Players().getNumberProtoss() > 0)
			minThreshold = 0.6, maxThreshold = 1.2;
		else if (Players().getNumberTerran() > 0)
			minThreshold = 0.6, maxThreshold = 1.0;
		else if (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() == Races::Terran)
			minThreshold = 0.6, maxThreshold = 1.0;
		else if (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() != Races::Terran)
			minThreshold = 0.8, maxThreshold = 1.2;
	}
	else {
		minThreshold = 0.8, maxThreshold = 1.2;
	}

	// Update Enemy Units
	for (auto &u : enemyUnits) {
		UnitInfo &unit = u.second;
		if (!unit.unit())
			continue;

		//// If a building doesn't exist where it's stored, remove it, assume it has either moved or been destroyed
		//if (unit.getType().isBuilding() && !unit.unit()->exists() && Broodwar->isVisible(unit.getTilePosition()) && Broodwar->getFrameCount() - unit.getLastVisibleFrame() > 100) {
		//	TilePosition end = TilePosition(unit.getTilePosition()) + TilePosition(unit.getType().tileWidth(), unit.getType().tileHeight());

		//	if (Broodwar->isVisible(end)) {
		//		onUnitDestroy(unit.unit());				
		//		break;
		//	}
		//}

		if (unit.unit()->exists() && unit.unit()->isFlying() && unit.getType().isBuilding()) {
			
			for (int x = unit.getLastTile().x; x < unit.getLastTile().x + unit.getType().tileWidth(); x++) {
				for (int y = unit.getLastTile().y; y < unit.getLastTile().y + unit.getType().tileHeight(); y++) {
					TilePosition t(x, y);
					if (!t.isValid())
						continue;
					mapBWEB.getUsedTiles().erase(t);
				}
			}
			Broodwar << "Stale Building Removed" << endl;
			unit.setPosition(Positions::Invalid);
			unit.setTilePosition(TilePositions::Invalid);
			unit.setWalkPosition(WalkPositions::Invalid);
		}


		// If unit is visible, update it
		if (unit.unit()->exists() && unit.unit()->isVisible())
			updateEnemy(unit);

		// Must see a 3x3 grid of Tiles to set a unit to invalid position
		if (!unit.unit()->exists() && (!unit.isBurrowed() || Commands().overlapsAllyDetection(unit.getPosition()) || Grids().getAGroundCluster(unit.getWalkPosition()) >= 1) && unit.getPosition().isValid()) {
			bool move = true;
			for (int x = unit.getTilePosition().x - 1; x < unit.getTilePosition().x + 1; x++) {
				for (int y = unit.getTilePosition().y - 1; y < unit.getTilePosition().y + 1; y++) {
					TilePosition t(x, y);
					if (!Broodwar->isVisible(t))
						move = false;
				}
			}
			if (move) {
				unit.setPosition(Positions::Invalid);
				unit.setTilePosition(TilePositions::Invalid);
				unit.setWalkPosition(WalkPositions::Invalid);
			}
		}


		// If unit has a valid type, update enemy composition tracking
		if (unit.getType().isValid())
			enemyComposition[unit.getType()] += 1;

		// If unit is not a worker or building, add it to global strength	
		if (!unit.getType().isWorker())
			unit.getType().isFlyer() ? globalEnemyAirStrength += unit.getVisibleAirStrength() : globalEnemyGroundStrength += unit.getVisibleGroundStrength();

		// If a unit is threatening our position
		if (isThreatening(unit) && unit.getType() != UnitTypes::Protoss_Pylon) {
			if (unit.getType().isBuilding())
				immThreat += max(1.5, unit.getVisibleGroundStrength());
			else
				immThreat += unit.getVisibleGroundStrength();
		}

	//	if (unit.isBurrowed())
			//Broodwar->drawCircleMap(unit.getPosition(), 6, Colors::Red);
	}

	// Update Ally Defenses
	for (auto &u : allyDefenses) {
		UnitInfo &unit = u.second;
		if (!unit.unit())
			continue;

		updateAlly(unit);

		if (unit.unit()->isCompleted())
			allyDefense += unit.getMaxGroundStrength();
	}

	repWorkers = 0;
	// Update myUnits
	for (auto &u : myUnits) {
		UnitInfo &unit = u.second;
		if (!unit.unit())
			continue;

		updateAlly(unit);
		updateLocalSimulation(unit);
		updateStrategy(unit);

		// Remove the worker role if needed
		if (unit.getType().isWorker() && Workers().getMyWorkers().find(unit.unit()) != Workers().getMyWorkers().end())
			Workers().removeWorker(unit.unit());

		// If this is a worker and is ready to go back to being a worker
		if (unit.getType().isWorker() && (!Util().proactivePullWorker(unit.unit()) && !Util().reactivePullWorker(unit.unit())) && (unit.getType() != UnitTypes::Terran_SCV || !Util().pullRepairWorker(unit.unit())))
			Workers().storeWorker(unit.unit());

		// If unit is not a building and deals damage, add it to global strength	
		if (!unit.getType().isBuilding())
			unit.getType().isFlyer() ? globalAllyAirStrength += unit.getVisibleAirStrength() : globalAllyGroundStrength += unit.getVisibleGroundStrength();

		// TEMP
		if (unit.getType() == UnitTypes::Terran_SCV)
			repWorkers++;
	}

	for (auto &u : neutrals) {
		UnitInfo &unit = u.second;
		if (!unit.unit() || !unit.unit()->exists())
			continue;

		updateNeutral(unit);
		//Broodwar->drawTextMap(unit.getPosition(), "%d, %d", unit.getPosition());
	}
}

void UnitManager::updateLocalSimulation(UnitInfo& unit)
{
	// Notes:
	// Speeds are multipled by 24.0 because we want pixels per second

	// Variables for calculating local strengths
	double enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
	double enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
	double unitToEngage = max(0.0, unit.getPosition().getDistance(unit.getEngagePosition()) / (24.0 * unit.getSpeed()));
	double simulationTime = unitToEngage + 5.0;	
	double unitSpeed = unit.getSpeed() * 24.0; 

	bool sync = false;

	// If we have excessive resources, ignore our simulation and engage
	if (!ignoreSim && Broodwar->self()->minerals() >= 5000 && Broodwar->self()->gas() >= 5000 && Units().getSupply() >= 380) {
		ignoreSim = true;
		Broodwar->sendText("RRRRAAAAAAAAMMMMPPAAAAAAAAAAAAAAAAGE");
	}
	if (ignoreSim && Broodwar->self()->minerals() <= 2000 || Broodwar->self()->gas() <= 2000 || Units().getSupply() <= 240)
		ignoreSim = false;

	if (ignoreSim) {
		unit.setLocalStrategy(1);
		return;
	}

	if (Terrain().isIslandMap() && !unit.getType().isFlyer()) {
		unit.setLocalStrategy(1);
		return;
	}

	// Check every enemy unit being in range of the target
	for (auto &e : enemyUnits) {
		UnitInfo &enemy = e.second;

		// Ignore workers and stasised units
		if (!enemy.unit() || (enemy.getType().isWorker() && !Strategy().enemyRush()) || (enemy.unit() && enemy.unit()->exists() && (enemy.unit()->isStasised() || enemy.unit()->isMorphing())))
			continue;

		// Distance parameters
		double widths = (double)enemy.getType().tileWidth() * 16.0 + (double)unit.getType().tileWidth() * 16.0;
		double enemyRange = (unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange());
		double airDist = enemy.getPosition().getDistance(unit.getPosition());

		// True distance
		double distance = airDist - enemyRange - widths;

		// Sim values
		double enemyToEngage = 0.0;
		double simRatio = 0.0;
		double speed = enemy.getSpeed() * 24.0;

		// If enemy can move, distance/speed is time to engage
		if (speed > 0.0) {
			enemyToEngage = max(0.0, distance / speed);
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}

		// If enemy can't move, it must be in range of our engage position to be added
		else if (enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths <= 0.0) {
			//Broodwar->drawLineMap(unit.getPosition(), enemy.getPosition(), Colors::Red);
			enemyToEngage = max(0.0, distance / unitSpeed);
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else
			continue;

		// Situations where an enemy should be treated as stronger than it actually is
		if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected())
			simRatio = simRatio * 5.0;
		if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))
			simRatio = simRatio * 2.0;
		if (enemy.getLastVisibleFrame() < Broodwar->getFrameCount())
			simRatio = simRatio * (1.0 + min(0.2, (double(Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) / 5000.0)));

		enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
		enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
	}

	// Check every ally being in range of the target
	for (auto &a : myUnits) {
		UnitInfo &ally = a.second;

		if (!ally.hasTarget() || ally.unit()->isStasised() || ally.unit()->isMorphing() || ally.getType() == UnitTypes::Terran_Vulture_Spider_Mine)
			continue;

		// Setup distance values
		double dist = ally.getType().isFlyer() ? ally.getPosition().getDistance(ally.getEngagePosition()) : max(ally.getPosition().getDistance(ally.getEngagePosition()), ally.getEngDist());
		double widths = (double)ally.getType().tileWidth() * 16.0 + (double)unit.getType().tileWidth() * 16.0;
		double speed = (ally.getTransport() && ally.getTransport()->exists()) ? ally.getTransport()->getType().topSpeed() * 24.0 : (24.0 * ally.getSpeed());

		// Setup true distance
		double distance = dist - widths;

		if (ally.getPosition().getDistance(unit.getEngagePosition()) / speed > simulationTime)
			continue;
		if ((ally.getType() == UnitTypes::Protoss_Scout || ally.getType() == UnitTypes::Protoss_Corsair) && ally.getShields() < 30)
			continue;
		if (ally.getType() == UnitTypes::Terran_Wraith && ally.getHealth() <= 100)
			continue;
		if (ally.getHealth() + ally.getShields() < 20 && Broodwar->getFrameCount() < 12000)
			continue;

		double allyToEngage = max(0.0, (distance / speed));
		double simRatio = max(0.0, simulationTime - allyToEngage);

		// Situations where an ally should be treated as stronger than it actually is
		if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && !Commands().overlapsEnemyDetection(ally.getEngagePosition()))
			simRatio = simRatio * 5.0;
		if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTarget().getPosition())))
			simRatio = simRatio * 2.0;

		if (!sync && simRatio > 0.0 && ((unit.getType().isFlyer() && !ally.getType().isFlyer()) || (!unit.getType().isFlyer() && ally.getType().isFlyer())))
			sync = true;

		allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
		allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;
	}

	double airair = enemyLocalAirStrength > 0.0 ? allyLocalAirStrength / enemyLocalAirStrength : 10.0;
	double airgrd = enemyLocalGroundStrength > 0.0 ? allyLocalAirStrength / enemyLocalGroundStrength : 10.0;
	double grdair = enemyLocalAirStrength > 0.0 ? allyLocalGroundStrength / enemyLocalAirStrength : 10.0;
	double grdgrd = enemyLocalGroundStrength > 0.0 ? allyLocalGroundStrength / enemyLocalGroundStrength : 10.0;

	if (unit.hasTarget()) {

		if (unit.getPosition().getDistance(unit.getSimPosition()) > 720.0 - (24.0 * unit.getSpeed()))
			unit.setLocalStrategy(0);

		// If simulations need to be synchdd
		else if (sync && !unit.getTarget().getType().isFlyer()) {

			if (grdair >= maxThreshold && grdgrd >= maxThreshold)
				unit.setLocalStrategy(1);
			else if (unit.getType().isFlyer() && enemyLocalAirStrength == 0.0)
				unit.setLocalStrategy(1);
			else if (grdair < minThreshold && grdgrd < minThreshold)
				unit.setLocalStrategy(0);

			Display().displaySim(unit, (grdgrd + grdair) / 2.0);
		}

		else if (sync && unit.getTarget().getType().isFlyer()) {

			if (airgrd >= maxThreshold && airair >= maxThreshold)
				unit.setLocalStrategy(1);
			else if (!unit.getType().isFlyer() && enemyLocalGroundStrength == 0.0)
				unit.setLocalStrategy(1);
			else if (airgrd < minThreshold && airair < minThreshold)
				unit.setLocalStrategy(0);

			Display().displaySim(unit, (airgrd + airair) / 2.0);
		}

		else if (unit.getTarget().getType().isFlyer()) {
			if (unit.getType().isFlyer()) {

				if (airair >= maxThreshold)
					unit.setLocalStrategy(1);
				else if (airair < minThreshold)
					unit.setLocalStrategy(0);

				Display().displaySim(unit, airair);
			}
			else {

				if (airgrd >= maxThreshold)
					unit.setLocalStrategy(1);
				else if (airgrd < minThreshold)
					unit.setLocalStrategy(0);

				Display().displaySim(unit, airgrd);
			}
		}
		else
		{
			if (unit.getType().isFlyer()) {

				if (grdair >= maxThreshold)
					unit.setLocalStrategy(1);
				else if (grdair < minThreshold)
					unit.setLocalStrategy(0);

				Display().displaySim(unit, grdair);
			}
			else {

				if (grdgrd >= maxThreshold)
					unit.setLocalStrategy(1);
				else if (grdgrd < minThreshold)
					unit.setLocalStrategy(0);

				Display().displaySim(unit, grdgrd);
			}
		}
	}
}

void UnitManager::updateStrategy(UnitInfo& unit)
{
	// Global strategy
	if (Broodwar->self()->getRace() == Races::Protoss) {
		if ((!BuildOrder().isFastExpand() && Strategy().enemyFastExpand())
			|| Terrain().isIslandMap()
			|| Strategy().enemyProxy())
			unit.setGlobalStrategy(1);

		else if ((Strategy().enemyRush() && Players().getNumberTerran() <= 0)
			|| (!Strategy().enemyRush() && BuildOrder().isHideTech() && BuildOrder().isOpener())
			|| unit.getType().isWorker()
			|| (BuildOrder().isOpener() && BuildOrder().isPlayPassive())
			|| (unit.getType() == UnitTypes::Protoss_Corsair && !BuildOrder().firstReady() && globalEnemyAirStrength > 0.0))
			unit.setGlobalStrategy(0);
		else
			unit.setGlobalStrategy(1);
	}
	else if (Broodwar->self()->getRace() == Races::Terran) {
		if (unit.getType() == UnitTypes::Terran_Vulture)
			unit.setGlobalStrategy(1);
		else if (!BuildOrder().firstReady())
			unit.setGlobalStrategy(0);
		else if (BuildOrder().isHideTech() && BuildOrder().isOpener())
			unit.setGlobalStrategy(0);
		else if (!BuildOrder().isBioBuild() && supply < 200)
			unit.setGlobalStrategy(0);
		else
			unit.setGlobalStrategy(1);
	}
	else if (Broodwar->self()->getRace() == Races::Zerg) {
		if (Broodwar->self()->getUpgradeLevel(BuildOrder().getFirstUpgrade()) <= 0 && !Broodwar->self()->hasResearched(BuildOrder().getFirstTech()))
			unit.setGlobalStrategy(0);
		else if (BuildOrder().isHideTech() && BuildOrder().isOpener())
			unit.setGlobalStrategy(0);
		else
			unit.setGlobalStrategy(1);
	}

	// Forced local engage/retreat
	if ((unit.hasTarget() && Units().isThreatening(unit.getTarget()))
		|| (Terrain().isInAllyTerritory(unit.getTilePosition()) && Util().unitInRange(unit) && !unit.getType().isFlyer() && (Strategy().defendChoke() || unit.getGroundRange() > 64.0)))
		unit.setEngage();

	else if ((unit.getType() == UnitTypes::Terran_Wraith && unit.getHealth() <= 100)
		|| (unit.getType() == UnitTypes::Terran_Battlecruiser && unit.getHealth() <= 200)
		|| (unit.getType() == UnitTypes::Protoss_High_Templar && unit.unit()->getEnergy() < TechTypes::Psionic_Storm.energyCost())
		//|| Commands().isInDanger(unit)
		|| Grids().getESplash(unit.getWalkPosition()) > 0
		|| (unit.getGlobalStrategy() == 0))
		unit.setRetreat();

	else if (unit.hasTarget() && unit.getPosition().getDistance(unit.getSimPosition()) <= 720.0 - (24.0 * unit.getSpeed())) {
		if ((unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture && Grids().getMobility(unit.getTarget().getWalkPosition()) > 6 && Grids().getCollision(unit.getTarget().getWalkPosition()) < 2)
			|| ((unit.getType() == UnitTypes::Protoss_Scout || unit.getType() == UnitTypes::Protoss_Corsair) && unit.getTarget().getType() == UnitTypes::Zerg_Overlord && Grids().getEAirThreat((WalkPosition)unit.getEngagePosition()) * 5.0 > (double)unit.getShields())
			|| (unit.getType() == UnitTypes::Protoss_Corsair && unit.getTarget().getType() == UnitTypes::Zerg_Scourge && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Corsair) < 6)
			|| (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= TechTypes::Healing.energyCost())
			|| (unit.getTarget().unit() && (unit.getTarget().unit()->isCloaked() || unit.getTarget().isBurrowed()) && !unit.getTarget().unit()->isDetected() && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= unit.getTarget().getGroundRange() + 96)
			|| (unit.getHealth() + unit.getShields() < 20 && Broodwar->getFrameCount() < 12000))
			unit.setRetreat();

		else if (((unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && unit.getPosition().getDistance(unit.getTarget().getPosition()) < 200)
			|| ((unit.unit()->isCloaked() || unit.isBurrowed()) && !Commands().overlapsEnemyDetection(unit.getEngagePosition()))
			|| (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util().unitInRange(unit))
			|| (unit.getGlobalStrategy() == 1 && unit.getLocalStrategy() == 1))
			unit.setEngage();
		else
			unit.setRetreat();
	}
}

bool UnitManager::isThreatening(UnitInfo& unit)
{
	if ((unit.isBurrowed() || (unit.unit() && unit.unit()->exists() && unit.unit()->isCloaked())) && !Commands().overlapsAllyDetection(unit.getPosition()))
		return false;

	if (unit.unit() && unit.unit()->exists() && unit.getType().isWorker() && !unit.unit()->isAttacking() && unit.unit()->getOrder() != Orders::AttackUnit && unit.unit()->getOrder() != Orders::PlaceBuilding && !unit.unit()->isConstructing())
		return false;

	if (unit.getPosition().getDistance(Terrain().getMineralHoldPosition()) <= 160.0)
		return true;

	if ((unit.getPosition().getDistance(Terrain().getDefendPosition()) <= max(64.0, unit.getGroundRange()) || unit.getPosition().getDistance(Terrain().getDefendPosition()) <= max(64.0, unit.getAirRange()) || (Strategy().defendChoke() && Terrain().isInAllyTerritory(unit.getTilePosition())))/* && (!unit.getType().isWorker() || Broodwar->getFrameCount() - unit.getLastAttackFrame() < 1500 || unit.unit()->isRepairing() || unit.unit()->isConstructing())*/)
		return true;

	if (unit.getType().isBuilding() && Terrain().isInAllyTerritory(unit.getTilePosition()))
		return true;

	// If unit is constructing a bunker near our defend position
	if (unit.unit() && unit.unit()->exists() && unit.unit()->isConstructing() && unit.unit()->getBuildUnit() && unit.unit()->getBuildUnit()->getType() == UnitTypes::Terran_Bunker && unit.getPosition().getDistance(Terrain().getDefendPosition()) <= 640)
		return true;

	// If there is a building close to our wall
	if (BuildOrder().isFastExpand() && Terrain().getWall()) {
		for (auto &piece : Terrain().getWall()->largeTiles()) {
			Position center = Position(piece) + Position(64, 48);
			if ((unit.getType().isBuilding() && unit.getPosition().getDistance(center) < 480)
				|| unit.getPosition().getDistance(center) <= unit.getGroundRange())
				return true;
		}
		for (auto &piece : Terrain().getWall()->mediumTiles()) {
			Position center = Position(piece) + Position(48, 32);
			if ((unit.getType().isBuilding() && unit.getPosition().getDistance(center) < 480)
				|| unit.getPosition().getDistance(center) <= unit.getGroundRange())
				return true;
		}
		for (auto &piece : Terrain().getWall()->smallTiles()) {
			Position center = Position(piece) + Position(32, 32);
			if ((unit.getType().isBuilding() && unit.getPosition().getDistance(center) < 480)
				|| unit.getPosition().getDistance(center) <= unit.getGroundRange())
				return true;
		}
	}
	return false;
}

int UnitManager::getEnemyCount(UnitType t)
{
	map<UnitType, int>::iterator itr = enemyComposition.find(t);
	if (itr != enemyComposition.end())
		return itr->second;
	return 0;
}

UnitInfo& UnitManager::getUnitInfo(Unit unit)
{
	if (unit)
	{
		auto&units = (!unit->exists() || unit->getPlayer() == Broodwar->self()) ? myUnits : enemyUnits;
		auto it = units.find(unit);
		if (it != units.end()) return it->second;
	}
	static UnitInfo dummy{}; // Should never have to be used, but let's not crash if we do
	return dummy;
}

UnitInfo* UnitManager::getClosestInvisEnemy(BuildingInfo& unit)
{
	double distBest = DBL_MAX;
	UnitInfo* best = nullptr;
	for (auto&e : enemyUnits)
	{
		UnitInfo& enemy = e.second;
		if (unit.unit() != enemy.unit() && enemy.getPosition().isValid() && !Commands().overlapsAllyDetection(enemy.getPosition()) && (enemy.isBurrowed() || enemy.unit()->isCloaked()))
		{
			double dist = unit.getPosition().getDistance(enemy.getPosition());
			if (dist < distBest)
				best = &enemy, distBest = dist;
		}
	}
	return best;
}

void UnitManager::storeEnemy(Unit unit)
{
	enemyUnits[unit].setUnit(unit);
	enemySizes[unit->getType().size()] += 1;
	if (unit->getType().isResourceDepot())
		Stations().storeStation(unit);
}

void UnitManager::updateEnemy(UnitInfo& unit)
{
	// Update units
	auto t = unit.unit()->getType();
	auto p = unit.unit()->getPlayer();

	unit.setLastPositions();

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
	unit.setGroundRange(Util().groundRange(unit));
	unit.setAirRange(Util().airRange(unit));
	unit.setGroundDamage(Util().groundDamage(unit));
	unit.setAirDamage(Util().airDamage(unit));
	unit.setSpeed(Util().speed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));
	unit.setLastVisibleFrame(Broodwar->getFrameCount());
	unit.setBurrowed(unit.unit()->isBurrowed() || unit.unit()->getOrder() == Orders::Burrowing || unit.unit()->getOrder() == Orders::VultureMine);

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));

	// Set last command frame
	if (unit.unit()->isStartingAttack() || unit.unit()->isRepairing()) unit.setLastAttackFrame(Broodwar->getFrameCount());

	unit.setTarget(nullptr);
	if (unit.unit()->getTarget() && myUnits.find(unit.unit()->getTarget()) != myUnits.end())
		unit.setTarget(&myUnits[unit.unit()->getTarget()]);
	else if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.unit()->getOrderTarget() && unit.unit()->getOrderTarget()->getPlayer() == Broodwar->self())
		unit.setTarget(&myUnits[unit.unit()->getOrderTarget()]);

	if (unit.hasTarget() && (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Protoss_Scarab))
		splashTargets.insert(unit.getTarget().unit());
}

void UnitManager::storeAlly(Unit unit)
{
	if (unit->getType().isBuilding()) {
		allyDefenses[unit].setUnit(unit);
		allyDefenses[unit].setType(unit->getType());
		allyDefenses[unit].setTilePosition(unit->getTilePosition());
		allyDefenses[unit].setPosition(unit->getPosition());
		Grids().updateDefense(allyDefenses[unit]);
	}
	else myUnits[unit].setUnit(unit);
}

void UnitManager::updateAlly(UnitInfo& unit)
{
	auto t = unit.unit()->getType();
	auto p = unit.unit()->getPlayer();

	unit.setLastPositions();

	// Update information
	unit.setType(t);
	unit.setPosition(unit.unit()->getPosition());
	unit.setDestination(Positions::None);
	unit.setTilePosition(unit.unit()->getTilePosition());
	unit.setWalkPosition(Util().getWalkPosition(unit.unit()));
	unit.setPlayer(unit.unit()->getPlayer());

	// Update statistics
	unit.setHealth(unit.unit()->getHitPoints());
	unit.setShields(unit.unit()->getShields());
	unit.setPercentHealth(Util().getPercentHealth(unit));
	unit.setGroundRange(Util().groundRange(unit));
	unit.setAirRange(Util().airRange(unit));
	unit.setGroundDamage(Util().groundDamage(unit));
	unit.setAirDamage(Util().airDamage(unit));
	unit.setSpeed(Util().speed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));

	// Update target
	if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine)
	{
		if (unit.unit()->getOrderTarget())
			unit.setTarget(&enemyUnits[unit.unit()->getOrderTarget()]);
		else unit.setTarget(nullptr);
	}
	else
		Targets().getTarget(unit);

	// Set last command frame
	if (unit.unit()->isStartingAttack())
		unit.setLastAttackFrame(Broodwar->getFrameCount());

	// Set fun ding effect
	if (unit.unit()->getKillCount() / 10 > unit.getKillCount() / 10)
		Display().storeDing(unit.unit());
	unit.setKillCount(unit.unit()->getKillCount());

	unit.resetForces();
}

void UnitManager::storeNeutral(Unit unit)
{
	neutrals[unit].setUnit(unit);
}

void UnitManager::updateNeutral(UnitInfo& unit)
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
	unit.setHealth(unit.unit()->getHitPoints());
	unit.setShields(unit.unit()->getShields());
	unit.setPercentHealth(Util().getPercentHealth(unit));
	unit.setGroundRange(Util().groundRange(unit));
	unit.setAirRange(Util().airRange(unit));
	unit.setGroundDamage(Util().groundDamage(unit));
	unit.setAirDamage(Util().airDamage(unit));
	unit.setSpeed(Util().speed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));
}
