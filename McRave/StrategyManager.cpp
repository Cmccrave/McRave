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
	for (auto &unit : unitScore) unit.second = 0;

	// Get strategy based on race
	if (Broodwar->self()->getRace() == Races::Protoss) protossStrategy();
	else if (Broodwar->self()->getRace() == Races::Terran) terranStrategy();
	else if (Broodwar->self()->getRace() == Races::Zerg) zergStrategy();
}

void StrategyTrackerClass::protossStrategy()
{
	// Check if it's early enough to run specific strategies
	if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
	{
		// Check if we're fast expanding
		if (BuildOrder().getCurrentBuild() == "TwelveNexus" || BuildOrder().isForgeExpand())
		{
			allyFastExpand = true;
		}

		// Check if we hit our Zealot cap
		if (!rush && ((BuildOrder().getCurrentBuild() == "ZZCore" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) >= 2) || (BuildOrder().getCurrentBuild() == "ZCore" && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) >= 1) || (BuildOrder().getCurrentBuild() == "NZCore") || (Players().getNumberTerran() > 0 && !BuildOrder().isOneGateCore())))
		{
			lockedType.insert(UnitTypes::Protoss_Zealot);
		}
		else
		{
			lockedType.erase(UnitTypes::Protoss_Zealot);
		}

		// Check if enemy is rushing
		if (Units().getSupply() < 60 && ((Players().getNumberProtoss() > 0 || Players().getNumberRandom() > 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Forge] == 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] >= 2 || Units().getEnemyComposition()[UnitTypes::Protoss_Gateway] == 0) && Units().getEnemyComposition()[UnitTypes::Protoss_Assimilator] == 0 && Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] == 1)
			|| ((Players().getNumberRandom() > 0 || Players().getNumberZerg() > 0) && Units().getEnemyComposition()[UnitTypes::Zerg_Zergling] >= 6 && Units().getEnemyComposition()[UnitTypes::Zerg_Drone] < 6))
		{
			rush = true;
		}
		else
		{
			rush = false;
		}

		// Check if enemy is fast expanding
		if (Units().getEnemyComposition()[UnitTypes::Terran_Command_Center] > 1 || Units().getEnemyComposition()[UnitTypes::Zerg_Hatchery] > 1 || Units().getEnemyComposition()[UnitTypes::Protoss_Nexus] > 1)
		{
			enemyFastExpand = true;
		}
		else
		{
			enemyFastExpand = false;
		}

		// Check if we should play passive and/or hold the choke
		if (allyFastExpand)
		{
			playPassive = !enemyFastExpand;
			holdChoke = true;
		}
		else
		{
			playPassive = rush;
			holdChoke = (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon) + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot)) >= 6;
		}
	}
	else
	{
		lockedType.erase(UnitTypes::Protoss_Zealot);
		rush = false;
		holdChoke = true;
		playPassive = false;
		allyFastExpand = false;
		enemyFastExpand = false;
	}

	// Check if we need an observer
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) <= 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Dark_Templar] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Citadel_of_Adun] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Templar_Archives] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Vulture] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Ghost] > 0 || Units().getEnemyComposition()[UnitTypes::Zerg_Lurker] > 0))
	{
		invis = true;
	}
	else
	{
		invis = false;
	}
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
			holdChoke = false;
		}
		else
		{
			holdChoke = true;
		}

		// If we are being 4/5 pooled
		if (Players().getNumberZerg() > 0 && Units().getEnemyComposition()[UnitTypes::Zerg_Zergling] >= 6)
		{
			rush = true;
		}
	}
	else
	{
		holdChoke = true;
		rush = false;
	}

	// Check if we need detection
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Comsat_Station) <= 0 && (Units().getEnemyComposition()[UnitTypes::Protoss_Dark_Templar] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Citadel_of_Adun] > 0 || Units().getEnemyComposition()[UnitTypes::Protoss_Templar_Archives] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Vulture] > 0 || Units().getEnemyComposition()[UnitTypes::Terran_Ghost] > 0 || Units().getEnemyComposition()[UnitTypes::Zerg_Lurker] > 0))
	{
		invis = true;
	}
	else
	{
		invis = false;
	}

	if (BuildOrder().getCurrentBuild() == "TwoFactVult" && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) >= 4)
	{
		lockedType.insert(UnitTypes::Terran_Marine);
	}
	return;
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
				Grids().updatePsiStorm(bullet);
			}
			if (bullet->getType() == BulletTypes::EMP_Missile)
			{
				Grids().updateEMP(bullet);
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

void StrategyTrackerClass::updateProtossUnitScore(UnitType unit, int cnt)
{
	double size = double(cnt) * double(unit.supplyRequired());
	switch (unit)
	{
	case UnitTypes::Enum::Terran_Marine:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Terran_Medic:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Terran_Firebat:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Terran_Vulture:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.10) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.90) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Goliath:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Siege_Mode:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.85) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.15) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Arbiter] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter)));
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
		unitScore[UnitTypes::Protoss_Corsair] += (size * 0.65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 0.35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Zerg_Hydralisk:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.50) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Zerg_Lurker:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Zerg_Ultralisk:
		unitScore[UnitTypes::Protoss_Zealot] += (size * 0.25) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot)));
		unitScore[UnitTypes::Protoss_Dragoon] += (size * 0.75) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon)));
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
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
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;

	case UnitTypes::Enum::Protoss_Zealot:
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		unitScore[UnitTypes::Protoss_Reaver] += (size * .65) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
		unitScore[UnitTypes::Protoss_High_Templar] += (size * .35) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		unitScore[UnitTypes::Protoss_High_Templar] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar)));
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		unitScore[UnitTypes::Protoss_Reaver] += (size * 1.00) / max(1.0, double(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver)));
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