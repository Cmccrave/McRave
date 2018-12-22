#include "McRave.h"

// Information from: https://docs.google.com/document/d/1p7Rw4v56blhf5bzhSnFVfgrKviyrapDFHh9J4FNUXM0

namespace McRave
{
	void UnitManager::onUnitDiscover(Unit unit)
	{
		BWEB::Map::onUnitDiscover(unit);

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

			// Store buildings so grids are updated
			if (unit->getType().isBuilding())
				storeUnit(unit);

			if (unit->getType() == UnitTypes::Protoss_Pylon)
				Pylons().storePylon(unit);
		}

		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
		if (unit->getType().isResourceDepot())
			MyStations().storeStation(unit);
	}

	void UnitManager::onUnitDestroy(Unit unit)
	{
		if (Terrain().isIslandMap() && neutrals.find(unit) != neutrals.end())
			neutrals.erase(unit);

		BWEB::Map::onUnitDestroy(unit);

		// My unit
		if (unit->getPlayer() == Broodwar->self()) {
			auto &info = myUnits[unit];

			supply -= unit->getType().supplyRequired();

			Transport().removeUnit(unit);

			if (info.hasResource())
				info.getResource().setGathererCount(info.getResource().getGathererCount() - 1);
			if (info.getRole() != Role::None)
				myRoles[info.getRole()]--;
			if (info.getRole() == Role::Scouting)
				scoutDeadFrame = Broodwar->getFrameCount();

			myUnits.erase(unit);
		}

		// Enemy unit
		else if (unit->getPlayer() == Broodwar->enemy())
			enemyUnits.erase(unit);
		else if (unit->getPlayer()->isAlly(Broodwar->self()))
			allyUnits.erase(unit);

		// Resource
		if (unit->getType().isResourceContainer())
			Resources().removeResource(unit);

		// Station
		if (unit->getType().isResourceDepot())
			MyStations().removeStation(unit);
	}

	void UnitManager::onUnitMorph(Unit unit)
	{
		BWEB::Map::onUnitMorph(unit);

		// My unit
		if (unit->getPlayer() == Broodwar->self()) {
			auto isEgg = unit->getType() == UnitTypes::Zerg_Egg || unit->getType() == UnitTypes::Zerg_Lurker_Egg;

			// Zerg morphing
			if (unit->getType().getRace() == Races::Zerg) {

				if (isEgg) {
					supply -= unit->getType().supplyRequired();
					supply += unit->getBuildType().supplyRequired();
				}
				if (unit->getType().isBuilding())
					supply -= 2;

				auto &info = myUnits[unit];
				if (info.hasResource())
					info.getResource().setGathererCount(info.getResource().getGathererCount() - 1);

				storeUnit(unit);
			}
		}

		// Enemy unit
		else if (unit->getPlayer()->isEnemy(Broodwar->self())) {

			// Remove any stations on a canceled hatchery
			if (unit->getType() == UnitTypes::Zerg_Drone)
				MyStations().removeStation(unit);
			else
				storeUnit(unit);
		}

		// Refinery that morphed as an enemy
		else if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
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
		if (unit->getPlayer() == Broodwar->self())
			storeUnit(unit);
		if (unit->getType().isResourceDepot())
			MyStations().storeStation(unit);
		if (unit->getType().isResourceContainer())
			Resources().storeResource(unit);
	}
}