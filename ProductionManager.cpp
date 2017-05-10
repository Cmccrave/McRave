#include "ProductionManager.h"
#include "GridManager.h"
#include "UnitManager.h"
#include "StrategyManager.h"
#include "BuildingManager.h"



void ProductionTrackerClass::update()
{
	int supply = UnitTracker::Instance().getSupply();
	int queuedMineral = BuildingTracker::Instance().getQueuedMineral();
	int queuedGas = BuildingTracker::Instance().getQueuedGas();
	if (Broodwar->enemy()->getRace() == Races::Terran && !Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements))
	{
		noZealots = true;
	}

	for (auto building : Broodwar->self()->getUnits())
	{
		if (building->isIdle() && building->getType().isBuilding())
		{
			switch (building->getType())
			{
				// Research Buildings
			case UnitTypes::Enum::Protoss_Forge:
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Ground_Weapons.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Ground_Weapons.gasPrice() + queuedGas + reservedGas)
				{
					building->upgrade(UpgradeTypes::Protoss_Ground_Weapons);
				}
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Ground_Armor.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Ground_Armor.gasPrice() + queuedGas + reservedGas)
				{
					building->upgrade(UpgradeTypes::Protoss_Ground_Armor);
				}
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Plasma_Shields.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Plasma_Shields.gasPrice() + queuedGas + reservedGas)
				{
					building->upgrade(UpgradeTypes::Protoss_Plasma_Shields);
				}
				break;
			case UnitTypes::Enum::Protoss_Cybernetics_Core:
				if (!Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon) >= 1)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Singularity_Charge.mineralPrice() && Broodwar->self()->gas() >= UpgradeTypes::Singularity_Charge.gasPrice())
					{
						building->upgrade(UpgradeTypes::Singularity_Charge);
						idleUpgrade.erase(building->getID());
						return;
					}
					else
					{
						idleUpgrade.emplace(building->getID(), UpgradeTypes::Singularity_Charge);
					}
				}
				break;
			case UnitTypes::Enum::Protoss_Robotics_Support_Bay:		
				if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Reaver) > 2)
				{
					if (Broodwar->enemy()->getRace() != Races::Zerg && Broodwar->self()->minerals() >= UpgradeTypes::Scarab_Damage.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Scarab_Damage.gasPrice() + queuedGas + reservedGas)
					{
						building->upgrade(UpgradeTypes::Scarab_Damage);
					}
				}				
				break;
			case UnitTypes::Enum::Protoss_Fleet_Beacon:
				if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Carrier) > 2)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Carrier_Capacity.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Carrier_Capacity.gasPrice() + queuedGas + reservedGas)
					{
						building->upgrade(UpgradeTypes::Carrier_Capacity);
					}
				}
				break;
			case UnitTypes::Enum::Protoss_Citadel_of_Adun:
				if (Broodwar->self()->minerals() >= UpgradeTypes::Leg_Enhancements.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Leg_Enhancements.gasPrice() + queuedGas + reservedGas)
				{
					building->upgrade(UpgradeTypes::Leg_Enhancements);
				}
				break;

				// Production Buildings
			case UnitTypes::Enum::Protoss_Gateway:
				updateGateway(building);
				break;
			case UnitTypes::Enum::Protoss_Stargate:
				updateStargate(building);
				break;
			case UnitTypes::Enum::Protoss_Robotics_Facility:
				updateRobo(building);
				break;

				// Tech Research
			case UnitTypes::Enum::Protoss_Templar_Archives:
				if (!Broodwar->self()->hasResearched(TechTypes::Psionic_Storm))
				{
					if (Broodwar->self()->minerals() >= TechTypes::Psionic_Storm.mineralPrice() && Broodwar->self()->gas() >= TechTypes::Psionic_Storm.gasPrice())
					{
						building->research(TechTypes::Psionic_Storm);
						idleTech.erase(building->getID());
						return;
					}
					else
					{
						idleTech.emplace(building->getID(), TechTypes::Psionic_Storm);
					}
				}
				else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Amulet) == 0)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Khaydarin_Amulet.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= UpgradeTypes::Khaydarin_Amulet.gasPrice())
					{
						building->upgrade(UpgradeTypes::Khaydarin_Amulet);
						idleUpgrade.erase(building->getID());
						return;
					}
					else
					{
						idleUpgrade.emplace(building->getID(), UpgradeTypes::Khaydarin_Amulet);
					}
				}
				break;
			case UnitTypes::Enum::Protoss_Arbiter_Tribunal:
				if (!Broodwar->self()->hasResearched(TechTypes::Stasis_Field))
				{
					if (Broodwar->self()->minerals() >= TechTypes::Stasis_Field.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= TechTypes::Stasis_Field.gasPrice() + queuedGas)
					{
						building->research(TechTypes::Stasis_Field);
						idleTech.erase(building->getID());
						return;
					}
					else
					{
						idleTech.emplace(building->getID(), TechTypes::Stasis_Field);
					}
				}
				break;
			}
		}
	}
}

void ProductionTrackerClass::updateReservedResources()
{
	// Reserved minerals for idle buildings, tech and upgrades
	reservedMineral = 0, reservedGas = 0;
	for (auto b : idleBuildings)
	{
		reservedMineral += b.second.mineralPrice();
		reservedGas += b.second.gasPrice();
	}
	for (auto t : idleTech)
	{
		reservedMineral += t.second.mineralPrice();
		reservedGas += t.second.gasPrice();
	}
	for (auto u : idleUpgrade)
	{
		reservedMineral += u.second.mineralPrice();
		reservedGas += u.second.gasPrice();
	}
}

void ProductionTrackerClass::updateGateway(Unit building)
{
	int supply = UnitTracker::Instance().getSupply();
	int queuedMineral = BuildingTracker::Instance().getQueuedMineral();
	int queuedGas = BuildingTracker::Instance().getQueuedGas();
	// If we need a High Templar
	if (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Templar_Archives) >= 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar) < 5)
	{
		// If we can afford a High Temlar, train
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_High_Templar.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_High_Templar.gasPrice() + queuedGas + reservedGas && supply + UnitTypes::Protoss_High_Templar.supplyRequired() <= Broodwar->self()->supplyTotal())
		{
			building->train(UnitTypes::Protoss_High_Templar);
			idleGates.erase(building->getID());
			return;
		}
		else
		{
			idleGates.emplace(building->getID(), UnitTypes::Protoss_High_Templar);
		}
	}
	// If we need a Dragoon
	if (StrategyTracker::Instance().getUnitScore()[UnitTypes::Protoss_Dragoon] >= StrategyTracker::Instance().getUnitScore()[UnitTypes::Protoss_Zealot] && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0)
	{
		// If we can afford a Dragoon, train
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Dragoon.mineralPrice() + queuedMineral + reservedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Dragoon.gasPrice() + queuedGas + reservedGas && supply + UnitTypes::Protoss_Dragoon.supplyRequired() <= Broodwar->self()->supplyTotal())
		{
			building->train(UnitTypes::Protoss_Dragoon);
			idleGates.erase(building->getID());
			return;
		}
		else
		{
			idleGates.emplace(building->getID(), UnitTypes::Protoss_Dragoon);
		}
	}
	// If we need a Zealot
	if (!noZealots && (StrategyTracker::Instance().getUnitScore()[UnitTypes::Protoss_Dragoon] < StrategyTracker::Instance().getUnitScore()[UnitTypes::Protoss_Zealot] || ((Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Cybernetics_Core) < 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Assimilator) < 1) || Broodwar->self()->gas() < UnitTypes::Protoss_Dragoon.gasPrice() + queuedGas + reservedGas)))
	{
		// If we can afford a Zealot, train
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Zealot.mineralPrice() + queuedMineral + reservedMineral && supply + UnitTypes::Protoss_Zealot.supplyRequired() <= Broodwar->self()->supplyTotal())
		{
			building->train(UnitTypes::Protoss_Zealot);
			idleGates.erase(building->getID());
			return;
		}
		else
		{
			idleGates.emplace(building->getID(), UnitTypes::Protoss_Zealot);
		}
	}
}

void ProductionTrackerClass::updateRobo(Unit building)
{
	int supply = UnitTracker::Instance().getSupply();
	int queuedMineral = BuildingTracker::Instance().getQueuedMineral();
	int queuedGas = BuildingTracker::Instance().getQueuedGas();
	// If we need an Observer
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer) < (floor(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) / 3) + 1))
	{
		// If we can afford an Observer, train, otherwise, add to priority
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Observer.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Observer.gasPrice() + queuedGas)
		{
			building->train(UnitTypes::Protoss_Observer);
			idleBuildings.erase(building->getID());
			return;
		}
		else
		{
			idleBuildings.emplace(building->getID(), UnitTypes::Protoss_Observer);
		}
	}
	// If we need a Reaver			
	else if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Support_Bay) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) < 10)
	{
		// If we can afford a Reaver, train, otherwise, add to priority
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Reaver.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Reaver.gasPrice() + queuedGas)
		{
			building->train(UnitTypes::Protoss_Reaver);
			idleBuildings.erase(building->getID());
			return;
		}
		else
		{
			idleBuildings.emplace(building->getID(), UnitTypes::Protoss_Reaver);
		}
	}
	// If we need a Shuttle
	//else if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) || Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) < 3)
	//{
	//	// If we can afford a Shuttle, train, otherwise, add to priority
	//	if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Shuttle.mineralPrice() + queuedMineral)
	//	{
	//		building->train(UnitTypes::Protoss_Shuttle);
	//		idleBuildings.erase(building->getID());
	//	}
	//	else
	//	{
	//		idleBuildings.emplace(building->getID(), UnitTypes::Protoss_Shuttle);
	//	}
	//}
}

void ProductionTrackerClass::updateStargate(Unit building)
{
	int supply = UnitTracker::Instance().getSupply();
	int queuedMineral = BuildingTracker::Instance().getQueuedMineral();
	int queuedGas = BuildingTracker::Instance().getQueuedGas();
	/*if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Fleet_Beacon) >= 1 && Broodwar->self()->minerals() >= UnitTypes::Protoss_Carrier.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Carrier.gasPrice() + queuedGas)
	{
	building->train(UnitTypes::Protoss_Carrier);
	}*/

	// Set as visible so it saves resources for Arbiters if we're teching to them
	if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter_Tribunal) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter) < 3)
	{
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Arbiter.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Arbiter.gasPrice() + queuedGas)
		{
			building->train(UnitTypes::Protoss_Arbiter);
			idleBuildings.erase(building->getID());
			return;
		}
		else
		{
			idleBuildings.emplace(building->getID(), UnitTypes::Protoss_Arbiter);
		}
	}
	// Only build corsairs against Zerg
	if (Broodwar->enemy()->getRace() == Races::Zerg && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair) < 10)
	{
		if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Corsair.mineralPrice() + queuedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Corsair.gasPrice() + queuedGas)
		{
			building->train(UnitTypes::Protoss_Corsair);
		}
	}
}

void ProductionTrackerClass::updateLuxuryTech(Unit building)
{

}

