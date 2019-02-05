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

        void defaultPvP() {
            firstUpgrade =		UpgradeTypes::Singularity_Charge;
            firstTech =			TechTypes::None;
            scout =				vis(Protoss_Gateway) > 0;
            gasLimit =			INT_MAX;
            zealotLimit =		1;
            dragoonLimit =		INT_MAX;
        }
    }

    void PvP2GateDefensive() {
        gasLimit =			(com(Protoss_Cybernetics_Core) && s >= 46) ? INT_MAX : 0;
        getOpening =		vis(Protoss_Dark_Templar) == 0;
        playPassive	=		vis(Protoss_Dark_Templar) == 0;
        firstUpgrade =		UpgradeTypes::None;
        firstTech =			TechTypes::None;
        fastExpand =		false;

        zealotLimit =		INT_MAX;
        dragoonLimit =		vis(Protoss_Templar_Archives) > 0 ? INT_MAX : 0;

        if (com(Protoss_Cybernetics_Core) > 0 && techList.find(Protoss_Dark_Templar) == techList.end() && s >= 80)
            firstUnit = Protoss_Dark_Templar;

        itemQueue[Protoss_Nexus] =					Item(1);
        itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =			Item(s >= 40);
        itemQueue[Protoss_Shield_Battery] =			Item(vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
        itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 58);
    }

    void PvP2Gate()
    {
        defaultPvP();
        zealotLimit = 5;
        proxy = currentOpener == "Proxy";
        wallNat = currentOpener == "Natural";
        scout = Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;

        // Openers
        if (currentOpener == "Proxy") {
            itemQueue[Protoss_Pylon] =					Item((s >= 12), (s >= 16));
            itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
        }
        else if (currentOpener == "Natural") {
            if (Broodwar->getStartLocations().size() >= 3) {
                itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0) + (s >= 20), (s >= 18) + (s >= 20));
            }
            else {
                itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
            }
        }
        else if (currentOpener == "Main") {
            if (Broodwar->getStartLocations().size() >= 3) {
                itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
                itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24));
            }
            else {
                itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0) + (s >= 20));
            }
        }

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush())
                currentTransition = "Panic";
            else if (Strategy::enemyPressure() && currentOpener == "Natural")
                currentTransition = "Defensive";
            else if (Units::getEnemyCount(UnitTypes::Zerg_Sunken_Colony) >= 2)
                currentTransition = "Expand";

            // Change Opener

            // Change Build
        }

        // Transitions
        if (currentTransition == "DT") {
            // https://liquipedia.net/starcraft/2_Gateway_Dark_Templar_(vs._Protoss)
            lockedTransition =  vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =		s < 70;

            hideTech =			currentOpener == "Main" && com(Protoss_Dark_Templar) < 1;           
            firstUnit =			vis(Protoss_Zealot) >= 3 ? Protoss_Dark_Templar : UnitTypes::None;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Assimilator] =		Item(s >= 44);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 56);
        }
        else if (currentTransition == "Standard") {
            // https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)#10.2F12_Gateway_Expand
            lockedTransition =  vis(Protoss_Nexus) >= 2;
            getOpening =		s < 100;

            itemQueue[Protoss_Assimilator] =		Item(s >= 58);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 60);
            itemQueue[Protoss_Forge] =				Item(s >= 70);
            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
            itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
        }
        else if (currentTransition == "Reaver") {
            // https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)
            lockedTransition =  vis(Protoss_Robotics_Facility) > 0;
            getOpening =		s < 70;

            firstUnit =			com(Protoss_Robotics_Facility) >= 1 ? (Strategy::needDetection() ? Protoss_Observer : Protoss_Reaver) : UnitTypes::None;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Assimilator] =		Item(s >= 44);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 56);
            itemQueue[Protoss_Robotics_Facility] =	Item(com(Protoss_Dragoon) >= 2);
        }
        else if (currentTransition == "Defensive") {
            lockedTransition =  vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =		s < 100;

            playPassive =		com(Protoss_Gateway) < 5;
            zealotLimit	=		INT_MAX;
            firstUnit =			Protoss_Dark_Templar;            

            itemQueue[Protoss_Assimilator] =        Item(s >= 64);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
            itemQueue[Protoss_Forge] =				Item(1);
            itemQueue[Protoss_Photon_Cannon] =		Item(6 * (com(Protoss_Forge) > 0));
            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));
        }
        else if (currentTransition == "Panic")
            PvP2GateDefensive();
    }

    void PvP1GateCore()
    {
        // https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)
        firstUpgrade =		UpgradeTypes::Singularity_Charge;
        firstTech =			TechTypes::None;
        scout =				Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        gasLimit =			INT_MAX;
               
        // Openers
        if (currentOpener == "1Zealot") {
            zealotLimit = 1;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));
            itemQueue[Protoss_Assimilator] =		Item((addGas() || Strategy::enemyScouted()));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
        }
        else if (currentOpener == "2Zealot") {
            zealotLimit = 2;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));
            itemQueue[Protoss_Assimilator] =		Item((addGas() || Strategy::enemyScouted()));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
        }

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush())
                currentTransition = "Defensive";

            // Change Opener

            // Change Build
        }

        // Transitions
        if (currentTransition == "3GateRobo") {
            lockedTransition =  vis(Protoss_Robotics_Facility) > 0;
            getOpening =        s < 80;
            dragoonLimit =      INT_MAX;

            itemQueue[Protoss_Robotics_Facility] =	Item(s >= 52);
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (2 * addGates()));

            // Decide whether to Reaver first or Obs first
            if (com(Protoss_Robotics_Facility) > 0) {
                if (vis(Protoss_Observer) == 0 && (Units::getEnemyCount(UnitTypes::Protoss_Dragoon) <= 2 || enemyBuild() == "1GateDT"))
                    firstUnit = Protoss_Observer;
                else
                    firstUnit = Protoss_Reaver;
            }
        }
        else if (currentTransition == "Reaver") {
            // http://liquipedia.net/starcraft/1_Gate_Reaver
            lockedTransition =  vis(Protoss_Robotics_Facility) > 0;
            getOpening =		s < 60;
            dragoonLimit =		INT_MAX;
            
            playPassive =		!Strategy::enemyFastExpand() && com(Protoss_Reaver) < 2;
            getOpening =		Strategy::enemyPressure() ? vis(Protoss_Reaver) < 3 : s < 70;
            zealotLimit =		com(Protoss_Robotics_Facility) >= 1 ? 6 : zealotLimit;

            itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 60) + (s >= 62));
            itemQueue[Protoss_Assimilator] =			Item((addGas() || Strategy::enemyScouted()));
            itemQueue[Protoss_Robotics_Facility] =		Item(s >= 52);

            // Decide whether to Reaver first or Obs first
            if (com(Protoss_Robotics_Facility) > 0) {
                if (vis(Protoss_Observer) == 0 && (Units::getEnemyCount(UnitTypes::Protoss_Dragoon) <= 2 || enemyBuild() == "1GateDT"))
                    firstUnit = Protoss_Observer;
                else
                    firstUnit = Protoss_Reaver;
            }
        }
        else if (currentTransition == "4Gate") {
            // https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
            lockedTransition =  vis(Protoss_Gateway) >= 4;
            getOpening = s < 140;

            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 54) + (2 * (s >= 62)));
            itemQueue[Protoss_Assimilator] =		Item(s >= 32);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 34);
        }
        else if (currentTransition == "DT") {
            // https://liquipedia.net/starcraft/2_Gate_DT_(vs._Protoss) -- is actually 1 Gate
            lockedTransition =  vis(Protoss_Citadel_of_Adun) > 0;
        }
        else if (currentTransition == "Defensive")
            PvP2GateDefensive();
    }
}