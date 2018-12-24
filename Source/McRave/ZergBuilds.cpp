//#include "McRave.h"
//#define s Units().getSupply()
//using namespace UnitTypes;
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
//	static bool lingSpeed() {
//		return Broodwar->self()->isUpgrading(UpgradeTypes::Metabolic_Boost) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost);
//	}
//
//	static string enemyBuild = Strategy().getEnemyBuild();
//
//	void BuildOrderManager::Z2HatchMuta()
//	{
//		getOpening		= s < 70;
//		fastExpand		= true;
//		gasLimit		= INT_MAX;
//		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
//		firstTech		= TechTypes::None;
//		scout			= s >= 22;
//
//		lingLimit = vis(Zerg_Lair) == 1 ? 0 : vis(Zerg_Spawning_Pool) * 6;
//		droneLimit = 14;
//
//		if (Terrain().isIslandMap()) {
//			firstUpgrade = UpgradeTypes::Zerg_Flyer_Attacks;
//			itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
//			itemQueue[Zerg_Extractor]				= Item((vis(Zerg_Hatchery) >= 3 && (s >= 30)));
//			itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 26));
//		}
//		else if (Players().vP()) {
//			itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (com(Zerg_Lair) >= 1) + (com(Zerg_Hatchery) >= 3));
//			itemQueue[Zerg_Spawning_Pool]			= Item((vis(Zerg_Hatchery) >= 2 && s >= 22));
//			itemQueue[Zerg_Extractor]				= Item((vis(Zerg_Spawning_Pool) >= 1 && s >= 24) + (vis(Zerg_Lair)));
//			itemQueue[Zerg_Lair]					= Item(Broodwar->self()->gas() > 90);
//		}
//		else if (Players().vT()) {
//			firstUpgrade	= vis(Zerg_Lair) > 0 ? UpgradeTypes::Metabolic_Boost : UpgradeTypes::None;
//
//			itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (com(Zerg_Lair) >= 1) + (com(Zerg_Hatchery) >= 3));
//			itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
//			itemQueue[Zerg_Extractor]				= Item((vis(Zerg_Spawning_Pool) >= 1) + (vis(Zerg_Spire) > 0));
//			itemQueue[Zerg_Lair]					= Item(Broodwar->self()->gas() > 90);
//			itemQueue[Zerg_Creep_Colony]			= Item(vis(Zerg_Extractor) > 0);
//		}
//
//		if (com(Zerg_Lair) > 0 && techList.find(Zerg_Mutalisk) == techList.end())
//			techUnit = Zerg_Mutalisk;
//
//		int sunken = Units().getEnemyCount(UnitTypes::Protoss_Zealot) / 4;
//		itemQueue[Zerg_Creep_Colony] = Item(sunken);
//	}
//
//	void BuildOrderManager::Z3HatchLing()
//	{
//		getOpening		= s < 40;
//		fastExpand		= true;
//		wallNat			= vis(Zerg_Spawning_Pool) > 0;
//		gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
//		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
//		firstTech		= TechTypes::None;
//		scout			= vis(Zerg_Spawning_Pool) > 0;
//		rush			= vis(Zerg_Spawning_Pool) > 0;
//
//		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 26));
//		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
//		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Hatchery) >= 3);
//	}
//
//	void BuildOrderManager::Z4Pool()
//	{
//		getOpening		= s < 30;
//		fastExpand		= false;
//		wallNat			= false;
//		gasLimit		= 0;
//		firstUpgrade	= UpgradeTypes::None;
//		firstTech		= TechTypes::None;
//		scout			= false;
//		rush			= true;
//		droneLimit		= 4;
//
//		itemQueue[Zerg_Spawning_Pool]			= Item(Broodwar->self()->minerals() > 176, s >= 8);
//	}
//
//	void BuildOrderManager::Z9Pool()
//	{
//		getOpening		= s < 26;
//		fastExpand		= true;
//		wallNat			= vis(Zerg_Spawning_Pool) > 0;
//		gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
//		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
//		firstTech		= TechTypes::None;
//		scout			= vis(Zerg_Spawning_Pool) > 0;
//		rush			= vis(Zerg_Spawning_Pool) > 0;
//
//		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 18));
//		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
//		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
//	}
//
//	void BuildOrderManager::Z2HatchHydra()
//	{
//		getOpening		= s < 40;
//		fastExpand		= true;
//		wallNat			= vis(Zerg_Spawning_Pool) > 0;
//		gasLimit		= 3;
//		firstUpgrade	= UpgradeTypes::Grooved_Spines;
//		firstTech		= TechTypes::None;
//		scout			= vis(Zerg_Spawning_Pool) > 0;
//
//		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 18));
//		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
//		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
//		itemQueue[Zerg_Hydralisk_Den]			= Item(Broodwar->self()->gas() > 40);
//
//		if (vis(Zerg_Hydralisk_Den) > 0)
//			techUnit = Zerg_Hydralisk;
//	}
//
//	void BuildOrderManager::Z3HatchBeforePool()
//	{
//		getOpening		= s < 40;
//		fastExpand		= true;
//		wallNat			= vis(Zerg_Spawning_Pool) > 0;
//		gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
//		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
//		firstTech		= TechTypes::None;
//		scout			= vis(Zerg_Spawning_Pool) > 0;
//
//		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 28));
//		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 3);
//		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
//	}
//
//	void BuildOrderManager::ZLurkerTurtle()
//	{
//		getOpening		= s < 60;
//		fastExpand		= true;
//		wallNat			= vis(Zerg_Spawning_Pool) > 0;
//		gasLimit		= 3;
//		firstUpgrade	= UpgradeTypes::None;
//		firstTech		= TechTypes::Lurker_Aspect;
//		scout			= vis(Zerg_Spawning_Pool) > 0;
//		playPassive		= true;
//
//		if (vis(Zerg_Hydralisk_Den) > 0)
//			techUnit = Zerg_Lurker;
//
//		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24));
//		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
//		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
//		itemQueue[Zerg_Hydralisk_Den]			= Item(Broodwar->self()->gas() > 40);
//		itemQueue[Zerg_Lair]					= Item(vis(Zerg_Hydralisk_Den) >= 1);
//		itemQueue[Zerg_Creep_Colony]			= Item(3 * (com(Zerg_Hatchery) >= 2));
//	}
//
//	void BuildOrderManager::Z9PoolSpire()
//	{
//		getOpening		= s < 70;
//		gasLimit		= INT_MAX;
//		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
//		firstTech		= TechTypes::None;
//		scout			= false;
//		droneLimit		= 10;
//		lingLimit		= 12;
//
//		auto gas100 = Broodwar->self()->gas() >= 100;
//
//		itemQueue[Zerg_Spawning_Pool]			= Item(s >= 18);
//		itemQueue[Zerg_Extractor]				= Item((s >= 18 && vis(Zerg_Spawning_Pool) > 0));
//		itemQueue[Zerg_Lair]					= Item(gas100 && vis(Zerg_Spawning_Pool) > 0);
//		itemQueue[Zerg_Spire]					= Item(lingSpeed() && gas100);
//		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 56));
//
//		if (com(Zerg_Lair) > 0)
//			techUnit = Zerg_Mutalisk;
//	}
//}