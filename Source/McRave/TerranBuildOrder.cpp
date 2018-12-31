//#include "McRave.h"
//
//void BuildOrderManager::terranOpener()
//{
//	if (getOpening)
//	{
//		if (currentBuild == "T2Fact")			T2Fact();
//		if (currentBuild == "TSparks")			TSparks();
//		if (currentBuild == "T2PortWraith")		T2PortWraith();
//		if (currentBuild == "T1RaxFE")			T1RaxFE();
//		if (currentBuild == "T2RaxFE")			T2RaxFE();
//		if (currentBuild == "TNukeMemes")		TNukeMemes();
//		if (currentBuild == "T1FactFE")			T1FactFE();
//		if (currentBuild == "TBCMemes")			TBCMemes();
//	}
//	return;
//}
//
//void BuildOrderManager::terranTech()
//{
//	bool moreToAdd;
//	set<UnitType> toCheck;
//
//	if (!firstReady())
//		return;
//
//	if (getTech) {
//		if (techUnit == UnitTypes::None) {
//			double highest = 0.0;
//			for (auto &tech : Strategy().getUnitScores()) {
//				if (tech.second > highest) {
//					highest = tech.second;
//					techUnit = tech.first;
//				}
//			}
//		}
//
//		// No longer need to choose a tech
//		if (techUnit != UnitTypes::None) {
//			getTech = false;
//			techList.insert(techUnit);
//			unlockedType.insert(techUnit);
//		}
//	}
//
//	if (techUnit == UnitTypes::Terran_Battlecruiser) {
//		unlockedType.insert(UnitTypes::Terran_Dropship);
//	}
//
//	// For every unit in our tech list, ensure we are building the required buildings
//	for (auto &type : techList) {
//		toCheck.insert(type);
//		toCheck.insert(type.whatBuilds().first);
//	}
//
//	// Iterate all required branches of buildings that are required for this tech unit
//	do {
//		moreToAdd = false;
//		for (auto &check : toCheck) {
//			for (auto &pair : check.requiredUnits()) {
//				UnitType type(pair.first);
//				if (Broodwar->self()->completedUnitCount(type) == 0 && toCheck.find(type) == toCheck.end()) {
//					toCheck.insert(type);
//					moreToAdd = true;
//				}
//			}
//		}
//	} while (moreToAdd);
//
//	// For each building we need to check, add to our queue whatever is possible to build based on its required branch
//	int i = 0;
//	for (auto &check : toCheck) {
//
//		if (!check.isBuilding())
//			continue;
//
//		Broodwar->drawTextScreen(0, 100 + i, "%s", check.c_str());
//		i += 10;
//
//		bool canAdd = true;
//		for (auto &pair : check.requiredUnits()) {
//			UnitType type(pair.first);
//			if (type.isBuilding() && Broodwar->self()->completedUnitCount(type) == 0) {
//				canAdd = false;
//			}
//		}
//
//		int s = Units::getSupply();
//		if (canAdd) {
//			itemQueue[check] = Item(1);
//		}
//	}
//
//}
//
//void BuildOrderManager::terranSituational()
//{
//	// Some temp stuff
//	bool bioUnlocked, mechUnlocked, airUnlocked;
//	bioUnlocked = mechUnlocked = airUnlocked = false;
//
//	satVal = 2;
//	prodVal = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Factory) + Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Starport);
//	baseVal = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center);
//
//	techVal = techList.size();
//	productionSat = (prodVal >= satVal * baseVal);
//	techSat = (techVal > baseVal);
//
//	if (techComplete())
//		techUnit = UnitTypes::None; // If we have our tech unit, set to none	
//	if (Strategy().needDetection() || (!getOpening && !getTech && !techSat /*&& productionSat*/ && techUnit == UnitTypes::None))
//		getTech = true; // If production is saturated and none are idle or we need detection, choose a tech
//
//	// Unlocked type logic
//	if (currentBuild == "TSparks")
//	{
//		bioUnlocked = true;
//		productionSat = (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= min(12, (3 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))));
//		unlockedType.insert(UnitTypes::Terran_Marine);
//		unlockedType.insert(UnitTypes::Terran_Medic);
//	}
//	else if (currentBuild == "T2PortWraith")
//	{
//		airUnlocked = true;
//		mechUnlocked = true;
//		unlockedType.insert(UnitTypes::Terran_Wraith);
//		unlockedType.insert(UnitTypes::Terran_Vulture);
//		unlockedType.insert(UnitTypes::Terran_Valkyrie);
//		techList.insert(UnitTypes::Terran_Wraith);
//		techList.insert(UnitTypes::Terran_Valkyrie);
//
//		if (!getOpening || Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) >= 4)
//			unlockedType.erase(UnitTypes::Terran_Marine);
//		else
//			unlockedType.insert(UnitTypes::Terran_Marine);
//
//		// Machine Shop
//		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 1)
//			itemQueue[UnitTypes::Terran_Machine_Shop] = Item(max(1, Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) - (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center))));
//
//		// Control Tower
//		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Starport) >= 1)
//			itemQueue[UnitTypes::Terran_Control_Tower] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Starport));
//	}
//	else if (currentBuild == "T1RaxFE" || currentBuild == "T2RaxFE")
//	{
//		bioUnlocked = true;
//		unlockedType.insert(UnitTypes::Terran_Marine);
//		unlockedType.insert(UnitTypes::Terran_Medic);
//		productionSat = (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= min(12, (3 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))));
//	}
//
//	else if (currentBuild == "TNukeMemes")
//	{
//		bioUnlocked = true;
//		unlockedType.insert(UnitTypes::Terran_Marine);
//		unlockedType.insert(UnitTypes::Terran_Ghost);
//		unlockedType.insert(UnitTypes::Terran_Nuclear_Missile);
//		techList.insert(UnitTypes::Terran_Ghost);
//		techList.insert(UnitTypes::Terran_Nuclear_Missile);
//
//		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Covert_Ops) > 0)
//			itemQueue[UnitTypes::Terran_Nuclear_Silo] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center));
//	}
//	else if (currentBuild == "TBCMemes") {
//		unlockedType.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);
//		unlockedType.insert(UnitTypes::Terran_Vulture);
//		unlockedType.insert(UnitTypes::Terran_Goliath);
//
//		if (!getOpening || Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) >= 10)
//			unlockedType.erase(UnitTypes::Terran_Marine);
//		else
//			unlockedType.insert(UnitTypes::Terran_Marine);
//
//		mechUnlocked = true;
//	}
//	else
//	{
//		mechUnlocked = true;
//		productionSat = (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= min(12, (2 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))));
//		unlockedType.insert(UnitTypes::Terran_Goliath);
//		unlockedType.insert(UnitTypes::Terran_Vulture);
//		unlockedType.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);
//
//		if (!getOpening || Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) >= 4)
//			unlockedType.erase(UnitTypes::Terran_Marine);
//		else
//			unlockedType.insert(UnitTypes::Terran_Marine);
//	}
//
//	// Control Tower
//	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Starport) >= 1)
//		itemQueue[UnitTypes::Terran_Control_Tower] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Starport));
//
//	// Machine Shop
//	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 1)
//		itemQueue[UnitTypes::Terran_Machine_Shop] = Item(max(0, Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) - (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center))));
//
//	// Supply Depot logic
//	if (Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Supply_Depot) > (int)fastExpand)
//	{
//		int count = min(22, (int)floor((Units::getSupply() / max(15, (16 - Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Supply_Depot) - Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))))));
//		itemQueue[UnitTypes::Terran_Supply_Depot] = Item(count);
//	}
//
//	// Expansion logic
//	if (shouldExpand())
//		itemQueue[UnitTypes::Terran_Command_Center] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) + 1);
//
//	// Bunker logic
//	if (Strategy().enemyRush() && !wallMain)
//		itemQueue[UnitTypes::Terran_Bunker] = Item(1);
//
//	if (!getOpening)
//	{
//		// Refinery logic
//		if (shouldAddGas())
//			itemQueue[UnitTypes::Terran_Refinery] = Item(Resources::getGasCount());
//
//		// Armory logic TODO
//		if (!isBioBuild())
//			itemQueue[UnitTypes::Terran_Armory] = Item((Units::getSupply() > 160) + (Units::getSupply() > 200));
//
//		// Academy logic
//		if (Strategy().needDetection())
//		{
//			itemQueue[UnitTypes::Terran_Academy] = Item(1);
//			itemQueue[UnitTypes::Terran_Comsat_Station] = Item(2);
//		}
//
//		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Science_Facility) > 0)
//			itemQueue[UnitTypes::Terran_Physics_Lab] = Item(1);
//
//		// Engineering Bay logic
//		if (Units::getSupply() > 200)
//			itemQueue[UnitTypes::Terran_Engineering_Bay] = Item(1 + (bioBuild));
//
//		// Barracks logic
//		if (bioUnlocked && shouldAddProduction::)
//			itemQueue[UnitTypes::Terran_Barracks] = Item(min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) + 1));
//
//		// Factory logic
//		if (mechUnlocked && shouldAddProduction::)
//			itemQueue[UnitTypes::Terran_Factory] = Item(min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 2, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Factory) + 1));
//
//		// Starport logic
//		if (airUnlocked && shouldAddProduction::)
//			itemQueue[UnitTypes::Terran_Starport] = Item(min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 2, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Starport) + 1));
//
//		// Missle Turret logic
//		if (Players::getNumberZerg() > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Engineering_Bay) > 0)
//			itemQueue[UnitTypes::Terran_Missile_Turret] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 2);
//	}
//}