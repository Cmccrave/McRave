#include "McRave.h"
#ifdef MCRAVE_TERRAN

void BuildOrderManager::T2Fact()
{
	int s = Units().getSupply();
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Tank_Siege_Mode;
	getOpening = s < 36;	
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;
	wallMain = true;

	itemQueue[UnitTypes::Terran_Supply_Depot] =			Item(s >= 18);
	itemQueue[UnitTypes::Terran_Barracks] =				Item(s >= 20);
	itemQueue[UnitTypes::Terran_Refinery] =				Item(s >= 24);
	itemQueue[UnitTypes::Terran_Factory] =				Item((s >= 30) + (s >= 36));
}

void BuildOrderManager::TSparks()
{
	int s = Units().getSupply();
	firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
	firstTech = TechTypes::Stim_Packs;
	getOpening = s < 60;
	bioBuild = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;
	gasLimit = 3 - (2 * int(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Engineering_Bay) > 0));
	
	itemQueue[UnitTypes::Terran_Supply_Depot] =			Item(s >= 18);
	itemQueue[UnitTypes::Terran_Barracks] =				Item((Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Supply_Depot) > 0) + (s >= 24) + (s >= 42));
	itemQueue[UnitTypes::Terran_Refinery] =				Item(s >= 30);
	itemQueue[UnitTypes::Terran_Academy] =				Item(s >= 48);
	itemQueue[UnitTypes::Terran_Engineering_Bay] =		Item(s >= 36);
	itemQueue[UnitTypes::Terran_Comsat_Station] =		Item(Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Academy) > 0);
}

void BuildOrderManager::T2PortWraith()
{
	int s = Units().getSupply();
	
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Cloaking_Field;
	getOpening = s < 80;
	fastExpand = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;

	if (mapBWEB.getWall(mapBWEB.getNaturalArea()))
		wallNat = true;
	else if (mapBWEB.getWall(mapBWEB.getMainArea()))
		wallMain = true;

	itemQueue[UnitTypes::Terran_Supply_Depot] =			Item((s >= 16) + (s >= 26), (s >= 18) + (s >= 26));
	itemQueue[UnitTypes::Terran_Barracks] =				Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Supply_Depot) > 0);
	itemQueue[UnitTypes::Terran_Refinery] =				Item(s >= 24);
	itemQueue[UnitTypes::Terran_Factory] =				Item(s >= 32);
	itemQueue[UnitTypes::Terran_Starport] =				Item(2 * (s >= 44));
	itemQueue[UnitTypes::Terran_Command_Center] =		Item(1 + (s >= 70));
}

void BuildOrderManager::T1RaxFE()
{
	int s = Units().getSupply();
	bioBuild = true;
	fastExpand = true;
	firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
	firstTech = TechTypes::Stim_Packs;
	getOpening = s < 86;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;
	gasLimit = 1 + (Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center) > 1);

	if (mapBWEB.getWall(mapBWEB.getNaturalArea()))
		wallNat = true;
	else if (mapBWEB.getWall(mapBWEB.getMainArea()))
		wallMain = true;

	itemQueue[UnitTypes::Terran_Supply_Depot] =			Item((s >= 16) + (s >= 26), (s >= 18) + (s >= 26));
	itemQueue[UnitTypes::Terran_Barracks] =				Item((Broodwar->self()->completedUnitCount(UnitTypes::Terran_Supply_Depot) > 0) + (s >= 64) + (s >= 70) + (2 * (s >= 80)));
	itemQueue[UnitTypes::Terran_Command_Center] =		Item(1 + (s >= 36));
	itemQueue[UnitTypes::Terran_Refinery] =				Item(s >= 46);
	itemQueue[UnitTypes::Terran_Engineering_Bay] =		Item(s >= 50);
	itemQueue[UnitTypes::Terran_Academy] =				Item(s >= 60);
	itemQueue[UnitTypes::Terran_Comsat_Station] =		Item(s >= 80);
}

void BuildOrderManager::T2RaxFE()
{
	int s = Units().getSupply();
	bioBuild = true;
	fastExpand = true;
	firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
	firstTech = TechTypes::Stim_Packs;
	getOpening = s < 86;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;
	gasLimit = 1 + (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) > 1);
	wallNat = true;

	itemQueue[UnitTypes::Terran_Supply_Depot] = Item((s >= 16) + (Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Bunker) > 0), (s >= 18) + (Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Bunker) > 0));
	itemQueue[UnitTypes::Terran_Bunker] = Item((Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) > 0));
	itemQueue[UnitTypes::Terran_Barracks] = Item((Broodwar->self()->completedUnitCount(UnitTypes::Terran_Supply_Depot) > 0) + (s >= 26) + (s >= 70) + (2 * (s >= 80)));	
	itemQueue[UnitTypes::Terran_Refinery] = Item(s >= 50);
	itemQueue[UnitTypes::Terran_Engineering_Bay] = Item(s >= 44);
	itemQueue[UnitTypes::Terran_Academy] = Item(s >= 60);
	itemQueue[UnitTypes::Terran_Comsat_Station] = Item(s >= 70);
	itemQueue[UnitTypes::Terran_Command_Center] = Item(1 + (s >= 80));
}

void BuildOrderManager::T1FactFE()
{
	int s = Units().getSupply();
	fastExpand = true;
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Tank_Siege_Mode;
	getOpening = s < 80;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;	
	wallNat = true;

	itemQueue[UnitTypes::Terran_Supply_Depot] = Item((s >= 16) + (s >= 34), (s >= 18) + (s >= 34));
	itemQueue[UnitTypes::Terran_Bunker] = Item(s >= 30);
	itemQueue[UnitTypes::Terran_Barracks] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Supply_Depot) > 0);
	itemQueue[UnitTypes::Terran_Refinery] = Item(s >= 28);
	itemQueue[UnitTypes::Terran_Factory] = Item((s >= 32) + (s >= 64));
	itemQueue[UnitTypes::Terran_Machine_Shop] = Item(s >= 40);
	itemQueue[UnitTypes::Terran_Command_Center] = Item(1 + (s >= 56));
}

void BuildOrderManager::TNukeMemes()
{
	int s = Units().getSupply();
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Personnel_Cloaking;
	getOpening = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Covert_Ops) <= 0;
	bioBuild = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;

	itemQueue[UnitTypes::Terran_Supply_Depot] =			Item(s >= 20);
	itemQueue[UnitTypes::Terran_Barracks] =				Item((s >= 18) + (s >= 22) + (s >= 46));
	itemQueue[UnitTypes::Terran_Refinery] =				Item(s >= 40);
	itemQueue[UnitTypes::Terran_Academy] =				Item(s >= 42);
	itemQueue[UnitTypes::Terran_Factory] =				Item(s >= 50);
	itemQueue[UnitTypes::Terran_Starport] =				Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) > 0);
	itemQueue[UnitTypes::Terran_Science_Facility] =		Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Starport) > 0);
	itemQueue[UnitTypes::Terran_Covert_Ops] =			Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Science_Facility) > 0);
}

void BuildOrderManager::TBCMemes()
{
	int s = Units().getSupply();
	firstUpgrade = UpgradeTypes::None;
	firstTech = TechTypes::Tank_Siege_Mode;	
	getOpening = s < 80;
	fastExpand = true;
	scout = Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) > 0;

	techUnit = UnitTypes::Terran_Battlecruiser;
	techList.insert(UnitTypes::Terran_Battlecruiser);
	unlockedType.insert(UnitTypes::Terran_Battlecruiser);

	if (mapBWEB.getWall(mapBWEB.getNaturalArea()))
		wallNat = true;

	itemQueue[UnitTypes::Terran_Supply_Depot] = Item((s >= 16) + (s >= 32), (s >= 18) + (s >= 32));
	itemQueue[UnitTypes::Terran_Bunker] = Item(s >= 26);
	itemQueue[UnitTypes::Terran_Barracks] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Supply_Depot) > 0);
	itemQueue[UnitTypes::Terran_Refinery] = Item(s >= 28);
	itemQueue[UnitTypes::Terran_Factory] = Item(s >= 32);
	//itemQueue[UnitTypes::Terran_Starport] = Item(2 * (s >= 44));
	itemQueue[UnitTypes::Terran_Command_Center] = Item(1 + (s >= 70));
}
#endif // 