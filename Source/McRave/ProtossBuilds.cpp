#include "McRave.h"
#define s Units().getSupply()
#define vis Broodwar->self()->visibleUnitCount
#define com Broodwar->self()->completedUnitCount
#define enemyBuild Strategy().getEnemyBuild()
using namespace UnitTypes;

#ifdef MCRAVE_PROTOSS

namespace McRave
{
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
	}

	void BuildOrderManager::P4Gate()
	{
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 120;
		scout =				vis(Protoss_Gateway) > 0;
		gasLimit =			INT_MAX;

		if (enemyBuild == "Z5Pool" || enemyBuild == "Z9Pool") {
			currentVariant =		"Defensive";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
			itemQueue[Protoss_Shield_Battery] =		Item(s >= 70);
			itemQueue[Protoss_Assimilator] =		Item(s >= 64);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);

			if (s < 60)
				gasLimit = 0;
			else
				gasLimit = 2;
		}

		else if (Broodwar->enemy()->getRace() == Races::Zerg) {
			currentVariant =		"Default";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
			itemQueue[Protoss_Assimilator] =		Item(s >= 44);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
			gasLimit = 2;
		}
		else {
			currentVariant =		"Default";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (3 * (s >= 62)));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
	}

	void BuildOrderManager::PFFE()
	{
		fastExpand =		true;
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::Protoss_Ground_Weapons;
		firstTech =			TechTypes::None;
		getOpening =		s < 80;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		// Unknown information, expect the worst
		if ((enemyBuild == "Unknown" && !Terrain().getEnemyStartingPosition().isValid()) || enemyBuild == "Z9Pool") {
			currentVariant =	"Semi-Panic";

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit =			Protoss_Corsair;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item(s >= 32);
			itemQueue[Protoss_Forge] =				Item(s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Photon_Cannon) > 0) + (s >= 24) + (s >= 28));
			itemQueue[Protoss_Assimilator] =		Item((s >= 50) + (s >= 76));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 54);
		}

		// 5 pool, cut probes and get Cannons
		else if (enemyBuild == "Z5Pool") {
			gasLimit =			INT_MAX;			
			currentVariant =	"Panic";

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit =			Protoss_Corsair;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 32) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 18);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Forge) > 0 && Broodwar->self()->minerals() > 100) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 26));
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 68));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 42);
		}

		// 3H ling, expand early and get 3 Cannons
		else if (enemyBuild == "Z3HatchLing") {
			currentVariant =	"5GateGoon";

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit =			Protoss_Corsair;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 32) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Photon_Cannon) > 0) + (s >= 24) + (s >= 26));
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 68));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 42);
		}

		// Unknown information, +1 Speedlot
		else if (enemyBuild == "Z5Hatch" || (enemyBuild  == "Unknown" && Terrain().getEnemyStartingPosition().isValid() && Strategy().enemyFastExpand())) {
			getOpening =		s < 130;
			currentVariant =	"+1Speedlot";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 32) + (s >= 46) + (s >= 56));
			itemQueue[Protoss_Forge] =				Item(s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item(vis(UnitTypes::Protoss_Nexus) > 1);
			itemQueue[Protoss_Assimilator] =		Item((s >= 32) + (s >= 74));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 42);
			itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 54);
			itemQueue[Protoss_Templar_Archives] =	Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));

		}

		// Hydra bust, get High Templars and Cannon defensively
		else if (enemyBuild == "Z2HatchHydra" || enemyBuild == "Z3HatchHydra") {
			currentVariant =	"StormRush";
			firstUpgrade =		UpgradeTypes::None;
			firstTech =			TechTypes::Psionic_Storm;

			if (techList.find(Protoss_High_Templar) == techList.end())
				techUnit =			Protoss_High_Templar;

			// Force to build it right now
			techList.insert(Protoss_High_Templar);
			unlockedType.insert(Protoss_High_Templar);

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 26) + (s >= 42));
			itemQueue[Protoss_Forge] =				Item((s >= 28));
			itemQueue[Protoss_Photon_Cannon] =		Item((s >= 30) + (3 * (s > 44)));
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 68));
			itemQueue[Protoss_Cybernetics_Core] =	Item((s >= 42));
		}

		// Muta bust, get Corsairs and Cannon defensively
		else if (enemyBuild == "Z2HatchMuta" || enemyBuild == "Z3HatchMuta") {
			currentVariant =	"DoubleStargate";
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			firstTech =			TechTypes::None;

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit =			Protoss_Corsair;

			// Force to build it right now
			techList.insert(Protoss_Corsair);
			unlockedType.insert(Protoss_Corsair);

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 26));
			itemQueue[Protoss_Forge] =				Item((s >= 28));
			itemQueue[Protoss_Photon_Cannon] =		Item(4 * (vis(Protoss_Stargate) > 0) + (s >= 74));
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 60));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
			itemQueue[Protoss_Stargate] =			Item((vis(Protoss_Corsair) > 0) + (vis(Protoss_Cybernetics_Core) > 0));
		}

		// 
		//else {
		//	currentVariant =	"NeoBisu";
		//	if (techList.find(Protoss_Corsair) == techList.end())
		//		techUnit = UnitTypes::Protoss_Corsair;

		//	itemQueue[Protoss_Nexus] =				Item(1 + (s >= 30));
		//	itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
		//	itemQueue[Protoss_Gateway] =			Item(s >= 32);
		//	itemQueue[Protoss_Forge] =				Item(s >= 20);
		//	itemQueue[Protoss_Photon_Cannon] =		Item(s >= 22, com(UnitTypes::Protoss_Forge));
		//	itemQueue[Protoss_Assimilator] =		Item((vis(UnitTypes::Protoss_Gateway) > 0) + (com(UnitTypes::Protoss_Cybernetics_Core) > 0));
		//	itemQueue[Protoss_Cybernetics_Core] =	Item(com(UnitTypes::Protoss_Gateway) > 0);
		//	itemQueue[Protoss_Stargate] =			Item(com(UnitTypes::Protoss_Cybernetics_Core) > 0);
		//	itemQueue[Protoss_Citadel_of_Adun] =	Item(com(UnitTypes::Protoss_Stargate) > 0);
		//	itemQueue[Protoss_Templar_Archives] =	Item(com(UnitTypes::Protoss_Citadel_of_Adun) > 0);
		//}

		else
		{
			getOpening =		s < 100;
			currentVariant =	"+1Speedlot";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Nexus) >= 2) + (s >= 46) + (s >= 56));
			itemQueue[Protoss_Forge] =				Item(s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Nexus) >= 2) + (Units().getEnemyCount(Zerg_Zergling) >= 6));
			itemQueue[Protoss_Assimilator] =		Item((s >= 32) + (s >= 74));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 42);
			itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 54);
			itemQueue[Protoss_Templar_Archives] =	Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));
		}
	}

	void BuildOrderManager::P12Nexus()
	{
		fastExpand =		true;
		wallNat =			true;
		playPassive =		true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 120;
		scout =				vis(Protoss_Cybernetics_Core) > 0;
		gasLimit =			2 + (s >= 60);

		if (Terrain().isIslandMap()) {
			firstUpgrade =		UpgradeTypes::Gravitic_Drive;
			getOpening =		s < 50;
			currentVariant =	"Island";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Nexus) > 1));
			itemQueue[Protoss_Assimilator] =		Item(s >= 26);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
		}
		else if (Strategy().enemyFastExpand()) {
			currentVariant =	"DoubleExpand";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24) + (com(Protoss_Cybernetics_Core) > 0));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Cybernetics_Core) > 0) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
			itemQueue[Protoss_Assimilator] =		Item(s >= 28);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
		}
		else {
			getOpening =		s < 120;
			currentVariant =	"Default";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Cybernetics_Core) > 0) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
			itemQueue[Protoss_Assimilator] =		Item((s >= 30) + (s >= 90));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
		}
	}

	void BuildOrderManager::P21Nexus()
	{
		fastExpand =		true;
		wallNat =			true;
		playPassive =		true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			2 + (s >= 60);

		if (Strategy().enemyFastExpand() || enemyBuild == "TSiegeExpand") {
			getOpening =		s < 100;
			currentVariant =	"DoubleExpand";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42) + (s >= 70));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 44) + (s >= 72));
			itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (s >= 76));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			itemQueue[Protoss_Robotics_Facility] =	Item(s >= 80);
		}
		else if (Strategy().enemyRush()) {
			gasLimit =			0;
			fastExpand =		false;
			getOpening =		s < 70;
			currentVariant =	"Defensive";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 72));
			itemQueue[Protoss_Assimilator] =		Item((s >= 40));
			itemQueue[Protoss_Shield_Battery] =		Item(s >= 48);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
		}
		else {
			currentVariant =	"Default";
			getOpening =		s < 70;
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (vis(Protoss_Nexus) >= 2) + (s >= 76));
			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			itemQueue[Protoss_Robotics_Facility] =	Item(s >= 56);
		}
	}

	void BuildOrderManager::PDTExpand()
	{
		firstUpgrade =		UpgradeTypes::Khaydarin_Core;
		firstTech =			TechTypes::None;
		getOpening =		s < 60;
		scout =				vis(Protoss_Gateway) > 0;		
		gasLimit =			INT_MAX;
		hideTech =			true;

		if (techList.find(Protoss_Dark_Templar) == techList.end())
			techUnit =			Protoss_Dark_Templar;

		// Force to build it right now		
		unlockedType.insert(Protoss_Dark_Templar);
		techList.insert(Protoss_Dark_Templar);

		if ((enemyBuild == "T3Fact" || (Units().getEnemyCount(Terran_Vulture) >= 2 && !Strategy().enemyFastExpand())) && com(Protoss_Dark_Templar) > 0) {
			techUnit = Protoss_Scout;
			unlockedType.insert(Protoss_Scout);
			techList.insert(Protoss_Scout);
		}
		else {
			unlockedType.insert(Protoss_Arbiter);
			techList.insert(Protoss_Arbiter);
		}

		if (vis(Protoss_Scout) >= 2) {
			techList.erase(Protoss_Scout);
			unlockedType.erase(Protoss_Scout);
			techList.insert(Protoss_Arbiter);
			unlockedType.insert(Protoss_Arbiter);
		}

		if (Strategy().enemyRush()) {
			getOpening =		s < 80;
			currentVariant =	"Defensive";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 72));
			itemQueue[Protoss_Assimilator] =		Item((s >= 30));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
		}
		else if (enemyBuild == "T3Fact" || (Units().getEnemyCount(Terran_Vulture) >= 2 && !Strategy().enemyFastExpand())) {
			getOpening =		s < 60;
			currentVariant =	"Stove";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 48));
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
		else {
			getOpening =		s < 60;
			currentVariant =	"Default";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 48));
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30));
			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 28);
			itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 36);
			itemQueue[Protoss_Templar_Archives] =	Item(s >= 48);
		}
	}

	void BuildOrderManager::P2GateDragoon()
	{
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Cybernetics_Core) > 0;
		getOpening =		s < 100;
		gasLimit =			INT_MAX;

		if (Strategy().enemyFastExpand() || enemyBuild == "TSiegeExpand") {
			getOpening =		s < 70;
			currentVariant =	"DT";

			if (techList.find(Protoss_Dark_Templar) == techList.end())
				techUnit = UnitTypes::Protoss_Dark_Templar;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30));
			itemQueue[Protoss_Assimilator] =		Item(s >= 22);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);			
		}
		else if (enemyBuild == "TBBS") {
			gasLimit =			0;
			fastExpand =		false;
			getOpening =		s < 70;
			currentVariant =	"Defensive";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 72));
			itemQueue[Protoss_Assimilator] =		Item((s >= 40));
			itemQueue[Protoss_Shield_Battery] =		Item(s >= 48);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
		}
		else {
			getOpening =		s < 100;
			currentVariant =	"Expansion";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50));
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 28));
			itemQueue[Protoss_Assimilator] =		Item(s >= 22);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);

		}
	}

	void BuildOrderManager::PNZCore()
	{
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 60;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		if (Strategy().enemyRush())
			gasLimit = 1 + BuildOrder().firstReady();

		if (enemyBuild == "P1GateRobo" || enemyBuild == "P2GateExpand") {
			getOpening =		s < 120;
			gasLimit =			2;
			currentVariant =	"4Gate";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (3 * (s >= 58)));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
		else {
			getOpening = s < 60;
			currentVariant =	"Default";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 36));
			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
		}
	}

	void BuildOrderManager::PZCore()
	{
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;
		
		if (Strategy().enemyRush())
			gasLimit = 1 + BuildOrder().firstReady();

		if (enemyBuild == "P1GateRobo" || enemyBuild == "P2GateExpand") {
			getOpening =		s < 120;
			gasLimit =			2;
			currentVariant =	"4Gate";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (3 * (s >= 58)));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
		else {
			getOpening =		s < 60;
			currentVariant =	"Default";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 38));
			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
	}

	void BuildOrderManager::PZZCore()
	{
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		if (Strategy().enemyRush())
			gasLimit = 1 + BuildOrder().firstReady();

		if (enemyBuild == "P1GateRobo" || enemyBuild == "P2GateExpand") {
			getOpening =		s < 120;
			gasLimit =			2;
			currentVariant =	"4Gate";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item(s >= 16);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (3 * (s >= 58)));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
		else {
			getOpening =		s < 60;
			currentVariant =	"Default";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 42));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
		}
	}

	void BuildOrderManager::PProxy6()
	{
		proxy =				true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 30;
		scout =				vis(Protoss_Gateway) >= 1;
		currentVariant =	"Default";

		itemQueue[Protoss_Nexus] =					Item(1);
		itemQueue[Protoss_Pylon] =					Item((s >= 10), (s >= 12));
		itemQueue[Protoss_Gateway] =				Item(vis(Protoss_Pylon) > 0);
	}

	void BuildOrderManager::PProxy99()
	{
		proxy =				true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 50;
		scout =				vis(Protoss_Gateway) >= 2;
		gasLimit =			INT_MAX;
		currentVariant =	"Default";

		itemQueue[Protoss_Pylon] =					Item((s >= 12), (s >= 16));
		itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
	}

	void BuildOrderManager::P2GateExpand()
	{
		wallNat =			true;
		fastExpand =		true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 80;
		gasLimit =			INT_MAX;

		// 9/10 Gates on 3Player+ maps
		if (Broodwar->getStartLocations().size() >= 3) {
			scout =									vis(Protoss_Gateway) >= 1;
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0) + (s >= 20), (s >= 18) + (s >= 20));
		}
		// 9/9 Gates
		else {
			scout =									vis(Protoss_Gateway) >= 2;
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
		}
				
		if (Units().getEnemyCount(UnitTypes::Zerg_Sunken_Colony) >= 2) {
			currentVariant =	"Expansion";

			itemQueue[Protoss_Assimilator] =		Item(s >= 76);
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Forge] =				Item(s >= 62);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
			itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
		}
		else if (enemyBuild == "Z9Pool") {
			currentVariant =	"Defensive";

			itemQueue[Protoss_Forge] =				Item(1);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
			itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
		}
		else if (enemyBuild == "Z5Pool") {
			getOpening =		s < 120;
			currentVariant =	"4Gate";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
			itemQueue[Protoss_Assimilator] =		Item(s >= 64);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
			gasLimit = 1;
		}
		else {
			currentVariant =	"Default";

			itemQueue[Protoss_Assimilator] =		Item(s >= 58);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 62);
			itemQueue[Protoss_Forge] =				Item(s >= 70);
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
			itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
		}
	}

	void BuildOrderManager::P1GateRobo()
	{
		fastExpand =		true;
		wallNat =			true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;
		getOpening =		(vis(Protoss_Reaver) == 0 && vis(Protoss_Observer) == 0);
		currentVariant =	"Default";

		itemQueue[Protoss_Nexus] =					Item(1 + (s >= 66));
		itemQueue[Protoss_Pylon] =					Item((s >= 16) + (s >= 30));
		itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
		itemQueue[Protoss_Assimilator] =			Item(s >= 28);
		itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 40);
		itemQueue[Protoss_Robotics_Facility] =		Item(s >= 58);

		if (Strategy().needDetection())
			itemQueue[Protoss_Observatory] =		Item(com(Protoss_Robotics_Facility) > 0);
		else
			itemQueue[Protoss_Robotics_Support_Bay] = Item(com(Protoss_Robotics_Facility) > 0);

		if (vis(Protoss_Observatory) > 0) {			
			techList.insert(Protoss_Observer);
			unlockedType.insert(Protoss_Observer);
		}
		if (vis(Protoss_Robotics_Support_Bay) > 0) {
			techList.insert(Protoss_Reaver);
			unlockedType.insert(Protoss_Reaver);
		}
	}

	void BuildOrderManager::P3Nexus()
	{
		fastExpand =		true;
		wallNat =			true;
		playPassive =		true;
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 120;
		scout =				vis(Protoss_Cybernetics_Core) > 0;
		gasLimit =			2 + (s >= 60);
		currentVariant =	"Default";

		itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24) + (s >= 30));
		itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
		itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Cybernetics_Core) > 0) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
		itemQueue[Protoss_Assimilator] =		Item(s >= 28);
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
	}
}
#endif // MCRAVE_PROTOSS