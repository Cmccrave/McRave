#include "McRave.h"
#define s Units().getSupply()
#define vis Broodwar->self()->visibleUnitCount
#define com Broodwar->self()->completedUnitCount
using namespace UnitTypes;

#ifdef MCRAVE_ZERG
void BuildOrderManager::Z2HatchMuta()
{
	getOpening		= s < 40;
	fastExpand		= true;
	wallNat			= true;
	gasLimit		= 3;
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
	wallNat			= true;
	gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
	firstUpgrade	= UpgradeTypes::Metabolic_Boost;
	firstTech		= TechTypes::None;
	scout			= vis(Zerg_Spawning_Pool) > 0;

	itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 24) + (s >= 26));
	itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
	itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Hatchery) >= 3);
}

void BuildOrderManager::Z4Pool()
{
	getOpening		= s < 40;
	fastExpand		= false;
	wallNat			= false;
	gasLimit		= 0;
	firstUpgrade	= UpgradeTypes::None;
	firstTech		= TechTypes::None;
	scout			= vis(Zerg_Spawning_Pool) > 0;

	itemQueue[Zerg_Spawning_Pool]			= Item(s >= 8);
}

void BuildOrderManager::Z9Pool()
{
	getOpening		= s < 40;
	fastExpand		= true;
	wallNat			= true;
	gasLimit		= Broodwar->self()->isUpgrading(firstUpgrade) ? 0 : 3;
	firstUpgrade	= UpgradeTypes::Metabolic_Boost;
	firstTech		= TechTypes::None;
	scout			= vis(Zerg_Spawning_Pool) > 0;

	itemQueue[Zerg_Hatchery]				= Item(1 + (s >= 18));
	itemQueue[Zerg_Spawning_Pool]			= Item(vis(Zerg_Hatchery) >= 2);
	itemQueue[Zerg_Extractor]				= Item(vis(Zerg_Spawning_Pool) >= 1);
}

void BuildOrderManager::Z2HatchHydra()
{
	getOpening		= s < 50;
	fastExpand		= true;
	wallNat			= true;
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
#endif // MCRAVE_ZERG
