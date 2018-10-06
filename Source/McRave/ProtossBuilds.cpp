#include "McRave.h"
using namespace UnitTypes;
#define s Units().getSupply()

namespace McRave
{
	static int vis(UnitType t) {
		return Broodwar->self()->visibleUnitCount(t);
	}
	static int com(UnitType t) {
		return Broodwar->self()->completedUnitCount(t);
	}
	static string enemyBuild() {
		return Strategy().getEnemyBuild();
	}


	// TODO: When player upgrades are added, make this a variable instead
	static bool goonRange() {
		return Broodwar->self()->isUpgrading(UpgradeTypes::Singularity_Charge) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge);
	}
	static bool addgates() {
		goonRange() && Broodwar->self()->minerals() >= 100;
	}

	void BuildOrderManager::Reaction2GateAggresive() {
		gasLimit =				(s >= 50) + (2 * (s >= 64));
		currentVariant =		"2GateA";
		getOpening =			s < 80;
		rush =					true;
		playPassive =			com(Protoss_Zealot) < 4;

		zealotLimit =			INT_MAX;
		dragoonLimit =			INT_MAX;

		itemQueue[Protoss_Nexus] =				Item(1);
		itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
		itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 20) + (s >= 62) + (s >= 70));
		itemQueue[Protoss_Assimilator] =		Item(s >= 64);
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
	}

	void BuildOrderManager::Reaction2Gate() {
		zealotLimit =		vis(Protoss_Assimilator) > 0 ? INT_MAX : 4;
		gasLimit =			INT_MAX;
		getOpening =		s < 100;
		rush =				com(Protoss_Dragoon) < 2 && vis(Protoss_Assimilator) > 0;
		playPassive =		com(Protoss_Dragoon) >= 2 && com(Protoss_Gateway) < 6 && com(Protoss_Reaver) < 2 && com(Protoss_Dark_Templar) < 2;
		currentVariant =	"3GateGoon";

		bool addGates = goonRange() && Broodwar->self()->minerals() >= 100;

		itemQueue[Protoss_Nexus] =					Item(1);
		itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
		itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 24) + (addGates));
		itemQueue[Protoss_Assimilator] =			Item(s >= 44);
		itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 52);
	}

	void BuildOrderManager::Reaction2GateDefensive() {
		currentVariant =	"2GateD";
		gasLimit =			3 * (com(Protoss_Cybernetics_Core) && s >= 50);
		getOpening =		vis(Protoss_Dark_Templar) == 0;
		playPassive	=		vis(Protoss_Dark_Templar) == 0;

		zealotLimit =		INT_MAX;
		dragoonLimit =		(vis(Protoss_Templar_Archives) > 0 || Players().vT()) ? INT_MAX : 0;

		if (Players().vP()) {
			if (com(Protoss_Cybernetics_Core) > 0 && techList.find(Protoss_Dark_Templar) == techList.end() && s >= 80)
				techUnit = Protoss_Dark_Templar;
		}
		else if (Players().vZ()) {
			if (com(Protoss_Cybernetics_Core) > 0 && techList.find(Protoss_Corsair) == techList.end() && s >= 80)
				techUnit = Protoss_Corsair;
		}

		itemQueue[Protoss_Nexus] =					Item(1);
		itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
		itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 24) + (s >= 66));
		itemQueue[Protoss_Assimilator] =			Item(s >= 40);
		itemQueue[Protoss_Shield_Battery] =			Item(vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
		itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 58);
	}

	void BuildOrderManager::Reaction4Gate() {
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Gateway) > 0;
		zealotLimit =		INT_MAX;
		dragoonLimit =		INT_MAX;
		gasLimit =			INT_MAX;

		if (Players().vZ()) {
			currentVariant =		"4Gate";
			getOpening =			s < 120;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
			itemQueue[Protoss_Assimilator] =		Item(s >= 44);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
		}
		else {
			currentVariant =		"4Gate";
			getOpening =			s < 140;
			zealotLimit	=			2;			

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 54) + (2 * (s >= 62)));
			itemQueue[Protoss_Assimilator] =		Item(s >= 32);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
	}

	void BuildOrderManager::P4Gate()
	{
		if (enemyBuild() == "P2Gate")
			Reaction2GateDefensive();
		else if (Strategy().enemyGasSteal())
			Reaction2Gate();
		else
			Reaction4Gate();
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

		zealotLimit =		INT_MAX;
		dragoonLimit =		0;

		// Unknown information, expect the worst
		if ((enemyBuild() == "Unknown" && !Terrain().getEnemyStartingPosition().isValid()) || enemyBuild() == "Z9Pool") {
			currentVariant =	"Semi-Panic";

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit =			Protoss_Corsair;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item(s >= 32);
			itemQueue[Protoss_Forge] =				Item(s >= 18, s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Forge) > 0) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 28), (s >= 22) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 28));
			itemQueue[Protoss_Assimilator] =		Item((s >= 50) + (s >= 76));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 54);
		}

		// 5 pool, cut probes and get Cannons
		else if (enemyBuild() == "Z5Pool") {
			gasLimit =			INT_MAX;
			currentVariant =	"Panic";

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit =			Protoss_Corsair;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 28) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 18);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Forge) > 0 && Broodwar->self()->minerals() > 100) + (vis(Protoss_Photon_Cannon) > 0) + (s >= 24));
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 68));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 42);
		}

		// 3H ling, expand early and get 3 Cannons
		else if (enemyBuild() == "Z3HatchLing") {
			currentVariant =	"Defensive";

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

		// Hydra bust, get High Templars and Cannon defensively
		else if (enemyBuild() == "Z2HatchHydra" || enemyBuild() == "Z3HatchHydra") {
			currentVariant =	"StormRush";
			firstUpgrade =		UpgradeTypes::None;
			firstTech =			TechTypes::Psionic_Storm;

			if (techList.find(Protoss_High_Templar) == techList.end())
				techUnit =			Protoss_High_Templar;

			if (Units().getEnemyCount(UnitTypes::Zerg_Hydralisk) > 0) {
				if (enemyBuild() == "Z2HatchHydra")
					itemQueue[Protoss_Photon_Cannon] =		Item((s >= 30) + (3 * (s >= 46)));
				else
					itemQueue[Protoss_Photon_Cannon] =		Item((s >= 30) + (3 * (s >= 50)) + (2 * (s >= 54)));
			}

			// Force to build it right now
			techList.insert(Protoss_High_Templar);
			unlockedType.insert(Protoss_High_Templar);

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 26) + (s >= 42));
			itemQueue[Protoss_Forge] =				Item((s >= 28));
			itemQueue[Protoss_Assimilator] =		Item(2);
			itemQueue[Protoss_Cybernetics_Core] =	Item((s >= 42));
		}

		// Muta bust, get Corsairs and Cannon defensively
		else if (enemyBuild() == "Z2HatchMuta" || enemyBuild() == "Z3HatchMuta") {
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
		else {
			getOpening =		s < 100;
			currentVariant =	"NeoBisu";
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			auto twoCannons =	Units().getEnemyCount(Zerg_Zergling) >= 5;
			auto threeCannons = Strategy().getEnemyBuild() == "Z1HatchHydra";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 32));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 34));
			itemQueue[Protoss_Forge] =				Item(s >= 18, s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((s >= 22) + (twoCannons)+(2 * threeCannons), (com(Protoss_Forge) >= 1) + (twoCannons)+(2 * threeCannons));
			itemQueue[Protoss_Assimilator] =		Item((vis(Protoss_Gateway) >= 1) + (vis(Protoss_Stargate) >= 1));
			itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Zealot) >= 1);
			itemQueue[Protoss_Citadel_of_Adun] =	Item(vis(Protoss_Assimilator) >= 2);
			itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) >= 1);
			itemQueue[Protoss_Templar_Archives] =	Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));
		}
	}

	void BuildOrderManager::P12Nexus()
	{
		fastExpand =		true;
		//wallNat =			true;
		playPassive =		!firstReady();
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 120;
		scout =				vis(Protoss_Cybernetics_Core) > 0;
		gasLimit =			2 + (s >= 60);

		zealotLimit =		1;
		dragoonLimit =		INT_MAX;

		if (Broodwar->mapFileName().find("BlueStorm") != string::npos)
			firstUpgrade = UpgradeTypes::Carrier_Capacity;

		if (Terrain().isIslandMap()) {
			firstUpgrade =		UpgradeTypes::Gravitic_Drive;
			getOpening =		s < 50;
			currentVariant =	"Island";
			scout =				0;

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
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 1) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
			itemQueue[Protoss_Assimilator] =		Item((s >= 30) + (s >= 90));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
		}
	}

	void BuildOrderManager::P21Nexus()
	{
		fastExpand =		true;
		wallNat =			true;
		playPassive =		!firstReady();
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			2 + (s >= 60);

		zealotLimit =		0;
		dragoonLimit =		INT_MAX;

		if (Strategy().enemyFastExpand() || enemyBuild() == "TSiegeExpand") {
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
			zealotLimit =		INT_MAX;
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

		zealotLimit =		1 * (vis(Protoss_Cybernetics_Core) > 0);
		dragoonLimit =		INT_MAX;

		if (techList.find(Protoss_Dark_Templar) == techList.end())
			techUnit =			Protoss_Dark_Templar;

		if ((enemyBuild() == "T3Fact" || (Units().getEnemyCount(Terran_Vulture) >= 2 && !Strategy().enemyFastExpand())) && com(Protoss_Dark_Templar) > 0) {
			techUnit = Protoss_Scout;
			unlockedType.insert(Protoss_Scout);
			techList.insert(Protoss_Scout);
		}
		else if (vis(Protoss_Dark_Templar) >= 2) {
			unlockedType.insert(Protoss_Arbiter);
			techList.insert(Protoss_Arbiter);
		}

		if (vis(Protoss_Scout) >= 2) {
			techList.erase(Protoss_Scout);
			unlockedType.erase(Protoss_Scout);
			techList.insert(Protoss_Arbiter);
			unlockedType.insert(Protoss_Arbiter);
		}

		if (Strategy().enemyRush())
			Reaction2GateDefensive();

		else if (enemyBuild() == "T3Fact" || (Units().getEnemyCount(Terran_Vulture) >= 2 && !Strategy().enemyFastExpand())) {
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
			itemQueue[Protoss_Gateway] =			Item((s >= 20));
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

		zealotLimit =		0;
		dragoonLimit =		INT_MAX;

		if (Strategy().enemyFastExpand() || enemyBuild() == "TSiegeExpand") {
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
		else if (enemyBuild() == "TBBS") {
			gasLimit =			0;
			fastExpand =		false;
			Reaction2GateDefensive();
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
		scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;
		playPassive	=		(com(Protoss_Gateway) < 6 || s <= 200) && com(Protoss_Reaver) < 3 && com(Protoss_Dark_Templar) < 2;

		zealotLimit =		0;
		dragoonLimit =		INT_MAX;

		bool addGates = goonRange() && Broodwar->self()->minerals() >= 100;
		bool addGas = Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);

		if (enemyBuild() == "P2Gate" && !Strategy().enemyFastExpand())
			Reaction2GateDefensive();
		else if (enemyBuild() == "P1GateRobo" || enemyBuild() == "P2GateExpand")
			Reaction4Gate();
		else if (Strategy().enemyGasSteal())
			Reaction2Gate();
		else {
			getOpening = s < 80;
			currentVariant =	"3GateGoon";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
		}
	}

	void BuildOrderManager::PZCore()
	{
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;
		playPassive	=		(com(Protoss_Gateway) < 6 || s <= 200) && com(Protoss_Reaver) < 3 && com(Protoss_Dark_Templar) < 2;

		zealotLimit =		1;
		dragoonLimit =		INT_MAX;

		bool addGates = goonRange() && Broodwar->self()->minerals() >= 100;
		bool addGas = Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);

		if (enemyBuild() == "P2Gate" && !Strategy().enemyFastExpand())
			Reaction2GateDefensive();
		else if (enemyBuild() == "P1GateRobo" || enemyBuild() == "P2GateExpand")
			Reaction4Gate();
		else if (Strategy().enemyGasSteal())
			Reaction2Gate();
		else if (Terrain().isNarrowNatural()) {
			P1GateRobo();
			currentVariant =	"1GateRobo";
		}
		else {
			getOpening =		s < 80;
			currentVariant =	"3GateGoon";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
	}

	void BuildOrderManager::PZZCore()
	{
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;
		playPassive	=		(com(Protoss_Gateway) < 6 || s <= 200) && com(Protoss_Reaver) < 3 && com(Protoss_Dark_Templar) < 2;

		zealotLimit =		2;
		dragoonLimit =		INT_MAX;

		bool addGates = goonRange() && Broodwar->self()->minerals() >= 100;
		bool addGas = Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);

		if (enemyBuild() == "P2Gate" && !Strategy().enemyFastExpand())
			Reaction2GateDefensive();
		else if (enemyBuild() == "P1GateRobo" || enemyBuild() == "P2GateExpand")
			Reaction4Gate();
		else if (Strategy().enemyGasSteal())
			Reaction2Gate();
		else {
			getOpening =		s < 80;
			currentVariant =	"3GateGoon";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
		}
	}

	void BuildOrderManager::PProxy6()
	{
		proxy =				true;
		firstUpgrade =		UpgradeTypes::None;
		firstTech =			TechTypes::None;
		getOpening =		s < 30;
		scout =				vis(Protoss_Gateway) >= 1;
		currentVariant =	"Default";
		rush =				true;

		zealotLimit =		INT_MAX;
		dragoonLimit =		INT_MAX;

		itemQueue[Protoss_Nexus] =					Item(1);
		itemQueue[Protoss_Pylon] =					Item((s >= 10), (s >= 12));
		itemQueue[Protoss_Gateway] =				Item(vis(Protoss_Pylon) > 0);
	}

	void BuildOrderManager::PProxy99()
	{
		proxy =				vis(Protoss_Gateway) < 2;
		firstUpgrade =		UpgradeTypes::None;
		firstTech =			TechTypes::None;
		getOpening =		s < 50;
		scout =				vis(Protoss_Gateway) >= 2;
		gasLimit =			INT_MAX;
		currentVariant =	"Default";
		rush =				true;

		zealotLimit =		INT_MAX;
		dragoonLimit =		INT_MAX;

		itemQueue[Protoss_Pylon] =					Item((s >= 12), (s >= 16));
		itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
	}

	void BuildOrderManager::P2GateExpand()
	{
		wallNat =			true;
		fastExpand =		true;
		firstUpgrade =		Players().vZ() ? UpgradeTypes::Protoss_Ground_Weapons : UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		getOpening =		s < 80;
		gasLimit =			INT_MAX;

		zealotLimit =		INT_MAX;
		dragoonLimit =		Players().vP() ? INT_MAX : 0;

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

		// Versus Zerg
		if (Players().vZ()) {
			if (Units().getEnemyCount(UnitTypes::Zerg_Sunken_Colony) >= 2) {
				currentVariant =	"Expansion";

				itemQueue[Protoss_Assimilator] =		Item(s >= 76);
				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
				itemQueue[Protoss_Forge] =				Item(s >= 62);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
				itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
			}
			else if (enemyBuild() == "Z9Pool") {
				currentVariant =	"Defensive";

				itemQueue[Protoss_Forge] =				Item(1);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
				itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
			}
			else if (enemyBuild() == "Z5Pool") {
				getOpening =		s < 120;
				currentVariant =	"Panic";

				itemQueue[Protoss_Nexus] =				Item(1);
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
				itemQueue[Protoss_Assimilator] =		Item(s >= 64);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
				gasLimit = 1;
			}
			else {
				currentVariant =	"Default";
				zealotLimit	=		5;

				itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24), (s >= 20) + (s >= 24));
				itemQueue[Protoss_Assimilator] =		Item(s >= 58);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
				itemQueue[Protoss_Forge] =				Item(s >= 70);
				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
				itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
			}
		}

		// Versus Protoss
		else {

			//rush = com(Protoss_Nexus) != 2;

			if (!Strategy().enemyFastExpand() && (enemyBuild() == "P4Gate" || Units().getEnemyCount(Protoss_Gateway) >= 2 || Units().getEnemyCount(UnitTypes::Protoss_Dragoon) >= 2)) {
				currentVariant =	"Defensive";
				playPassive =		com(Protoss_Gateway) < 5;
				zealotLimit	=		INT_MAX;

				itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 50), (s >= 20) + (s >= 24) + (s >= 50));
				itemQueue[Protoss_Assimilator] =		Item(s >= 64);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
				itemQueue[Protoss_Forge] =				Item(1);
				itemQueue[Protoss_Photon_Cannon] =		Item(6 * (com(Protoss_Forge) > 0));
				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
			}
			else if (enemyBuild() == "P2Gate") {
				Reaction2GateDefensive();
			}
			else if (Strategy().enemyFastExpand())
				Reaction4Gate();
			else {
				currentVariant =	"Default";
				zealotLimit	=		5;

				itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24), (s >= 20) + (s >= 24));
				itemQueue[Protoss_Assimilator] =		Item(s >= 58);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
				itemQueue[Protoss_Forge] =				Item(s >= 70);
				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
				itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
			}
		}
	}

	void BuildOrderManager::P1GateRobo()
	{
		playPassive =		!Strategy().enemyFastExpand() && (com(Protoss_Reaver) < 2 || com(Protoss_Shuttle) < 1);
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
		getOpening =		(com(Protoss_Reaver) < 2 && s < 140);
		zealotLimit =		(com(Protoss_Robotics_Facility) >= 1) ? 6 : 1;
		dragoonLimit =		INT_MAX;

		bool addGas = Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);

		if (enemyBuild() == "P2Gate" && !Strategy().enemyFastExpand())
			Reaction2GateDefensive();
		else if (Strategy().enemyGasSteal())
			Reaction2Gate();
		else {
			currentVariant =	"Default";
			gasLimit =			INT_MAX;

			itemQueue[Protoss_Nexus] =					Item(1);
			itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
			itemQueue[Protoss_Shield_Battery] =			Item(vis(Protoss_Robotics_Facility) > 0);
			itemQueue[Protoss_Assimilator] =			Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 34);
			itemQueue[Protoss_Robotics_Facility] =		Item(s >= 52);

			if (Strategy().needDetection())
				itemQueue[Protoss_Observatory] =			Item(com(Protoss_Robotics_Facility) > 0);
			else
				itemQueue[Protoss_Robotics_Support_Bay] =	Item(com(Protoss_Robotics_Facility) > 0);

			// Unlock our tech based on which we built first
			if (vis(Protoss_Observatory) > 0) {
				techList.insert(Protoss_Observer);
				unlockedType.insert(Protoss_Observer);
			}
			if (vis(Protoss_Robotics_Support_Bay) > 0) {
				techList.insert(Protoss_Reaver);
				unlockedType.insert(Protoss_Reaver);
				techList.insert(Protoss_Shuttle);
				unlockedType.insert(Protoss_Shuttle);
			}
		}
	}

	void BuildOrderManager::P3Nexus()
	{
		fastExpand =		true;
		wallNat =			true;
		playPassive =		!firstReady();
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

	void BuildOrderManager::PZealotDrop()
	{
		firstUpgrade =		UpgradeTypes::Gravitic_Drive;
		firstTech =			TechTypes::None;
		getOpening =		s < 60;
		scout =				0;
		gasLimit =			INT_MAX;
		hideTech =			true;
		currentVariant =	"Island";


		if (techList.find(Protoss_Shuttle) == techList.end())
			techUnit =			UnitTypes::Protoss_Shuttle;

		itemQueue[Protoss_Nexus] =				Item(1);
		itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
		itemQueue[Protoss_Gateway] =			Item((s >= 20) + (vis(Protoss_Robotics_Facility) > 0));
		itemQueue[Protoss_Assimilator] =		Item(s >= 24);
		itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
		itemQueue[Protoss_Robotics_Facility] =	Item(com(Protoss_Cybernetics_Core) > 0);
	}

	void BuildOrderManager::P1GateCorsair()
	{
		firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
		firstTech =			TechTypes::None;
		getOpening =		s < 60;
		scout =				0;
		gasLimit =			INT_MAX;
		hideTech =			true;

		zealotLimit =		INT_MAX;
		dragoonLimit =		0;

		if (techList.find(Protoss_Corsair) == techList.end())
			techUnit =			UnitTypes::Protoss_Corsair;

		if (Terrain().isIslandMap()) {
			currentVariant =	"Island";
			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20));
			itemQueue[Protoss_Forge] =				Item(vis(Protoss_Stargate) > 0);
			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			itemQueue[Protoss_Stargate] =			Item((vis(Protoss_Stargate) > 0) + (com(Protoss_Cybernetics_Core) > 0));
		}
		else if (Strategy().enemyRush()) {
			scout =				vis(Protoss_Pylon) > 0;
			currentVariant =	"Defensive";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 18) * 2);
			itemQueue[Protoss_Forge] =				Item(com(Protoss_Stargate) >= 1);
			itemQueue[Protoss_Assimilator] =		Item(s >= 54);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
			itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) > 0);
		}
		else {
			scout =				vis(Protoss_Pylon) > 0;
			currentVariant =	"Default";

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 18) + vis(Protoss_Stargate) > 0);
			itemQueue[Protoss_Forge] =				Item(vis(Protoss_Gateway) >= 2);
			itemQueue[Protoss_Assimilator] =		Item(s >= 40);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 36);
			itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) > 0);
		}
	}
}