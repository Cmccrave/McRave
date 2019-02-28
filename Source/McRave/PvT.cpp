#include "McRave.h"
#include "BuildOrder.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;
#define s Units::getSupply()

namespace McRave::BuildOrder::Protoss {

    namespace {

        string enemyBuild() { return Strategy::getEnemyBuild(); }

        bool goonRange() {
            return Broodwar->self()->isUpgrading(UpgradeTypes::Singularity_Charge) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge);
        }

        bool addGates() {
            return goonRange() && Broodwar->self()->minerals() >= 100;
        }

        bool addGas() {
            return Broodwar->getStartLocations().size() >= 3 ? (s >= 22) : (s >= 24);
        }

        void defaultPvT() {
            hideTech =          false;
            playPassive =       false;
            fastExpand =        false;
            wallNat =           false;
            wallMain =          false;

            desiredDetection =  UnitTypes::Protoss_Observer;
            firstUnit =         UnitTypes::None;

            firstUpgrade =		UpgradeTypes::Singularity_Charge;
            firstTech =			TechTypes::None;
            scout =				vis(Protoss_Cybernetics_Core) > 0;
            gasLimit =			INT_MAX;
            zealotLimit =		0;
            dragoonLimit =		INT_MAX;
        }
    }

    void PvT2GateDefensive() {
        gasLimit =			(com(Protoss_Cybernetics_Core) && s >= 50) ? INT_MAX : 0;
        getOpening =		vis(Protoss_Dark_Templar) == 0;
        playPassive	=		vis(Protoss_Dark_Templar) == 0;
        firstUpgrade =		UpgradeTypes::None;
        firstTech =			TechTypes::None;
        fastExpand =		false;

        zealotLimit =		INT_MAX;
        dragoonLimit =		INT_MAX;

        if (com(Protoss_Cybernetics_Core) > 0 && techList.find(Protoss_Dark_Templar) == techList.end() && s >= 80)
            firstUnit = Protoss_Dark_Templar;

        itemQueue[Protoss_Nexus] =					Item(1);
        itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =			Item(s >= 40);
        itemQueue[Protoss_Shield_Battery] =			Item(vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
        itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 58);
    }

    void PvT2Gate()
    {
        // https://liquipedia.net/starcraft/10/15_Gates_(vs._Terran)
        defaultPvT();
        playPassive = false;
        proxy = currentOpener == "Proxy";
        wallNat = currentOpener == "Natural";
        scout = Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;

        // Openers
        if (currentOpener == "Proxy") {
            itemQueue[Protoss_Pylon] =					Item((s >= 12), (s >= 16));
            itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
        }
        else if (currentOpener == "Main") {
            itemQueue[Protoss_Pylon] =				Item((s >= 14), (s >= 16));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30));
        }

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyFastExpand())
                currentTransition = "DT";

            // Change Opener

            // Change Build
        }

        // Transitions
        if (currentTransition == "DT") {
            lockedTransition =  vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =		s < 70;
            firstUnit =			vis(Protoss_Dragoon) >= 3 ? Protoss_Dark_Templar : UnitTypes::None;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Assimilator] =		Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentTransition == "Reaver") {
            lockedTransition =  vis(Protoss_Robotics_Facility) > 0;
            getOpening =		s < 70;
            firstUnit =			com(Protoss_Dragoon) >= 3 ? Protoss_Reaver : UnitTypes::None;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Assimilator] =		Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentTransition == "Expand") {
            lockedTransition =  vis(Protoss_Nexus) >= 2;
            getOpening =		s < 100;

            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50));
            itemQueue[Protoss_Assimilator] =		Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentTransition == "DoubleExpand") {
            lockedTransition =  vis(Protoss_Nexus) >= 3;
            getOpening =		s < 120;

            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 50) + (s >= 50));
            itemQueue[Protoss_Assimilator] =		Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
    }

    void PvT1GateCore()
    {
        // https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)
        defaultPvT();
        firstUpgrade =		UpgradeTypes::Singularity_Charge;
        firstTech =			TechTypes::None;
        scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        gasLimit =			INT_MAX;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush())
                currentTransition = "Defensive";
            else if (Strategy::enemyFastExpand())
                currentTransition = "DT";

            // Change Opener

            // Change Build
        }

        // Openers
        if (currentOpener == "0Zealot") {
            zealotLimit = 0;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));
            itemQueue[Protoss_Assimilator] =		Item((addGas() || Strategy::enemyScouted()));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentOpener == "1Zealot") {
            zealotLimit = 1;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));
            itemQueue[Protoss_Assimilator] =		Item((addGas() || Strategy::enemyScouted()));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
        }

        // Transitions
        if (currentTransition == "Reaver") {
            // http://liquipedia.net/starcraft/1_Gate_Reaver
            getOpening =		s < 60;
            lockedTransition =  vis(Protoss_Robotics_Facility) > 0;

            hideTech =			com(Protoss_Reaver) == 0;
            getOpening =		(com(Protoss_Reaver) < 1);

            itemQueue[Protoss_Nexus] =					Item(1 + (s >= 74));
            itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
            itemQueue[Protoss_Assimilator] =			Item((addGas() || Strategy::enemyScouted()));
            itemQueue[Protoss_Robotics_Facility] =		Item(s >= 52);

            // Decide whether to Reaver first or Obs first
            if (com(Protoss_Robotics_Facility) > 0) {
                if (vis(Protoss_Observer) == 0 && (Units::getEnemyCount(Terran_Vulture_Spider_Mine) > 0 || enemyBuild() == "2Fact"))
                    firstUnit = Protoss_Observer;
                else
                    firstUnit = Protoss_Reaver;
            }
        }
        else if (currentTransition == "4Gate") {
            // https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
            getOpening = s < 80;
            lockedTransition = vis(Protoss_Gateway) >= 4;

            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 30) + (2 * (s >= 62)));
            itemQueue[Protoss_Assimilator] =		Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentTransition == "DT") {
            // https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)
            firstUpgrade =		UpgradeTypes::Khaydarin_Core;
            getOpening =		s < 60;
            hideTech =			com(Protoss_Dark_Templar) < 1;
            dragoonLimit =		INT_MAX;
            firstUnit =         vis(Protoss_Citadel_of_Adun) > 0 ? Protoss_Dark_Templar : UnitTypes::None;
            lockedTransition =  vis(Protoss_Citadel_of_Adun) > 0;

            itemQueue[Protoss_Citadel_of_Adun] =	Item(s >= 36);
            itemQueue[Protoss_Templar_Archives] =	Item(s >= 48);
        }
        else if (currentTransition == "Defensive")
            PvT2GateDefensive();
    }

    void PvTNexusGate()
    {
        // http://liquipedia.net/starcraft/12_Nexus
        defaultPvT();
        fastExpand =		true;
        playPassive =		Units::getEnemyCount(Terran_Siege_Tank_Tank_Mode) == 0 && Units::getEnemyCount(Terran_Siege_Tank_Siege_Mode) == 0 && Strategy::enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
        firstUpgrade =		vis(Protoss_Dragoon) >= 2 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        firstTech =			TechTypes::None;
        scout =				vis(Protoss_Cybernetics_Core) >= 1;
        wallNat =			vis(Protoss_Nexus) >= 2 ? true : false;
        cutWorkers =		Production::hasIdleProduction() && com(Protoss_Cybernetics_Core) > 0;

        gasLimit =			goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;

        // Openers
        if (currentOpener == "Zealot") {
            zealotLimit = 1;

            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 48));
            itemQueue[Protoss_Assimilator] =		Item((s >= 30) + (s >= 60));
            itemQueue[Protoss_Gateway] =			Item((s >= 28) + (s >= 34) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Gateway) >= 2);
        }
        else if (currentOpener == "Dragoon") {
            zealotLimit = 0;

            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 48));
            itemQueue[Protoss_Assimilator] =		Item((s >= 28) + (s >= 60));
            itemQueue[Protoss_Gateway] =			Item((s >= 26) + (s >= 32) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 30);
        }

        // Reactions
        if (!lockedTransition) {
            
            // Change Transition
            if (Strategy::enemyFastExpand() || enemyBuild() == "TSiegeExpand")
                currentTransition =	"DoubleExpand";
            else if (!Strategy::enemyFastExpand() && currentTransition == "DoubleExpand")
                currentTransition = "Standard";

            // Change Opener

            // Change Build
        }

        // Transitions
         if (currentTransition == "DoubleExpand") {
            getOpening =		s < 140;
            lockedTransition =  vis(Protoss_Nexus) >= 3;

            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 24) + (com(Protoss_Cybernetics_Core) > 0));
            itemQueue[Protoss_Assimilator] =		Item(s >= 28);
        }
        else if (currentTransition == "Standard") {
            getOpening =		s < 90;
        }
        else if (currentTransition == "ReaverCarrier") {
            getOpening =		s < 120;
            lockedTransition =  vis(Protoss_Robotics_Facility) > 0;

            if (techList.find(Protoss_Reaver) != techList.end())
                firstUnit = Protoss_Reaver;
            if (com(Protoss_Reaver) && techList.find(Protoss_Carrier) != techList.end())
                firstUnit = Protoss_Carrier;

            itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 1) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
        }
    }

    void PvTGateNexus()
    {
        // 1 Gate - "http://liquipedia.net/starcraft/21_Nexus"
        // 2 Gate - "https://liquipedia.net/starcraft/2_Gate_Range_Expand"
        defaultPvT();
        fastExpand =		true;
        playPassive =		Units::getEnemyCount(Terran_Siege_Tank_Tank_Mode) == 0 && Units::getEnemyCount(Terran_Siege_Tank_Siege_Mode) == 0 && Strategy::enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
        firstUpgrade =		UpgradeTypes::Singularity_Charge;
        firstTech =			TechTypes::None;
        scout =				Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
        wallNat =			com(Protoss_Nexus) >= 2 ? true : false;

        // Pull 1 probe when researching goon range, add 1 after we have a Nexus, then add 3 when 2 gas
        gasLimit =			goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;
        zealotLimit =		0;

        // Want to make only 1 goon before Nexus in case of weird supply count when expanding
        dragoonLimit =		vis(Protoss_Nexus) >= 2 ? INT_MAX : 1;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyFastExpand() || enemyBuild() == "TSiegeExpand")
                currentTransition =	"DoubleExpand";
            else if ((!Strategy::enemyFastExpand() && Terrain::foundEnemy() && currentTransition == "DoubleExpand") || Strategy::enemyPressure())
                currentTransition = "Standard";

			// Change Opener

            // Change Build
            if (s < 42 && Strategy::enemyRush()) {
                currentBuild = "2Gate";
                currentOpener = "Main";
                currentTransition = "DT";
            }
        }

        // Openers - 1Gate / 2Gate
        if (currentOpener == "1Gate") {
            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (vis(Protoss_Nexus) >= 2) + (s >= 76));
        }
        else if (currentOpener == "2Gate") {
            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 40));
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 36) + (s >= 76));
        }

        // Transitions - DoubleExpand / Standard / Carrier
        if (currentTransition == "DoubleExpand") {
            getOpening =		s < 140;
            playPassive =		com(Protoss_Nexus) < 3;
            lockedTransition =  vis(Protoss_Nexus) >= 3;
            gasLimit =          s >= 80 ? INT_MAX : 3;

            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42) + (s >= 70));
            itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentTransition == "Standard") {
            getOpening =		s < 80;
            firstUnit =         com(Protoss_Nexus) >= 2 ? Protoss_Observer : UnitTypes::None;
            lockedTransition =  com(Protoss_Nexus) >= 2;

            itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (s >= 50));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
        else if (currentTransition == "Carrier") {
            getOpening =		s < 80;
            firstUnit =         com(Protoss_Nexus) >= 2 ? Protoss_Carrier : UnitTypes::None;
            lockedTransition =  com(Protoss_Nexus) >= 2;
			gasLimit =			INT_MAX;

            itemQueue[Protoss_Assimilator] =		Item((s >= 24) + (s >= 50));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 26);
        }
    }
}