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
	updateScoutTargets();
	Display().performanceTest(__FUNCTION__);
}

void StrategyManager::updateSituationalBehaviour()
{
	// Reset unit score for toss
	if (Broodwar->self()->getRace() == Races::Protoss) {
		for (auto &unit : unitScore)
			unit.second = 0;
	}

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
	if (BuildOrder().isFastExpand())
		return true;

	if (Broodwar->self()->getRace() == Races::Zerg)
		return (rush && Units().getSupply() >= 36);

	if (Units().getGlobalEnemyGroundStrength() > Units().getGlobalAllyGroundStrength())
		return false;

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
	if (BuildOrder().getCurrentBuild() == "PDTExpand" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Templar_Archives) == 0)
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

	if (enemyBuild == "Z1HatchLurker" || enemyBuild == "Z2HatchLurker" || enemyBuild == "P1GateDT")
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
	if (Players().getPlayers().size() > 1 || (Broodwar->getFrameCount() - enemyFrame > 2000 && enemyFrame != 0 && enemyBuild != "Unknown") || confidentEnemyBuild)
		return;

	if (enemyFrame == 0 && enemyBuild != "Unknown")
		enemyFrame = Broodwar->getFrameCount();

	for (auto &p : Players().getPlayers()) {
		PlayerInfo &player = p.second;
		if (player.getRace() == Races::Zerg || Broodwar->enemy()->getRace() == Races::Zerg) {

			// 5 Hatch build detection
			if (Stations().getEnemyStations().size() >= 3 || (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) + Units().getEnemyCount(UnitTypes::Zerg_Lair) >= 4 && Units().getEnemyCount(UnitTypes::Zerg_Drone) >= 14))
				enemyBuild = "Z5Hatch";

			// Zergling frame
			if (lingFrame == 0 && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 6) {
				lingFrame = Broodwar->getFrameCount();
				if (!Terrain().getEnemyStartingPosition().isValid())
					rush = true;
			}

			for (auto &u : Units().getEnemyUnits()) {
				UnitInfo &unit = u.second;

				// Monitor gas intake or gas steal
				if (unit.getType().isRefinery() && unit.unit()->exists()) {
					if (Terrain().isInAllyTerritory(unit.getTilePosition()))
						gasSteal = true;
					else
						enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
				}

				// Zergling build detection and pool timing
				if (unit.getType() == UnitTypes::Zerg_Spawning_Pool) {

					if (poolFrame == 0 && unit.unit()->exists())
						poolFrame = Broodwar->getFrameCount() + int(double(unit.getType().buildTime()) * (double(unit.getType().maxHitPoints() - unit.unit()->getHitPoints()) / double(unit.getType().maxHitPoints())));

					if (poolFrame > 0 && Units().getEnemyCount(UnitTypes::Zerg_Spire) == 0 && Units().getEnemyCount(UnitTypes::Zerg_Hydralisk_Den) == 0 && Units().getEnemyCount(UnitTypes::Zerg_Lair) == 0) {
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
				else if (unit.getType() == UnitTypes::Zerg_Hydralisk_Den) {
					if (Units().getEnemyCount(UnitTypes::Zerg_Spire) == 0) {
						if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 3)
							enemyBuild = "Z3HatchHydra";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 2)
							enemyBuild = "Z2HatchHydra";
						else if(Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 1)
							enemyBuild = "Z1HatchHydra";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Lair) + Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 2)
							enemyBuild = "Z2HatchLurker";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Lair) == 1 && Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 0)
							enemyBuild = "Z1HatchLurker";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) >= 4)
							enemyBuild = "Z5Hatch";
					}
					else
						enemyBuild = "Unknown";
				}

				// Mutalisk build detection
				else if (unit.getType() == UnitTypes::Zerg_Spire || unit.getType() == UnitTypes::Zerg_Lair) {
					if (Units().getEnemyCount(UnitTypes::Zerg_Hydralisk_Den) == 0) {
						if (Units().getEnemyCount(UnitTypes::Zerg_Lair) + Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 3 && Units().getEnemyCount(UnitTypes::Zerg_Drone) < 14)
							enemyBuild = "Z3HatchMuta";
						else if (Units().getEnemyCount(UnitTypes::Zerg_Lair) + Units().getEnemyCount(UnitTypes::Zerg_Hatchery) == 2)
							enemyBuild = "Z2HatchMuta";
					}
					else if (Units().getEnemyCount(UnitTypes::Zerg_Hatchery) >= 4)
						enemyBuild = "Z5Hatch";
					else
						enemyBuild = "Unknown";
				}
			}
		}
		if (player.getRace() == Races::Protoss || Broodwar->enemy()->getRace() == Races::Protoss) {

			// Detect missing buildings as a potential 2Gate
			if (Terrain().getEnemyStartingPosition().isValid() && Broodwar->isExplored((TilePosition)Terrain().getEnemyStartingPosition()) && Units().getEnemyCount(UnitTypes::Protoss_Gateway) == 0 && Units().getEnemyCount(UnitTypes::Protoss_Forge) == 0 && Units().getEnemyCount(UnitTypes::Protoss_Assimilator) == 0 && Units().getEnemyCount(UnitTypes::Protoss_Nexus) == 1 && Units().getEnemyCount(UnitTypes::Protoss_Photon_Cannon) == 0) {
				enemyBuild = "P2Gate";
				//confidentEnemyBuild = true;
			}
			else if (enemyBuild == "P2Gate")
				enemyBuild = "Unknown";
			

			for (auto &u : Units().getEnemyUnits()) {
				UnitInfo &unit = u.second;

				if (Terrain().isInAllyTerritory(unit.getTilePosition()) || (inboundScoutFrame > 0 && inboundScoutFrame - Broodwar->getFrameCount() < 64))
					enemyScout = true;
				if (unit.getType().isWorker() && inboundScoutFrame == 0) {
					auto dist = unit.getPosition().getDistance(mapBWEB.getMainPosition());
					inboundScoutFrame = Broodwar->getFrameCount() + (dist / unit.getType().topSpeed());
				}

				// Monitor gas intake or gas steal
				if (unit.getType().isRefinery() && unit.unit()->exists()) {
					if (Terrain().isInAllyTerritory(unit.getTilePosition()))
						gasSteal = true;
					else
						enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
				}

				// PCannonRush
				if (unit.getType() == UnitTypes::Protoss_Forge) {
					if (unit.getPosition().getDistance(Terrain().getEnemyStartingPosition()) < 320.0 && Units().getEnemyCount(UnitTypes::Protoss_Gateway) == 0) {
						enemyBuild = "PCannonRush";
						proxy = true;
					}
					else if (enemyBuild == "PCannonRush") {
						enemyBuild = "Unknown";
						proxy = false;
					}
				}

				// PFFE
				if (unit.getType() == UnitTypes::Protoss_Photon_Cannon && Units().getEnemyCount(UnitTypes::Protoss_Robotics_Facility) == 0) {
					if (unit.getPosition().getDistance((Position)Terrain().getEnemyNatural()) < 320.0)
						enemyBuild = "PFFE";
					else if (enemyBuild == "PFFE")
						enemyBuild = "Unknown";
				}

				// P2GateExpand
				if (unit.getType() == UnitTypes::Protoss_Nexus) {
					if (!Terrain().isStartingBase(unit.getTilePosition()) && Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2) {
						enemyBuild = "P2GateExpand";
						scoutTargets.insert((Position)Terrain().getEnemyNatural());
					}
				}

				// Proxy Builds
				if (unit.getType() == UnitTypes::Protoss_Gateway || unit.getType() == UnitTypes::Protoss_Pylon) {
					if (Terrain().isInAllyTerritory(unit.getTilePosition()) || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0 || (mapBWEB.getNaturalChoke() && unit.getPosition().getDistance((Position)mapBWEB.getNaturalChoke()->Center()) < 480.0)) {
						proxy = true;
						scoutTargets.insert(unit.getPosition());

						if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2)
							enemyBuild = "P2Gate";
					}
				}

				// P1GateCore
				if (unit.getType() == UnitTypes::Protoss_Cybernetics_Core) {
					if (unit.unit()->isUpgrading())
						goonRange = true;

					if (Units().getEnemyCount(UnitTypes::Protoss_Robotics_Facility) >= 1 && Units().getEnemyCount(UnitTypes::Protoss_Gateway) <= 1)
						enemyBuild = "P1GateRobo";
					else if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 4)
						enemyBuild = "P4Gate";
					else if (Units().getEnemyCount(UnitTypes::Protoss_Nexus) == 1 && (Units().getEnemyCount(UnitTypes::Protoss_Citadel_of_Adun) >= 1 || Units().getEnemyCount(UnitTypes::Protoss_Templar_Archives) >= 1 || (!goonRange && Units().getEnemyCount(UnitTypes::Protoss_Dragoon) < 2 && Units().getSupply() > 80)))
						enemyBuild = "P1GateDT";
					else
						enemyBuild = "Unknown";
				}

				if (unit.getType() == UnitTypes::Protoss_Gateway) {
					if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2 && Units().getEnemyCount(UnitTypes::Protoss_Nexus) <= 1 && Units().getEnemyCount(UnitTypes::Protoss_Assimilator) <= 0 && Units().getEnemyCount(UnitTypes::Protoss_Cybernetics_Core) <= 0 && Units().getEnemyCount(UnitTypes::Protoss_Dragoon) <= 0)
						enemyBuild = "P2Gate";
					else if (enemyBuild == "P2Gate")
						enemyBuild = "Unknown";
				}				

				// Temp test for 4Gate
				//if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Gateway) >= 3 && Broodwar->getFrameCount() < 12000 && Units().getSupply() >= 40 && Units().getEnemyCount(UnitTypes::Protoss_Dragoon) >= Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon))
					//enemyBuild = "P4Gate";

				// Proxy Detection
				if (unit.getType() == UnitTypes::Protoss_Pylon && unit.getPosition().getDistance(Terrain().getPlayerStartingPosition()) < 960.0)
					proxy = true;

				// FE Detection
				if (unit.getType().isResourceDepot() && !Terrain().isStartingBase(unit.getTilePosition())) {
					enemyFE = true;
				}
			}
		}
		if (player.getRace() == Races::Terran || Broodwar->enemy()->getRace() == Races::Terran) {
			for (auto &u : Units().getEnemyUnits()) {
				UnitInfo &unit = u.second;

				// Monitor gas intake or gas steal
				if (unit.getType().isRefinery() && unit.unit()->exists()) {
					if (Terrain().isInAllyTerritory(unit.getTilePosition()))
						gasSteal = true;
					else
						enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
				}

				// TSiegeExpand
				if ((unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && Units().getEnemyCount(UnitTypes::Terran_Vulture) == 0) || (unit.getType().isResourceDepot() && !Terrain().isStartingBase(unit.getTilePosition()) && Units().getEnemyCount(UnitTypes::Terran_Machine_Shop) > 0))
					enemyBuild = "TSiegeExpand";

				// Barracks Builds
				if (unit.getType() == UnitTypes::Terran_Barracks) {

					if (Terrain().isInAllyTerritory(unit.getTilePosition()) || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0 || (mapBWEB.getNaturalChoke() && unit.getPosition().getDistance((Position)mapBWEB.getNaturalChoke()->Center()) < 320)) {
						enemyBuild = "TBBS";
						proxy = true;
						scoutTargets.insert(unit.getPosition());
					}
					else if (Units().getEnemyCount(UnitTypes::Terran_Academy) >= 1 && Units().getEnemyCount(UnitTypes::Terran_Engineering_Bay) >= 1)
						enemyBuild = "TSparks";
					else {
						enemyBuild = "Unknown";
						proxy = false;
					}
				}

				// FE Detection
				if (unit.getType().isResourceDepot() && !Terrain().isStartingBase(unit.getTilePosition()))
					enemyFE = true;
			}

			if (Broodwar->getFrameCount() - enemyFrame > 200 && Terrain().getEnemyStartingPosition().isValid() && Broodwar->isExplored((TilePosition)Terrain().getEnemyStartingPosition()) && Units().getEnemyCount(UnitTypes::Terran_Barracks) == 0)
				scoutTargets.insert(mapBWEM.Center());
			else
				scoutTargets.erase(mapBWEM.Center());

			if (Units().getSupply() < 60 && ((Units().getEnemyCount(UnitTypes::Terran_Barracks) >= 2 && Units().getEnemyCount(UnitTypes::Terran_Refinery) == 0) || (Units().getEnemyCount(UnitTypes::Terran_Marine) > 5 && Units().getEnemyCount(UnitTypes::Terran_Bunker) <= 0 && Broodwar->getFrameCount() < 6000)))
				enemyBuild = "TBBS";
			if (Units().getEnemyCount(UnitTypes::Terran_Factory) >= 3)
				enemyBuild = "T3Fact";

		}
	}
}

void StrategyManager::updateScoutTargets()
{	
	scoutTargets.clear();

	// If enemy start is valid and explored, add a target to the most recent one to scout
	if (Terrain().foundEnemy()) {	
		for (auto &s : Stations().getEnemyStations()) {
			auto &station = s.second;
			TilePosition tile(station.BWEMBase()->Center());
			if (tile.isValid())
				scoutTargets.insert(Position(tile));
		}
		if (Players().vZ() && Stations().getEnemyStations().size() == 1 && Strategy().getEnemyBuild() != "Unknown")
			scoutTargets.insert((Position)Terrain().getEnemyExpand());
	}

	// If we know where it is but it isn't explored
	else if (Terrain().getEnemyStartingTilePosition().isValid())
		scoutTargets.insert(Terrain().getEnemyStartingPosition());

	// If we have no idea where the enemy is
	else if (!Terrain().getEnemyStartingTilePosition().isValid()) {
		double best = DBL_MAX;
		Position pos = Positions::Invalid;
		int basesExplored = 0;
		for (auto &tile : mapBWEM.StartingLocations()) {
			Position center = Position(tile) + Position(64, 48);
			double dist = center.getDistance(mapBWEB.getMainPosition());
			if (Broodwar->isExplored(tile))
				basesExplored++;

			if (!Broodwar->isExplored(tile))
				scoutTargets.insert(center);
		}

		// If we have scouted 2 bases (including our own), scout the middle for a proxy if it's walkable
		if (basesExplored == 2 && !Broodwar->isExplored((TilePosition)mapBWEM.Center()) && mapBWEB.getGroundDistance(mapBWEB.getMainPosition(), mapBWEM.Center()) != DBL_MAX)
			scoutTargets.insert(mapBWEM.Center());
	}

	// If it's a 2gate, scout for an expansion if we found the gates
	if (enemyBuild == "P2Gate") {
		if (Units().getEnemyCount(UnitTypes::Protoss_Gateway) >= 2)
			scoutTargets.insert((Position)Terrain().getEnemyExpand());
		else if (Units().getEnemyCount(UnitTypes::Protoss_Pylon) == 0 || proxy)
			scoutTargets.insert(mapBWEM.Center());
	}

	// If it's a cannon rush, scout the main
	if (enemyBuild == "PCannonRush")
		scoutTargets.insert(mapBWEB.getMainPosition());
}

void StrategyManager::updateBullets()
{

}

void StrategyManager::updateScoring()
{
	// Unit score based off enemy composition	
	for (auto &t : Units().getEnemyComposition()) {
		if (t.first.isBuilding())
			continue;

		// For each type, add a score to production based on the unit count divided by our current unit count
		if (Broodwar->self()->getRace() == Races::Protoss)
			updateProtossUnitScore(t.first, t.second);
	}

	bool MadMix = Broodwar->self()->getRace() != Races::Protoss;
	if (MadMix)
		updateMadMixScore();

	if (Broodwar->self()->getRace() == Races::Terran)
		unitScore[UnitTypes::Terran_Medic] = unitScore[UnitTypes::Terran_Marine];

	if (Broodwar->self()->getRace() == Races::Protoss) {

		for (auto &t : unitScore) {
			t.second = log(t.second);
		}

		unitScore[UnitTypes::Protoss_Shuttle] = getUnitScore(UnitTypes::Protoss_Reaver);

		if (Broodwar->mapFileName().find("BlueStorm") != string::npos)
			unitScore[UnitTypes::Protoss_Carrier] = unitScore[UnitTypes::Protoss_Arbiter];

		if (Broodwar->enemy()->getRace() == Races::Protoss && Broodwar->getFrameCount() >= 20000 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0) {
			unitScore[UnitTypes::Protoss_Zealot] = unitScore[UnitTypes::Protoss_Dragoon];
			unitScore[UnitTypes::Protoss_Archon] = unitScore[UnitTypes::Protoss_Dragoon];
			unitScore[UnitTypes::Protoss_High_Templar] += unitScore[UnitTypes::Protoss_Dragoon];
			unitScore[UnitTypes::Protoss_Dragoon] = 0.0;
		}
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
	for (auto &unit : unitScore) {
		if (BuildOrder().isUnitUnlocked(unit.first) && unit.second > best) {
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
		unitScore[Protoss_Zealot]				+= (size * 0.25) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.75) / vis(Protoss_Dragoon);
		unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / (Protoss_High_Templar);
		break;
	case Enum::Terran_Siege_Tank_Siege_Mode:
		unitScore[Protoss_Zealot]				+= (size * 0.75) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.25) / vis(Protoss_Dragoon);
		unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
		unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
		break;
	case Enum::Terran_Siege_Tank_Tank_Mode:
		unitScore[Protoss_Zealot]				+= (size * 0.75) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.25) / vis(Protoss_Dragoon);
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
		unitScore[Protoss_Dark_Templar]			+= (size * 0.20) / vis(Protoss_Dark_Templar);
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
		unitScore[Protoss_Zealot]				+= (size * 0.15) / vis(Protoss_Zealot);
		unitScore[Protoss_Dragoon]				+= (size * 0.85) / vis(Protoss_Dragoon);
		unitScore[Protoss_Reaver]				+= (size * 0.60) / vis(Protoss_Reaver);
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
		if (Terrain().isIslandMap())
			unitScore[Protoss_Scout]				+= (size * 1.00) / vis(Protoss_Scout);
		break;
	case Enum::Protoss_Carrier:
		if (Terrain().isIslandMap())
			unitScore[Protoss_Scout]				+= (size * 1.00) / vis(Protoss_Scout);
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
	//for (auto &t : unitScore)
	//	if (!BuildOrder().isUnitUnlocked(t.first))
	//		t.second = 0.0;


	//for (auto &t : BuildOrder().getUnlockedList()) {
	//	UnitInfo dummy;
	//	dummy.setType(t);
	//	dummy.setPlayer(Broodwar->self());
	//	dummy.setGroundRange(Util().groundRange(dummy));
	//	dummy.setAirRange(Util().airRange(dummy));
	//	dummy.setGroundDamage(Util().groundDamage(dummy));
	//	dummy.setAirDamage(Util().airDamage(dummy));
	//	dummy.setSpeed(Util().speed(dummy));

	//	double dps = unit.isFlyer() ? Util().airDPS(dummy) : Util().groundDPS(dummy);
	//	if (t == UnitTypes::Terran_Medic)
	//		dps = 0.775;

	//	if (t == UnitTypes::Terran_Dropship)
	//		unitScore[t] = 10.0;

	//	else if (unitScore[t] <= 0.0)
	//		unitScore[t] += (dps*count / max(1.0, (double)Broodwar->self()->visibleUnitCount(t)));
	//	else
	//		unitScore[t] = (unitScore[t] * (9999.0 / 10000.0)) + ((dps * (double)count) / (10000.0 * max(1.0, (double)Broodwar->self()->visibleUnitCount(t))));
	//}
}

void StrategyManager::updateMadMixScore()
{

	using namespace UnitTypes;
	vector<UnitType> allUnits;
	if (Broodwar->self()->getRace() == Races::Protoss) {

	}
	else if (Broodwar->self()->getRace() == Races::Terran)
		allUnits.insert(allUnits.end(), { Terran_Marine, Terran_Medic, Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Goliath, Terran_Wraith, Terran_Dropship, Terran_Science_Vessel, Terran_Valkyrie });
	else if (Broodwar->self()->getRace() == Races::Zerg)
		allUnits.insert(allUnits.end(), { Zerg_Drone, Zerg_Zergling, Zerg_Hydralisk, Zerg_Lurker, Zerg_Mutalisk, Zerg_Scourge });

	// TODO: tier 1,2,3 vectors
	if (Broodwar->getFrameCount() > 20000) {
		if (Broodwar->self()->getRace() == Races::Terran) {
			allUnits.push_back(Terran_Battlecruiser);
			allUnits.push_back(Terran_Ghost);
		}
		else if (Broodwar->self()->getRace() == Races::Zerg) {
			allUnits.push_back(Zerg_Ultralisk);
			allUnits.push_back(Zerg_Guardian);
			allUnits.push_back(Zerg_Devourer);
		}
	}

	for (auto &u : Units().getEnemyUnits()) {
		auto &unit = u.second;
		auto type = unit.getType();

		if (Broodwar->self()->getRace() == Races::Zerg && type.isWorker()) {
			UnitType t = Zerg_Drone;
			double vis = max(1.0, (double(Broodwar->self()->visibleUnitCount(t))));
			int s = t.supplyRequired();
			unitScore[t] = (5.0 / max(1.0, vis));
		}
		else {
			for (auto &t : allUnits) {

				UnitInfo dummy;
				dummy.createDummy(t);

				if (!unit.getPosition().isValid() || type.isBuilding() || type.isSpell())
					continue;

				double myDPS = type.isFlyer() ? Util().airDPS(dummy) : Util().groundDPS(dummy);
				double enemyDPS = t.isFlyer() ? Util().airDPS(unit) : Util().groundDPS(unit);

				if (unit.getType() == UnitTypes::Terran_Medic)
					enemyDPS = 0.775;

				double overallMatchup = enemyDPS > 0.0 ? (myDPS, myDPS / enemyDPS) : myDPS;
				double distTotal = Terrain().getEnemyStartingPosition().isValid() ? mapBWEB.getMainPosition().getDistance(Terrain().getEnemyStartingPosition()) : 1.0;
				double distUnit = Terrain().getEnemyStartingPosition().isValid() ? unit.getPosition().getDistance(mapBWEB.getMainPosition()) / distTotal : 1.0;

				if (distUnit == 0.0)
					distUnit = 0.1;

				double vis = max(1.0, (double(Broodwar->self()->visibleUnitCount(t))));

				if (unitScore[t] <= 0.0)
					unitScore[t] += (overallMatchup / max(1.0, vis * distUnit));
				else
					unitScore[t] = (unitScore[t] * (999.0 / 1000.0)) + (overallMatchup / (1000.0 * vis * distUnit));
			}
		}
	}
}