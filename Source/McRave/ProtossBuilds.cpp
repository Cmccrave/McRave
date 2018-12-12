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
	namespace {
		static int vis(UnitType t) {
			return Broodwar->self()->visibleUnitCount(t);
		}
		static int com(UnitType t) {
			return Broodwar->self()->completedUnitCount(t);
		}

		string enemyBuild =	Strategy().getEnemyBuild();
		bool goonRange() {
			return Broodwar->self()->isUpgrading(UpgradeTypes::Singularity_Charge) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge);
		}
		bool addGates() {
			return goonRange() && Broodwar->self()->minerals() >= 100;
		}
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
		
		itemQueue[Protoss_Nexus] =					Item(1);
		itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
		itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 24) + (addGates()));
		itemQueue[Protoss_Assimilator] =			Item(s >= 44);
		itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 52);
	}

	void BuildOrderManager::Reaction2GateDefensive() {
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

		auto min100 = Broodwar->self()->minerals() >= 100;
		auto cannonCount = int(com(Protoss_Forge) > 0) + (Units().getEnemyCount(Zerg_Zergling) >= 6) + (Units().getEnemyCount(Zerg_Zergling) >= 12) + (Units().getEnemyCount(Zerg_Zergling) >= 24);

		if (techList.find(Protoss_Corsair) == techList.end())
			techUnit = Protoss_Corsair;

		// TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons		
		if (enemyBuild == "Z2HatchHydra")
			cannonCount = 5;
		else if (enemyBuild == "Z3HatchHydra")
			cannonCount = 4;
		else if (enemyBuild == "Z2HatchMuta")
			cannonCount = 7;
		else if (enemyBuild == "Z3HatchMuta")
			cannonCount = 8;

		// Reactions
		if ((enemyBuild == "Unknown" && !Terrain().getEnemyStartingPosition().isValid()) || enemyBuild == "Z9Pool")
			false;// currentTransition = "Defensive";
		else if (enemyBuild == "Z5Pool")
			currentTransition =	"Defensive";
		else if (enemyBuild == "Z2HatchHydra" || enemyBuild == "Z3HatchHydra")
			currentTransition =	"StormRush";
		else if (enemyBuild == "Z2HatchMuta" || enemyBuild == "Z3HatchMuta")
			currentTransition =	"DoubleStargate";

		// Openers
		if (currentOpener == "Forge") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 28));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Assimilator] =		Item((vis(Protoss_Gateway) >= 1) + (vis(Protoss_Stargate) >= 1));
			itemQueue[Protoss_Gateway] =			Item((s >= 32) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 20);
		}
		else if (currentOpener == "Nexus") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
			itemQueue[Protoss_Assimilator] =		Item((vis(Protoss_Gateway) >= 1) + (vis(Protoss_Stargate) >= 1));
			itemQueue[Protoss_Gateway] =			Item(vis(Protoss_Forge) > 0);
			itemQueue[Protoss_Forge] =				Item(vis(Protoss_Nexus) >= 2);
		}
		else if (currentOpener == "Gate") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 24), (s >= 16) + (s >= 24));
			itemQueue[Protoss_Assimilator] =		Item((s >= 50) + (s >= 64));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 46));
			itemQueue[Protoss_Forge] =				Item(s >= 60);
		}

		// HACK: No cannons before forge obviously
		if (cannonCount > 0 && com(Protoss_Forge) == 0) {
			cannonCount = 0;
			itemQueue[Protoss_Forge] = Item(1);
		}

		// Transitions
		if (currentTransition == "StormRush") {
			firstUpgrade =		UpgradeTypes::None;
			firstTech =			TechTypes::Psionic_Storm;

			if (techList.find(Protoss_High_Templar) == techList.end())
				techUnit = Protoss_High_Templar;

			itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount);
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 60));
			itemQueue[Protoss_Cybernetics_Core] =	Item((s >= 42));
		}
		else if (currentTransition == "DoubleStargate") {
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			firstTech =			TechTypes::None;

			itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount);
			itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 60));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
			itemQueue[Protoss_Stargate] =			Item((vis(Protoss_Corsair) > 0) + (vis(Protoss_Cybernetics_Core) > 0));
		}
		else {
			getOpening =		s < 100;
			currentTransition =	"NeoBisu";
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;

			itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount);
			itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Zealot) >= 1);
			itemQueue[Protoss_Citadel_of_Adun] =	Item(vis(Protoss_Assimilator) >= 2);
			itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) >= 1);
			itemQueue[Protoss_Templar_Archives] =	Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));
		}
	}

	void BuildOrderManager::P2Gate()
	{
		if (currentOpener == "Proxy") {
			proxy =				vis(Protoss_Gateway) < 2;

			itemQueue[Protoss_Pylon] =					Item((s >= 12), (s >= 16));
			itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
		}
		else if (currentOpener == "Natural") {
			wallNat =			true;

			if (Players().vT()) {
				itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30));
			}
			else {
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
			}
		}
		else if (currentOpener == "Main") {

			// 10/12 Gates on 3Player+ maps
			if (Broodwar->getStartLocations().size() >= 3) {
				scout =									vis(Protoss_Gateway) >= 1;
				itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24));
			}
			// 9/10 Gates
			else {
				scout =									vis(Protoss_Gateway) >= 2;
				itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
				itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0) + (s >= 20));
			}
		}


		if (currentTransition == "ZealotRush") {
			zealotLimit = INT_MAX;
			getOpening = s < 80;
		}

		else if (Players().vT()) {
			firstUpgrade =		UpgradeTypes::Singularity_Charge;
			firstTech =			TechTypes::None;
			scout =				vis(Protoss_Cybernetics_Core) > 0;
			getOpening =		s < 100;
			gasLimit =			INT_MAX;

			zealotLimit =		0;
			dragoonLimit =		INT_MAX;

			if (Strategy().enemyFastExpand() || enemyBuild == "TSiegeExpand")
				currentTransition = "DT";
			else if (enemyBuild == "TBBS")
				currentTransition = "Defensive";

			if (currentTransition == "DT") {
				getOpening =		s < 70;

				if (com(Protoss_Dragoon) >= 3 && techList.find(Protoss_Dark_Templar) == techList.end())
					techUnit = UnitTypes::Protoss_Dark_Templar;

				itemQueue[Protoss_Nexus] =				Item(1);
				itemQueue[Protoss_Assimilator] =		Item(s >= 22);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			}
			else if (currentTransition == "Reaver") {
				getOpening =		s < 70;

				if (com(Protoss_Dragoon) >= 3 && techList.find(Protoss_Reaver) == techList.end())
					techUnit = UnitTypes::Protoss_Reaver;

				itemQueue[Protoss_Nexus] =				Item(1);
				itemQueue[Protoss_Assimilator] =		Item(s >= 22);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			}
			else if (currentTransition == "Defensive") {
				gasLimit =			0;
				fastExpand =		false;
				Reaction2GateDefensive();
			}
			else if (currentTransition == "Expansion"){
				getOpening =		s < 100;

				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50));
				itemQueue[Protoss_Assimilator] =		Item(s >= 22);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			}
			else if (currentTransition == "DoubleExpand") {
				getOpening =		s < 120;

				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50) + (s >= 50));
				itemQueue[Protoss_Assimilator] =		Item(s >= 22);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			}
			else {
				itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50));
				itemQueue[Protoss_Assimilator] =		Item(s >= 22);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			}
		}
		else
		{
			wallNat =			true;
			firstUpgrade =		Players().vZ() ? UpgradeTypes::Protoss_Ground_Weapons : UpgradeTypes::Singularity_Charge;
			firstTech =			TechTypes::None;
			getOpening =		s < 80;
			gasLimit =			INT_MAX;
			zealotLimit =		INT_MAX;
			dragoonLimit =		Players().vP() ? INT_MAX : 0;

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
				else if (enemyBuild == "Z9Pool") {
					currentTransition =	"Defensive";

					itemQueue[Protoss_Forge] =				Item(1);
					itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
					itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
				}
				else if (enemyBuild == "Z5Pool") {
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

					itemQueue[Protoss_Assimilator] =		Item(s >= 58);
					itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
					itemQueue[Protoss_Forge] =				Item(s >= 70);
					itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
					itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
				}
			}

			// Versus Protoss
			else {
				if (!Strategy().enemyFastExpand() && (enemyBuild == "P4Gate" || Units().getEnemyCount(Protoss_Gateway) >= 2 || Units().getEnemyCount(UnitTypes::Protoss_Dragoon) >= 2)) {
					currentTransition =	"Defensive";
					playPassive =		com(Protoss_Gateway) < 5;
					zealotLimit	=		INT_MAX;

					itemQueue[Protoss_Assimilator] =		Item(s >= 64);
					itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
					itemQueue[Protoss_Forge] =				Item(1);
					itemQueue[Protoss_Photon_Cannon] =		Item(6 * (com(Protoss_Forge) > 0));
					itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
				}
				else if (enemyBuild == "P2Gate") {
					Reaction2GateDefensive();
				}
				else {
					currentTransition =	"Default";
					zealotLimit	=		5;

					itemQueue[Protoss_Assimilator] =		Item(s >= 58);
					itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
					itemQueue[Protoss_Forge] =				Item(s >= 70);
					itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
					itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
				}
			}
		}
	}

	void BuildOrderManager::P1GateCore()
	{
		// https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)
		// https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
		gasLimit =			INT_MAX;
				
		bool addGas = Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);

		if (enemyBuild == "P2Gate")
			currentTransition = "Defensive";

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
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
		}
		else if (currentOpener == "2Zealot") {
			zealotLimit = 2;

			itemQueue[Protoss_Nexus] =				Item(1);
			itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));
			itemQueue[Protoss_Assimilator] =		Item((addGas || Strategy().enemyScouted()));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
		}

		// Transitions
		if (currentTransition == "3GateRobo") {
			getOpening = s < 80;
			dragoonLimit = INT_MAX;

			itemQueue[Protoss_Robotics_Facility] =	Item(s >= 52);
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));

			// TODO: Decide whether to reaver or obs here, reaver for now
			if (vis(Protoss_Robotics_Facility) > 0 && techList.find(Protoss_Reaver) == techList.end())
				techUnit = Protoss_Reaver;
		}
		else if (currentTransition == "Reaver") {
			// http://liquipedia.net/starcraft/1_Gate_Reaver
			getOpening =		s < 60;
			dragoonLimit =		INT_MAX;

			if (Players().vP()) {
				playPassive =		!Strategy().enemyFastExpand() && (com(Protoss_Reaver) < 2 || com(Protoss_Shuttle) < 1);
				getOpening =		(com(Protoss_Reaver) < 2 && s < 140);
				zealotLimit =		(com(Protoss_Robotics_Facility) >= 1) ? 6 : zealotLimit;

				itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
				itemQueue[Protoss_Assimilator] =			Item((addGas || Strategy().enemyScouted()));
				itemQueue[Protoss_Robotics_Facility] =		Item(s >= 52);
			}
			else {
				hideTech =			true;
				getOpening =		(com(Protoss_Reaver) < 1);

				itemQueue[Protoss_Nexus] =					Item(1 + (s >= 74));
				itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
				itemQueue[Protoss_Assimilator] =			Item((addGas || Strategy().enemyScouted()));
				itemQueue[Protoss_Robotics_Facility] =		Item(s >= 52);
			}

			// TODO: Decide whether to reaver or obs here, reaver for now
			if (vis(Protoss_Robotics_Facility) > 0 && techList.find(Protoss_Reaver) == techList.end())
				techUnit = Protoss_Reaver;
		}
		else if (currentTransition == "Corsair") {
			getOpening =		s < 60;
			firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
			firstTech =			TechTypes::None;
			dragoonLimit =		0;
			zealotLimit	=		INT_MAX;
			playPassive =		com(Protoss_Stargate) == 0;

			if (techList.find(Protoss_Corsair) == techList.end())
				techUnit = Protoss_Corsair;

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
			else if (Players().vT()) {
				getOpening = s < 80;

				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30) + (2 * (s >= 62)));
				itemQueue[Protoss_Assimilator] =		Item(s >= 22);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
			}
			else {
				getOpening = s < 140;

				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 54) + (2 * (s >= 62)));
				itemQueue[Protoss_Assimilator] =		Item(s >= 32);
				itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
			}
		}
		else if (currentTransition == "DT") {
			if (Players().vT()) {
				// https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)
				firstUpgrade =		UpgradeTypes::Khaydarin_Core;
				getOpening =		s < 60;
				hideTech =			true;
				dragoonLimit =		INT_MAX;

				itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 36);
				itemQueue[Protoss_Templar_Archives] =	Item(s >= 48);
			}
			else if (Players().vZ()) {
				// Experimental build from Best
				firstUpgrade =		UpgradeTypes::None;
				firstTech =			TechTypes::Psionic_Storm;
				getOpening =		s < 70;
				hideTech =			com(Protoss_Dark_Templar) < 1;
				dragoonLimit =		1;
				zealotLimit =		vis(Protoss_Dark_Templar) >= 2 ? INT_MAX : 2;

				if (techList.find(Protoss_Dark_Templar) == techList.end())
					techUnit =			Protoss_Dark_Templar;
				if (com(Protoss_Dark_Templar) >= 2 && techList.find(Protoss_High_Templar) == techList.end())
					techUnit =			Protoss_High_Templar;

				itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 42));
				itemQueue[Protoss_Cybernetics_Core] =	Item(com(Protoss_Gateway) > 0);
				itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 34);
				itemQueue[Protoss_Templar_Archives] =	Item(vis(Protoss_Gateway) >= 2);
			}
		}
		else if (currentTransition == "Defensive")
			Reaction2GateDefensive();
	}

	void BuildOrderManager::P12Nexus()
	{
		// 12 Nexus - "http://liquipedia.net/starcraft/12_Nexus"
		fastExpand =		true;
		playPassive =		Strategy().enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
		firstUpgrade =		vis(Protoss_Dragoon) >= 2 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
		firstTech =			TechTypes::None;
		scout =				vis(Protoss_Cybernetics_Core) >= 1;
		wallNat =			vis(Protoss_Nexus) >= 2 ? true : false;
		cutWorkers =		Production().hasIdleProduction() && com(Protoss_Cybernetics_Core) > 0;

		// Pull 1 probe when researching goon range, add 1 after we have a Nexus, then add 3 when 2 gas
		gasLimit =			INT_MAX;
		zealotLimit =		0;
		dragoonLimit =		INT_MAX;

		// Reactions
		if (Terrain().isIslandMap())
			currentTransition =	"Island"; // TODO: Island stuff		
		else if (Strategy().enemyFastExpand() || Strategy().getEnemyBuild() == "TSiegeExpand")
			currentTransition =	"DoubleExpand";
		else if (Strategy().enemyPressure())
			currentTransition = "Standard";

		// Openers
		if (currentOpener == "Zealot") {
			zealotLimit = 1;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 48));
			itemQueue[Protoss_Assimilator] =		Item((s >= 30) + (s >= 50));
			itemQueue[Protoss_Gateway] =			Item((s >= 28) + (s >= 34) + (s >= 80));
			itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Gateway) >= 2);
		}
		else if (currentOpener == "Dragoon") {
			zealotLimit = 0;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
			itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 48));
			itemQueue[Protoss_Assimilator] =		Item((s >= 28) + (s >= 50));
			itemQueue[Protoss_Gateway] =			Item((s >= 26) + (s >= 32) + (s >= 80));
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
			getOpening =		s < 140;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24) + (com(Protoss_Cybernetics_Core) > 0));
			itemQueue[Protoss_Assimilator] =		Item(s >= 28);
		}
		else if (currentTransition == "Standard") {
			getOpening =		s < 90;			
		}
		else if (currentTransition == "ReaverCarrier") {
			getOpening =		s < 120;

			if (techList.find(Protoss_Reaver) != techList.end())
				techUnit = Protoss_Reaver;
			if (com(Protoss_Reaver) && techList.find(Protoss_Carrier) != techList.end())
				techUnit = Protoss_Carrier;

			itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 1) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
		}
	}

	void BuildOrderManager::P21Nexus()
	{
		// 21 Nexus - "http://liquipedia.net/starcraft/21_Nexus"
		fastExpand =		true;
		playPassive =		Strategy().enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
		firstUpgrade =		UpgradeTypes::Singularity_Charge;
		firstTech =			TechTypes::None;
		scout =				Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
		wallNat =			com(Protoss_Nexus) >= 2 ? true : false;

		// Pull 1 probe when researching goon range, add 1 after we have a Nexus, then add 3 when 2 gas
		gasLimit =			goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;
		zealotLimit =		0;
		dragoonLimit =		vis(Protoss_Nexus) >= 2 ? INT_MAX : 1;

		// Reactions
		if (s < 100 && (Strategy().enemyFastExpand() || enemyBuild == "TSiegeExpand"))
			currentTransition =	"DoubleExpand";
		else if (s < 80 && Strategy().enemyRush())
			currentTransition = "Defensive";
		else if (Strategy().enemyPressure())
			currentTransition = "Standard";

		// Openers
		if (currentOpener == "1Gate") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (vis(Protoss_Nexus) >= 2) + (s >= 76));
		}
		else if (currentOpener == "2Gate") {
			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
			itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
			itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 36) + (s >= 76));
		}

		// Transitions
		if (currentTransition == "DoubleExpand") {
			getOpening =		s < 140;
			playPassive =		com(Protoss_Nexus) < 3;

			itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42) + (s >= 70));
			itemQueue[Protoss_Assimilator] =		Item(s >= 24);
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

			if (s >= 100 && techList.find(Protoss_Observer) == techList.end())
				techUnit = Protoss_Observer;
		}
		else if (currentTransition == "Carrier") {
			getOpening =		s < 160;

			itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (com(Protoss_Nexus) >= 2));
			itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);

			if (s >= 100 && techList.find(Protoss_Carrier) == techList.end())
				techUnit = Protoss_Carrier;
		}
	}
}




//void BuildOrderManager::P2GateDragoon()
//{
//
//}
//
//void BuildOrderManager::PProxy6()
//{
//	proxy =				true;
//	firstUpgrade =		UpgradeTypes::None;
//	firstTech =			TechTypes::None;
//	getOpening =		s < 30;
//	scout =				vis(Protoss_Gateway) >= 1;
//	currentTransition =	"Default";
//	rush =				true;
//
//	zealotLimit =		INT_MAX;
//	dragoonLimit =		INT_MAX;
//
//	itemQueue[Protoss_Nexus] =					Item(1);
//	itemQueue[Protoss_Pylon] =					Item((s >= 10), (s >= 12));
//	itemQueue[Protoss_Gateway] =				Item(vis(Protoss_Pylon) > 0);
//}
//
//void BuildOrderManager::PProxy99()
//{
//	proxy =				vis(Protoss_Gateway) < 2;
//	firstUpgrade =		UpgradeTypes::None;
//	firstTech =			TechTypes::None;
//	getOpening =		s < 50;
//	scout =				vis(Protoss_Gateway) >= 2;
//	gasLimit =			INT_MAX;
//	currentTransition =	"Default";
//	rush =				true;
//
//	zealotLimit =		INT_MAX;
//	dragoonLimit =		INT_MAX;
//
//	itemQueue[Protoss_Pylon] =					Item((s >= 12), (s >= 16));
//	itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
//}
//
//void BuildOrderManager::P2GateExpand()
//{
//
//}
//
//void BuildOrderManager::P3Nexus()
//{
//	fastExpand =		true;
//	wallNat =			true;
//	playPassive =		!firstReady();
//	firstUpgrade =		UpgradeTypes::Singularity_Charge;
//	firstTech =			TechTypes::None;
//	getOpening =		s < 120;
//	scout =				vis(Protoss_Cybernetics_Core) > 0;
//	gasLimit =			2 + (s >= 60);
//	currentTransition =	"Default";
//
//	itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24) + (s >= 30));
//	itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 48), (s >= 16) + (s >= 48));
//	itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Cybernetics_Core) > 0) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
//	itemQueue[Protoss_Assimilator] =		Item(s >= 28);
//	itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
//}
//
//void BuildOrderManager::PZealotDrop()
//{
//	firstUpgrade =		UpgradeTypes::Gravitic_Drive;
//	firstTech =			TechTypes::None;
//	getOpening =		s < 60;
//	scout =				0;
//	gasLimit =			INT_MAX;
//	hideTech =			true;
//	currentTransition =	"Island";
//
//
//	if (techList.find(Protoss_Shuttle) == techList.end())
//		techUnit =			UnitTypes::Protoss_Shuttle;
//
//	itemQueue[Protoss_Nexus] =				Item(1);
//	itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
//	itemQueue[Protoss_Gateway] =			Item((s >= 20) + (vis(Protoss_Robotics_Facility) > 0));
//	itemQueue[Protoss_Assimilator] =		Item(s >= 24);
//	itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
//	itemQueue[Protoss_Robotics_Facility] =	Item(com(Protoss_Cybernetics_Core) > 0);
//}