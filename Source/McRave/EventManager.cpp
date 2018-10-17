#include "McRave.h"

// Information from: https://docs.google.com/document/d/1p7Rw4v56blhf5bzhSnFVfgrKviyrapDFHh9J4FNUXM0

namespace McRave
{
	void UnitManager::onUnitDiscover(Unit unit)
	{
		mapBWEB.onUnitDiscover(unit);

		if (unit->getPlayer()->isEnemy(Broodwar->self()))
			storeUnit(unit);

		if (Terrain().isIslandMap() && unit->getPlayer() == Broodwar->neutral() && !unit->getType().isResourceContainer() && unit->getType().isBuilding())
			storeUnit(unit);
	}

	void UnitManager::onUnitCreate(Unit unit)
	{
		if (unit->getPlayer() == Broodwar->self()) {

			// Store supply if it costs supply
			if (unit->getType().supplyRequired() > 0)
				supply += unit->getType().supplyRequired();

			if (unit->getType().isBuilding())
				storeUnit(unit);

			if (unit->getType() == UnitTypes::Protoss_Pylon)
				Pylons().storePylon(unit);				
		}

		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
		if (unit->getType().isResourceDepot())
			Stations().storeStation(unit);
	}

	void UnitManager::onUnitDestroy(Unit unit)
	{
		if (Terrain().isIslandMap() && neutrals.find(unit) != neutrals.end())
			neutrals.erase(unit);

		mapBWEB.onUnitDestroy(unit);

		// My unit
		if (unit->getPlayer() == Broodwar->self()) {
			supply -= unit->getType().supplyRequired();
			Transport().removeUnit(unit);
			myUnits.erase(unit);			
		}

		// Enemy unit
		else if (unit->getPlayer() == Broodwar->enemy()){
			//enemySizes[unit->getType().size()] -= 1;
			enemyUnits.erase(unit);
		}
		else if (unit->getPlayer()->isAlly(Broodwar->self())) {
			allyUnits.erase(unit);
		}

		// Resource
		if (unit->getType().isResourceContainer())
			Resources().removeResource(unit);

		// Station
		if (unit->getType().isResourceDepot())
			Stations().removeStation(unit);
	}

	void UnitManager::onUnitMorph(Unit unit)
	{
		mapBWEB.onUnitMorph(unit);

		// My unit
		if (unit->getPlayer() == Broodwar->self()) {

			// TODO: Zerg morphing
			if (unit->getType().getRace() == Races::Zerg) {
				supply += unit->getType().supplyRequired();

				if (unit->getType().isBuilding() && myUnits.find(unit) != myUnits.end()) {
					myUnits.erase(unit);
					onUnitCreate(unit);
					supply -= 2;
				}
				storeUnit(unit);
			}

			// Protoss morphing
			if (myUnits.find(unit) != myUnits.end() && (unit->getType() == UnitTypes::Protoss_Archon || unit->getType() == UnitTypes::Protoss_Dark_Archon)) {
				//allySizes[unit->getType().whatBuilds().first.size()] --;
				myUnits[unit].setType(unit->getType());
				//allySizes[unit->getType().size()] ++;
			}
		}

		// Enemy unit
		else if (unit->getPlayer()->isEnemy(Broodwar->self())) {

			// Remove any stations on a canceled hatchery
			if (unit->getType() == UnitTypes::Zerg_Drone)
				Stations().removeStation(unit);

			if (enemyUnits.find(unit) != enemyUnits.end()) {
				//enemySizes[unit->getType().whatBuilds().first.size()] --;
				enemyUnits[unit].setType(unit->getType());
				//enemySizes[unit->getType().size()] ++;
			}
			else
				storeUnit(unit);
		}

		// Refinery that morphed as an enemy
		else if (unit->getType().isResourceContainer()) {
			if (enemyUnits.find(unit) != enemyUnits.end()) {
				//enemySizes[unit->getType().whatBuilds().first.size()] --;
				enemyUnits[unit].setType(unit->getType());
				//enemySizes[unit->getType().size()] ++;
			}

			if (myUnits.find(unit) != myUnits.end()) {
				//allySizes[unit->getType().whatBuilds().first.size()] --;
				myUnits[unit].setType(unit->getType());
				//allySizes[unit->getType().size()] ++;
			}
			Resources().storeResource(unit);
		}
	}

	void UnitManager::onUnitRenegade(Unit unit)
	{
		// TODO: Refinery is added in onUnitDiscover for enemy units (keep resource unit the same)
		// Destroy the unit otherwise
		if (!unit->getType().isRefinery()) {
			enemyUnits.erase(unit);
			myUnits.erase(unit);
		}

		if (unit->getPlayer() == Broodwar->self())
			onUnitComplete(unit);
	}

	void UnitManager::onUnitComplete(Unit unit)
	{
		if (unit->getPlayer() == Broodwar->self()) {
			//allySizes[unit->getType().size()] += 1;
			storeUnit(unit);
		}

		if (unit->getType().isResourceDepot())
			Stations().storeStation(unit);
		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
	}
}