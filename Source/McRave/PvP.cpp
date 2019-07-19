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

        void defaultPvP() {
            hideTech =                                  false;
            playPassive =                               false;
            fastExpand =                                false;
            wallNat =                                   vis(Protoss_Nexus) >= 2;
            wallMain =                                  false;
            delayFirstTech =                            false;
            desiredDetection =                          Protoss_Observer;
            firstUnit =                                 None;
            firstUpgrade =		                        vis(Protoss_Dragoon) > 0 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
            firstTech =			                        TechTypes::None;
            scout =				                        vis(Protoss_Gateway) > 0;
            gasLimit =			                        INT_MAX;
            zealotLimit =		                        1;
            dragoonLimit =		                        INT_MAX;
        }
    }

    void PvP2GateDefensive() {

        auto enemyMoreZealots =                         com(Protoss_Zealot) <= Units::getEnemyCount(Protoss_Zealot) || Strategy::enemyProxy();

        lockedTransition =                              true;
        firstUnit =                                     Units::getEnemyCount(Protoss_Dragoon) > 0 || Units::getEnemyCount(Protoss_Assimilator) > 0 ? Protoss_Dark_Templar : Protoss_Reaver;

        gasLimit =			                            (vis(Protoss_Cybernetics_Core) > 0 && s >= 36) ? INT_MAX : 0;
        getOpening =		                            s < 100;
        playPassive	=		                            enemyMoreZealots && s < 100;
        firstUpgrade =		                            UpgradeTypes::None;
        firstTech =			                            TechTypes::None;
        fastExpand =		                            false;

        zealotLimit =		                            !enemyMoreZealots && s > 80 ? 0 : INT_MAX;
        dragoonLimit =		                            s > 80 ? INT_MAX : 0;
        
        desiredDetection =                              Protoss_Forge;
        cutWorkers =                                    enemyMoreZealots && Production::hasIdleProduction();

        itemQueue[Protoss_Nexus] =					    Item(1);
        itemQueue[Protoss_Pylon] =					    Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =				    Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =			    Item(s >= 36);
        itemQueue[Protoss_Shield_Battery] =			    Item(enemyMoreZealots && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
        itemQueue[Protoss_Cybernetics_Core] =		    Item(com(Protoss_Zealot) >= 5);

        if (firstUnit == Protoss_Dark_Templar) {
            itemQueue[Protoss_Citadel_of_Adun] =            Item(isAlmostComplete(Protoss_Cybernetics_Core));
            itemQueue[Protoss_Templar_Archives] =           Item(isAlmostComplete(Protoss_Citadel_of_Adun));
        }
        else if (firstUnit == Protoss_Reaver)
            itemQueue[Protoss_Robotics_Facility] =          Item(isAlmostComplete(Protoss_Cybernetics_Core));
        
    }

    void PvP2Gate()
    {
        // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)"
        defaultPvP();
        zealotLimit =		                            s > 100 ? 0 : INT_MAX;
        proxy =                                         currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        wallNat =                                       !Strategy::enemyPressure() && s >= 80;
        scout =                                         currentOpener != "Proxy" && startCount >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        rush =                                          currentOpener == "Proxy";
        transitionReady =                               vis(Protoss_Gateway) >= 2;

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

            // Change Transition
            if (Strategy::enemyRush() && currentOpener != "Proxy")
                currentTransition = "Defensive";
            else if (Strategy::enemyPressure() && currentOpener == "Natural")
                currentTransition = "Defensive";
            else if (Strategy::getEnemyBuild() == "FFE" || Strategy::getEnemyBuild() == "1GateDT")
                currentTransition = "Robo";
            else if (Strategy::getEnemyBuild() == "CannonRush")
                currentTransition = "Robo";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "DT") {
                lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
                getOpening =		                        vis(Protoss_Dark_Templar) < 2 && s <= 80;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =			                        Protoss_Dark_Templar;

                hideTech =			                        currentOpener == "Main" && com(Protoss_Zealot) < 2;
                desiredDetection =                          Protoss_Forge;

                itemQueue[Protoss_Gateway] =                Item(2 + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Nexus] =				    Item(1);
                itemQueue[Protoss_Assimilator] =		    Item(s >= 50);
                itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Zealot) >= 5);
                itemQueue[Protoss_Citadel_of_Adun] =        Item(isAlmostComplete(Protoss_Cybernetics_Core));
                itemQueue[Protoss_Templar_Archives] =       Item(isAlmostComplete(Protoss_Citadel_of_Adun));
                itemQueue[Protoss_Forge] =				    Item(s >= 70);

                auto cannonCount =                          int(1 + Units::getEnemyCount(Protoss_Zealot) + Units::getEnemyCount(Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
            }
            else if (currentTransition == "Expand") {       // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)#10.2F12_Gateway_Expand"            
                lockedTransition =                          vis(Protoss_Nexus) >= 2;
                getOpening =		                        s < 100;
                firstUnit =                                 None;

                delayFirstTech =                            true;
                wallNat =                                   currentOpener == "Natural" || s >= 56;
                desiredDetection =                          Protoss_Forge;

                itemQueue[Protoss_Gateway] =                Item(2 + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Assimilator] =		    Item(s >= 50);
                itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Zealot) >= 5);
                itemQueue[Protoss_Forge] =				    Item(s >= 70);
                itemQueue[Protoss_Nexus] =				    Item(1 + (vis(Protoss_Zealot) >= 3));

                auto cannonCount =                          int(1 + Units::getEnemyCount(Protoss_Zealot) + Units::getEnemyCount(Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
            }
            else if (currentTransition == "Robo") {         // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"            
                lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
                getOpening =		                        s < 130;
                firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;
                desiredDetection =                          Protoss_Forge;
                playPassive =                               Units::getEnemyCount(Protoss_Dragoon) >= 4 && vis(Protoss_Reaver) < 2;

                itemQueue[Protoss_Gateway] =                Item(2 + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Nexus] =				    Item(1 + (s >= 130));
                itemQueue[Protoss_Assimilator] =		    Item(s >= 50);
                itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Zealot) >= 5);
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

                //auto cannonCount = int(1 + Units::getEnemyCount(Protoss_Zealot) + Units::getEnemyCount(Protoss_Dragoon)) / 2;
                //itemQueue[Protoss_Photon_Cannon] =		Item(cannonCount * (com(Protoss_Forge) > 0));
            }
        }
    }

    void PvP1GateCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)"
        defaultPvP();

        // Openers
        if (currentOpener == "1Zealot") {               // ZCoreZ
            zealotLimit =                               vis(Protoss_Cybernetics_Core) > 0 ? max(2, Units::getEnemyCount(Protoss_Zealot)) : (s < 60);
            scout =				                        Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
        }
        else if (currentOpener == "2Zealot") {          // ZZCore
            zealotLimit =                               2 * (s <= 60);
            scout =				                        vis(Protoss_Gateway) > 0;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 32);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 40);
        }

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush() && vis(Protoss_Cybernetics_Core) == 0)
                currentTransition = "Defensive";
            else if (currentTransition != "Robo" && Strategy::getEnemyBuild() == "1GateDT")
                currentTransition = "3GateRobo";
            else if (Strategy::enemyBlockedScout() && currentTransition == "4Gate")
                currentTransition = "3GateRobo";
            else if (Strategy::getEnemyBuild() == "FFE")
                currentTransition = "4Gate";
        }

        // Transitions
        if (currentTransition == "3GateRobo") {         // "https://liquipedia.net/starcraft/3_Gate_Robo_(vs._Protoss)"
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =		                        Strategy::getEnemyBuild() == "1GateDT" ? com(firstUnit) == 0 : Broodwar->getFrameCount() < 12500;
            playPassive =		                        Strategy::getEnemyBuild() == "1GateDT" ? com(firstUnit) == 0 : Broodwar->getFrameCount() < 13500;

            zealotLimit =		                        vis(Protoss_Robotics_Facility) >= 1 ? 3 : zealotLimit;

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (2 * (s >= 58)));
            itemQueue[Protoss_Robotics_Facility] =	    Item(s >= 52);

            if (!Strategy::enemyFastExpand() && Strategy::enemyPressure())
                itemQueue[Protoss_Shield_Battery] =     Item(vis(Protoss_Robotics_Facility) > 0);
        }
        else if (currentTransition == "Robo") {         // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"   
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =		                        Strategy::getEnemyBuild() == "1GateDT" ? com(firstUnit) == 0 : Broodwar->getFrameCount() < 12500;
            playPassive =		                        Strategy::getEnemyBuild() == "1GateDT" ? com(firstUnit) == 0 : Broodwar->getFrameCount() < 13500;

            zealotLimit =		                        vis(Protoss_Robotics_Facility) >= 1 ? 3 : zealotLimit;

            itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 58));
            itemQueue[Protoss_Robotics_Facility] =		Item(s >= 50);

            if (!Strategy::enemyFastExpand() && Strategy::enemyPressure())
                itemQueue[Protoss_Shield_Battery] =     Item(vis(Protoss_Robotics_Facility) > 0);
        }
        else if (currentTransition == "4Gate") {        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)" 
            firstUnit =                                 None;
            lockedTransition =                          s > 24;
            getOpening =                                s < 140 && Broodwar->getFrameCount() < 10000;
            playPassive =                               !firstReady();

            desiredDetection =                          Protoss_Forge;
            zealotLimit =                               vis(Protoss_Cybernetics_Core) > 0 ? 2 : 1;

            // HACK
            if (Strategy::enemyRush()) {
                auto enemyMoreZealots =                 com(Protoss_Zealot) <= Units::getEnemyCount(Protoss_Zealot);
                zealotLimit =                           INT_MAX;
                gasLimit =                              vis(Protoss_Dragoon) > 2 ? 3 : 1;
                itemQueue[Protoss_Shield_Battery] =		Item(enemyMoreZealots && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
            }

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (s >= 54) + (2 * (s >= 62)));
            itemQueue[Protoss_Assimilator] =		    Item(s >= 32);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
        }
        else if (currentTransition == "DT") {           // "https://liquipedia.net/starcraft/2_Gate_DT_(vs._Protoss)"
            firstUnit =                                 Protoss_Dark_Templar;
            lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =		                        vis(Protoss_Dark_Templar) < 2 && s < 80;
            playPassive =		                        Broodwar->getFrameCount() < 13500;

            desiredDetection =                          Protoss_Forge;
            firstUpgrade =                              UpgradeTypes::None;
            hideTech =                                  true;
            wallNat =                                   s >= 52;
            zealotLimit =                               vis(Protoss_Photon_Cannon) >= 2 ? INT_MAX : zealotLimit;

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (vis(Protoss_Templar_Archives) > 0));
            itemQueue[Protoss_Forge] =                  Item(s >= 70);
            itemQueue[Protoss_Photon_Cannon] =          Item(2 * (com(Protoss_Forge) > 0));
            itemQueue[Protoss_Citadel_of_Adun] =        Item(isAlmostComplete(Protoss_Cybernetics_Core));
            itemQueue[Protoss_Templar_Archives] =       Item(isAlmostComplete(Protoss_Citadel_of_Adun));

            auto cannonCount =                          int(1 + Units::getEnemyCount(Protoss_Zealot) + Units::getEnemyCount(Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
            itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
        }
        else if (currentTransition == "Defensive") {
            lockedTransition =                         true;

            desiredDetection =  Protoss_Forge;
            PvP2GateDefensive();
        }
    }
}