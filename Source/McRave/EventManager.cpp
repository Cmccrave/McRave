#include "McRave.h"

namespace McRave
{
	void UnitManager::onUnitDiscover(Unit unit)
	{
		mapBWEB.onUnitDiscover(unit);

		if (unit->getPlayer()->isEnemy(Broodwar->self()))
			Units().storeEnemy(unit);

		
		if (Terrain().isIslandMap() && unit->getPlayer() == Broodwar->neutral() && !unit->getType().isResourceContainer() && unit->getType().isBuilding()) 
			Units().storeNeutral(unit);		
	}

	void UnitManager::onUnitCreate(Unit unit)
	{
		if (unit->getPlayer() == Broodwar->self()) {
			if (unit->getType().supplyRequired() > 0) supply += unit->getType().supplyRequired(); // Store supply if it costs supply		

			// Store Buildings on creation rather than completion
			if (unit->getType().isBuilding())
				Buildings().storeBuilding(unit);

			if (unit->getType().isResourceDepot())
				Stations().storeStation(unit);
			else if (unit->getType() == UnitTypes::Protoss_Pylon)
				Pylons().storePylon(unit);
			else if (unit->getType() == UnitTypes::Protoss_Photon_Cannon || unit->getType() == UnitTypes::Terran_Missile_Turret || unit->getType() == UnitTypes::Zerg_Creep_Colony)
				storeAlly(unit);
		}
	}

	void UnitManager::onUnitDestroy(Unit unit)
	{
		if (Terrain().isIslandMap() && neutrals.find(unit) != neutrals.end())
			neutrals.erase(unit);

		mapBWEB.onUnitDestroy(unit);

		if (myUnits.find(unit) != myUnits.end()) {
			if (myUnits[unit].getTransport()) {
				Transport().removeUnit(unit);
			}
			allySizes[unit->getType().size()] -= 1;
			myUnits.erase(unit);
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

		if (unit->getPlayer() == Broodwar->self())
		{
			supply -= unit->getType().supplyRequired();

			if (unit->getType().isResourceDepot())
			{
				Stations().removeStation(unit);
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
				Stations().removeStation(unit);
			}
		}

		if (unit->getType().isResourceContainer())
		{
			Resources().removeResource(unit);
		}
		return;
	}

	void UnitManager::onUnitMorph(Unit unit)
	{
		mapBWEB.onUnitMorph(unit);
		if (unit->getType().isRefinery())
		{
			Buildings().storeBuilding(unit);
		}
		if (unit->getPlayer() == Broodwar->self())
		{
			// Zerg morphing
			if (unit->getType().getRace() == Races::Zerg)
			{
				if (unit->getType().supplyRequired() > 0)
					supply += unit->getType().supplyRequired(); // Store supply if it costs supply	

				if (unit->getType().isBuilding() && Workers().getMyWorkers().find(unit) != Workers().getMyWorkers().end()) {
					Workers().removeWorker(unit);
					onUnitCreate(unit);
					supply -= 2;
				}

				else if (unit->getType().isWorker())
					Workers().storeWorker(unit);
				else if (!unit->getType().isWorker() && !unit->getType().isBuilding())
					storeAlly(unit);
			}

			// Protoss morphing
			if (myUnits.find(unit) != myUnits.end() && (unit->getType() == UnitTypes::Protoss_Archon || unit->getType() == UnitTypes::Protoss_Dark_Archon))
			{
				allySizes[unit->getType().whatBuilds().first.size()] --;
				myUnits[unit].setType(unit->getType());
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
			if (myUnits.find(unit) != myUnits.end())
			{
				allySizes[unit->getType().whatBuilds().first.size()] --;
				myUnits[unit].setType(unit->getType());
				allySizes[unit->getType().size()] ++;
			}
			Resources().storeResource(unit);
		}
	}

	void UnitManager::onUnitRenegade(Unit unit)
	{
		// Exception is refineries, see: https://docs.google.com/document/d/1p7Rw4v56blhf5bzhSnFVfgrKviyrapDFHh9J4FNUXM0/edit
		if (!unit->getType().isRefinery())
			onUnitDestroy(unit); 
		if (unit->getPlayer() == Broodwar->self())
			onUnitComplete(unit);
	}

	void UnitManager::onUnitComplete(Unit unit)
	{
		if (unit->getPlayer() == Broodwar->self() && !unit->getType().isBuilding())	{
			allySizes[unit->getType().size()] += 1;
			if (unit->getType().isWorker())
				Workers().storeWorker(unit);
			else if (unit->getType() == UnitTypes::Protoss_Shuttle || unit->getType() == UnitTypes::Terran_Dropship || unit->getType() == UnitTypes::Zerg_Overlord)
				Transport().storeUnit(unit);
			else if (!unit->getType().isWorker())
				storeAlly(unit);
		}

		if (unit->getType().isResourceDepot())
			Stations().storeStation(unit);
		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
	}
}