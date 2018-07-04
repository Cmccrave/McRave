#include "McRave.h"
#ifdef MCRAVE_ZERG

void BuildOrderManager::zergOpener()
{
	if (getOpening)
	{
		if (currentBuild == "Z2HatchMuta") Z2HatchMuta();
		if (currentBuild == "Z3HatchLing") Z3HatchLing();
		if (currentBuild == "Z4Pool") Z4Pool();
		if (currentBuild == "Z9Pool") Z9Pool();
		if (currentBuild == "Z2HatchHydra") Z2HatchHydra();
	}
}

void BuildOrderManager::zergTech()
{
	bool moreToAdd;
	set<UnitType> toCheck;

	if (getTech) {

		if (techList.empty()) {
			if (Strategy().getEnemyBuild() == "PFFE")
				techUnit = UnitTypes::Zerg_Hydralisk;
			else
				techUnit = UnitTypes::Zerg_Mutalisk;
		}
		else {
			if (techUnit == UnitTypes::None) {
				double highest = 0.0;
				for (auto &tech : Strategy().getUnitScores()) {
					if (tech.second > highest) {
						highest = tech.second;
						techUnit = tech.first;
					}
				}
			}

		}
	}	

	// No longer need to choose a tech
	if (techUnit != UnitTypes::None) {
		getTech = false;
		techList.insert(techUnit);
		unlockedType.insert(techUnit);
	}

	if (techUnit == UnitTypes::Zerg_Mutalisk) {
		techList.insert(UnitTypes::Zerg_Scourge);
		unlockedType.insert(UnitTypes::Zerg_Scourge);
	}

	// For every unit in our tech list, ensure we are building the required buildings
	for (auto &type : techList) {
		toCheck.insert(type);
		toCheck.insert(type.whatBuilds().first);
	}

	// Iterate all required branches of buildings that are required for this tech unit
	do {
		moreToAdd = false;
		for (auto &check : toCheck) {
			for (auto &pair : check.requiredUnits()) {
				UnitType type(pair.first);
				if (Broodwar->self()->completedUnitCount(type) == 0 && toCheck.find(type) == toCheck.end()) {
					toCheck.insert(type);
					moreToAdd = true;
				}
			}
		}
	} while (moreToAdd);

	// For each building we need to check, add to our queue whatever is possible to build based on its required branch
	int i = 0;
	for (auto &check : toCheck) {

		if (!check.isBuilding())
			continue;

		Broodwar->drawTextScreen(0, 100 + i, "%s", check.c_str());
		i += 10;

		bool canAdd = true;
		for (auto &pair : check.requiredUnits()) {
			UnitType type(pair.first);
			if (type.isBuilding() && Broodwar->self()->completedUnitCount(type) == 0) {
				canAdd = false;
			}
		}

		int s = Units().getSupply();
		if (canAdd) {
			itemQueue[check] = Item(1);
		}
	}
}

void BuildOrderManager::zergSituational()
{
	if (Strategy().getEnemyBuild() == "PFFE")
		itemQueue[UnitTypes::Zerg_Hatchery] = Item(5);

	// Adding hatcheries when needed
	if (shouldExpand() || shouldAddProduction())
		itemQueue[UnitTypes::Zerg_Hatchery] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive) + 1);

	// When to tech
	if (Broodwar->getFrameCount() > 5000 && Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) > (1 + techList.size()) * 2)
		getTech = true;
	if (techComplete())
		techUnit = UnitTypes::None;

	// When to add colonies
	if (Strategy().enemyRush() || (Units().getGlobalEnemyGroundStrength() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense() && Units().getSupply() >= 40))
		itemQueue[UnitTypes::Zerg_Creep_Colony] = Item(min(6, max(2, Units().getSupply() / 20)));

	// Island play
	if (!Terrain().isIslandMap() || Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Zergling) <= 6)
		unlockedType.insert(UnitTypes::Zerg_Zergling);
	else
		unlockedType.erase(UnitTypes::Zerg_Zergling);

	if (Units().getSupply() >= 200) {
		itemQueue[UnitTypes::Zerg_Queens_Nest] = Item(1);
		itemQueue[UnitTypes::Zerg_Hive] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Queens_Nest) >= 1);
		itemQueue[UnitTypes::Zerg_Lair] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Queens_Nest) < 1);
	}

	unlockedType.insert(UnitTypes::Zerg_Drone);
	unlockedType.insert(UnitTypes::Zerg_Overlord);

	if (!getOpening) {
		gasLimit = INT_MAX;

		if (shouldAddGas())
			itemQueue[UnitTypes::Zerg_Extractor] = Item(Resources().getGasCount());

		if (Units().getSupply() >= 100)
			itemQueue[UnitTypes::Zerg_Evolution_Chamber] = Item(2);		
	}
}

#endif // 