//#include "McRave.h"
//
//void BuildOrderManager::zergOpener()
//{
//	if (getOpening)	{
//		if (currentBuild == "Z2HatchMuta") Z2HatchMuta();
//		if (currentBuild == "Z3HatchLing") Z3HatchLing();
//		if (currentBuild == "Z4Pool") Z4Pool();
//		if (currentBuild == "Z9Pool") Z9Pool();
//		if (currentBuild == "Z2HatchHydra") Z2HatchHydra();
//		if (currentBuild == "Z3HatchBeforePool") Z3HatchBeforePool();
//		if (currentBuild == "ZLurkerTurtle") ZLurkerTurtle();
//		if (currentBuild == "Z9PoolSpire") Z9PoolSpire();
//	}
//}
//
//void BuildOrderManager::zergTech()
//{
//	getNewTech();
//	checkNewTech();
//	checkAllTech();
//	checkExoticTech();
//}
//
//void BuildOrderManager::zergSituational()
//{
//	// Adding hatcheries when needed
//	if (shouldExpand() || shouldAddProduction::)
//		itemQueue[UnitTypes::Zerg_Hatchery] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive) + 1);
//
//	if (Strategy::enemyFastExpand() && !rush)
//		itemQueue[UnitTypes::Zerg_Hatchery] = Item(min(3, Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive) + 1));
//
//	// When to tech
//	if (Broodwar->getFrameCount() > 5000 && Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) > (1 + (int)techList.size()) * 2)
//		getTech = true;
//	if (techComplete())
//		techUnit = UnitTypes::None;
//
//	//// When to add colonies
//	//if (Units::getGlobalEnemyGroundStrength() > Units::getGlobalAllyGroundStrength() + Units::getAllyDefense() && Units::getSupply() >= 40)
//	//	itemQueue[UnitTypes::Zerg_Creep_Colony] = Item(min(6, max(2, Units::getSupply() / 20)));
//	
//	// Hack
//	if (currentBuild == "ZLurkerTurtle") {
//		if (Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Zergling) >= 6)
//			unlockedType.erase(UnitTypes::Zerg_Zergling);
//		if (Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lurker) >= 1)
//			itemQueue[UnitTypes::Zerg_Creep_Colony] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lurker));
//	}
//
//	unlockedType.insert(UnitTypes::Zerg_Drone);
//	unlockedType.insert(UnitTypes::Zerg_Overlord);
//
//	if (!getOpening) {
//		gasLimit = INT_MAX;
//
//		if (shouldAddGas())
//			itemQueue[UnitTypes::Zerg_Extractor] = Item(Resources::getGasCount());		
//
//		if (Units::getSupply() >= 100)
//			itemQueue[UnitTypes::Zerg_Evolution_Chamber] = Item(2);
//	}
//}
//
//void BuildOrderManager::zergUnlocks()
//{
//	if (getOpening) {
//		if (droneLimit > Units::getMyTypeCount(UnitTypes::Zerg_Drone))
//			unlockedType.insert(UnitTypes::Zerg_Drone);
//		else
//			unlockedType.erase(UnitTypes::Zerg_Drone);
//
//		if (lingLimit > Units::getMyTypeCount(UnitTypes::Zerg_Zergling))
//			unlockedType.insert(UnitTypes::Zerg_Zergling);
//		else
//			unlockedType.erase(UnitTypes::Zerg_Zergling);
//	}
//	else {
//		unlockedType.insert(UnitTypes::Zerg_Drone);
//		unlockedType.insert(UnitTypes::Zerg_Zergling);
//	}
//}