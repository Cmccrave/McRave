#include "McRave.h"

double UtilTrackerClass::getPercentHealth(UnitInfo& unit)
{
	// Returns the percent of health for a unit, with higher emphasis on health over shields
	return double(unit.unit()->getHitPoints() + (unit.unit()->getShields() / 2)) / double(unit.getType().maxHitPoints() + (unit.getType().maxShields() / 2));
}

double UtilTrackerClass::getMaxGroundStrength(UnitInfo& unit)
{
	// Some hardcoded values that don't have attacks but should still be considered for strength
	if (unit.getType() == UnitTypes::Protoss_Scarab || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Zerg_Egg || unit.getType() == UnitTypes::Zerg_Larva) return 0.0;
	else if (unit.getType() == UnitTypes::Terran_Medic) return 2.5;
	else if (unit.getType() == UnitTypes::Protoss_High_Templar) return 25.0;
	else if (unit.getType() == UnitTypes::Protoss_Reaver) return 25.0;

	double range, damage, hp, speed;
	range = max(0.0, log(unit.getGroundRange()));
	hp = log(double(unit.getType().maxHitPoints() + unit.getType().maxShields()) / 10.0);

	if (unit.getType() == UnitTypes::Protoss_Interceptor) damage = unit.getGroundDamage() / 36.0;
	else if (unit.getType() == UnitTypes::Terran_Bunker) damage = unit.getGroundDamage() / 15.0;
	else if (unit.getType().groundWeapon().damageCooldown() > 0) damage = unit.getGroundDamage() / double(unit.getType().groundWeapon().damageCooldown());	

	double effectiveness = 1.0;
	speed = max(24.0, unit.getSpeed());

	double aLarge = double(Units().getAllySizes()[UnitSizeTypes::Large]);
	double aMedium = double(Units().getAllySizes()[UnitSizeTypes::Medium]);
	double aSmall = double(Units().getAllySizes()[UnitSizeTypes::Small]);

	double eLarge = double(Units().getEnemySizes()[UnitSizeTypes::Large]);
	double eMedium = double(Units().getEnemySizes()[UnitSizeTypes::Medium]);
	double eSmall = double(Units().getEnemySizes()[UnitSizeTypes::Small]);

	if (unit.getPlayer()->isEnemy(Broodwar->self()))
	{
		if (unit.getType().groundWeapon().damageType() == DamageTypes::Explosive)
		{
			effectiveness = double((aLarge*1.0) + (aMedium*0.75) + (aSmall*0.5)) / max(1.0, double(aLarge + aMedium + aSmall));
		}
		else if (unit.getType().groundWeapon().damageType() == DamageTypes::Concussive)
		{
			effectiveness = double((aLarge*0.25) + (aMedium*0.5) + (aSmall*1.0)) / max(1.0, double(aLarge + aMedium + aSmall));
		}
	}
	else
	{
		if (unit.getType().groundWeapon().damageType() == DamageTypes::Explosive)
		{
			effectiveness = double((eLarge*1.0) + (eMedium*0.75) + (eSmall*0.5)) / max(1.0, double(eLarge + eMedium + eSmall));
		}
		else if (unit.getType().groundWeapon().damageType() == DamageTypes::Concussive)
		{
			effectiveness = double((eLarge*0.25) + (eMedium*0.5) + (eSmall*1.0)) / max(1.0, double(eLarge + eMedium + eSmall));
		}
	}

	// Check for Zergling attack speed upgrade
	if (unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Adrenal_Glands))
	{
		damage = damage * 1.33;
	}
	return range * damage * effectiveness * hp;
}

double UtilTrackerClass::getVisibleGroundStrength(UnitInfo& unit)
{
	if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised()) return 0.0;
	return unit.getPercentHealth() * unit.getMaxGroundStrength();
}

double UtilTrackerClass::getMaxAirStrength(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Zerg_Scourge) return 2.0;
	else if (unit.getType() == UnitTypes::Protoss_High_Templar) return 25.0;
	
	double range, damage, hp, speed;
	range = max(0.0, log(unit.getAirRange()));
	hp = log(double(unit.getType().maxHitPoints() + unit.getType().maxShields()) / 10.0);
	damage = unit.getAirDamage() / double(unit.getType().airWeapon().damageCooldown());

	
	if (unit.getType() == UnitTypes::Protoss_Interceptor) damage = unit.getAirDamage() / 36.0;
	else if (unit.getType() == UnitTypes::Terran_Bunker) damage = unit.getAirDamage() / 15.0;
	else if (unit.getType().airWeapon().damageCooldown() > 0) damage = unit.getAirDamage() / double(unit.getType().airWeapon().damageCooldown());	
	else damage = unit.getAirDamage() / 24.0;	

	double effectiveness = 1.0;
	speed = max(24.0, unit.getSpeed());

	double aLarge = double(Units().getAllySizes()[UnitSizeTypes::Large]);
	double aMedium = double(Units().getAllySizes()[UnitSizeTypes::Medium]);
	double aSmall = double(Units().getAllySizes()[UnitSizeTypes::Small]);

	double eLarge = double(Units().getEnemySizes()[UnitSizeTypes::Large]);
	double eMedium = double(Units().getEnemySizes()[UnitSizeTypes::Medium]);
	double eSmall = double(Units().getEnemySizes()[UnitSizeTypes::Small]);

	if (unit.getPlayer()->isEnemy(Broodwar->self()))
	{
		if (unit.getType().airWeapon().damageType() == DamageTypes::Explosive)
		{
			effectiveness = double((aLarge*1.0) + (aMedium*0.75) + (aSmall*0.5)) / max(1.0, double(aLarge + aMedium + aSmall));
		}
		else if (unit.getType().airWeapon().damageType() == DamageTypes::Concussive)
		{
			effectiveness = double((aLarge*0.25) + (aMedium*0.5) + (aSmall*1.0)) / max(1.0, double(aLarge + aMedium + aSmall));
		}
	}
	else
	{
		if (unit.getType().airWeapon().damageType() == DamageTypes::Explosive)
		{
			effectiveness = double((eLarge*1.0) + (eMedium*0.75) + (eSmall*0.5)) / max(1.0, double(eLarge + eMedium + eSmall));
		}
		else if (unit.getType().airWeapon().damageType() == DamageTypes::Concussive)
		{
			effectiveness = double((eLarge*0.25) + (eMedium*0.5) + (eSmall*1.0)) / max(1.0, double(eLarge + eMedium + eSmall));
		}
	}
	return range * damage * effectiveness * hp;
}

double UtilTrackerClass::getVisibleAirStrength(UnitInfo& unit)
{
	if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised()) return 0.0;
	return unit.getPercentHealth() * unit.getMaxAirStrength();
}

double UtilTrackerClass::getPriority(UnitInfo& unit)
{
	// If an enemy detector is within range of an Arbiter, give it higher priority
	if (Grids().getArbiterGrid(unit.getWalkPosition()) > 0 && unit.getType().isDetector() && unit.getPlayer()->isEnemy(Broodwar->self())) return 10.0;
	if (unit.getType().isWorker()) return (Broodwar->getFrameCount() - unit.getLastAttackFrame()) < 500 ? 10.0: 3.0;
	if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine) return 10.0;
	if (unit.getType() == UnitTypes::Protoss_Carrier) return 0.5;

	double mineral, gas;

	if (unit.getType() == UnitTypes::Protoss_Archon) mineral = 100.0, gas = 300.0;
	else if (unit.getType() == UnitTypes::Protoss_Dark_Archon) mineral = 250.0, gas = 200.0;
	else if (unit.getType() == UnitTypes::Zerg_Sunken_Colony || unit.getType() == UnitTypes::Zerg_Spore_Colony) mineral = 175.0, gas = 0.0;
	else mineral = unit.getType().mineralPrice(), gas = unit.getType().gasPrice();

	double strength = max({ unit.getMaxGroundStrength(), unit.getMaxAirStrength(), 1.0 });
	double cost = ((mineral * 0.33) + (gas * 0.66)) * max(double(unit.getType().supplyRequired()), 0.50);
	double survivability = max(24.0, unit.getSpeed()) * (unit.getType().maxHitPoints() + unit.getType().maxShields()) / 100.0;

	if (unit.getType() == UnitTypes::Zerg_Scourge || unit.getType() == UnitTypes::Zerg_Infested_Terran) strength = strength * 5.0; // Higher priority on suicidal units

	return strength * cost / survivability;
}

double UtilTrackerClass::getTrueGroundRange(UnitInfo& unit)
{
	// Range upgrade check for Dragoons, Marines and Hydralisks ground attack
	if (unit.getType() == UnitTypes::Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
	{
		return 192.0;
	}
	else if ((unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines)))
	{
		return 160.0;
	}

	// Range upgrade check and correction of initial range for Bunkers ground attack
	else if (unit.getType() == UnitTypes::Terran_Bunker)
	{
		if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells))
		{
			return 192.0;
		}
		else
		{
			return 160.0;
		}
	}

	// Range assumption for High Templars
	else if (unit.getType() == UnitTypes::Protoss_High_Templar)
	{
		return 288.0;
	}

	// Correction of initial range for Reavers 
	else if (unit.getType() == UnitTypes::Protoss_Reaver)
	{
		return 256.0;
	}
	return double(unit.getType().groundWeapon().maxRange());
}

double UtilTrackerClass::getTrueAirRange(UnitInfo& unit)
{
	// Range upgrade check for Dragoons, Marines and Hydralisks air attack
	if (unit.getType() == UnitTypes::Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
	{
		return 192.0;
	}
	else if ((unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines)))
	{
		return 160.0;
	}

	// Range upgrade check and correction of initial range for Bunkers air attack
	else if (unit.getType() == UnitTypes::Terran_Bunker)
	{
		if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells))
		{
			return 192.0;
		}
		else
		{
			return 160.0;
		}
	}

	// Range upgrade check for Goliaths air attack
	else if (unit.getType() == UnitTypes::Terran_Goliath && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Charon_Boosters))
	{
		return 256.0;
	}

	// Range assumption for High Templars
	else if (unit.getType() == UnitTypes::Protoss_High_Templar)
	{
		return 288.0;
	}

	// Else return the Units base range for air weapons
	return double(unit.getType().airWeapon().maxRange());
}

double UtilTrackerClass::getTrueGroundDamage(UnitInfo& unit)
{
	// Damage upgrade check for Reavers and correction of initial damage
	if (unit.getType() == UnitTypes::Protoss_Reaver)
	{
		if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Scarab_Damage))
		{
			return 125.00;
		}
		else
		{
			return 100.00;
		}
	}

	// Damage assumption for Bunkers ground attack
	else if (unit.getType() == UnitTypes::Terran_Bunker)
	{
		return 24.0;
	}

	// Damage correction for Zealots and Firebats which attack twice for 8 damage
	else if (unit.getType() == UnitTypes::Terran_Firebat || unit.getType() == UnitTypes::Protoss_Zealot)
	{
		return 16.0;
	}

	// Else return the Units base ground weapon damage
	else
	{
		return unit.getType().groundWeapon().damageAmount();
	}

	return 0.0;
}

double UtilTrackerClass::getTrueAirDamage(UnitInfo& unit)
{
	// Damage assumption for Bunkers air attack
	if (unit.getType() == UnitTypes::Terran_Bunker)
	{
		return 24.0;
	}

	if (unit.getType() == UnitTypes::Protoss_Scout)
	{
		return 28.0;
	}

	if (unit.getType() == UnitTypes::Terran_Valkyrie)
	{
		return 48.0;
	}

	// Else return the Units base air weapon damage
	return unit.getType().airWeapon().damageAmount();
}

double UtilTrackerClass::getTrueSpeed(UnitInfo& unit)
{
	double speed = unit.getType().topSpeed() * 24.0;

	if ((unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments)) || (unit.getType() == UnitTypes::Zerg_Ultralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Anabolic_Synthesis)) || (unit.getType() == UnitTypes::Protoss_Shuttle && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Drive)) || (unit.getType() == UnitTypes::Protoss_Observer && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Boosters)) || (unit.getType() == UnitTypes::Protoss_Zealot && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements)) || (unit.getType() == UnitTypes::Terran_Vulture && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)))
	{
		speed = speed * 1.5;
	}
	else if (unit.getType() == UnitTypes::Zerg_Overlord && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace))
	{
		speed = speed * 4.01;
	}
	else if (unit.getType() == UnitTypes::Protoss_Scout && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments))
	{
		speed = speed * 1.33;
	}
	else if (unit.getType().isBuilding())
	{
		return 0.0;
	}
	return speed;
}

int UtilTrackerClass::getMinStopFrame(UnitType unitType)
{
	if (unitType == UnitTypes::Protoss_Dragoon)
	{
		return 9;
	}

	// TODO: Add other units
	return 0;
}

WalkPosition UtilTrackerClass::getWalkPosition(Unit unit)
{
	int pixelX = unit->getPosition().x;
	int pixelY = unit->getPosition().y;

	// If it's a unit, we want to find the closest mini tile with the highest resolution (actual pixel width/height)
	if (!unit->getType().isBuilding())
	{
		int walkX = int((pixelX - (0.5*unit->getType().width())) / 8);
		int walkY = int((pixelY - (0.5*unit->getType().height())) / 8);
		return WalkPosition(walkX, walkY);
	}
	// For buildings, we want the actual tile size resolution (convert half the tile size to pixels by 0.5*tile*32 = 16.0)
	else
	{
		int walkX = int((pixelX - (16.0*unit->getType().tileWidth())) / 8);
		int walkY = int((pixelY - (16.0*unit->getType().tileHeight())) / 8);
		return WalkPosition(walkX, walkY);
	}
	return WalkPositions::None;
}

set<WalkPosition> UtilTrackerClass::getWalkPositionsUnderUnit(Unit unit)
{
	WalkPosition start = getWalkPosition(unit);
	set<WalkPosition> returnValues;

	for (int x = start.x; x < start.x + unit->getType().tileWidth(); x++)
	{
		for (int y = start.y; y < start.y + unit->getType().tileHeight(); y++)
		{
			returnValues.emplace(WalkPosition(x, y));
		}
	}
	return returnValues;
}

bool UtilTrackerClass::isSafe(WalkPosition end, UnitType unitType, bool groundCheck, bool airCheck)
{
	int walkWidth = unitType.width() / 8;
	int halfWidth = walkWidth / 2;
	for (int x = end.x - halfWidth; x <= end.x + halfWidth; x++)
	{
		for (int y = end.y - halfWidth; y <= end.y + halfWidth; y++)
		{
			if (!WalkPosition(x, y).isValid()) continue;
			if ((groundCheck && Grids().getEGroundThreat(x, y) != 0.0) || (airCheck && Grids().getEAirThreat(x, y) != 0.0))	return false;
			if (Grids().getPsiStormGrid(x, y) > 0 || (Grids().getEMPGrid(x, y) > 0 && unitType.isSpellcaster())) return false;
		}
	}
	return true;
}

bool UtilTrackerClass::isMobile(WalkPosition start, WalkPosition end, UnitType unitType)
{
	if (unitType.isFlyer()) return true;
	int walkWidth = unitType.width() / 8;
	int halfWidth = walkWidth / 2;
	for (int x = end.x - halfWidth; x <= end.x + halfWidth; x++)
	{
		for (int y = end.y - halfWidth; y <= end.y + halfWidth; y++)
		{
			if (!WalkPosition(x, y).isValid()) return false;		
			if (x >= start.x && x < start.x + walkWidth && y >= start.y && y < start.y + walkWidth && Grids().getMobilityGrid(x, y) > 0) continue; // If WalkPosition shared with WalkPositions under unit, ignore
			if (Grids().getMobilityGrid(x, y) <= 0 || Grids().getAntiMobilityGrid(x, y) > 0) return false;
		}
	}
	return true;
}

bool UtilTrackerClass::unitInRange(UnitInfo& unit)
{
	if (!unit.getTarget()) return false;
	UnitInfo& target = Units().getEnemyUnit(unit.getTarget());
	double widths = target.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
	double allyRange = widths + (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
	if (unit.getPosition().getDistance(unit.getTargetPosition()) <= allyRange) return true;
	return false;
}

bool UtilTrackerClass::targetInRange(UnitInfo& unit)
{
	if (!unit.getTarget()) return false;
	UnitInfo& target = Units().getEnemyUnit(unit.getTarget());
	double widths = target.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
	double enemyRange = widths + (target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
	if (target.getPosition().getDistance(unit.getPosition()) <= enemyRange) return true;
	return false;
}

bool UtilTrackerClass::isWalkable(TilePosition here)
{
	WalkPosition start = WalkPosition(here);
	for (int x = start.x; x < start.x + 4; x++)
	{
		for (int y = start.y; y < start.y + 4; y++)
		{
			if (Grids().getMobilityGrid(WalkPosition(x,y)) == -1) return false;
		}
	}
	return true;
}

bool UtilTrackerClass::shouldPullWorker(Unit unit)
{
	if (!Strategy().isHoldChoke() && Terrain().isInAllyTerritory(unit->getTilePosition()) && Grids().getEGroundThreat(getWalkPosition(unit)) > 0.0 && Grids().getResourceGrid(unit->getTilePosition()) > 0 && Units().getSupply() < 60) return true;
	else if (BuildOrder().getCurrentBuild() == "Sparks" && Units().getGlobalGroundStrategy() == 1) return true;
	else if (Strategy().isHoldChoke() && Units().getProxThreat() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon) <= 2) return true;
	else if (!Strategy().isHoldChoke() && Units().getImmThreat() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense()) return true;
	else if ((Strategy().getEnemyBuild() == "Z5Pool" || Strategy().isRush()) && BuildOrder().isForgeExpand() && Units().getGlobalEnemyGroundStrength() > 0.00 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon) < 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) return true;
	else if (Strategy().getEnemyBuild() == "Unknown" && BuildOrder().isForgeExpand() && Units().getGlobalAllyGroundStrength() < 3.00 && Units().getGlobalEnemyGroundStrength() > 0.00 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon) < 2 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) return true;
	return false;
}