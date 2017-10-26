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
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		updateProtoss();
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		updateTerran();
	}
	else
	{
		updateZerg();
	}
}

bool ProductionTrackerClass::canAfford(UnitType unit)
{
	// If not a tech unit
	if (BuildOrder().getTechList().find(unit) == BuildOrder().getTechList().end())
	{
		// If we can afford it including buildings queued and tech units queued
		if (Broodwar->self()->minerals() >= (unit.mineralPrice() + reservedMineral + Buildings().getQueuedMineral()) && Broodwar->self()->gas() >= unit.gasPrice() + reservedGas + Buildings().getQueuedGas())
		{
			return true;
		}
	}
	// If a tech unit and we can afford it including buildings queued
	else if (Broodwar->self()->minerals() >= unit.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= unit.gasPrice() + Buildings().getQueuedGas())
	{
		return true;
	}
	return false;
}

bool ProductionTrackerClass::canAfford(TechType tech)
{
	// If we planned on making this tech unit
	for (auto &type : tech.whatUses())
	{
		if (BuildOrder().getTechList().find(type) != BuildOrder().getTechList().end())
		{
			// If we can afford it including buildings queued
			if (Broodwar->self()->minerals() >= tech.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= tech.gasPrice() + Buildings().getQueuedGas())
			{
				return true;
			}
		}
	}
	return false;
}

bool ProductionTrackerClass::canAfford(UpgradeType upgrade)
{
	// If it's an important tech
	for (auto &type : upgrade.whatUses())
	{
		if (BuildOrder().getTechList().find(type) != BuildOrder().getTechList().end())
		{
			// If we can afford it including buildings queued
			if (Broodwar->self()->minerals() >= upgrade.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= upgrade.gasPrice() + Buildings().getQueuedGas())
			{
				return true;
			}
		}
	}
	// If we can afford it including buildings queued and tech units queued
	if (Broodwar->self()->minerals() >= (upgrade.mineralPrice() + reservedMineral + Buildings().getQueuedMineral()) && Broodwar->self()->gas() >= (upgrade.gasPrice() + reservedGas + Buildings().getQueuedGas()))
	{
		return true;
	}

	return false;
}

bool ProductionTrackerClass::canMake(Unit building, UnitType unit)
{
	if (Strategy().isUnitLocked(unit))
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
		return (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0 && (Broodwar->self()->isResearching(TechTypes::Psionic_Storm) || Broodwar->self()->hasResearched(TechTypes::Psionic_Storm)));
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
		return building->getAddon();
		break;
	case UnitTypes::Enum::Terran_Goliath:
		return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Armory) > 0;
		break;
	}
	return false;
}

bool ProductionTrackerClass::canMake(UpgradeType upgrade)
{
	for (auto &unit : upgrade.whatUses())
	{
		if (!Strategy().isUnitLocked(unit) && Broodwar->self()->visibleUnitCount(unit) > 0 && Broodwar->self()->getUpgradeLevel(upgrade) <= 0 && !Broodwar->self()->isUpgrading(upgrade)) return true;
	}
	return false;
}

bool ProductionTrackerClass::canMake(TechType tech)
{
	for (auto &unit : tech.whatUses())
	{
		if (!Strategy().isUnitLocked(unit) && Broodwar->self()->visibleUnitCount(unit) > 0 && Broodwar->self()->hasResearched(tech) <= 0 && !Broodwar->self()->isResearching(tech)) return true;
	}
	return false;
}

void ProductionTrackerClass::updateReservedResources()
{
	// Reserved minerals for idle buildings, tech and upgrades
	reservedMineral = 0, reservedGas = 0;
	for (auto &b : idleHighProduction)
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

void ProductionTrackerClass::updateProtoss()
{
	// Gateway saturation - max of 12 so the bot can exceed 4 bases
	int techSize = max(0, int(BuildOrder().getTechList().size() - 1));
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= min(12, 3 + (2 * techSize)))
	{
		productionSat = true;
	}

	// Production
	for (auto &thisBuilding : Buildings().getMyBuildings())
	{
		BuildingInfo &building = thisBuilding.second;
		if (building.unit() && building.unit()->isIdle())
		{
			// Forge
			if (building.getType() == UnitTypes::Protoss_Forge && Units().getSupply() > 100)
			{
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Ground_Weapons.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Ground_Weapons.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.unit()->upgrade(UpgradeTypes::Protoss_Ground_Weapons);
				}
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Ground_Armor.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Ground_Armor.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.unit()->upgrade(UpgradeTypes::Protoss_Ground_Armor);
				}
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Plasma_Shields.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Plasma_Shields.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.unit()->upgrade(UpgradeTypes::Protoss_Plasma_Shields);
				}
			}

			// Cybernetics Core
			else if (building.getType() == UnitTypes::Protoss_Cybernetics_Core)
			{
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0 && idleLowProduction.size() == 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon) >= 3)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Singularity_Charge.mineralPrice() && Broodwar->self()->gas() >= UpgradeTypes::Singularity_Charge.gasPrice())
					{
						building.unit()->upgrade(UpgradeTypes::Singularity_Charge);
						idleUpgrade.erase(building.unit());
					}
					else
					{
						idleUpgrade.emplace(building.unit(), UpgradeTypes::Singularity_Charge);
					}
				}
			}

			// Robotics Support Bay
			else if (building.getType() == UnitTypes::Protoss_Robotics_Support_Bay)
			{
				if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Reaver) > 2)
				{
					if (Broodwar->enemy()->getRace() != Races::Zerg && Broodwar->self()->minerals() >= UpgradeTypes::Scarab_Damage.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Scarab_Damage.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Scarab_Damage);
					}
				}
				if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) >= 1)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Gravitic_Drive.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Gravitic_Drive.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Gravitic_Drive);
					}
				}
			}

			// Fleet Beacon
			else if (building.getType() == UnitTypes::Protoss_Fleet_Beacon)
			{
				if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Carrier) > 2)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Carrier_Capacity.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Carrier_Capacity.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Carrier_Capacity);
					}
				}
				if (Strategy().getUnitScore()[UnitTypes::Protoss_Scout] > 0.0)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Gravitic_Thrusters.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Gravitic_Thrusters.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Gravitic_Thrusters);
					}
				}
			}

			// Citadel Of Adun
			else if (building.getType() == UnitTypes::Protoss_Citadel_of_Adun)
			{
				if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) > 0)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Leg_Enhancements.mineralPrice() && Broodwar->self()->gas() >= UpgradeTypes::Leg_Enhancements.gasPrice())
					{
						building.unit()->upgrade(UpgradeTypes::Leg_Enhancements);
						idleUpgrade.erase(building.unit());
					}
					else
					{
						idleUpgrade.emplace(building.unit(), UpgradeTypes::Leg_Enhancements);
					}
				}
			}

			// Gateway
			else if (building.getType() == UnitTypes::Protoss_Gateway)
			{
				if (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Templar_Archives) >= 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar) < 4)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_High_Templar.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_High_Templar.gasPrice() + Buildings().getQueuedGas() && Units().getSupply() + UnitTypes::Protoss_High_Templar.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_High_Templar);
						idleHighProduction.erase(building.unit());
						continue;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_High_Templar);
					}
				}
				if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Templar_Archives) >= 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dark_Templar) < 2)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Dark_Templar.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Dark_Templar.gasPrice() + Buildings().getQueuedGas() && Units().getSupply() + UnitTypes::Protoss_Dark_Templar.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_Dark_Templar);
						idleHighProduction.erase(building.unit());
						continue;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Dark_Templar);
					}
				}
				if ((Strategy().isUnitLocked(UnitTypes::Protoss_Zealot) || Strategy().getUnitScore()[UnitTypes::Protoss_Dragoon] >= Strategy().getUnitScore()[UnitTypes::Protoss_Zealot]) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Dragoon.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Dragoon.gasPrice() + Buildings().getQueuedGas() + reservedGas && Units().getSupply() + UnitTypes::Protoss_Dragoon.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_Dragoon);
						idleLowProduction.erase(building.unit());
						continue;
					}
					else
					{
						idleLowProduction.emplace(building.unit(), UnitTypes::Protoss_Dragoon);
					}
				}
				if (!Strategy().isUnitLocked(UnitTypes::Protoss_Zealot) && (Strategy().getUnitScore()[UnitTypes::Protoss_Dragoon] < Strategy().getUnitScore()[UnitTypes::Protoss_Zealot] || ((Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) < 1) || Broodwar->self()->gas() < UnitTypes::Protoss_Dragoon.gasPrice() + Buildings().getQueuedGas() + reservedGas)))
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Zealot.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Units().getSupply() + UnitTypes::Protoss_Zealot.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_Zealot);
						idleLowProduction.erase(building.unit());
						continue;
					}
					else
					{
						idleLowProduction.emplace(building.unit(), UnitTypes::Protoss_Zealot);
					}
				}
				return;
			}

			// Stargate
			else if (building.getType() == UnitTypes::Protoss_Stargate)
			{
				// Set as visible so it saves resources for Arbiters if we're teching to them
				if ((Broodwar->self()->isUpgrading(UpgradeTypes::Khaydarin_Core) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core)) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter_Tribunal) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter) < 3)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Arbiter.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Arbiter.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Arbiter);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Arbiter);
					}
				}
				// Only build corsairs against Zerg
				if (Broodwar->enemy()->getRace() == Races::Zerg && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair) < 10)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Corsair.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Corsair.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Corsair);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Arbiter);
					}
				}

				// Only build scouts against Carriers
				if (Strategy().getUnitScore()[UnitTypes::Protoss_Scout] > 0.0)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Scout.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Scout.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Scout);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Scout);
					}
				}
			}

			// Robotics Facility
			else if (building.getType() == UnitTypes::Protoss_Robotics_Facility)
			{
				// If we need an Observer
				if (Strategy().needDetection() || Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer) < (floor(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) / 3) + 1))
				{
					// If we can afford an Observer, train, otherwise, add to priority
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Observer.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Observer.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Observer);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Observer);
					}
				}

				// If we need a Shuttle
				else if ((Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) / 2 > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle)) || (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) <= 0))
				{
					// If we can afford a Shuttle, train, otherwise, add to priority
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Shuttle.mineralPrice() + Buildings().getQueuedMineral())
					{
						building.unit()->train(UnitTypes::Protoss_Shuttle);
						idleHighProduction.erase(building.unit());
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Shuttle);
					}
				}

				// If we need a Reaver			
				else if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Support_Bay) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) < 10)
				{
					// If we can afford a Reaver, train, otherwise, add to priority
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Reaver.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Reaver.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Reaver);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Reaver);
					}
				}
			}

			// Templar Archives
			else if (building.getType() == UnitTypes::Protoss_Templar_Archives)
			{
				if (Players().getNumberTerran() == 0 || (Players().getNumberTerran() > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Arbiter) > 0))
				{
					if (!Broodwar->self()->hasResearched(TechTypes::Psionic_Storm))
					{
						if (Broodwar->self()->minerals() >= TechTypes::Psionic_Storm.mineralPrice() && Broodwar->self()->gas() >= TechTypes::Psionic_Storm.gasPrice())
						{
							building.unit()->research(TechTypes::Psionic_Storm);
							idleTech.erase(building.unit());
						}
						else
						{
							idleTech.emplace(building.unit(), TechTypes::Psionic_Storm);
						}
					}
					else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Amulet) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_High_Templar) > 2)
					{
						if (Broodwar->self()->minerals() >= UpgradeTypes::Khaydarin_Amulet.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UpgradeTypes::Khaydarin_Amulet.gasPrice())
						{
							building.unit()->upgrade(UpgradeTypes::Khaydarin_Amulet);
							idleUpgrade.erase(building.unit());
						}
						else
						{
							idleUpgrade.emplace(building.unit(), UpgradeTypes::Khaydarin_Amulet);
						}
					}
				}
			}

			// Arbiter Tribunal
			else if (building.getType() == UnitTypes::Protoss_Arbiter_Tribunal)
			{
				if (!Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core))
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Khaydarin_Core.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UpgradeTypes::Khaydarin_Core.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->upgrade(UpgradeTypes::Khaydarin_Core);
						idleUpgrade.erase(building.unit());
					}
					else
					{
						idleUpgrade.emplace(building.unit(), UpgradeTypes::Khaydarin_Core);
					}
				}
				else if (!Broodwar->self()->hasResearched(TechTypes::Stasis_Field))
				{
					if (Broodwar->self()->minerals() >= TechTypes::Stasis_Field.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= TechTypes::Stasis_Field.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->research(TechTypes::Stasis_Field);
						idleTech.erase(building.unit());
					}
					else
					{
						idleTech.emplace(building.unit(), TechTypes::Stasis_Field);
					}
				}
				else if (!Broodwar->self()->hasResearched(TechTypes::Recall))
				{
					if (Broodwar->self()->minerals() >= TechTypes::Recall.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= TechTypes::Recall.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->research(TechTypes::Recall);
						idleTech.erase(building.unit());
					}
					else
					{
						idleTech.emplace(building.unit(), TechTypes::Recall);
					}
				}
			}
		}
	}
	return;
}

void ProductionTrackerClass::updateTerran()
{
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= min(6, (3 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))))
	{
		productionSat = true;
	}
	// Gateway saturation - max of 12 so the bot can exceed 4 bases
	int techSize = max(0, int(BuildOrder().getTechList().size() - 1));
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= min(12, 3 + (2 * techSize)))
	{
		productionSat = true;
	}

	// TEMP Production testing
	// Goal here is to check of all the units that are possible for a building, which is the best to build that we can afford based on resources, score and priority
	// Could become a much simpler approach to production than what is currently implemented
	for (auto &b : Buildings().getMyBuildings())
	{
		BuildingInfo &building = b.second;
		double highestPriority = 0.0;
		UnitType highestType = UnitTypes::None;
		if (building.unit()->isIdle() && !building.getType().isResourceDepot())
		{
			for (auto &unit : building.getType().buildsWhat())
			{
				if (unit.isAddon() && BuildOrder().getBuildingDesired()[unit] > Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Machine_Shop))
				{
					building.unit()->buildAddon(unit);
					continue;
				}
				if (Strategy().getUnitScore()[unit] >= highestPriority && !Strategy().isUnitLocked(unit) && canMake(building.unit(), unit))
				{
					highestPriority = Strategy().getUnitScore()[unit];
					highestType = unit;
				}
			}

			if (highestType != UnitTypes::None)
			{
				if (canAfford(highestType))
				{
					building.unit()->train(highestType);
					idleLowProduction.erase(building.unit());
				}
				else if (!Strategy().isUnitLocked(highestType))
				{
					idleLowProduction[building.unit()] = highestType;
				}
			}

			// If this building researches things
			for (auto &research : building.getType().researchesWhat())
			{
				if (canMake(research)) building.unit()->research(research);
			}

			// If this building upgrades things
			for (auto &upgrade : building.getType().upgradesWhat())
			{
				if (canMake(upgrade)) building.unit()->upgrade(upgrade);
			}

		}
	}
}

void ProductionTrackerClass::updateZerg()
{
}