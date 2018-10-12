#include "McRave.h"
using namespace UnitTypes;
#define s Units().getSupply()

namespace McRave {
	static int vis(UnitType t) {
		return Broodwar->self()->visibleUnitCount(t);
	}
	static int com(UnitType t) {
		return Broodwar->self()->completedUnitCount(t);
	}

	static string enemyBuild = Strategy().getEnemyBuild();

	void BuildOrderManager::PScoutMemes()
	{
		getOpening =		s < 120;
		fastExpand =		true;
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		if (techList.find(Protoss_Scout) == techList.end())
			techUnit =			Protoss_Scout;

		itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
		itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
		itemQueue[Protoss_Gateway] =			Item(s >= 28);
		itemQueue[Protoss_Forge] =				Item(s >= 20);
		itemQueue[Protoss_Photon_Cannon] =		Item((s >= 24) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 60) + (s >= 64));
		itemQueue[Protoss_Assimilator] =		Item((s >= 32) + (s >= 66));
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
		itemQueue[Protoss_Stargate] =			Item((s >= 60) + (s >= 80));
		itemQueue[Protoss_Fleet_Beacon] =		Item((vis(Protoss_Scout) > 0));

		itemQueue[UnitTypes::Protoss_Shield_Battery] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus));
	}

	void BuildOrderManager::PDWEBMemes()
	{
		fastExpand =		true;
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::None;
		firstTech =			TechTypes::Disruption_Web;
		getOpening =		s < 120;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		if (techList.find(Protoss_Corsair) == techList.end())
			techUnit =			Protoss_Corsair;

		itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
		itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
		itemQueue[Protoss_Gateway] =			Item(s >= 28);
		itemQueue[Protoss_Forge] =				Item(s >= 20);
		itemQueue[Protoss_Photon_Cannon] =		Item((s >= 24) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 60) + (s >= 64));
		itemQueue[Protoss_Assimilator] =		Item((s >= 32) + (s >= 66));
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
		itemQueue[Protoss_Stargate] =			Item((s >= 60) + (s >= 80));
		itemQueue[Protoss_Fleet_Beacon] =		Item((vis(Protoss_Corsair) > 0));

		itemQueue[UnitTypes::Protoss_Shield_Battery] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus));
	}

	void BuildOrderManager::PArbiterMemes()
	{
		fastExpand =		true;
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::Khaydarin_Core;
		firstTech =			TechTypes::None;
		getOpening =		s < 80;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		if (techList.find(Protoss_Arbiter) == techList.end())
			techUnit =			Protoss_Arbiter;

		itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
		itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
		itemQueue[Protoss_Gateway] =			Item(s >= 28);
		itemQueue[Protoss_Forge] =				Item(s >= 20);
		itemQueue[Protoss_Photon_Cannon] =		Item((s >= 24) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 60) + (s >= 64));
		itemQueue[Protoss_Assimilator] =		Item((s >= 26) + (vis(Protoss_Nexus) >= 2));
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
		itemQueue[Protoss_Stargate] =			Item(2 * (com(Protoss_Cybernetics_Core)));

		itemQueue[UnitTypes::Protoss_Shield_Battery] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus));
	}

	void BuildOrderManager::PShuttleMemes()
	{
		getOpening =		s < 120;
		fastExpand =		true;
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::Gravitic_Drive;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		if (techList.find(Protoss_Reaver) == techList.end())
			techUnit =			Protoss_Reaver;

		itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
		itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
		itemQueue[Protoss_Gateway] =			Item(s >= 28);
		itemQueue[Protoss_Forge] =				Item(s >= 20);
		itemQueue[Protoss_Photon_Cannon] =		Item((s >= 24) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 60) + (s >= 64));
		itemQueue[Protoss_Assimilator] =		Item((s >= 32) + (s >= 66));
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
		itemQueue[Protoss_Robotics_Facility] =	Item((s >= 60) + (s >= 80));
	}
}
