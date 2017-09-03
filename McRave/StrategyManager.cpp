#include "McRave.h"

void StrategyTrackerClass::update()
{
	Display().startClock();
	updateSituationalBehaviour();
	updateBullets();
	updateScoring();
	Display().performanceTest(__FUNCTION__);
	return;
}

void StrategyTrackerClass::updateSituationalBehaviour()
{
	// Reset unit score
	for (auto &unit : unitScore)
	{
		unit.second = 0;
	}

	// Specific behaviours
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		protossStrategy();
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		terranStrategy();
	}
	else if (Broodwar->self()->getRace() == Races::Zerg)
	{
		zergStrategy();
	}
}

void StrategyTrackerClass::protossStrategy()
{
	// If it's early on and we're being rushed
	if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
	{
		// Ramp holding logic
		if ((allyFastExpand && Players().getNumberZerg() > 0) || (Players().getNumberProtoss() > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon) >= 4))
		{
			holdRamp = true;
		}
		else
		{
			holdRamp = false;
		}

		if (Players().getNumberProtoss() > 0 && Units().getEnemyComposition()[UnitTypes::Protoss_Forge] == 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] >= 2 || Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] == 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Assimilator] == 0 && Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] == 1)
		{
			rush = true;
		}
		else
		{
			rush = false;
		}

		
		if (Players().getNumberTerran() > 0)
		{			
			// If we are being BBS'd, unlock Zealots
			if ((Units().getEnemyComposition()[UnitTypes::Terran_Barracks] == 0 || Units().getEnemyComposition()[UnitTypes::Terran_Barracks] == 2) && Units().getEnemyComposition()[UnitTypes::Terran_Command_Center] == 1 && Units().getEnemyComposition()[UnitTypes::Terran_Refinery] == 0)
			{
				zealotsLocked = false;
				lockedType.insert(UnitTypes::Protoss_Zealot);
			}
			else
			{
				zealotsLocked = true;
				lockedType.erase(UnitTypes::Protoss_Zealot);
			}	

			// Always play passive with Terran due to 12 Nexus until Goon range
			playPassive = true;
			holdRamp = true;
		}

		// Check if enemy is playing defensive so we can expand off it
		if ((Players().getNumberZerg() > 0 && BuildOrder().getOpener() == 1) || Units().getEnemyComposition()[UnitTypes::Terran_Bunker] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Photon_Cannon] >= 2)
		{
			allyFastExpand = true;
		}
		else
		{
			allyFastExpand = false;
		}

		// Check if enemy is fast expanding so we can try to punish it and end the game early
		if (Units().getEnemyComposition()[UnitTypes::Terran_Command_Center] > 1 || Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] > 2 || Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] > 1)
		{
			enemyFastExpand = true;
		}
		else
		{
			enemyFastExpand = false;
		}

		// Check to see if a bust is coming
		if (allyFastExpand && Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk_Den] > 0 || Units().getEnemyComposition()[UnitTypes::Zerg_Hydralisk] >= 4)
		{
			bust = true;
		}
	}
	else
	{
		rush = false;
		allyFastExpand = false;
		enemyFastExpand = false;
		bust = false;
		holdRamp = true;
		zealotsLocked = false;
		playPassive = false;
	}

	// Check if we need detection
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0)
	{
		invis = false;
	}
	else if (Units().getEnemyComposition()[UnitTypes::Protoss_Dark_Templar] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Citadel_of_Adun] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Templar_Archives] > 0 ||/* Units().getEnemyComposition()[UnitTypes::Terran_Wraith] > 0 ||*/ Units().getEnemyComposition()[UnitTypes::Terran_Ghost] > 0 || Units().getEnemyComposition()[UnitTypes::Zerg_Lurker] > 0)
	{
		invis = true;
	}

	// Test locked units
	lockedType.insert(UnitTypes::Protoss_Shuttle);
	return;
}

void StrategyTrackerClass::terranStrategy()
{
	// If it's early on and we're being rushed
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) < 2)
	{
		// Ramp holding logic
		if ((Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)) < 2)
		{
			holdRamp = false;
		}
		else
		{
			holdRamp = true;
		}

		// If we are being BBS'd, unlock Marines
		if (Players().getNumberTerran() > 0)
		{
			if ((Units().getEnemyComposition()[UnitTypes::Terran_Barracks] == 0 || Units().getEnemyComposition()[UnitTypes::Terran_Barracks] == 2) && Units().getEnemyComposition()[UnitTypes::Terran_Command_Center] == 1 && Units().getEnemyComposition()[UnitTypes::Terran_Refinery] == 0)
			{
				marinesLocked = false;
			}
			else
			{
				marinesLocked = true;
			}
		}

		// If we are being 4/5 pooled
		if (Players().getNumberZerg() > 0 && Units().getEnemyComposition()[UnitTypes::Zerg_Zergling] >= 6)
		{
			rush = true;
		}
	}
	else
	{
		marinesLocked = false;
		holdRamp = false;
		rush = false;
	}
}

void StrategyTrackerClass::zergStrategy()
{

}

void StrategyTrackerClass::updateBullets()
{
	// TESTING -- Calculate how a unit is performing
	for (auto& bullet : Broodwar->getBullets())
	{
		if (bullet->exists() && bullet->getSource() && bullet->getSource()->exists() && bullet->getTarget() && bullet->getTarget()->exists())
		{
			if (bullet->getType() == BulletTypes::Psionic_Storm)
			{
				Broodwar << "Psi Storm Active at: " << bullet->getPosition() << endl;
				Grids().updatePsiStorm(bullet);
			}
			if (bullet->getType() == BulletTypes::EMP_Missile)
			{
				Broodwar << "EMP Sent to: " << bullet->getTargetPosition() << endl;
			}		
			if (bullet->getSource()->getPlayer() == Broodwar->self() && myBullets.find(bullet) == myBullets.end())
			{
				myBullets.emplace(bullet);
				double typeMod = 1.0;

				if (!bullet->getTarget()->getType().isFlyer())
				{
					if (bullet->getSource()->getType().groundWeapon().damageType() == DamageTypes::Explosive)
					{
						if (bullet->getTarget()->getType().size() == UnitSizeTypes::Small)
						{
							typeMod = 0.5;
						}
						if (bullet->getTarget()->getType().size() == UnitSizeTypes::Medium)
						{
							typeMod = 0.75;
						}
					}
					if (bullet->getSource()->getType().groundWeapon().damageType() == DamageTypes::Concussive)
					{
						if (bullet->getTarget()->getType().size() == UnitSizeTypes::Large)
						{
							typeMod = 0.25;
						}
						if (bullet->getTarget()->getType().size() == UnitSizeTypes::Medium)
						{
							typeMod = 0.5;
						}
					}

					//unitPerformance[bullet->getSource()->getType()] += double(bullet->getSource()->getType().groundWeapon().damageAmount()) * typeMod;
				}
				else
				{
					//unitPerformance[bullet->getSource()->getType()] += double(bullet->getSource()->getType().airWeapon().damageAmount());
				}
			}			
		}
	}
}

void StrategyTrackerClass::updateScoring()
{
	// Unit score based off enemy composition
	int offset = 0;
	for (auto &t : Units().getEnemyComposition())
	{
		// For each type, add a score to production based on the unit count divided by our current unit count
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			updateProtossUnitScore(t.first, t.second);
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			updateTerranUnitScore(t.first, t.second);
		}
		t.second = 0;
	}

	// Update Unit score based on performance
	for (auto &t : unitScore)
	{
		t.second += unitPerformance[t.first];
	}
}

void StrategyTrackerClass::updateProtossUnitScore(UnitType unit, int count)
{
	switch (unit)
	{
	case UnitTypes::Enum::Terran_Marine:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Terran_Medic:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Terran_Firebat:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Terran_Vulture:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Goliath:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Siege_Mode:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Wraith:
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		break;
	case UnitTypes::Enum::Terran_Science_Vessel:
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		break;
	case UnitTypes::Enum::Terran_Battlecruiser:
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		break;
	case UnitTypes::Enum::Terran_Valkyrie:
		break;

	case UnitTypes::Enum::Zerg_Zergling:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Corsair] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		break;
	case UnitTypes::Enum::Zerg_Hydralisk:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Zerg_Lurker:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Zerg_Ultralisk:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Zerg_Mutalisk:
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Corsair] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		break;
	case UnitTypes::Enum::Zerg_Guardian:
		unitScore[UnitTypes::Protoss_Dragoon] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Corsair] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		break;
	case UnitTypes::Enum::Zerg_Devourer:
		break;
	case UnitTypes::Enum::Zerg_Defiler:
		unitScore[UnitTypes::Protoss_Zealot] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;

	case UnitTypes::Enum::Protoss_Zealot:
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		unitScore[UnitTypes::Protoss_High_Templar] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Protoss_Reaver:
		unitScore[UnitTypes::Protoss_Reaver] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Protoss_Archon:
		unitScore[UnitTypes::Protoss_High_Templar] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Archon:
		unitScore[UnitTypes::Protoss_High_Templar] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Scout:
		break;
	case UnitTypes::Enum::Protoss_Carrier:
		unitScore[UnitTypes::Protoss_Scout] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Scout)));
		break;
	case UnitTypes::Enum::Protoss_Arbiter:
		unitScore[UnitTypes::Protoss_High_Templar] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Corsair:
		unitScore[UnitTypes::Protoss_High_Templar] += (count * unit.supplyRequired() * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	}
}

void StrategyTrackerClass::updateTerranUnitScore(UnitType unit, int count)
{
	switch (unit)
	{
	case UnitTypes::Enum::Terran_Marine:		
		break;
	case UnitTypes::Enum::Terran_Medic:
		break;
	case UnitTypes::Enum::Terran_Firebat:		
		break;
	case UnitTypes::Enum::Terran_Vulture:
		break;
	case UnitTypes::Enum::Terran_Goliath:
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Siege_Mode:
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		break;
	case UnitTypes::Enum::Terran_Wraith:
		break;
	case UnitTypes::Enum::Terran_Science_Vessel:
		break;
	case UnitTypes::Enum::Terran_Battlecruiser:
		break;
	case UnitTypes::Enum::Terran_Valkyrie:
		break;

	case UnitTypes::Enum::Zerg_Zergling:
		break;
	case UnitTypes::Enum::Zerg_Hydralisk:
		break;
	case UnitTypes::Enum::Zerg_Lurker:
		break;
	case UnitTypes::Enum::Zerg_Ultralisk:
		break;
	case UnitTypes::Enum::Zerg_Mutalisk:
		break;
	case UnitTypes::Enum::Zerg_Guardian:
		break;
	case UnitTypes::Enum::Zerg_Devourer:
		break;
	case UnitTypes::Enum::Zerg_Defiler:
		break;

	case UnitTypes::Enum::Protoss_Zealot:
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		break;
	case UnitTypes::Enum::Protoss_Reaver:
		break;
	case UnitTypes::Enum::Protoss_Archon:
		break;
	case UnitTypes::Enum::Protoss_Dark_Archon:
		break;
	case UnitTypes::Enum::Protoss_Scout:
		break;
	case UnitTypes::Enum::Protoss_Carrier:
		break;
	case UnitTypes::Enum::Protoss_Arbiter:
		break;
	case UnitTypes::Enum::Protoss_Corsair:
		break;
	}
}

void StrategyTrackerClass::updateZergUnitScore(UnitType unit, int count)
{

}