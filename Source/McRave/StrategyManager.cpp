#include "McRave.h"

// Information from Jays blog
//4 pool	2630
//5 pool	2750
//6 pool	2920
//7 pool cutting drones	2990
//7 pool going to 9 drones	3120
//8 pool	3160
//9 pool	3230

void StrategyManager::onFrame()
{
	Display().startClock();
	updateSituationalBehaviour();
	updateEnemyBuild();
	updateBullets();
	updateScoring();
	Display().performanceTest(__FUNCTION__);
}

void StrategyManager::updateSituationalBehaviour()
{
	// Reset unit score for toss
	if (Broodwar->self()->getRace() == Races::Protoss)
		for (auto &unit : unitScore)
			unit.second = 0;

	// Get strategy based on race
	if (Broodwar->self()->getRace() == Races::Protoss)
		protossStrategy();
	else if (Broodwar->self()->getRace() == Races::Terran)
		terranStrategy();
	else if (Broodwar->self()->getRace() == Races::Zerg)
		zergStrategy();
}

void StrategyManager::protossStrategy()
{
	// Check if it's early enough to run specific strategies
	if (BuildOrder().isOpener()) {
		rush = checkEnemyRush();
		hideTech = shouldHideTech();
		holdChoke = shouldHoldChoke();
		proxy = checkEnemyProxy();
	}
	else {
		rush = false;
		holdChoke = true;
		enemyFE = false;
		proxy = false;
	}

	if (Units().getEnemyCount(UnitTypes::Terran_Command_Center) >= 2 || Units().getEnemyCount(UnitTypes::Zerg_Hatchery) >= 3 || (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) >= 2 && Units().getEnemyCount(UnitTypes::Zerg_Lair) >= 1) || Units().getEnemyCount(UnitTypes::Protoss_Nexus) >= 2)
		enemyFE = true;

	invis = shouldGetDetection();
}

void StrategyManager::terranStrategy()
{
	// Check if it's early enough to run specific strategies
	if (BuildOrder().isOpener()) {
		rush = checkEnemyRush();
		hideTech = shouldHideTech();
		holdChoke = shouldHoldChoke();
	}
	else {
		rush = false;
		holdChoke = true;
		allyFastExpand = false;
		enemyFE = false;
	}

	invis = shouldGetDetection();
}

void StrategyManager::zergStrategy()
{
	// Check if it's early enough to run specific strategies
	if (BuildOrder().isOpener()) {
		rush = checkEnemyRush();
		hideTech = shouldHoldChoke();
		holdChoke = true;
	}
	else {
		rush = false;
		holdChoke = true;
		allyFastExpand = false;
		enemyFE = false;
	}
}

bool StrategyManager::checkEnemyRush()
{
	if (Units().getSupply() > 80)
		return false;

	if (enemyBuild == "TBBS"
		|| enemyBuild == "P2Gate"
		|| enemyBuild == "Z5Pool"
		|| enemyBuild == "Z9Pool")
		return true;
	return false;
}

bool StrategyManager::shouldHoldChoke()
{
	if (Broodwar->self()->getRace() == Races::Zerg)
		return rush;

	if (BuildOrder().isWallNat()
		|| hideTech
		|| Units().getSupply() > 60
		|| (Broodwar->self()->getRace() == Races::Protoss && Players().getNumberTerran() > 0)
		|| (Broodwar->self()->getRace() == Races::Protoss && Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() == Races::Terran))
		return true;
	return false;
}

bool StrategyManager::shouldHideTech()
{
	if (BuildOrder().getCurrentBuild() == "PDTExpand" && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dark_Templar) == 0)
		return true;
	return false;
}

bool StrategyManager::shouldGetDetection()
{
	if (Broodwar->self()->getRace() == Races::Protoss) {
		if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) > 0)
			return false;
	}
	else if (Broodwar->self()->getRace() == Races::Terran) {
		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Comsat_Station) > 0)
			return false;

		if (Broodwar->self()->getRace() == Races::Terran && (Units().getEnemyCount(UnitTypes::Zerg_Hydralisk) > 0 || Units().getEnemyCount(UnitTypes::Zerg_Hydralisk_Den) > 0))
			return true;
	}

	// DTs
	if (Units().getEnemyCount(UnitTypes::Protoss_Dark_Templar) >= 1 || Units().getEnemyCount(UnitTypes::Protoss_Citadel_of_Adun) >= 1 || Units().getEnemyCount(UnitTypes::Protoss_Templar_Archives) >= 1)
		return true;

	// Ghosts/Vultures
	if (Units().getEnemyCount(UnitTypes::Terran_Ghost) >= 1 || Units().getEnemyCount(UnitTypes::Terran_Vulture) >= 6)
		return true;

	// Lurkers
	if (Units().getEnemyCount(UnitTypes::Zerg_Lurker) >= 1 || (Units().getEnemyCount(UnitTypes::Zerg_Lair) >= 1 && Units().getEnemyCount(UnitTypes::Zerg_Hydralisk_Den) >= 1 && Units().getEnemyCount(UnitTypes::Zerg_Hatchery) <= 0))
		return true;

	if (enemyBuild == "Z1HatchLurker" || enemyBuild == "Z2HatchLurker")
		return true;

	return false;
}

bool StrategyManager::checkEnemyProxy()
{
	if (Units().getSupply() > 60)
		return false;
	else if (enemyBuild == "PCannonRush" || enemyBuild == "TBunkerRush")
		return true;
	else if (proxy)
		return true;
	return false;
}

void StrategyManager::updateEnemyBuild()
{
	if (BuildOrder().firstReady())
		scoutTargets.clear();

	if (Broodwar->isExplored(Terrain().getEnemyStartingTilePosition())) {
		double best = DBL_MAX;
		TilePosition bestTile = TilePositions::Invalid;
		for (auto &station : Stations().getLastVisible()) {
			if (Broodwar->getFrameCount() - station.second > 500 && station.second < best) {
				best = station.second;
				bestTile = station.first;
			}
		}
		if (bestTile.isValid())
			scoutTargets.insert(Position(bestTile));
	}

	if (Players().getPlayers().size() > 1 || (Broodwar->getFrameCount() - enemyFrame > 1000 && enemyFrame != 0 && enemyBuild != "Unknown"))
		return;

	if (enemyFrame == 0 && Terrain().getEnemyStartingPosition().isValid())
		enemyFrame = Broodwar->getFrameCount();

	for (auto &p : Players().getPlayers()) {
		PlayerInfo &player = p.second;
		if (player.getRace() == Races::Zerg) {

			// 5 Hatch build detection
			if (Stations().getEnemyStations().size() >= 3 || (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) + Units().getEnemyCount(UnitTypes::Zerg_Lair) >= 4 && Units().getEnemyCount(UnitTypes::Zerg_Drone) >= 14))
				enemyBuild = "Z5Hatch";

			for (auto &e : Units().getEnemyUnits()) {
				UnitInfo &enemy = e.second;

				// Monitor Zerg gas intake
				if (enemy.getType() == UnitTypes::Zerg_Extractor && enemy.unit()->exists())
					enemyGas = enemy.unit()->getInitialResources() - enemy.unit()->getResources();

				// Zergling build detection and pool timing
				if (enemy.getType() == UnitTypes::Zerg_Spawning_Pool) {

					if (poolFrame == 0 && enemy.unit()->exists())
						poolFrame = Broodwar->getFrameCount() + int(double(enemy.getType().buildTime()) * (double(enemy.getType().maxHitPoints() - enemy.unit()->getHitPoints()) / double(enemy.getType().maxHitPoints())));

					if (Units().getEnemyCount(UnitTypes::Zerg_Spire) == 0 && Units().getEnemyCount(UnitTypes::Zerg_Hydralisk_Den) == 0 && Units().getEnemyCount(UnitTypes::Zerg_Lair) == 0) {
						if (enemyGas <= 0 && ((poolFrame < 2500 && poolFrame > 0) || (lingFrame < 3000 && lingFrame > 0)))
							enemyBuild = "Z5Pool";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 1 && enemyGas < 148 && enemyGas >= 50 && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 8)
							enemyBuild = "Z9Pool";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) >= 1 && Units().getEnemyCount(UnitTypes::Zerg_Drone) <= 11 && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 8)
							enemyBuild = "Z9Pool";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 3 && enemyGas < 148 && enemyGas >= 100)
							enemyBuild = "Z3HatchLing";
						else
							enemyBuild = "Unknown";
					}
				}

				// Hydralisk/Lurker build detection
				else if (enemy.getType() == UnitTypes::Zerg_Hydralisk_Den) {
					if (Units().getEnemyCount(UnitTypes::Zerg_Spire) == 0) {
						if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 3 && Units().getEnemyCount(UnitTypes::Zerg_Drone) < 14)
							enemyBuild = "Z3HatchHydra";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 2)
							enemyBuild = "Z2HatchHydra";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Lair) + Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 2)
							enemyBuild = "Z2HatchLurker";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Lair) == 1)
							enemyBuild = "Z1HatchLurker";
						else
							enemyBuild = "Z1HatchHydra";
					}
					else
						enemyBuild = "Unknown";
				}

				// Mutalisk build detection
				else if (enemy.getType() == UnitTypes::Zerg_Spire || enemy.getType() == UnitTypes::Zerg_Lair) {
					if (Units().getEnemyCount(UnitTypes::Zerg_Hydralisk_Den) == 0) {
						if (Units().getEnemyCount(UnitTypes::Zerg_Lair) + Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 3 && Units().getEnemyCount(UnitTypes::Zerg_Drone) < 14)
							enemyBuild = "Z3HatchMuta";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Lair) + Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 2)
							enemyBuild = "Z2HatchMuta";
					}
					//else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) >= 4)
					//	enemyBuild = "Z5Hatch";
					else
						enemyBuild = "Unknown";
				}
			}

			// Zergling frame
			if (lingFrame == 0 && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 6) {
				lingFrame = Broodwar->getFrameCount();
			}
		}
		if (player.getRace() == Races::Protoss) {
			for (auto &u : Units().getEnemyUnits()) {
				UnitInfo &unit = u.second;

				// PCannonRush
				if (unit.getType() == UnitTypes::Protoss_Forge) {
					if (unit.getPosition().getDistance(Terrain().getEnemyStartingPosition()) < 320.0 && Units().getEnemyCount(UnitTypes::Protoss_Gateway) == 0) {
						enemyBuild = "PCannonRush";
						proxy = true;
					}
					else {
						enemyBuild = "Unknown";
						proxy = false;
					}
				}

				// PFFE
				if (unit.getType() == UnitTypes::Protoss_Photon_Cannon) {
					if (unit.getPosition().getDistance((Position)Terrain().getEnemyNatural()) < 320)
						enemyBuild = "PFFE";
					else
						enemyBuild = "Unknown";
				}

				// P2GateExpand
				if (unit.getType() == UnitTypes::Protoss_Nexus) {
					if (!Terrain().isStartingBase(unit.getTilePosition()) && Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2)
						enemyBuild = "P2GateExpand";
					else
						enemyBuild = "Unknown";
				}

				// P1GateCore
				if (unit.getType() == UnitTypes::Protoss_Cybernetics_Core) {
					if (unit.unit()->isUpgrading())
						goonRange = true;

					if (Units().getEnemyCount(UnitTypes::Protoss_Robotics_Facility) >= 1)
						enemyBuild = "P1GateRobo";
					else if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 4)
						enemyBuild = "P4Gate";
					else if (Units().getEnemyCount(UnitTypes::Protoss_Citadel_of_Adun) >= 1 || Units().getEnemyCount(UnitTypes::Protoss_Templar_Archives) >= 1 || (!goonRange && Units().getSupply() > 60))
						enemyBuild = "P1GateDT";
					else
						enemyBuild = "Unknown";
				}

				// Proxy Detection
				if ((unit.getType() == UnitTypes::Protoss_Pylon || unit.getType().isWorker()) && unit.getPosition().getDistance(Terrain().getPlayerStartingPosition()) < 960.0) {
					/*if (!unit.unit()->exists() || Terrain().isInAllyTerritory(unit.getTilePosition()))
						proxy = true;
					else
						proxy = false;*/
				}

				// FE Detection
				if (unit.getType().isResourceDepot() && !Terrain().isStartingBase(unit.getTilePosition()))
					enemyFE = true;
			}

			if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2 && Units().getEnemyCount(UnitTypes::Protoss_Assimilator) <= 0)
				enemyBuild = "P2Gate";

		}
		if (player.getRace() == Races::Terran) {
			for (auto &e : Units().getEnemyUnits()) {
				UnitInfo &enemy = e.second;

				// TSiegeExpand
				if ((enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && Units().getEnemyCount(UnitTypes::Terran_Vulture) == 0) || (enemy.getType().isResourceDepot() && !Terrain().isStartingBase(enemy.getTilePosition()) && Units().getEnemyCount(UnitTypes::Terran_Machine_Shop) > 0))
					enemyBuild = "TSiegeExpand";

				// Barracks Builds
				if (enemy.getType() == UnitTypes::Terran_Barracks) {

					if (Terrain().isInAllyTerritory(enemy.getTilePosition()) || enemy.getPosition().getDistance(mapBWEM.Center()) < 1280.0 || (mapBWEB.getNaturalChoke() && enemy.getPosition().getDistance((Position)mapBWEB.getNaturalChoke()->Center()) < 320)) {
						enemyBuild = "TBBS";
						proxy = true;
						scoutTargets.insert(enemy.getPosition());
					}
					else if (Units().getEnemyCount(UnitTypes::Terran_Academy) >= 1 && Units().getEnemyCount(UnitTypes::Terran_Engineering_Bay) >= 1)
						enemyBuild = "TSparks";
					else {
						enemyBuild = "Unknown";
						proxy = false;
					}
				}

				// FE Detection
				if (enemy.getType().isResourceDepot() && !Terrain().isStartingBase(enemy.getTilePosition()))
					enemyFE = true;
			}

			if (Broodwar->getFrameCount() - enemyFrame > 200 && Terrain().getEnemyStartingPosition().isValid() && Broodwar->isExplored((TilePosition)Terrain().getEnemyStartingPosition()) && Units().getEnemyCount(UnitTypes::Terran_Barracks) == 0)
				scoutTargets.insert(mapBWEM.Center());
			else
				scoutTargets.erase(mapBWEM.Center());

			if (Units().getSupply() < 60 && ((Units().getEnemyCount(UnitTypes::Terran_Barracks) >= 2 && Units().getEnemyCount(UnitTypes::Terran_Refinery) == 0) || (Units().getEnemyCount(UnitTypes::Terran_Marine) > 5 && Units().getEnemyCount(UnitTypes::Terran_Bunker) <= 0)))
				enemyBuild = "TBBS";
			if (Units().getEnemyCount(UnitTypes::Terran_Factory) >= 3)
				enemyBuild = "T3Fact";

		}
	}
}

void StrategyManager::updateBullets()
{

}

void StrategyManager::updateScoring()
{
	// Unit score based off enemy composition
	bool MadMix = true;
	for (auto &t : Units().getEnemyComposition())
	{
		if (t.first.isBuilding())
			continue;

		// For each type, add a score to production based on the unit count divided by our current unit count
		if (Broodwar->self()->getRace() == Races::Protoss)
			updateProtossUnitScore(t.first, t.second);
		else if (Broodwar->self()->getRace() == Races::Terran)
			updateTerranUnitScore(t.first, t.second);
		else if (MadMix)
			updateMadMixScore(t.first, t.second);
	}

	if (Broodwar->self()->getRace() == Races::Protoss) {
		if (Terrain().isIslandMap())
			unitScore[UnitTypes::Protoss_Shuttle] = 10.0;
		else
			unitScore[UnitTypes::Protoss_Shuttle] = getUnitScore(UnitTypes::Protoss_Reaver);
	}
}

double StrategyManager::getUnitScore(UnitType unit)
{
	map<UnitType, double>::iterator itr = unitScore.find(unit);
	if (itr != unitScore.end())
		return itr->second;
	return 0.0;
}

UnitType StrategyManager::getHighestUnitScore()
{
	double best = 0.0;
	UnitType bestType = UnitTypes::None;
	for (auto & unit : unitScore) {
		if (unit.second > best) {
			best = unit.second, bestType = unit.first;
		}
	}

	if (bestType == UnitTypes::None && Broodwar->self()->getRace() == Races::Zerg)
		return UnitTypes::Zerg_Drone;
	return bestType;
}

void StrategyManager::updateProtossUnitScore(UnitType unit, int cnt)
{
	using namespace UnitTypes;
	double size = double(cnt) * double(unit.supplyRequired());

	auto const vis = [&](UnitType t) {
		return max(1.0, (double)Broodwar->self()->visibleUnitCount(t));
	};

	switch (unit)
	{
	case Enum::Terran_Marine:
		unitScore[Protoss_Zealot]				+= (size * 0.35) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.65) / vis(Protoss_Dragoon);
		unitScore[Protoss_High_Templar]			+= (size * 0.90) / vis(Protoss_High_Templar);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Terran_Medic:
		unitScore[Protoss_Zealot]				+= (size * 0.35) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.65) / vis(Protoss_Dragoon);
		unitScore[Protoss_High_Templar]			+= (size * 0.90) / vis(Protoss_High_Templar);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Terran_Firebat:
		unitScore[Protoss_Zealot]				+= (size * 0.35) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.65) / vis(Protoss_Dragoon);
		unitScore[Protoss_High_Templar]			+= (size * 0.90) / vis(Protoss_High_Templar);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Terran_Vulture:
		unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
		unitScore[Protoss_Observer]				+= (size * 0.70) / vis(Protoss_Observer);
		unitScore[Protoss_Arbiter]				+= (size * 0.15) / vis(Protoss_Arbiter);
		break;
	case Enum::Terran_Goliath:
		unitScore[Protoss_Zealot]				+= (size * 0.50) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.50) / vis(Protoss_Dragoon);
		unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / (Protoss_High_Templar);
		break;
	case Enum::Terran_Siege_Tank_Siege_Mode:
		unitScore[Protoss_Zealot]				+= (size * 0.85) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.15) / vis(Protoss_Dragoon);
		unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
		break;
	case Enum::Terran_Siege_Tank_Tank_Mode:
		unitScore[Protoss_Zealot]				+= (size * 0.85) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.15) / vis(Protoss_Dragoon);
		unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
		break;
	case Enum::Terran_Wraith:
		unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
		break;
	case Enum::Terran_Science_Vessel:
		unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
		break;
	case Enum::Terran_Battlecruiser:
		unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
		break;
	case Enum::Terran_Valkyrie:
		break;

	case Enum::Zerg_Zergling:
		unitScore[Protoss_Zealot]				+= (size * 0.85) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.15) / vis(Protoss_Dragoon);
		unitScore[Protoss_Corsair]				+= (size * 0.60) / vis(Protoss_Corsair);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
		unitScore[Protoss_Archon]				+= (size * 0.30) / vis(Protoss_Archon);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Zerg_Hydralisk:
		unitScore[Protoss_Zealot]				+= (size * 0.75) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.25) / vis(Protoss_Dragoon);
		unitScore[Protoss_High_Templar]			+= (size * 0.80) / vis(Protoss_High_Templar);
		unitScore[Protoss_Archon]				+= (size * 0.20) / vis(Protoss_Archon);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Zerg_Lurker:
		unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
		unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
		unitScore[Protoss_Observer]				+= (size * 1.00) / vis(Protoss_Observer);
		break;
	case Enum::Zerg_Ultralisk:
		unitScore[Protoss_Zealot]				+= (size * 0.25) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.75) / vis(Protoss_Dragoon);
		unitScore[Protoss_Reaver]				+= (size * 0.80) / vis(Protoss_Reaver);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.20) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Zerg_Mutalisk:
		unitScore[Protoss_Corsair]				+= (size * 1.00) / vis(Protoss_Corsair);
		break;
	case Enum::Zerg_Guardian:
		unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
		unitScore[Protoss_Corsair]				+= (size * 1.00) / vis(Protoss_Corsair);
		break;
	case Enum::Zerg_Devourer:
		break;
	case Enum::Zerg_Defiler:
		unitScore[Protoss_Zealot]				+= (size * 1.00) / vis(Protoss_Zealot);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		unitScore[Protoss_Reaver]				+= (size * 0.90) / vis(Protoss_Reaver);
		break;

	case Enum::Protoss_Zealot:
		unitScore[Protoss_Zealot]				+= (size * 0.05) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.95) / vis(Protoss_Dragoon);
		unitScore[Protoss_Reaver]				+= (size * 0.90) / vis(Protoss_Reaver);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Protoss_Dragoon:
		unitScore[Protoss_Zealot]				+= (size * 0.05) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.95) / vis(Protoss_Dragoon);
		unitScore[Protoss_Reaver]				+= (size * .60) / vis(Protoss_Reaver);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
		unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
		break;
	case Enum::Protoss_High_Templar:
		unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
		break;
	case Enum::Protoss_Dark_Templar:
		unitScore[Protoss_Reaver]				+= (size * 1.00) / vis(Protoss_Reaver);
		unitScore[Protoss_Observer]				+= (size * 1.00) / vis(Protoss_Observer);
		break;
	case Enum::Protoss_Reaver:
		unitScore[Protoss_Reaver]				+= (size * 1.00) / vis(Protoss_Reaver);
		break;
	case Enum::Protoss_Archon:
		unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
		break;
	case Enum::Protoss_Dark_Archon:
		unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
		break;
	case Enum::Protoss_Scout:
		break;
	case Enum::Protoss_Carrier:
		//unitScore[Protoss_Scout]				+= (size * 1.00) / vis(Protoss_Scout);
		break;
	case Enum::Protoss_Arbiter:
		unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
		break;
	case Enum::Protoss_Corsair:
		unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
		break;
	}
}

void StrategyManager::updateTerranUnitScore(UnitType unit, int count)
{
	for (auto &t : unitScore)
		if (!BuildOrder().isUnitUnlocked(t.first))
			t.second = 0.0;


	for (auto &t : BuildOrder().getUnlockedList()) {
		UnitInfo dummy;
		dummy.setType(t);
		dummy.setPlayer(Broodwar->self());
		dummy.setGroundRange(Util().groundRange(dummy));
		dummy.setAirRange(Util().airRange(dummy));
		dummy.setGroundDamage(Util().groundDamage(dummy));
		dummy.setAirDamage(Util().airDamage(dummy));
		dummy.setSpeed(Util().speed(dummy));

		double dps = unit.isFlyer() ? Util().airDPS(dummy) : Util().groundDPS(dummy);
		if (t == UnitTypes::Terran_Medic)
			dps = 0.775;

		if (t == UnitTypes::Terran_Dropship)
			unitScore[t] = 10.0;

		else if (unitScore[t] <= 0.0)
			unitScore[t] += (dps*count / max(1.0, (double)Broodwar->self()->visibleUnitCount(t)));
		else
			unitScore[t] = (unitScore[t] * (9999.0 / 10000.0)) + ((dps * (double)count) / (10000.0 * max(1.0, (double)Broodwar->self()->visibleUnitCount(t))));
	}
}

void StrategyManager::updateMadMixScore(UnitType unit, int count)
{
	if (unit.isBuilding())
		return;

	using namespace UnitTypes;
	vector<UnitType> allUnits;
	if (Broodwar->self()->getRace() == Races::Protoss) {

	}
	else if (Broodwar->self()->getRace() == Races::Terran)
		allUnits.insert(allUnits.end(), { Terran_Marine, Terran_Firebat, Terran_Medic, Terran_Ghost, Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Goliath, Terran_Wraith, Terran_Dropship, Terran_Science_Vessel, Terran_Battlecruiser, Terran_Valkyrie });
	else if (Broodwar->self()->getRace() == Races::Zerg)
		allUnits.insert(allUnits.end(), { Zerg_Drone, Zerg_Zergling, Zerg_Hydralisk, Zerg_Mutalisk, Zerg_Devourer, Zerg_Scourge });

	if (Broodwar->getFrameCount() > 30000)
		allUnits.push_back(Zerg_Ultralisk);

	if (Broodwar->self()->getRace() == Races::Zerg && unit.isWorker()) {
		UnitType t = Zerg_Drone;
		int s = t.supplyRequired();
		unitScore[t] = (1.0 * (double)Units().getSupply() * (double)count) / max(1.0, t * (double)Broodwar->self()->visibleUnitCount(t));
	}
	else {
		for (auto &t : allUnits) {
			UnitInfo dummy;
			dummy.setType(t);
			dummy.setPlayer(Broodwar->self());
			dummy.setGroundRange(Util().groundRange(dummy));
			dummy.setAirRange(Util().airRange(dummy));
			dummy.setGroundDamage(Util().groundDamage(dummy));
			dummy.setAirDamage(Util().airDamage(dummy));
			dummy.setSpeed(Util().speed(dummy));

			double dps = unit.isFlyer() ? Util().airDPS(dummy) : Util().groundDPS(dummy);
			if (t == UnitTypes::Terran_Medic)
				dps = 0.775;

			int s = t.supplyRequired();
			if (unitScore[t] <= 0.0)
				unitScore[t] += (dps*count / max(1.0, t * (double)Broodwar->self()->visibleUnitCount(t)));
			else
				unitScore[t] = (unitScore[t] * (999.0 / 1000.0)) + ((dps * (double)count) / (1000.0 * max(1.0, t * (double)Broodwar->self()->visibleUnitCount(t))));
		}
	}
}