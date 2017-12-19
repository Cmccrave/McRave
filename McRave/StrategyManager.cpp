#include "McRave.h"

void StrategyTrackerClass::update()
{
	Display().startClock();
	updateSituationalBehaviour();
	updateEnemyBuild();
	updateBullets();
	updateScoring();
	Display().performanceTest(__FUNCTION__);
}

void StrategyTrackerClass::updateSituationalBehaviour()
{
	// Reset unit score
	for (auto &unit : unitScore) unit.second = 0;

	// Get strategy based on race
	if (Broodwar->self()->getRace() == Races::Protoss)
		protossStrategy();
	else if (Broodwar->self()->getRace() == Races::Terran)
		terranStrategy();
	else if (Broodwar->self()->getRace() == Races::Zerg)
		zergStrategy();
}

void StrategyTrackerClass::protossStrategy()
{
	// Check if it's early enough to run specific strategies
	if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
	{
		if (Terrain().getEnemyStartingTilePosition().isValid() && !Broodwar->isExplored(Terrain().getEnemyStartingTilePosition()) && Units().getEnemyComposition()[UnitTypes::Terran_Command_Center] >= 1)
			enemyFastExpand = true;
		else enemyFastExpand = false;

		// Check if there's a fast expansion for ally or enemy
		if (BuildOrder().isNexusFirst() || BuildOrder().isForgeExpand())
			allyFastExpand = true;
		if (Units().getEnemyComposition()[UnitTypes::Terran_Command_Center] > 1 || (Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] + Units().getEnemyComposition()[UnitTypes::Zerg_Lair]) > 1 || Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] > 1)
			enemyFastExpand = true;

		

		// Check early game strategies
		rush = shouldDefendRush();
		hideTech = shouldHideTech();
		holdChoke = shouldHoldChoke();
		playPassive = shouldPlayPassive();
	}
	else
	{
		rush = false;
		holdChoke = true;
		playPassive = false;
		allyFastExpand = false;
		enemyFastExpand = false;
	}

	// Check if we need an observer
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) <= 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Dark_Templar] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Citadel_of_Adun] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Templar_Archives] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Ghost] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Vulture] >= 10 || Units().getEnemyComposition()[UnitTypes::Zerg_Lurker] > 0 || (Units().getEnemyComposition()[UnitTypes::Zerg_Lair] == 1 && Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk_Den] >= 1 && Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] == 0)))
		invis = true;
	else invis = false;
}

void StrategyTrackerClass::terranStrategy()
{
	// If it's early on and we're being rushed
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) < 2)
	{
		// Ramp holding logic
		if ((Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)) < 2)
			holdChoke = false;
		else holdChoke = true;

		// Check if enemy is rushing (detects early 2 gates and early pool)
		if (Units().getSupply() < 60 && ((Players().getNumberProtoss() > 0 || Players().getNumberRandom() > 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Forge] == 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] >= 2 || Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] == 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Assimilator] == 0 && Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] == 1) || ((Players().getNumberRandom() > 0 || Players().getNumberZerg() > 0) && Units().getEnemyComposition()[UnitTypes::Zerg_Zergling] >= 6 && Units().getEnemyComposition()[UnitTypes::Zerg_Drone] < 6))
			rush = true;
		else rush = false;
	}
	else
	{
		holdChoke = true;
		rush = false;
	}

	// Check if we need detection
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Comsat_Station) <= 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Dark_Templar] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Citadel_of_Adun] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Templar_Archives] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Vulture] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Ghost] > 0 || Units().getEnemyComposition()[UnitTypes::Zerg_Lurker] > 0 || (Units().getEnemyComposition()[UnitTypes::Zerg_Lair] == 1 && Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk] >= 1 && Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] == 0)))
		invis = true;
	else invis = false;
}

void StrategyTrackerClass::zergStrategy()
{

}

bool StrategyTrackerClass::shouldPlayPassive()
{
	if (Units().getEnemyComposition()[UnitTypes::Terran_Siege_Tank_Siege_Mode] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Siege_Tank_Tank_Mode] > 0) return false;
	else if (BuildOrder().isOpener() && allyFastExpand && !enemyFastExpand) return true;
	else if (hideTech || rush) return true;
	else if (Units().getSupply() > 60) return false;
	return false;
}

bool StrategyTrackerClass::shouldDefendRush()
{
	if (Units().getSupply() > 60) return false;
	else if ((Players().getNumberTerran() > 0 || Players().getNumberRandom() > 0) && Units().getEnemyComposition()[UnitTypes::Terran_SCV] <= 6 && Units().getEnemyComposition()[UnitTypes::Terran_Barracks] > 0 && Units().getEnemyComposition()[UnitTypes::Terran_Supply_Depot] <= 0) return true; // BBS Detection	
	else if ((Players().getNumberProtoss() > 0 || Players().getNumberRandom() > 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Forge] == 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] >= 2 || Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] == 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Assimilator] == 0 && Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] == 1) return true; // Zealot Pressure detection	
	else if ((Players().getNumberZerg() > 0 || Players().getNumberRandom() > 0) && Units().getEnemyComposition()[UnitTypes::Zerg_Zergling] >= 6 && Units().getEnemyComposition()[UnitTypes::Zerg_Drone] < 6) return true; // Early pool detection
	return false;
}

bool StrategyTrackerClass::shouldHoldChoke()
{
	if (allyFastExpand || hideTech) return true;
	else if (Units().getSupply() > 80 || Players().getNumberTerran() > 0 || (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() == Races::Terran)) return true;
	return false;
}

bool StrategyTrackerClass::shouldHideTech()
{
	if (BuildOrder().getCurrentBuild() == "PDTExpand" && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dark_Templar) == 0) return true;
	return false;
}

bool StrategyTrackerClass::shouldGetDetection()
{
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) > 0) return false;
	else if (Units().getEnemyComposition()[UnitTypes::Protoss_Dark_Templar] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Citadel_of_Adun] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Templar_Archives] > 0) return true;
	else if (Units().getEnemyComposition()[UnitTypes::Terran_Ghost] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Vulture] >= 10) return true;
	else if (Units().getEnemyComposition()[UnitTypes::Zerg_Lurker] > 0 || (Units().getEnemyComposition()[UnitTypes::Zerg_Lair] == 1 && Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk_Den] >= 1 && Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] == 0)) return true;
	return false;
}

void StrategyTrackerClass::updateEnemyBuild()
{
	if (Players().getPlayers().size() > 1 || !BuildOrder().isOpener()) return;

	for (auto &p : Players().getPlayers())
	{
		PlayerInfo &player = p.second;
		if (player.getRace() == Races::Zerg)
		{
			if (Units().getEnemyComposition()[UnitTypes::Zerg_Extractor] > 0)
			{
				for (auto &e : Units().getEnemyUnitsFilter(UnitTypes::Zerg_Extractor))
				{
					if (e->exists()) enemyGas = e->getInitialResources() - e->getResources();
				}
			}
			if (Units().getEnemyComposition()[UnitTypes::Zerg_Spawning_Pool] > 0)
			{
				for (auto &p : Units().getEnemyUnitsFilter(UnitTypes::Zerg_Spawning_Pool))
				{
					if (p->exists() && poolFrame == 0)
					{
						poolFrame = Broodwar->getFrameCount() + (double(p->getType().buildTime()) * (double(p->getType().maxHitPoints() - p->getHitPoints()) / double(p->getType().maxHitPoints())));						
					}
				}
			}
			if (Units().getEnemyComposition()[UnitTypes::Zerg_Zergling] >= 6)
			{
				if (lingFrame == 0)
				{
					lingFrame = Broodwar->getFrameCount();
				}
			}

			if (enemyGas < 124 && ((poolFrame < 2500 && poolFrame > 0) || (lingFrame < 3500 && lingFrame > 0))) enemyBuild = "Z5Pool";
			else if (enemyGas < 124 && enemyGas > 0 && poolFrame < 3000 && poolFrame > 0) enemyBuild = "Z9Pool";
			else if (enemyGas < 124 && enemyGas >= 100 && poolFrame > 3000 && Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] >= 3) enemyBuild = "Z3HatchLing";
			else if (enemyGas >= 124 && poolFrame > 3000 && Strategy().isEnemyFastExpand() && Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk_Den] >= 1) enemyBuild = "Z12HatchHydra";
			else if (enemyGas >= 124 && poolFrame > 3000 && Strategy().isEnemyFastExpand() && Units().getEnemyComposition()[UnitTypes::Zerg_Lair] >= 1) enemyBuild = "Z12HatchMuta";
		}
		if (player.getRace() == Races::Protoss)
		{
			bool cannonRush = false;
			if (Units().getEnemyComposition()[UnitTypes::Protoss_Forge] >= 1 && Units().getEnemyComposition()[UnitTypes::Protoss_Photon_Cannon] >= 2)
			{
				for (auto &p : Units().getEnemyUnitsFilter(UnitTypes::Protoss_Photon_Cannon))
				{
					if (p->exists() && !Terrain().isInAllyTerritory(p->getTilePosition())) cannonRush = true;
				}
				if (!cannonRush) enemyBuild = "PFFE";
				else enemyBuild = "PCannonRush";
				return;
			}
			else if (Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] >= 2 && Units().getEnemyComposition()[UnitTypes::Protoss_Assimilator] == 0) enemyBuild = "P2GateZealot";
			else if (Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] <= 1 && Units().getEnemyComposition()[UnitTypes::Protoss_Cybernetics_Core] >= 1) enemyBuild = "P1GateCore";
		}
	}


}

void StrategyTrackerClass::updateBullets()
{
	// Store current psi storms and EMPs
	for (auto& bullet : Broodwar->getBullets())
	{
		if (bullet && bullet->exists() && bullet->getSource() && bullet->getSource()->exists() && bullet->getTarget() && bullet->getTarget()->exists())
		{
			if (bullet->getType() == BulletTypes::Psionic_Storm)
				Grids().updatePsiStorm(bullet);
			if (bullet->getType() == BulletTypes::EMP_Missile)
				Grids().updateEMP(bullet);
		}
	}
}

void StrategyTrackerClass::updateScoring()
{
	// Unit score based off enemy composition
	for (auto &t : Units().getEnemyComposition())
	{
		// For each type, add a score to production based on the unit count divided by our current unit count
		if (Broodwar->self()->getRace() == Races::Protoss)
			updateProtossUnitScore(t.first, t.second);
		else if (Broodwar->self()->getRace() == Races::Terran)
			updateTerranUnitScore(t.first, t.second);
		t.second = 0;
	}

	if (Broodwar->self()->getRace() == Races::Protoss)
		unitScore[UnitTypes::Protoss_Shuttle] += ceil((double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)) / 4.0) + (double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) / 2))) - Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle);

	// Update Unit score based on performance
	for (auto &t : unitScore)
	{
		t.second += unitPerformance[t.first];
	}
}

void StrategyTrackerClass::updateProtossUnitScore(UnitType unit, int cnt)
{
	double size = double(cnt) * double(unit.supplyRequired());
	switch (unit)
	{
	case UnitTypes::Enum::Terran_Marine:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Terran_Medic:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Terran_Firebat:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Terran_Vulture:
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Observer] += (size * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Terran_Goliath:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Siege_Mode:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Terran_Wraith:
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		break;
	case UnitTypes::Enum::Terran_Science_Vessel:
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		break;
	case UnitTypes::Enum::Terran_Battlecruiser:
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		break;
	case UnitTypes::Enum::Terran_Valkyrie:
		break;

	case UnitTypes::Enum::Zerg_Zergling:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Corsair] += (size * 0.60) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Zerg_Hydralisk:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Zerg_Lurker:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Observer] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer)));
		break;
	case UnitTypes::Enum::Zerg_Ultralisk:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 0.80) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.20) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Zerg_Mutalisk:
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Corsair] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		break;
	case UnitTypes::Enum::Zerg_Guardian:
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Corsair] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		break;
	case UnitTypes::Enum::Zerg_Devourer:
		break;
	case UnitTypes::Enum::Zerg_Defiler:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;

	case UnitTypes::Enum::Protoss_Zealot:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * .60) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		unitScore[UnitTypes::Protoss_Dark_Templar] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar)));
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		unitScore[UnitTypes::Protoss_Observer] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer)));
		break;
	case UnitTypes::Enum::Protoss_Reaver:
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Protoss_Archon:
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Archon:
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Scout:
		break;
	case UnitTypes::Enum::Protoss_Carrier:
		unitScore[UnitTypes::Protoss_Scout] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Scout)));
		break;
	case UnitTypes::Enum::Protoss_Arbiter:
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Corsair:
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	}
}

void StrategyTrackerClass::updateTerranUnitScore(UnitType unit, int count)
{
	switch (unit)
	{
	case UnitTypes::Enum::Terran_Marine:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Medic:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Firebat:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.30) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Vulture:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.60) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Goliath:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.60) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Siege_Mode:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.60) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.60) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Wraith:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Science_Vessel:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Battlecruiser:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Terran_Valkyrie:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;

	case UnitTypes::Enum::Zerg_Zergling:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Hydralisk:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.80) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Lurker:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.80) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Ultralisk:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.80) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Mutalisk:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Guardian:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Devourer:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Zerg_Defiler:
		break;

	case UnitTypes::Enum::Protoss_Zealot:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.05) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.95) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Reaver:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.20) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Archon:
		unitScore[UnitTypes::Terran_Vulture] += (count * unit.supplyRequired() * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Vulture)));
		unitScore[UnitTypes::Terran_Siege_Tank_Tank_Mode] += (count * unit.supplyRequired() * 0.70) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)));
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 0.20) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Archon:
		break;
	case UnitTypes::Enum::Protoss_Scout:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Carrier:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Arbiter:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	case UnitTypes::Enum::Protoss_Corsair:
		unitScore[UnitTypes::Terran_Goliath] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Goliath)));
		break;
	}
}

void StrategyTrackerClass::updateZergUnitScore(UnitType unit, int count)
{

}