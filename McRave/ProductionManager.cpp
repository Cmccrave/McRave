#include "McRave.h"

void ProductionTrackerClass::update()
{
	Display().startClock();
	updateReservedResources();
	updateProduction();
	Display().performanceTest(__FUNCTION__);
	return;
}

void ProductionTrackerClass::updateProduction()
{
	idle = false;
	for (auto &b : Buildings().getMyBuildings())
	{
		BuildingInfo &building = b.second;
		double best = 0.0;
		UnitType bestType = UnitTypes::None;
		if (building.unit()->isIdle() && !building.getType().isResourceDepot())
		{
			idleProduction.erase(building.unit());
			idleUpgrade.erase(building.unit());
			idleTech.erase(building.unit());
			for (auto &unit : building.getType().buildsWhat())
			{
				double mineral = max(0.0, min(1.0, double(Broodwar->self()->minerals() - reservedMineral - Buildings().getQueuedMineral()) / unit.mineralPrice()));
				double gas = max(0.0, min(1.0, double(Broodwar->self()->gas() - reservedGas - Buildings().getQueuedGas()) / unit.gasPrice()));
				double value = Strategy().getUnitScore()[unit] * mineral * gas;
				if (unit.isAddon() && BuildOrder().getBuildingDesired()[unit] > Broodwar->self()->visibleUnitCount(unit))
				{
					building.unit()->buildAddon(unit);
					continue;
				}
				if (unit == UnitTypes::Protoss_Dark_Templar && BuildOrder().getCurrentBuild() == "PDTExpand" && Broodwar->self()->visibleUnitCount(unit) < 2 && isCreateable(building.unit(), unit) && isSuitable(unit)) // stupid hardcode stuff TODO
				{
					best = 100;
					bestType = unit;
				}
				else if (value >= best && isCreateable(building.unit(), unit) && isSuitable(unit))
				{					
					best = Strategy().getUnitScore()[unit];
					bestType = unit;
				}
			}
			if (bestType != UnitTypes::None)
			{
				// If we can afford it, train it
				if (isAffordable(bestType))
				{
					building.unit()->train(bestType);
					idleProduction.erase(building.unit());
				}
				// Else if this is a tech unit, add it to idle production
				else if (BuildOrder().getTechList().find(bestType) != BuildOrder().getTechList().end())
				{
					if (Units().getSupply() < 380) idle = true;
					idleProduction[building.unit()] = bestType;
					reservedMineral += bestType.mineralPrice();
					reservedGas += bestType.gasPrice();

				}
			}

			// If this building researches things
			for (auto &research : building.getType().researchesWhat())
			{
				if (isCreateable(research) && isSuitable(research))
				{
					if (isAffordable(research)) building.unit()->research(research), idleTech.erase(building.unit());
					else idleTech[building.unit()] = research;
					reservedMineral += research.mineralPrice();
					reservedGas += research.gasPrice();
				}
			}

			// If this building upgrades things
			for (auto &upgrade : building.getType().upgradesWhat())
			{
				if (isCreateable(upgrade) && isSuitable(upgrade))
				{
					if (isAffordable(upgrade)) building.unit()->upgrade(upgrade), idleUpgrade.erase(building.unit());
					else idleUpgrade[building.unit()] = upgrade;
					reservedMineral += upgrade.mineralPrice();
					reservedGas += upgrade.gasPrice();
				}
			}
		}
	}
}

bool ProductionTrackerClass::isAffordable(UnitType unit)
{
	// If not a tech unit
	if (BuildOrder().getTechList().find(unit) == BuildOrder().getTechList().end())
	{
		// If we can afford it including buildings queued and tech units queued
		if (Broodwar->self()->minerals() >= unit.mineralPrice() + reservedMineral + Buildings().getQueuedMineral() && (Broodwar->self()->gas() >= unit.gasPrice() + reservedGas + Buildings().getQueuedGas() || unit.gasPrice() == 0))
		{
			return true;
		}
	}
	// If a tech unit and we can afford it including buildings queued
	else if (Broodwar->self()->minerals() >= unit.mineralPrice() + Buildings().getQueuedMineral() && (Broodwar->self()->gas() >= unit.gasPrice() + Buildings().getQueuedGas() || unit.gasPrice() == 0))
	{
		return true;
	}
	return false;
}

bool ProductionTrackerClass::isAffordable(TechType tech)
{
	return Broodwar->self()->minerals() >= tech.mineralPrice() && Broodwar->self()->gas() >= tech.gasPrice();
}

bool ProductionTrackerClass::isAffordable(UpgradeType upgrade)
{
	return Broodwar->self()->minerals() >= upgrade.mineralPrice() && Broodwar->self()->gas() >= upgrade.gasPrice();
}

bool ProductionTrackerClass::isCreateable(Unit building, UnitType unit)
{
	if (!BuildOrder().isUnitUnlocked(unit))
	{
		return false;
	}

	switch (unit)
	{
		// Gateway Units
	case UnitTypes::Enum::Protoss_Zealot:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0;
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0;
		break;

		// Robo Units
	case UnitTypes::Enum::Protoss_Shuttle:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Reaver:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Support_Bay) > 0;
		break;
	case UnitTypes::Enum::Protoss_Observer:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0;
		break;

		// Stargate Units
	case UnitTypes::Enum::Protoss_Corsair:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Scout:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Carrier:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Fleet_Beacon) > 0;
		break;
	case UnitTypes::Enum::Protoss_Arbiter:
		return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Arbiter_Tribunal) > 0;
		break;

		// Barracks Units
	case UnitTypes::Enum::Terran_Marine:
		return true;
		break;
	case UnitTypes::Enum::Terran_Firebat:
		return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Academy) > 0;
		break;
	case UnitTypes::Enum::Terran_Medic:
		return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Academy) > 0;
		break;

		// Factory Units
	case UnitTypes::Enum::Terran_Vulture:
		return true;
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		return building->getAddon() != nullptr ? true : false;
		break;
	case UnitTypes::Enum::Terran_Goliath:
		return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Armory) > 0;
		break;
	}
	return false;
}

bool ProductionTrackerClass::isCreateable(UpgradeType upgrade)
{
	for (auto &unit : upgrade.whatUses())
	{
		if (BuildOrder().isUnitUnlocked(unit) && Broodwar->self()->getUpgradeLevel(upgrade) != upgrade.maxRepeats() && !Broodwar->self()->isUpgrading(upgrade)) return true;
	}
	return false;
}

bool ProductionTrackerClass::isCreateable(TechType tech)
{
	for (auto &unit : tech.whatUses())
	{
		if (BuildOrder().isUnitUnlocked(unit) && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech)) return true;
	}
	return false;
}

bool ProductionTrackerClass::isSuitable(UnitType unit)
{
	switch (unit)
	{
		// Gateway Units
	case UnitTypes::Enum::Protoss_Zealot:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Dragoon:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Dark_Templar:
		return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar) < 2;
		break;
	case UnitTypes::Enum::Protoss_High_Templar:
		return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar) < 10 && (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) || Broodwar->self()->isResearching(TechTypes::Psionic_Storm));
		break;

		// Robo Units
	case UnitTypes::Enum::Protoss_Shuttle:
		return (!Strategy().needDetection() && 2 * Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) < Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver));
		break;
	case UnitTypes::Enum::Protoss_Reaver:
		return !Strategy().needDetection();
		break;
	case UnitTypes::Enum::Protoss_Observer:
		return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer) < 4;
		break;

		// Stargate Units
	case UnitTypes::Enum::Protoss_Corsair:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Scout:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Carrier:
		return true;
		break;
	case UnitTypes::Enum::Protoss_Arbiter:
		return (Broodwar->self()->isUpgrading(UpgradeTypes::Khaydarin_Core) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core));
		break;


		// Barracks Units
	case UnitTypes::Enum::Terran_Marine:
		return true;
		break;
	case UnitTypes::Enum::Terran_Firebat:
		return true;
		break;
	case UnitTypes::Enum::Terran_Medic:
		return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Medic) * 4 < Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine);
		break;

		// Factory Units
	case UnitTypes::Enum::Terran_Vulture:
		return Broodwar->self()->minerals() > Broodwar->self()->gas();
		break;
	case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
		return true;
		break;
	case UnitTypes::Enum::Terran_Goliath:
		return true;
		break;
	}
	return false;
}

bool ProductionTrackerClass::isSuitable(UpgradeType upgrade)
{
	// If this is a specific unit upgrade, check if it's unlocked
	if (upgrade.whatUses().size() == 1)
	{
		for (auto &unit : upgrade.whatUses())
		{
			if (!BuildOrder().isUnitUnlocked(unit)) return false;
		}
	}

	// If we're playing Protoss, check Protoss upgrades
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (upgrade != UpgradeTypes::Singularity_Charge && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0) return false;

		switch (upgrade)
		{
			// Energy upgrades
		case UpgradeTypes::Enum::Khaydarin_Amulet:
			return (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar) >= 4 && Broodwar->self()->hasResearched(TechTypes::Psionic_Storm));
		case UpgradeTypes::Enum::Khaydarin_Core:
			return true;

			// Range upgrades
		case UpgradeTypes::Enum::Singularity_Charge:
			return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon) >= 2 * (1 + BuildOrder().isNexusFirst());

			// Sight upgrades
		case UpgradeTypes::Enum::Apial_Sensors:
			return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
		case UpgradeTypes::Enum::Sensor_Array:
			return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);

			// Capacity upgrades
		case UpgradeTypes::Enum::Carrier_Capacity:
			return true;
		case UpgradeTypes::Enum::Reaver_Capacity:
			return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
		case UpgradeTypes::Enum::Scarab_Damage:
			return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);

			// Speed upgrades
		case UpgradeTypes::Enum::Gravitic_Drive:
			return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) > 0;
		case UpgradeTypes::Enum::Gravitic_Thrusters:
			return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Scout) > 0;
		case UpgradeTypes::Enum::Gravitic_Boosters:
			return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
		case UpgradeTypes::Enum::Leg_Enhancements:
			return true;

			// Ground unit upgrades
		case UpgradeTypes::Enum::Protoss_Ground_Weapons:
			return (Units().getSupply() > 120 || (Players().getNumberZerg() > 0 && !BuildOrder().isOpener()));
		case UpgradeTypes::Enum::Protoss_Ground_Armor:
			return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) > Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Armor);
		case UpgradeTypes::Enum::Protoss_Plasma_Shields:
			return (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) >= 2 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Armor) >= 2);

			// Air unit upgrades
		case UpgradeTypes::Enum::Protoss_Air_Weapons:
			return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair) > 0;
		case UpgradeTypes::Enum::Protoss_Air_Armor:
			return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Air_Weapons) > Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Air_Armor);
		}
	}

	// If we're playing Terran, check Terran upgrades
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		switch (upgrade)
		{
		case UpgradeTypes::Enum::Ion_Thrusters:
			return true;
		case UpgradeTypes::Enum::Charon_Boosters:
			return Strategy().getUnitScore()[UnitTypes::Terran_Goliath] > 1.00;
		case UpgradeTypes::Enum::U_238_Shells:
			return Broodwar->self()->hasResearched(TechTypes::Stim_Packs);
		}
	}
	return false;
}

bool ProductionTrackerClass::isSuitable(TechType tech)
{
	// If this is a specific unit tech, check if it's unlocked
	if (tech.whatUses().size() == 1)
	{
		for (auto &unit : tech.whatUses())
		{
			if (!BuildOrder().isUnitUnlocked(unit)) return false;
		}
	}

	// If we're playing Protoss, check Protoss tech
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0) return false;

		switch (tech)
		{
		case TechTypes::Enum::Psionic_Storm:
			return true;
		case TechTypes::Enum::Stasis_Field:
			return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core) > 0;
		case TechTypes::Enum::Recall:
			return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
		}
	}

	// If we're playing Terran, check Terran tech
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		switch (tech)
		{
		case TechTypes::Enum::Stim_Packs:
			return BuildOrder().isBioBuild();
		case TechTypes::Enum::Spider_Mines:
			return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) > 0 || Broodwar->self()->isUpgrading(UpgradeTypes::Ion_Thrusters);
		case TechTypes::Enum::Tank_Siege_Mode:
			return Broodwar->self()->hasResearched(TechTypes::Spider_Mines) || Broodwar->self()->isResearching(TechTypes::Spider_Mines);
		}
	}
	return false;
}

void ProductionTrackerClass::updateReservedResources()
{
	// Reserved minerals for idle buildings, tech and upgrades
	reservedMineral = 0, reservedGas = 0;
	for (auto &b : idleProduction)
	{
		reservedMineral += b.second.mineralPrice();
		reservedGas += b.second.gasPrice();
	}
	for (auto &t : idleTech)
	{
		reservedMineral += t.second.mineralPrice();
		reservedGas += t.second.gasPrice();
	}
	for (auto &u : idleUpgrade)
	{
		reservedMineral += u.second.mineralPrice();
		reservedGas += u.second.gasPrice();
	}
	return;
}