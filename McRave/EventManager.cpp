#include "McRave.h"

void UnitTrackerClass::onUnitDiscover(Unit unit)
{
	if (unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		Units().storeEnemy(unit);
	}
	return;
}

void UnitTrackerClass::onUnitCreate(Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		// Store supply if it costs supply
		if (unit->getType().supplyRequired() > 0)
		{
			supply += unit->getType().supplyRequired();
		}

		// Store Buildings, Bases, Pylons
		if (unit->getType().isResourceDepot())
		{
			Bases().storeBase(unit);
			Buildings().storeBuilding(unit);
		}
		else if (unit->getType() == UnitTypes::Protoss_Pylon)
		{
			Pylons().storePylon(unit);
			Buildings().storeBuilding(unit);
		}
		else if (unit->getType() == UnitTypes::Protoss_Photon_Cannon)
		{
			storeAlly(unit);
		}
		else if (unit->getType().isBuilding())
		{
			Buildings().storeBuilding(unit);
		}
	}
	return;
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
		Grids().updateDefenseGrid(allyDefenses[unit]);
		allyDefenses.erase(unit);
	}
	else if (enemyUnits.find(unit) != enemyUnits.end())
	{
		enemySizes[unit->getType().size()] -= 1;
		enemyUnits.erase(unit);
	}

	if (unit->getPlayer() == Broodwar->self())
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
	if (unit->getType().isResourceContainer() && unit->getType().isMineralField())
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
			if (unit->getType() == UnitTypes::Zerg_Egg || unit->getType() == UnitTypes::Zerg_Lurker_Egg)
			{
				// Figure out how morphing works
			}
			else if (unit->getType().isBuilding() && Workers().getMyWorkers().find(unit) != Workers().getMyWorkers().end())
			{
				Workers().removeWorker(unit);
				Buildings().storeBuilding(unit);
				supply -= 2;
			}
			else if (unit->getType().isWorker())
			{
				Workers().storeWorker(unit);
			}
			else if (!unit->getType().isWorker() && !unit->getType().isBuilding())
			{
				storeAlly(unit);
			}
		}

		// Protoss morphing
		if (unit->getType() == UnitTypes::Protoss_Archon)
		{
			// Remove the two HTs and their respective supply
			// whatBuilds returns previous units size
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
	else if (unit->getType().isResourceContainer())
	{
		Resources().storeResource(unit);
	}
}

void UnitTrackerClass::onUnitComplete(Unit unit)
{
	// Don't store useless things
	if (unit->getType() == UnitTypes::Protoss_Scarab || unit->getType() == UnitTypes::Zerg_Larva)
	{
		return;
	}

	if (unit->getPlayer() == Broodwar->self())
	{
		allySizes[unit->getType().size()] += 1;

		// Store Workers, Units, Bases(update base grid?)
		if (unit->getType().isWorker())
		{
			Workers().storeWorker(unit);
		}

		if (unit->getType() == UnitTypes::Protoss_Shuttle || unit->getType() == UnitTypes::Terran_Dropship) //|| unit->getType() == UnitTypes::Zerg_Overlord
		{
			Transport().storeUnit(unit);
		}
		else if (!unit->getType().isWorker() && !unit->getType().isBuilding())
		{
			storeAlly(unit);
		}
	}
	else if (unit->getType().isResourceContainer())
	{
		Resources().storeResource(unit);
	}
}