#include "McRave.h"

namespace McRave
{
	void UnitManager::onUnitDiscover(Unit unit)
	{
		mapBWEB.onUnitDiscover(unit);

		if (unit->getPlayer()->isEnemy(Broodwar->self()))
			storeEnemyUnit(unit);

		if (Terrain().isIslandMap() && unit->getPlayer() == Broodwar->neutral() && !unit->getType().isResourceContainer() && unit->getType().isBuilding())
			storeNeutralUnit(unit);
	}

	void UnitManager::onUnitCreate(Unit unit)
	{
		if (unit->getPlayer() == Broodwar->self()) {

			// TODO - Move to store area
			// Store supply if it costs supply
			if (unit->getType().supplyRequired() > 0)
				supply += unit->getType().supplyRequired();
			storeMyUnit(unit);

			// Store Buildings on creation rather than completion
			//if (unit->getType().isBuilding())
			//	Buildings().storeBuilding(unit);

			//if (unit->getType().isResourceDepot())
			//	Stations().storeStation(unit);
			//else if (unit->getType() == UnitTypes::Protoss_Pylon)
			//	Pylons().storePylon(unit);
			//else if (unit->getType() == UnitTypes::Protoss_Photon_Cannon || unit->getType() == UnitTypes::Terran_Missile_Turret || unit->getType() == UnitTypes::Zerg_Creep_Colony)
			//	storeAlly(unit);
		}

		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);		
	}

	void UnitManager::onUnitDestroy(Unit unit)
	{
		auto myUnit = myUnits.find(unit) != myUnits.end();
		auto enemyUnit = enemyUnits.find(unit) != enemyUnits.end();
		mapBWEB.onUnitDestroy(unit);

		if (myUnit) {
			supply -= unit->getType().supplyRequired();
			allySizes[unit->getType().size()] -= 1;
			myUnits.erase(unit);

			Transport().removeUnit(unit);
			Workers().removeWorker(unit);
		}
		else if (enemyUnit) {
			enemySizes[unit->getType().size()] -= 1;
			enemyUnits.erase(unit);
			Stations().removeStation(unit);
		}
		else {
			neutrals.erase(unit);
			Resources().removeResource(unit);
		}
	}

	void UnitManager::onUnitMorph(Unit unit)
	{
		mapBWEB.onUnitMorph(unit);

		// My unit
		if (unit->getPlayer() == Broodwar->self()) {

			// Refinery
			if (unit->getType().isRefinery())
				Buildings().storeBuilding(unit);

			// Zerg morphing
			if (unit->getType().getRace() == Races::Zerg) {
				supply += unit->getType().supplyRequired();

				if (unit->getType().isBuilding() && Workers().getMyWorkers().find(unit) != Workers().getMyWorkers().end()) {
					Workers().removeWorker(unit);
					onUnitCreate(unit);
					supply -= 2;
				}

				else if (unit->getType().isWorker())
					Workers().storeWorker(unit);
				else if (!unit->getType().isWorker() && !unit->getType().isBuilding())
					storeMyUnit(unit);
			}

			// Protoss morphing
			if (myUnits.find(unit) != myUnits.end() && (unit->getType() == UnitTypes::Protoss_Archon || unit->getType() == UnitTypes::Protoss_Dark_Archon)) {
				allySizes[unit->getType().whatBuilds().first.size()] --;
				myUnits[unit].setType(unit->getType());
				allySizes[unit->getType().size()] ++;
			}
		}

		// Enemy unit
		else if (unit->getPlayer()->isEnemy(Broodwar->self())) {

			// Remove any stations on a canceled hatchery
			if (unit->getType() == UnitTypes::Zerg_Drone)
				Stations().removeStation(unit);

			if (enemyUnits.find(unit) != enemyUnits.end()) {
				enemySizes[unit->getType().whatBuilds().first.size()] --;
				enemyUnits[unit].setType(unit->getType());
				enemySizes[unit->getType().size()] ++;
			}
			else
				storeEnemyUnit(unit);
		}

		// Refinery that morphed as an enemy
		else if (unit->getType().isResourceContainer()) {
			if (enemyUnits.find(unit) != enemyUnits.end()) {
				enemySizes[unit->getType().whatBuilds().first.size()] --;
				enemyUnits[unit].setType(unit->getType());
				enemySizes[unit->getType().size()] ++;
			}

			if (myUnits.find(unit) != myUnits.end()) {
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
		if (unit->getPlayer() == Broodwar->self() && !unit->getType().isBuilding()) {
			allySizes[unit->getType().size()] += 1;
			if (unit->getType().isWorker())
				Workers().storeWorker(unit);
			else if (unit->getType() == UnitTypes::Protoss_Shuttle || unit->getType() == UnitTypes::Terran_Dropship)
				Transport().storeUnit(unit);
			else if (!unit->getType().isWorker())
				storeMyUnit(unit);
		}

		if (unit->getType().isResourceDepot())
			Stations().storeStation(unit);
		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
	}
}