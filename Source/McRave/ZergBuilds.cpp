#include "McRave.h"
#define s Units().getSupply()
using namespace UnitTypes;

namespace McRave 
{
	static int vis(UnitType t) {
		return Broodwar->self()->visibleUnitCount(t);
	}
	static int com(UnitType t) {
		return Broodwar->self()->completedUnitCount(t);
	}

	static string enemyBuild = Strategy().getEnemyBuild();

	void BuildOrderManager::Z2HatchMuta()
	{
		getOpening		= s < 40;
		fastExpand		= true;
		wallNat			= vis(Zerg_Spawning_Pool) > 0;
		gasLimit		= INT_MAX;
		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
		firstTech		= TechTypes::None;
		scout			= vis(Zerg_Spawning_Pool) > 0;

		if (Terrain().isIslandMap()) {
			firstUpgrade = UpgradeTypes::Zerg_Flyer_Attacks;
			itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
			itemQueue[Zerg_Extractor]				= Item((vis(Zerg_Hatchery) >= 3 && (s >= 30)));
			itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 26));
		}
		else {
			itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
			itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
			itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24));
			itemQueue[Zerg_Lair]					= Item(Broodwar->self()->gas() > 90);
		}

		if (vis(Zerg_Lair) > 0)
			techUnit = Zerg_Mutalisk;
	}

	void BuildOrderManager::Z3HatchLing()
	{
		getOpening		= s < 40;
		fastExpand		= true;
		wallNat			= vis(Zerg_Spawning_Pool) > 0;
		gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
		firstTech		= TechTypes::None;
		scout			= vis(Zerg_Spawning_Pool) > 0;
		rush			= vis(Zerg_Spawning_Pool) > 0;

		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 26));
		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Hatchery) >= 3);
	}

	void BuildOrderManager::Z4Pool()
	{
		getOpening		= s < 30;
		fastExpand		= false;
		wallNat			= false;
		gasLimit		= 0;
		firstUpgrade	= UpgradeTypes::None;
		firstTech		= TechTypes::None;
		scout			= vis(Zerg_Spawning_Pool) > 0;
		rush			= true;

		itemQueue[Zerg_Spawning_Pool]			= Item(Broodwar->self()->minerals() > 176, s >= 8);
	}

	void BuildOrderManager::Z9Pool()
	{
		getOpening		= s < 26;
		fastExpand		= true;
		wallNat			= vis(Zerg_Spawning_Pool) > 0;
		gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
		firstTech		= TechTypes::None;
		scout			= vis(Zerg_Spawning_Pool) > 0;
		rush			= vis(Zerg_Spawning_Pool) > 0;

		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 18));
		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
	}

	void BuildOrderManager::Z2HatchHydra()
	{
		getOpening		= s < 40;
		fastExpand		= true;
		wallNat			= vis(Zerg_Spawning_Pool) > 0;
		gasLimit		= 3;
		firstUpgrade	= UpgradeTypes::Grooved_Spines;
		firstTech		= TechTypes::None;
		scout			= vis(Zerg_Spawning_Pool) > 0;

		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 18));
		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
		itemQueue[Zerg_Hydralisk_Den]			= Item(Broodwar->self()->gas() > 40);

		if (vis(Zerg_Hydralisk_Den) > 0)
			techUnit = Zerg_Hydralisk;
	}

	void BuildOrderManager::Z3HatchBeforePool()
	{
		getOpening		= s < 40;
		fastExpand		= true;
		wallNat			= vis(Zerg_Spawning_Pool) > 0;
		gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
		firstUpgrade	= UpgradeTypes::Metabolic_Boost;
		firstTech		= TechTypes::None;
		scout			= vis(Zerg_Spawning_Pool) > 0;

		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 28));
		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 3);
		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
	}

	void BuildOrderManager::ZLurkerTurtle()
	{
		getOpening		= s < 60;
		fastExpand		= true;
		wallNat			= vis(Zerg_Spawning_Pool) > 0;
		gasLimit		= 3;
		firstUpgrade	= UpgradeTypes::None;
		firstTech		= TechTypes::Lurker_Aspect;
		scout			= vis(Zerg_Spawning_Pool) > 0;		
		playPassive		= true;

		if (vis(Zerg_Hydralisk_Den) > 0)
			techUnit = Zerg_Lurker;

		itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24));
		itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
		itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
		itemQueue[Zerg_Hydralisk_Den]			= Item(Broodwar->self()->gas() > 40);
		itemQueue[Zerg_Lair]					= Item(vis(Zerg_Hydralisk_Den) >= 1);
		itemQueue[Zerg_Creep_Colony]			= Item(3*(com(Zerg_Hatchery) >= 2));
	}
}