//#include "McRave.h"
//using namespace UnitTypes;
//#define s Units().getSupply()
//
//namespace McRave 
//{
//	static int vis(UnitType t) {
//		return Broodwar->self()->visibleUnitCount(t);
//	}
//	static int com(UnitType t) {
//		return Broodwar->self()->completedUnitCount(t);
//	}
//
//	static string enemyBuild = Strategy().getEnemyBuild();
//	void BuildOrderManager::T2Fact()
//	{
//		firstUpgrade = UpgradeTypes::Ion_Thrusters;
//		firstTech = TechTypes::None;
//		getOpening = s < 70;
//		scout = vis(Terran_Barracks) > 0;
//		wallMain = true;
//		gasLimit = 3;
//
//		itemQueue[Terran_Supply_Depot] =		Item(s >= 18);
//		itemQueue[Terran_Barracks] =			Item(s >= 20);
//		itemQueue[Terran_Refinery] =			Item(s >= 24);
//		itemQueue[Terran_Factory] =				Item((s >= 30) + (s >= 36) + (s >= 46));
//	}
//
//	void BuildOrderManager::TSparks()
//	{
//		firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
//		firstTech = TechTypes::Stim_Packs;
//		getOpening = s < 60;
//		bioBuild = true;
//		scout = vis(Terran_Barracks) > 0;
//		gasLimit = 3 - (2 * int(vis(Terran_Engineering_Bay) > 0));
//
//		itemQueue[Terran_Supply_Depot] =		Item(s >= 18);
//		itemQueue[Terran_Barracks] =			Item((vis(Terran_Supply_Depot) > 0) + (s >= 24) + (s >= 42));
//		itemQueue[Terran_Refinery] =			Item(s >= 30);
//		itemQueue[Terran_Academy] =				Item(s >= 48);
//		itemQueue[Terran_Engineering_Bay] =		Item(s >= 36);
//		itemQueue[Terran_Comsat_Station] =		Item(vis(Terran_Academy) > 0);
//	}
//
//	void BuildOrderManager::T2PortWraith()
//	{
//		firstUpgrade = UpgradeTypes::None;
//		firstTech = TechTypes::Cloaking_Field;
//		getOpening = s < 80;
//		fastExpand = true;
//		scout = vis(Terran_Barracks) > 0;
//
//		if (BWEB::Walls::getWall(BWEB::Map::getNaturalArea()))
//			wallNat = true;
//		else if (BWEB::Walls::getWall(BWEB::Map::getMainArea()))
//			wallMain = true;
//
//		itemQueue[Terran_Supply_Depot] =		Item((s >= 16) + (s >= 26), (s >= 18) + (s >= 26));
//		itemQueue[Terran_Barracks] =			Item(com(Terran_Supply_Depot) > 0);
//		itemQueue[Terran_Refinery] =			Item(s >= 24);
//		itemQueue[Terran_Factory] =				Item(s >= 32);
//		itemQueue[Terran_Starport] =			Item(2 * (s >= 44));
//		itemQueue[Terran_Command_Center] =		Item(1 + (s >= 70));
//	}
//
//	void BuildOrderManager::T1RaxFE()
//	{
//		bioBuild = true;
//		fastExpand = true;
//		firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
//		firstTech = TechTypes::Stim_Packs;
//		getOpening = s < 86;
//		scout = vis(Terran_Barracks) > 0;
//		gasLimit = 1 + (vis(Terran_Command_Center) > 1);
//
//		if (BWEB::Walls::getWall(BWEB::Map::getNaturalArea()))
//			wallNat = true;
//		else if (BWEB::Walls::getWall(BWEB::Map::getMainArea()))
//			wallMain = true;
//
//		itemQueue[Terran_Supply_Depot] =		Item((s >= 16) + (s >= 26), (s >= 18) + (s >= 26));
//		itemQueue[Terran_Barracks] =			Item((com(Terran_Supply_Depot) > 0) + (s >= 64) + (s >= 70) + (2 * (s >= 80)));
//		itemQueue[Terran_Command_Center] =		Item(1 + (s >= 36));
//		itemQueue[Terran_Refinery] =			Item(s >= 46);
//		itemQueue[Terran_Engineering_Bay] =		Item(s >= 50);
//		itemQueue[Terran_Academy] =				Item(s >= 60);
//		itemQueue[Terran_Comsat_Station] =		Item(s >= 80);
//	}
//
//	void BuildOrderManager::T2RaxFE()
//	{
//		bioBuild = true;
//		fastExpand = true;
//		firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
//		firstTech = TechTypes::Stim_Packs;
//		getOpening = s < 86;
//		scout = vis(Terran_Barracks) > 0;
//		gasLimit = 1 + (com(Terran_Command_Center) > 1);
//		wallNat = true;
//
//		itemQueue[Terran_Supply_Depot] =		Item((s >= 16) + (vis(Terran_Bunker) > 0), (s >= 18) + (vis(Terran_Bunker) > 0));
//		itemQueue[Terran_Bunker] =				Item((com(Terran_Barracks) > 0));
//		itemQueue[Terran_Barracks] =			Item((com(Terran_Supply_Depot) > 0) + (s >= 26) + (s >= 70) + (2 * (s >= 80)));
//		itemQueue[Terran_Refinery] =			Item(s >= 50);
//		itemQueue[Terran_Engineering_Bay] =		Item(s >= 44);
//		itemQueue[Terran_Academy] =				Item(s >= 60);
//		itemQueue[Terran_Comsat_Station] =		Item(s >= 70);
//		itemQueue[Terran_Command_Center] =		Item(1 + (s >= 80));
//	}
//
//	void BuildOrderManager::T1FactFE()
//	{
//		fastExpand = true;
//		firstUpgrade = UpgradeTypes::None;
//		firstTech = TechTypes::Tank_Siege_Mode;
//		getOpening = s < 80;
//		scout = vis(Terran_Barracks) > 0;
//		wallNat = true;
//
//		itemQueue[Terran_Supply_Depot] =		Item((s >= 16) + (s >= 34), (s >= 18) + (s >= 34));
//		itemQueue[Terran_Bunker] =				Item(s >= 30);
//		itemQueue[Terran_Barracks] =			Item(com(Terran_Supply_Depot) > 0);
//		itemQueue[Terran_Refinery] =			Item(s >= 28);
//		itemQueue[Terran_Factory] =				Item((s >= 32) + (s >= 64));
//		itemQueue[Terran_Machine_Shop] =		Item(s >= 40);
//		itemQueue[Terran_Command_Center] =		Item(1 + (s >= 56));
//	}
//
//	void BuildOrderManager::TNukeMemes()
//	{
//		firstUpgrade = UpgradeTypes::None;
//		firstTech = TechTypes::Personnel_Cloaking;
//		getOpening = vis(Terran_Covert_Ops) <= 0;
//		bioBuild = true;
//		scout = vis(Terran_Barracks) > 0;
//
//		itemQueue[Terran_Supply_Depot] =		Item(s >= 20);
//		itemQueue[Terran_Barracks] =			Item((s >= 18) + (s >= 22) + (s >= 46));
//		itemQueue[Terran_Refinery] =			Item(s >= 40);
//		itemQueue[Terran_Academy] =				Item(s >= 42);
//		itemQueue[Terran_Factory] =				Item(s >= 50);
//		itemQueue[Terran_Starport] =			Item(com(Terran_Factory) > 0);
//		itemQueue[Terran_Science_Facility] =	Item(com(Terran_Starport) > 0);
//		itemQueue[Terran_Covert_Ops] =			Item(com(Terran_Science_Facility) > 0);
//	}
//
//	void BuildOrderManager::TBCMemes()
//	{
//		firstUpgrade = UpgradeTypes::None;
//		firstTech = TechTypes::Tank_Siege_Mode;
//		getOpening = s < 80;
//		fastExpand = true;
//		scout = vis(Terran_Barracks) > 0;
//
//		techUnit = Terran_Battlecruiser;
//		techList.insert(Terran_Battlecruiser);
//		unlockedType.insert(Terran_Battlecruiser);
//
//		if (BWEB::Walls::getWall(BWEB::Map::getNaturalArea()))
//			wallNat = true;
//
//		itemQueue[Terran_Supply_Depot] =		Item((s >= 16) + (s >= 32), (s >= 18) + (s >= 32));
//		itemQueue[Terran_Bunker] =				Item(s >= 26);
//		itemQueue[Terran_Barracks] =			Item(com(Terran_Supply_Depot) > 0);
//		itemQueue[Terran_Refinery] =			Item(s >= 28);
//		itemQueue[Terran_Factory] =				Item(s >= 32);
//		itemQueue[Terran_Command_Center] =		Item(1 + (s >= 70));
//	}
//}