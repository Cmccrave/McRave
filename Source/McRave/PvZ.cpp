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

        void defaultPvZ() {
            firstUpgrade =		UpgradeTypes::Protoss_Ground_Weapons;
            firstTech =			TechTypes::None;
            scout =				vis(Protoss_Pylon) > 0;
            gasLimit =			INT_MAX;
            zealotLimit =		INT_MAX;
            dragoonLimit =		0;
            wallNat =           wallNat || vis(Protoss_Nexus) >= 2;
        }
    }

    void PvZ2GateDefensive() {
        gasLimit =			(com(Protoss_Cybernetics_Core) && s >= 50) ? INT_MAX : 0;
        getOpening =		vis(Protoss_Corsair) == 0;
        playPassive	=		s < 60;
        firstUpgrade =		UpgradeTypes::None;
        firstTech =			TechTypes::None;
        fastExpand =		false;
        cutWorkers =        Production::hasIdleProduction();

        zealotLimit =		INT_MAX;
        dragoonLimit =		(vis(Protoss_Templar_Archives) > 0 || Players::vT()) ? INT_MAX : 0;

        if (com(Protoss_Cybernetics_Core) > 0 && techList.find(Protoss_Corsair) == techList.end() && s >= 60)
            firstUnit = Protoss_Corsair;

        itemQueue[Protoss_Nexus] =					Item(1);
        itemQueue[Protoss_Pylon] =					Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =				Item((s >= 20) + (vis(Protoss_Zealot) > 0) + (s >= 66));
        itemQueue[Protoss_Assimilator] =			Item(s >= 40);
        itemQueue[Protoss_Cybernetics_Core] =		Item(s >= 58);
    }

    void PvZFFE()
    {
        fastExpand = true;
        wallNat = true;
        defaultPvZ();

        auto min100 = Broodwar->self()->minerals() >= 100;
        auto cannonCount = int(com(Protoss_Forge) > 0) + (Units::getEnemyCount(Zerg_Zergling) >= 6) + (Units::getEnemyCount(Zerg_Zergling) >= 12) + (Units::getEnemyCount(Zerg_Zergling) >= 24);

        // TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons		
        if (enemyBuild() == "2HatchHydra")
            cannonCount = 5;
        else if (enemyBuild() == "3HatchHydra")
            cannonCount = 4;
        else if (enemyBuild() == "2HatchMuta")
            cannonCount = 7;
        else if (enemyBuild() == "3HatchMuta")
            cannonCount = 8;

        // Reactions - Defensive / StormRush / DoubleStargate
        if (!lockedTransition) {
            if ((enemyBuild() == "Unknown" && !Terrain::getEnemyStartingPosition().isValid()) || enemyBuild() == "9Pool")
                false;// currentTransition = "Defensive";
            else if (enemyBuild() == "5Pool" || enemyBuild() == "4Pool")
                currentTransition =	"Defensive";
            else if (enemyBuild() == "2HatchHydra" || enemyBuild() == "3HatchHydra")
                currentTransition =	"StormRush";
            else if (enemyBuild() == "2HatchMuta" || enemyBuild() == "3HatchMuta")
                currentTransition =	"DoubleStargate";
        }

        // Openers - Forge / Nexus / Gate
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

        // If we want Cannons but have no Forge
        if (cannonCount > 0 && com(Protoss_Forge) == 0) {
            cannonCount = 0;
            itemQueue[Protoss_Forge] = Item(1);
        }

        // Transitions - StormRush / DoubleStargate / NeoBisu
        if (currentTransition == "StormRush") {
            firstUpgrade =		UpgradeTypes::None;
            firstTech =			TechTypes::Psionic_Storm;
            firstUnit =         Protoss_High_Templar;
            lockedTransition =  com(Protoss_Cybernetics_Core) > 0;

            itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount);
            itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 60));
            itemQueue[Protoss_Cybernetics_Core] =	Item((s >= 42));
            itemQueue[Protoss_Stargate] =           Item(0);
        }
        else if (currentTransition == "DoubleStargate") {
            firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
            firstTech =			TechTypes::None;
            firstUnit =         Protoss_Corsair;
            lockedTransition =  vis(Protoss_Stargate) >= 2;

            itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount);
            itemQueue[Protoss_Assimilator] =		Item((s >= 38) + (s >= 60));
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 40);
            itemQueue[Protoss_Citadel_of_Adun] =	Item(0);
            itemQueue[Protoss_Templar_Archives] =	Item(0);
            itemQueue[Protoss_Stargate] =			Item((vis(Protoss_Corsair) > 0) + (vis(Protoss_Cybernetics_Core) > 0));
        }
        else {
            getOpening =		s < 100;
            currentTransition =	"NeoBisu";
            firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
            firstUnit =         Protoss_Corsair;

            itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount);
            itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Zealot) >= 1);
            itemQueue[Protoss_Citadel_of_Adun] =	Item(vis(Protoss_Assimilator) >= 2);
            itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) >= 1);
            itemQueue[Protoss_Templar_Archives] =	Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));
        }
    }

    void PvZ2Gate()
    {
        defaultPvZ();
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

        // Transitions
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

        // Builds
        if (currentTransition == "Expand") {
            getOpening =		s < 80;
            wallNat =           currentOpener == "Natural" ? true : s >= 40;

            itemQueue[Protoss_Assimilator] =		Item(s >= 76);
            itemQueue[Protoss_Nexus] =				Item(1 + (s >= 42));
            itemQueue[Protoss_Forge] =				Item(s >= 62);
            itemQueue[Protoss_Cybernetics_Core] =	Item(vis(Protoss_Photon_Cannon) >= 2);
            itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
        }
        else if (currentTransition == "Defensive") {
            getOpening =		s < 80;

            itemQueue[Protoss_Forge] =				Item(1);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 70);
            itemQueue[Protoss_Photon_Cannon] =		Item(2 * (com(Protoss_Forge) > 0));
        }
        else if (currentTransition == "Panic") {
            getOpening =		s < 80;

            itemQueue[Protoss_Nexus] =				Item(1);
            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
            itemQueue[Protoss_Assimilator] =		Item(s >= 64);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
            gasLimit = 1;
        }
        else if (currentTransition == "4Gate") {
            // https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
            firstUpgrade =      UpgradeTypes::Singularity_Charge;
            lockedTransition =  true;
            dragoonLimit =      INT_MAX;
            getOpening =        s < 120;
            wallNat =           currentOpener == "Natural" ? true : s >= 120;

            itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24) + (s >= 62) + (s >= 70));
            itemQueue[Protoss_Assimilator] =		Item(s >= 44);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 50);
        }
    }

    void PvZ1GateCore()
    {
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

        // Transitions
        if (!lockedTransition) {
            if (Strategy::enemyRush())
                currentTransition = "Defensive";
        }

        // Builds
        if (currentTransition == "Corsair") {
            getOpening =		s < 60;
            firstUpgrade =		UpgradeTypes::Protoss_Air_Weapons;
            firstTech =			TechTypes::None;
            dragoonLimit =		0;
            zealotLimit	=		INT_MAX;
            playPassive =		com(Protoss_Stargate) == 0;
            firstUnit =         Protoss_Corsair;            

            itemQueue[Protoss_Gateway] =			Item((s >= 18) + vis(Protoss_Stargate) > 0);
            itemQueue[Protoss_Forge] =				Item(vis(Protoss_Gateway) >= 2);
            itemQueue[Protoss_Assimilator] =		Item(s >= 40);
            itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 36);
            itemQueue[Protoss_Stargate] =			Item(com(Protoss_Cybernetics_Core) > 0);
        }
        else if (currentTransition == "DT") {
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
        else if (currentTransition == "Defensive")
            PvZ2GateDefensive();
    }
}