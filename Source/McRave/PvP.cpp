#include "McRave.h"
#include "BuildOrder.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss {

    namespace {

        bool goonRange() {
            return Broodwar->self()->isUpgrading(UpgradeTypes::Singularity_Charge) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge);
        }

        bool enemyMoreZealots() {
            return com(Protoss_Zealot) <= Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) || Strategy::enemyProxy();
        }

        void defaultPvP() {

            getOpening =            true;
            wallNat =               vis(Protoss_Nexus) >= 2;
            wallMain =              false;
            scout =                 vis(Protoss_Gateway) > 0;
            fastExpand =            false;
            proxy =                 false;
            hideTech =              false;
            playPassive =           false;
            rush =                  false;
            cutWorkers =            false;
            transitionReady =       false;

            gasLimit =              INT_MAX;
            zealotLimit =           1;
            dragoonLimit =          INT_MAX;
            wallDefenseDesired =    0;

            desiredDetection =      Protoss_Observer;
            firstUpgrade =		    vis(Protoss_Dragoon) > 0 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
            firstTech =             TechTypes::None;
        }
    }

    void PvP2GateDefensive() {

        lockedTransition =                              true;

        // Make a tech decision before 3:30
        if (Util::getTime() < Time(3, 30))
            firstUnit =                                 (Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Players::getCurrentCount(PlayerState::Enemy, Protoss_Assimilator) > 0) ? Protoss_Reaver : None;

        gasLimit =			                            (2 * (vis(Protoss_Cybernetics_Core) > 0 && s >= 46)) + (com(Protoss_Cybernetics_Core) > 0);
        getOpening =		                            s < 120;
        playPassive	=		                            enemyMoreZealots() && s < 100 && com(Protoss_Dragoon) < 4;
        firstUpgrade =		                            com(Protoss_Dragoon) >= 2 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        firstTech =			                            TechTypes::None;
        fastExpand =		                            false;
        rush =                                          false;

        zealotLimit =		                            s > 80 ? 0 : INT_MAX;
        dragoonLimit =		                            s > 60 ? INT_MAX : 0;

        desiredDetection =                              Protoss_Forge;
        cutWorkers =                                    Util::getTime() < Time(3, 30) && enemyMoreZealots() && Production::hasIdleProduction();

        itemQueue[Protoss_Nexus] =					    Item(1);
        itemQueue[Protoss_Pylon] =					    Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =				    Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =		        Item(vis(Protoss_Zealot) >= 5);
        itemQueue[Protoss_Cybernetics_Core] =	        Item(vis(Protoss_Assimilator) >= 1);
        itemQueue[Protoss_Shield_Battery] =			    Item(vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);

        if (firstUnit == Protoss_Reaver)
            itemQueue[Protoss_Robotics_Facility] =          Item(isAlmostComplete(Protoss_Cybernetics_Core));
    }

    void PvP2Gate()
    {
        // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)"
        defaultPvP();
        zealotLimit =		                            s <= 80 ? INT_MAX : 0;
        proxy =                                         currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        scout =                                         currentOpener != "Proxy" && startCount >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        rush =                                          Util::getTime() < Time(5, 0) && (Util::getTime() > Time(3, 30) || com(Protoss_Zealot) >= 3);
        transitionReady =                               vis(Protoss_Gateway) >= 2;
        playPassive =                                   (!Strategy::enemyFastExpand() && Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) >= 4 && Broodwar->getFrameCount() < 15000) || (Util::getTime() < Time(3, 30) && com(Protoss_Zealot) < 3);
        desiredDetection =                              Protoss_Forge;

        // Openers
        if (currentOpener == "Proxy") {                 // 9/9            
            itemQueue[Protoss_Pylon] =					Item((s >= 12) + (s >= 26), (s >= 16) + (s >= 26));
            itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
        }
        else if (currentOpener == "Natural") {
            if (startCount >= 3) {                      // 9/10
                itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 26), (s >= 16) + (s >= 26));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0 && s >= 18) + (s >= 20), (s >= 18) + (s >= 20));
            }
            else {                                      // 9/9
                itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 26), (s >= 16) + (s >= 26));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
            }
        }
        else if (currentOpener == "Main") {
            if (startCount >= 3) {                      // 10/12
                itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 32));
                itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24));
            }
            else {                                      // 9/10
                itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 26));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0 && s >= 18) + (s >= 20));
            }
        }

        // Reactions
        if (!lockedTransition) {

            // If enemy is rushing
            if (Strategy::enemyRush() && currentOpener != "Proxy")
                currentTransition = "Defensive";
            else if (Strategy::enemyPressure() && currentOpener == "Natural")
                currentTransition = "Defensive";

            // If we should do a robo transition instead
            else if (Strategy::getEnemyBuild() == "FFE" || Strategy::getEnemyBuild() == "1GateDT")
                currentTransition = "Robo";
            else if (Strategy::getEnemyBuild() == "CannonRush")
                currentTransition = "Robo";

            else if (Strategy::enemyPressure())
                currentTransition = "DT";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "DT") {
                lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
                getOpening =		                        s < 80;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =			                        Protoss_Dark_Templar;

                wallNat =                                   (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
                hideTech =			                        com(Protoss_Dark_Templar) <= 0;

                itemQueue[Protoss_Gateway] =                Item(2 + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Nexus] =				    Item(1);
                itemQueue[Protoss_Assimilator] =		    Item(vis(Protoss_Zealot) >= 5);
                itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Assimilator) >= 1);
                itemQueue[Protoss_Citadel_of_Adun] =        Item(isAlmostComplete(Protoss_Cybernetics_Core));
                itemQueue[Protoss_Templar_Archives] =       Item(isAlmostComplete(Protoss_Citadel_of_Adun));
                itemQueue[Protoss_Forge] =				    Item(s >= 70);

                if (Util::getTime() < Time(10, 0) && vis(Protoss_Nexus) >= 2) {
                    auto cannonCount =                          2 + int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar));
                    cannonCount =                               min(vis(Protoss_Photon_Cannon) + 1, cannonCount);
                    itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
                    wallDefenseDesired =                        cannonCount;
                }
            }
            else if (currentTransition == "Expand") {       // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)#10.2F12_Gateway_Expand"            
                lockedTransition =                          total(Protoss_Nexus) >= 2;
                getOpening =		                        s < 80;
                firstUnit =                                 None;

                wallNat =                                   currentOpener == "Natural" || s >= 56;

                itemQueue[Protoss_Gateway] =                Item(2 + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Assimilator] =		    Item(vis(Protoss_Zealot) >= 5);
                itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Assimilator) >= 1);
                itemQueue[Protoss_Forge] =				    Item(s >= 70);
                itemQueue[Protoss_Nexus] =				    Item(1 + (vis(Protoss_Zealot) >= 3));

                auto cannonCount =                          int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                cannonCount =                               min(vis(Protoss_Photon_Cannon) + 1, cannonCount);
                itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
                wallDefenseDesired =                        cannonCount;
            }
            else if (currentTransition == "Robo") {         // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"            
                lockedTransition =                          total(Protoss_Robotics_Facility) > 0;
                getOpening =		                        s < 130;
                firstUnit =                                 Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) > Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) ? Protoss_Reaver : Protoss_Observer;

                itemQueue[Protoss_Gateway] =                Item(2 + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Nexus] =				    Item(1 + (s >= 130));
                itemQueue[Protoss_Assimilator] =		    Item(vis(Protoss_Zealot) >= 5);
                itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Assimilator) >= 1);
                itemQueue[Protoss_Robotics_Facility] =	    Item(com(Protoss_Dragoon) >= 2);
            }
            else if (currentTransition == "Defensive") {
                PvP2GateDefensive();

                // This was used for natural defensive build
                //lockedTransition =  vis(Protoss_Citadel_of_Adun) > 0;
                //getOpening =		s < 100;

                //playPassive =		com(Protoss_Gateway) < 5 && vis(Protoss_Dark_Templar) == 0;
                //zealotLimit	=		INT_MAX;
                //firstUnit =			Protoss_Dark_Templar;
                //wallNat =           s >= 56 || currentOpener == "Natural";
                //desiredDetection =  Protoss_Forge;

                //itemQueue[Protoss_Assimilator] =        Item(s >= 64);
                //itemQueue[Protoss_Cybernetics_Core] =	Item(s >= 66);
                //itemQueue[Protoss_Forge] =				Item(vis(Protoss_Zealot) >= 4);
                //itemQueue[Protoss_Nexus] =				Item(1 + (s >= 56));

                //auto cannonCount = int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon)) / 2;
                //cannonCount =                               min(vis(Protoss_Photon_Cannon) + 1, cannonCount);
                //itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount * (com(Protoss_Forge) > 0));
                //wallDefenseDesired =                        cannonCount;
            }
        }

        /*if (Terrain::isInAllyTerritory((TilePosition)Strategy::enemyScoutPosition())) {
            itemQueue[Protoss_Citadel_of_Adun] =        Item(0);
            itemQueue[Protoss_Robotics_Facility] =		Item(0);
        }*/
    }

    void PvP1GateCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)"
        defaultPvP();
        rush = !enemyMoreZealots() && Broodwar->getFrameCount() < 6000;

        if (currentOpener == "0Zealot") {               // NZCore
            zealotLimit =                               0;
            scout =				                        vis(Protoss_Pylon) > 0;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 26);
        }
        else if (currentOpener == "1Zealot") {          // ZCore
            zealotLimit =                               s >= 60 ? 0 : 1;
            scout =				                        Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 32));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
        }
        else if (currentOpener == "2Zealot") {          // ZCoreZ
            zealotLimit =                               s >= 60 ? 0 : 1 + (vis(Protoss_Cybernetics_Core) > 0);
            scout =				                        Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 32));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
        }

        // Reactions
        if (!lockedTransition) {

            // If enemy is rushing us
            if (Strategy::enemyRush() && vis(Protoss_Cybernetics_Core) == 0)
                currentTransition = "Defensive";
            else if (Strategy::enemyRush() && vis(Protoss_Cybernetics_Core) > 0)
                currentTransition = "4Gate";

            // If our 4Gate would likely kill us
            else if (Strategy::enemyBlockedScout() && currentTransition == "4Gate")
                currentTransition = "Robo";

            // If we didn't see enemy info by 3:30
            else if (!Terrain::foundEnemy() && Util::getTime() > Time(3, 30))
                currentTransition = "Robo";

            // If we're not doing Robo vs potential DT, switch
            else if (currentTransition != "Robo" && Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) > 0)
                currentTransition = "Robo";

            // If we see a FFE, 4Gate with an expansion
            else if (Strategy::getEnemyBuild() == "FFE")
                currentTransition = "4Gate";
        }

        // Transitions
        if (currentTransition == "Robo") {              // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"   
            firstUnit =                                 !Strategy::needDetection() && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2 ? Protoss_Reaver : Protoss_Observer;
            lockedTransition =                          total(Protoss_Robotics_Facility) > 0;
            getOpening =		                        Terrain::isNarrowNatural() ? Util::getTime() < Time(6, 30) : Util::getTime() < Time(7, 30);
            playPassive =		                        Terrain::isNarrowNatural() ? Util::getTime() < Time(8, 45) : Util::getTime() < Time(8, 00);
            wallNat =                                   Terrain::isNarrowNatural() ? Util::getTime() > Time(4, 30) : Util::getTime() > Time(5, 00);

            itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 58));
            itemQueue[Protoss_Robotics_Facility] =		Item(s >= 50);
            itemQueue[Protoss_Shield_Battery] =         Item((s >= 80) + (s >= 100));
        }
        else if (currentTransition == "3Gate") {        // 
            firstUnit =                                 None;
            lockedTransition =                          total(Protoss_Gateway) >= 3;
            getOpening =		                        Strategy::enemyPressure() ? Broodwar->getFrameCount() < 9000 : Broodwar->getFrameCount() < 8000;
            playPassive =		                        Strategy::enemyPressure() ? Broodwar->getFrameCount() < 13000 : false;
            gasLimit =                                  vis(Protoss_Gateway) >= 2 && com(Protoss_Gateway) < 3 ? 2 : INT_MAX;

            wallNat =                                   Util::getTime() > Time(4, 30);

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (2 * (s >= 58)));
        }
        else if (currentTransition == "4Gate") {        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)" 
            firstUnit =                                 None;
            lockedTransition =                          total(Protoss_Gateway) >= 3;
            getOpening =                                s < 140 && com(Protoss_Dragoon) < Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) + 8;
            playPassive =                               !firstReady();

            desiredDetection =                          Protoss_Forge;

            // If enemy is rushing
            if (Strategy::enemyRush()) {
                zealotLimit =                               s < 60 ? INT_MAX : 0;
                gasLimit =                                  vis(Protoss_Dragoon) > 2 ? 2 : 1;
                playPassive =                               com(Protoss_Dragoon) < 2;
                itemQueue[Protoss_Shield_Battery] =		    Item(enemyMoreZealots() && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
                itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (vis(Protoss_Pylon) >= 3) + (2 * (s >= 62)));
                itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
            }
            else {
                itemQueue[Protoss_Shield_Battery] =		    Item(0);
                itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (s >= 54) + (2 * (s >= 62)));
                itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
            }
        }
        else if (currentTransition == "DT") {           // "https://liquipedia.net/starcraft/2_Gate_DT_(vs._Protoss)"
            firstUnit =                                 Protoss_Dark_Templar;
            lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
            getOpening =		                        s < 90;
            playPassive =		                        Broodwar->getFrameCount() < 13500;

            wallNat =                                   (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
            desiredDetection =                          Protoss_Forge;
            firstUpgrade =                              UpgradeTypes::None;
            hideTech =                                  com(Protoss_Dark_Templar) <= 0;
            zealotLimit =                               vis(Protoss_Photon_Cannon) >= 2 && s < 60 ? INT_MAX : zealotLimit;

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (vis(Protoss_Templar_Archives) > 0));
            itemQueue[Protoss_Forge] =                  Item(s >= 70);
            itemQueue[Protoss_Photon_Cannon] =          Item(2 * (com(Protoss_Forge) > 0));
            itemQueue[Protoss_Citadel_of_Adun] =        Item(vis(Protoss_Dragoon) > 0);
            itemQueue[Protoss_Templar_Archives] =       Item(isAlmostComplete(Protoss_Citadel_of_Adun));

            if (Util::getTime() < Time(10, 0) && vis(Protoss_Nexus) >= 2) {
                auto cannonCount =                          2 + int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                cannonCount =                               min(vis(Protoss_Photon_Cannon) + 1, cannonCount);
                itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
                wallDefenseDesired =                        cannonCount;
            }
        }
        else if (currentTransition == "Defensive") {
            lockedTransition =                         true;

            desiredDetection =  Protoss_Forge;
            PvP2GateDefensive();
        }

        /*if (Terrain::isInAllyTerritory((TilePosition)Strategy::enemyScoutPosition())) {
            itemQueue[Protoss_Citadel_of_Adun] =        Item(0);
            itemQueue[Protoss_Robotics_Facility] =		Item(0);
        }*/
    }
}