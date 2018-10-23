#include "McRave.h"
using namespace UnitTypes;
#define s Units().getSupply()

// READING BUILD ORDERS
// I designed my build orders to be dynamic to my supply or visible/completed counts of other units/buildings
// Rather than queue items one at a time, this is constantly checked and updated

// ItemQueue:
// map<UnitType, int> of a desired quantity I want at any point in the game

// Item:
// The first int in my constructor represents how many I want to prepare placements for.
// The second int in my constructor represents how many I want to reserve resources for.

// Example: ItemQueue[Protoss_Pylon] = Item(s >= 14, s >= 16);
// I want 1 pylon, I will prepare a worker and a building placement at 7 supply and reserve resources at 8 supply.

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
		//currentTransition =		"2GateA";
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
		//currentTransition =	"3GateGoon";

		bool addGates = goonRange() && Broodwar->self()->minerals() >= 100;

		itemQueue[Protoss_Nexus] =					Item(1);
		itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
		itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 24) + (addGates));
		itemQueue[Protoss_Assimilator] =			Item(s >= 44);
		itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 52);
	}

	void BuildOrderManager::Reaction2GateDefensive() {
		//currentTransition =	"2GateD";
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
			getOpening =			s < 120;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
			itemQueue[Protoss_Assimilator] =		Item(s >= 44);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
		}
		else {
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
		techUnit =			Protoss_Corsair;

		zealotLimit =		INT_MAX;
		dragoonLimit =		0;

		auto min100 = Broodwar->self()->minerals() >= 100;

		// Reactions
		if ((enemyBuild() == "Unknown" && !Terrain().getEnemyStartingPosition().isValid()) || enemyBuild() == "Z9Pool")
			false;// currentTransition = "Defensive";
		else if (enemyBuild() == "Z5Pool")
			currentTransition =	"Defensive";
		else if (enemyBuild() == "Z2HatchHydra" || enemyBuild() == "Z3HatchHydra")
			currentTransition =	"StormRush";
		else if (enemyBuild() == "Z2HatchMuta" || enemyBuild() == "Z3HatchMuta")
			currentTransition =	"DoubleStargate";

		// Openers
		if (currentOpener == "Forge") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 32) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Forge) > 0 && min100) + (vis(Protoss_Photon_Cannon) > 0));
		}
		else if (currentOpener == "Nexus") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item(vis(Protoss_Forge) > 0);
			itemQueue[Protoss_Forge] =				Item(vis(Protoss_Nexus) >= 2);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Forge) > 0 && min100) + (vis(Protoss_Photon_Cannon) > 0));
		}
		else if (currentOpener == "Gate") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 24), (s >= 16) + (s >= 24));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 60);
			itemQueue[Protoss_Photon_Cannon] =		Item((vis(Protoss_Forge) > 0 && min100) + (vis(Protoss_Photon_Cannon) > 0));
		}

		// Transitions
		if (currentTransition == "Defensive") {					
			
		}
		else if (currentTransition == "StormRush") {
			firstUpgrade =		UpgradeTypes::None;
			firstTech =			TechTypes::Psionic_Storm;
			techUnit =			Protoss_High_Templar;

			if (Units().getEnemyCount(UnitTypes::Zerg_Hydralisk) > 0) {
				if (enemyBuild() == "Z2HatchHydra")
					itemQueue[Protoss_Photon_Cannon] =		Item((s >= 30) + (3 * (s >= 46)));
				else
					itemQueue[Protoss_Photon_Cannon] =		Item((s >= 30) + (3 * (s >= 50)) + (2 * (s >= 54)));
			}
			
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 60));
			itemQueue[Protoss_Cybernetics_Core] =	Item((s >= 42));
		}

		// Muta bust, get Corsairs and Cannon defensively
		else if (enemyBuild() == "Z2HatchMuta" || enemyBuild() == "Z3HatchMuta") {
			currentTransition =	"DoubleStargate";
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			firstTech =			TechTypes::None;

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
			currentTransition =	"NeoBisu";
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			auto twoCannons =	Units().getEnemyCount(Zerg_Zergling) >= 5 && vis(Protoss_Forge) > 0;
			auto threeCannons = Strategy().getEnemyBuild() == "Z1HatchHydra" && vis(Protoss_Forge) > 0;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 32));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 34));
			itemQueue[Protoss_Forge] =				Item(s >= 18, s >= 20);
			itemQueue[Protoss_Photon_Cannon] =		Item((s >= 22 && vis(Protoss_Forge) > 0) + (twoCannons)+(2 * threeCannons), (com(Protoss_Forge) >= 1) + (twoCannons)+(2 * threeCannons));
			itemQueue[Protoss_Assimilator] =		Item((vis(Protoss_Gateway) >= 1) + (vis(Protoss_Stargate) >= 1));
			itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Zealot) >= 1);
			itemQueue[Protoss_Citadel_of_Adun] =	Item(vis(Protoss_Assimilator) >= 2);
			itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) >= 1);
			itemQueue[Protoss_Templar_Archives] =	Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));
		}
	}



	void BuildOrderManager::PDTExpand()
	{

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
			currentTransition =	"DT";

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
			currentTransition =	"Expansion";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50));
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 28));
			itemQueue[Protoss_Assimilator] =		Item(s >= 22);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);

		}
	}

	void BuildOrderManager::PProxy6()
	{
		proxy =				true;
		firstUpgrade =		UpgradeTypes::None;
		firstTech =			TechTypes::None;
		getOpening =		s < 30;
		scout =				vis(Protoss_Gateway) >= 1;
		currentTransition =	"Default";
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
		currentTransition =	"Default";
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
				currentTransition =	"Expansion";

				itemQueue[Protoss_Assimilator] =		Item(s >= 76);
				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
				itemQueue[Protoss_Forge] =				Item(s >= 62);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
				itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
			}
			else if (enemyBuild() == "Z9Pool") {
				currentTransition =	"Defensive";

				itemQueue[Protoss_Forge] =				Item(1);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
				itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
			}
			else if (enemyBuild() == "Z5Pool") {
				getOpening =		s < 120;
				currentTransition =	"Panic";

				itemQueue[Protoss_Nexus] =				Item(1);
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
				itemQueue[Protoss_Assimilator] =		Item(s >= 64);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
				gasLimit = 1;
			}
			else {
				currentTransition =	"Default";
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
				currentTransition =	"Defensive";
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
				currentTransition =	"Default";
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
		currentTransition =	"Default";

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
		currentTransition =	"Island";


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

	}




	void BuildOrderManager::P1GateCore()
	{
		// https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)
		// https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)

		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;

		bool addGates = goonRange() && Broodwar->self()->minerals() >= 100;
		bool addGas = Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);

		// Openers
		if (currentOpener == "0Zealot") {
			zealotLimit = 1;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item(s >= 20);
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
		}
		else if (currentOpener == "1Zealot") {
			zealotLimit = 1;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
		else if (currentOpener == "2Zealot") {
			zealotLimit = 2;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
		}
		
		// Transitions
		if (currentTransition == "3GateRobo") {
			getOpening = s < 80;
			dragoonLimit = INT_MAX;

			itemQueue[Protoss_Robotics_Facility] =	Item(s >= 52);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates));

			// TODO: Decide whether to reaver or obs here, reaver for now
			if (vis(Protoss_Robotics_Facility) > 0)
				techUnit = Protoss_Reaver;
		}
		else if (currentTransition == "Reaver") {
		// http://liquipedia.net/starcraft/1_Gate_Reaver
			dragoonLimit = INT_MAX;

			if (Players().vP()) {
				playPassive =		!Strategy().enemyFastExpand() && (com(Protoss_Reaver) < 2 || com(Protoss_Shuttle) < 1);
				getOpening =		(com(Protoss_Reaver) < 2 && s < 140);
				zealotLimit =		(com(Protoss_Robotics_Facility) >= 1) ? 6 : zealotLimit;
			}
			else {
				getOpening =		(com(Protoss_Reaver) < 1);
			}

			itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
			itemQueue[Protoss_Assimilator] =			Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Robotics_Facility] =		Item(s >= 52);

			// TODO: Decide whether to reaver or obs here, reaver for now
			if (vis(Protoss_Robotics_Facility) > 0)
				techUnit = Protoss_Reaver;
		}
		else if (currentTransition == "Corsair") {
			getOpening =		s < 60;
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			firstTech =			TechTypes::None;
			dragoonLimit =		0;
			zealotLimit	=		INT_MAX;
			techUnit =			Protoss_Corsair;

			if (Strategy().enemyRush()) {
				itemQueue[Protoss_Gateway] =			Item((s >= 18) * 2);
				itemQueue[Protoss_Forge] =				Item(com(Protoss_Stargate) >= 1);
				itemQueue[Protoss_Assimilator] =		Item(s >= 54);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
				itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) > 0);
			}
			else {
				itemQueue[Protoss_Gateway] =			Item((s >= 18) + vis(Protoss_Stargate) > 0);
				itemQueue[Protoss_Forge] =				Item(vis(Protoss_Gateway) >= 2);
				itemQueue[Protoss_Assimilator] =		Item(s >= 40);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 36);
				itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) > 0);
			}
		}
		else if (currentTransition == "4Gate") {
			// https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
			if (Players().vZ()) {
				getOpening = s < 120;

				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
				itemQueue[Protoss_Assimilator] =		Item(s >= 44);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
			}
			else {
				getOpening = s < 140;

				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 54) + (2 * (s >= 62)));
				itemQueue[Protoss_Assimilator] =		Item(s >= 32);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
			}
		}
		else if (currentTransition == "DT") {
			// https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)
			firstUpgrade =		UpgradeTypes::Khaydarin_Core;
			getOpening =		s < 60;
			hideTech =			true;
			dragoonLimit =		INT_MAX;

			itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 36);
			itemQueue[Protoss_Templar_Archives] =	Item(s >= 48);
		}
	}	

	void BuildOrderManager::P12Nexus()
	{
		// 12 Nexus - "http://liquipedia.net/starcraft/12_Nexus"
		fastExpand =		true;
		playPassive =		!firstReady();
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Cybernetics_Core) >= 1;
		gasLimit =			2 + (s >= 60);

		// Pull 1 probe when researching goon range, add 1 after we have a Nexus, then add 3 when 2 gas
		gasLimit =			2 + (!goonRange()) + (2 * (vis(Protoss_Nexus) >= 2)) + (3 * (com(Protoss_Assimilator) >= 2));
		zealotLimit =		0;
		dragoonLimit =		INT_MAX;

		// Reactions
		if (Terrain().isIslandMap())
			currentTransition =	"Island"; // TODO: Island stuff		
		else if (Strategy().enemyFastExpand() || Strategy().getEnemyBuild() == "TSiegeExpand")
			currentTransition =	"DoubleExpand";
		else
			currentTransition = "Standard";

		// Openers
		if (currentOpener != "") {
			zealotLimit = currentOpener == "Zealot";

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
			itemQueue[Protoss_Assimilator] =		Item((s >= 28) + (s >= 90));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
		}

		// Transitions
		if (currentTransition == "Island") {
			firstUpgrade =		UpgradeTypes::Gravitic_Drive;
			getOpening =		s < 50;
			scout =				0;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
			itemQueue[Protoss_Gateway] =			Item(vis(Protoss_Nexus) > 1);
			itemQueue[Protoss_Assimilator] =		Item(s >= 26);
		}
		else if (currentTransition == "DoubleExpand") {
			getOpening =		s < 120;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24) + (com(Protoss_Cybernetics_Core) > 0));
			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Cybernetics_Core) > 0) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
			itemQueue[Protoss_Assimilator] =		Item(s >= 28);

		}
		else if (currentTransition == "Standard") {
			getOpening =		s < 120;

			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 1) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
		}
		else if (currentTransition == "ReaverCarrier") {
			getOpening =		s < 120;
			techUnit =			Protoss_Reaver;

			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 1) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
			if (com(Protoss_Reaver))
				techUnit = Protoss_Carrier;
		}

		// TODO: Move this hack
		if (Broodwar->mapFileName().find("BlueStorm") != string::npos)
			firstUpgrade = UpgradeTypes::Carrier_Capacity;
	}

	void BuildOrderManager::P21Nexus()
	{
		// 21 Nexus - "http://liquipedia.net/starcraft/21_Nexus"
		fastExpand =		true;
		playPassive =		!firstReady();
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;

		// Pull 1 probe when researching goon range, add 1 after we have a Nexus, then add 3 when 2 gas
		gasLimit =			2 + (!goonRange()) + (2 * (vis(Protoss_Nexus) >= 2)) + (3 * (com(Protoss_Assimilator) >= 2));
		zealotLimit =		0;
		dragoonLimit =		INT_MAX;

		// Reactions
		if (Strategy().enemyFastExpand() || enemyBuild() == "TSiegeExpand")
			currentTransition =	"DoubleExpand";
		else if (Strategy().enemyRush())
			currentTransition = "Defensive";

		// Openers
		if (currentOpener == "1Gate") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (vis(Protoss_Nexus) >= 2) + (s >= 76));
		}
		else if (currentOpener == "2Gate") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 36) + (s >= 76));
		}

		// Transitions
		if (currentTransition == "DoubleExpand") {
			getOpening =		s < 100;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42) + (s >= 70));
			itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (s >= 76));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
		}
		else if (currentTransition == "Defensive") {
			gasLimit =			1;
			fastExpand =		false;
			getOpening =		s < 80;
			zealotLimit =		INT_MAX;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 72));
			itemQueue[Protoss_Assimilator] =		Item((s >= 40));
			itemQueue[Protoss_Shield_Battery] =		Item(s >= 48);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
		}
		else if (currentTransition == "Standard") {
			getOpening =		s < 80;

			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
		}
		else if (currentTransition == "Carrier") {
			getOpening =		s < 120;

			itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (vis(Protoss_Nexus) >= 2));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);

			if (s >= 100)
				techUnit = Protoss_Carrier;
		}
	}
}