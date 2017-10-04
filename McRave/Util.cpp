#include "McRave.h"

double UtilTrackerClass::getPercentHealth(UnitInfo& unit)
{
	// Returns the percent of health for a unit, with higher emphasis on health over shields
	return double(unit.unit()->getHitPoints() + (unit.unit()->getShields() / 2)) / double(unit.getType().maxHitPoints() + (unit.getType().maxShields() / 2));
}

double UtilTrackerClass::getMaxGroundStrength(UnitInfo& unit)
{
	// Some hardcoded values that don't have attacks but should still be considered for strength
	if (unit.getType() == UnitTypes::Terran_Medic)
	{
		return 2.5;
	}
	if (unit.getType() == UnitTypes::Protoss_High_Templar)
	{
		return 10.0;
	}
	if (unit.getType() == UnitTypes::Protoss_Scarab || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Zerg_Egg || unit.getType() == UnitTypes::Zerg_Larva)
	{
		return 0.0;
	}
	if (unit.getType() == UnitTypes::Protoss_Interceptor)
	{
		return 1.0;
	}
	if (unit.getType().isWorker() && unit.getPlayer() == Broodwar->self())
	{
		return 1.5;
	}

	double range, damage;

	range = cbrt(unit.getGroundRange());

	if (unit.getType().groundWeapon().damageCooldown() > 0)
	{
		damage = unit.getGroundDamage() / double(unit.getType().groundWeapon().damageCooldown());
	}
	else if (unit.getType() == UnitTypes::Protoss_Reaver)
	{
		damage = unit.getGroundDamage() / 60.0;
	}
	else if (unit.getType() == UnitTypes::Terran_Bunker)
	{
		damage = unit.getGroundDamage() / 15.0;
	}

	if (!unit.getType().isWorker() && unit.getGroundDamage() > 0.0)
	{
		// Check for Zergling attack speed upgrade
		if (unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Adrenal_Glands))
		{
			damage = damage * 1.33;
		}
		return range * damage;
	}
	return 0.0;
}

double UtilTrackerClass::getVisibleGroundStrength(UnitInfo& unit)
{
	if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised())
	{
		return 0.0;
	}

	double effectiveness = 1.0;

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
	return unit.getPercentHealth() * unit.getMaxGroundStrength() * effectiveness;
}

double UtilTrackerClass::getMaxAirStrength(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Interceptor || unit.getType() == UnitTypes::Zerg_Scourge)
	{
		return 2.5;
	}
	double range, damage;
	damage = unit.getAirDamage() / double(unit.getType().airWeapon().damageCooldown());
	range = cbrt(unit.getAirRange());

	if (unit.getType().airWeapon().damageCooldown() > 0)
	{
		damage = unit.getAirDamage() / double(unit.getType().airWeapon().damageCooldown());
	}
	else
	{
		damage = unit.getAirDamage() / 24.0;
	}

	if (!unit.getType().isWorker() && damage > 0.0)
	{
		return range * damage;
	}
	return 0.0;
}

double UtilTrackerClass::getVisibleAirStrength(UnitInfo& unit)
{
	if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised())
	{
		return 0.0;
	}

	double effectiveness = 1.0;
	double hp = double(unit.unit()->getHitPoints() + (unit.unit()->getShields())) / double(unit.getType().maxHitPoints() + (unit.getType().maxShields()));

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
	return unit.getPercentHealth() * unit.getMaxAirStrength() * effectiveness;
}

double UtilTrackerClass::getPriority(UnitInfo& unit)
{
	// If an enemy detector is within range of an Arbiter, give it higher priority
	if (Grids().getArbiterGrid(unit.getWalkPosition()) > 0 && unit.getType().isDetector() && unit.getPlayer()->isEnemy(Broodwar->self()))
	{
		return 10.0;
	}

	// Support units gain higher priority due to their capabilities
	if (unit.getType() == UnitTypes::Protoss_Arbiter || unit.getType() == UnitTypes::Protoss_Observer || unit.getType() == UnitTypes::Protoss_Shuttle || unit.getType() == UnitTypes::Terran_Science_Vessel || unit.getType() == UnitTypes::Terran_Dropship || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine)
	{
		return 10.0;
	}

	// Carriers don't have any strength, manually modify priority
	else if (unit.getType() == UnitTypes::Protoss_Carrier)
	{
		return 10.0;
	}

	// Workers get a fairly low priority
	else if (unit.getType().isWorker())
	{
		if (unit.unit()->exists() && unit.unit()->isRepairing())
		{
			return 10.0;
		}
		else
		{
			return 1.0;
		}
	}

	// Buildings with no attack have the lowest priority
	else if (unit.getType().isBuilding() && unit.getMaxGroundStrength() == 0.0 && unit.getMaxAirStrength() == 0.0)
	{
		return double(unit.getType().mineralPrice() + (unit.getType().gasPrice() * 2.0)) / 400.0;
	}

	// Overlords have low priority but are worthwhile to pick off
	else if (unit.getType() == UnitTypes::Zerg_Overlord)
	{
		return 2.0;
	}

	// Else return the units max strength
	else if (unit.getMaxGroundStrength() > 0 || unit.getMaxAirStrength() > 0)
	{
		return max(unit.getMaxGroundStrength(), unit.getMaxAirStrength());
	}

	else
	{
		return 0.1;
	}
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

bool UtilTrackerClass::isSafe(WalkPosition start, WalkPosition end, UnitType unitType, bool groundCheck, bool airCheck, bool mobilityCheck)
{
	// TODO : Put unit ID in anti mobility grid so it's possible to compare if a unit is standing on it?
	for (int x = end.x - (unitType.width() / 8); x <= end.x + (unitType.width() / 8); x++)
	{
		for (int y = end.y - (unitType.height() / 8); y <= end.y + (unitType.height() / 8); y++)
		{
			if (!WalkPosition(x, y).isValid())
			{
				continue;
			}

			// If WalkPosition shared with WalkPositions under unit, ignore
			if (x >= start.x && x <= (start.x + (unitType.width() / 16)) && y >= start.y && y <= (start.y + (unitType.height() / 16)))
			{
				continue;
			}
			if ((groundCheck && Grids().getEGroundThreat(x, y) != 0.0) || (airCheck && Grids().getEAirThreat(x, y) != 0.0) || (mobilityCheck && (Grids().getMobilityGrid(x, y) == 0 || Grids().getAntiMobilityGrid(x, y) > 0)))
			{
				return false;
			}
		}
	}
	return true;
}