#include "McRave.h"

void UnitTrackerClass::onUnitDiscover(Unit unit)
{
	if (unit->getPlayer()->isEnemy(Broodwar->self())) Units().storeEnemy(unit);
}

void UnitTrackerClass::onUnitCreate(Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		if (unit->getType().supplyRequired() > 0) supply += unit->getType().supplyRequired(); // Store supply if it costs supply		

		// Store Buildings on creation rather than completion
		if (unit->getType().isBuilding()) Buildings().storeBuilding(unit);
		if (unit->getType().isResourceDepot()) Bases().storeBase(unit);
		else if (unit->getType() == UnitTypes::Protoss_Pylon) Pylons().storePylon(unit);
		else if (unit->getType() == UnitTypes::Protoss_Photon_Cannon) storeAlly(unit);
	}
}

void UnitTrackerClass::onUnitDestroy(Unit unit)
{
	if (allyUnits.find(unit) != allyUnits.end())
	{
		if (allyUnits[unit].getTransport())
		{
			Transport().removeUnit(unit);
		}
		allySizes[unit->getType().size()] -= 1;
		allyUnits.erase(unit);
	}
	else if (allyDefenses.find(unit) != allyDefenses.end())
	{
		allyDefenses.erase(unit);
	}
	else if (enemyUnits.find(unit) != enemyUnits.end())
	{
		enemySizes[unit->getType().size()] -= 1;
		enemyUnits.erase(unit);
	}

	else if (unit->getPlayer() == Broodwar->self())
	{
		supply -= unit->getType().supplyRequired();

		if (unit->getType().isResourceDepot())
		{
			Bases().removeBase(unit);
			Buildings().removeBuilding(unit);
		}
		else if (unit->getType().isBuilding())
		{
			Buildings().removeBuilding(unit);
		}
		else if (unit->getType().isWorker())
		{
			Workers().removeWorker(unit);
		}
		else if (unit->getType() == UnitTypes::Protoss_Shuttle)
		{
			Transport().removeUnit(unit);
		}
	}

	if (unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		if (unit->getType().isResourceDepot())
		{
			Bases().removeBase(unit);
		}
	}

	if (unit->getType().isResourceContainer())
	{
		Resources().removeResource(unit);
	}
	return;
}

void UnitTrackerClass::onUnitMorph(Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		// Zerg morphing
		if (unit->getType().getRace() == Races::Zerg)
		{
			if (unit->getType().supplyRequired() > 0) supply += unit->getType().supplyRequired(); // Store supply if it costs supply		
			if (unit->getType().isBuilding() && Workers().getMyWorkers().find(unit) != Workers().getMyWorkers().end())
			{
				Workers().removeWorker(unit);
				onUnitCreate(unit);
				supply -= 2;
			}
			else if (unit->getType().isWorker()) Workers().storeWorker(unit);
			else if (!unit->getType().isWorker() && !unit->getType().isBuilding())storeAlly(unit);
		}

		// Protoss morphing
		if (allyUnits.find(unit) != allyUnits.end() && (unit->getType() == UnitTypes::Protoss_Archon || unit->getType() == UnitTypes::Protoss_Dark_Archon))
		{
			allySizes[unit->getType().whatBuilds().first.size()] --;
			allyUnits[unit].setType(unit->getType());
			allySizes[unit->getType().size()] ++;
		}
	}
	else if (unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		if (enemyUnits.find(unit) != enemyUnits.end())
		{
			enemySizes[unit->getType().whatBuilds().first.size()] --;
			enemyUnits[unit].setType(unit->getType());
			enemySizes[unit->getType().size()] ++;
		}
		else
		{
			storeEnemy(unit);
		}
	}
	else if (unit->getType() == UnitTypes::Resource_Vespene_Geyser || unit->getType().isMineralField())
	{
		if (enemyUnits.find(unit) != enemyUnits.end())
		{
			enemySizes[unit->getType().whatBuilds().first.size()] --;
			enemyUnits[unit].setType(unit->getType());
			enemySizes[unit->getType().size()] ++;
		}
		if (allyUnits.find(unit) != allyUnits.end())
		{
			allySizes[unit->getType().whatBuilds().first.size()] --;
			allyUnits[unit].setType(unit->getType());
			allySizes[unit->getType().size()] ++;
		}
		Resources().storeResource(unit);
	}
}

void UnitTrackerClass::onUnitRenegade(Unit unit)
{
	if (!unit->getType().isRefinery()) onUnitDestroy(unit); // Exception is refineries, see: https://docs.google.com/document/d/1p7Rw4v56blhf5bzhSnFVfgrKviyrapDFHh9J4FNUXM0/edit
	if (unit->getPlayer() == Broodwar->self()) onUnitComplete(unit);
}

void UnitTrackerClass::onUnitComplete(Unit unit)
{
	if (unit->getType() == UnitTypes::Protoss_Scarab || unit->getType() == UnitTypes::Zerg_Larva || unit->getType() == UnitTypes::Terran_Vulture_Spider_Mine) return;

	if (unit->getPlayer() == Broodwar->self() && !unit->getType().isBuilding())
	{
		allySizes[unit->getType().size()] += 1;
		if (unit->getType().isWorker()) Workers().storeWorker(unit);
		else if (unit->getType() == UnitTypes::Protoss_Shuttle || unit->getType() == UnitTypes::Terran_Dropship || unit->getType() == UnitTypes::Zerg_Overlord) Transport().storeUnit(unit);
		else if (!unit->getType().isWorker()) storeAlly(unit);
	}

	if (unit->getType().isResourceDepot()) Bases().storeBase(unit);
	if (unit->getType().isResourceContainer()) Resources().storeResource(unit);
}