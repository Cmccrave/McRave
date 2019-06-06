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

            firstUpgrade =		                        UpgradeTypes::Singularity_Charge;
            firstTech =			                        TechTypes::None;
            scout =				                        vis(Protoss_Gateway) > 0;
            gasLimit =			                        INT_MAX;
            zealotLimit =		                        1;
            dragoonLimit =		                        INT_MAX;
        }
    }

    void PvP2GateDefensive() {

        auto enemyMoreZealots =                         com(Protoss_Zealot) <= Units::getEnemyCount(Protoss_Zealot);

        gasLimit =			                            (com(Protoss_Cybernetics_Core) > 0 && s >= 36) ? INT_MAX : 0;
        getOpening =		                            com(Protoss_Dark_Templar) <= 2 && s < 80;
        playPassive	=		                            com(Protoss_Dark_Templar) <= 2 && enemyMoreZealots;
        firstUpgrade =		                            UpgradeTypes::None;
        firstTech =			                            TechTypes::None;
        fastExpand =		                            false;

        zealotLimit =		                            INT_MAX;
        dragoonLimit =		                            vis(Protoss_Dark_Templar) > 0 ? INT_MAX : 0;
        firstUnit =                                     Protoss_Dark_Templar;
        desiredDetection =                              Protoss_Forge;
        cutWorkers =                                    Production::hasIdleProduction();

        itemQueue[Protoss_Nexus] =					    Item(1);
        itemQueue[Protoss_Pylon] =					    Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =				    Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =			    Item(s >= 36);
        itemQueue[Protoss_Shield_Battery] =			    Item(enemyMoreZealots && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
        itemQueue[Protoss_Cybernetics_Core] =		    Item(com(Protoss_Zealot) >= 5);
    }

    void PvP2Gate()
    {
        defaultPvP();
        zealotLimit =                                   5;
        proxy =                                         currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        wallNat =                                       vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
        scout =                                         currentOpener != "Proxy" && Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        cutWorkers =                                    Production::hasIdleProduction() && s <= 60;
        rush =                                          currentOpener == "Proxy";

        // Openers
        if (currentOpener == "Proxy") {
            // 9/9
            itemQueue[Protoss_Pylon] =					Item((s >= 12) + (s >= 26), (s >= 16) + (s >= 26));
            itemQueue[Protoss_Gateway] =				Item((vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
        }
        else if (currentOpener == "Natural") {
            // 9/10
            if (Broodwar->getStartLocations().size() >= 3) {
                itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 26), (s >= 16) + (s >= 26));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0 && s >= 18) + (s >= 20), (s >= 18) + (s >= 20));
            }
            // 9/9
            else {
                itemQueue[Protoss_Pylon] =				Item((s >= 14) + (s >= 26), (s >= 16) + (s >= 26));
                itemQueue[Protoss_Gateway] =			Item((vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
            }
        }
        else if (currentOpener == "Main") {
            // 10/12
            if (Broodwar->getStartLocations().size() >= 3) {
                itemQueue[Protoss_Pylon] =				Item((s >= 16) + (s >= 30));
                itemQueue[Protoss_Gateway] =			Item((s >= 20) + (s >= 24));
            }
            // 9/10
            else {
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
        if (currentTransition == "DT") {
            lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =		                        s < 80;
            firstUpgrade =                              UpgradeTypes::None;
            firstUnit =			                        Protoss_Dark_Templar;

            zealotLimit =                               INT_MAX;
            hideTech =			                        currentOpener == "Main" && com(Protoss_Zealot) < 2;
            desiredDetection =                          Protoss_Forge;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 44);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 56);
        }
        else if (currentTransition == "Expand") {
            // https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)#10.2F12_Gateway_Expand
            lockedTransition =                          vis(Protoss_Nexus) >= 2;
            getOpening =		                        s < 100;
            zealotLimit =                               INT_MAX;
            firstUnit =                                 None;

            delayFirstTech =                            true;
            wallNat =                                   currentOpener == "Natural" || s >= 56;
            desiredDetection =                          Protoss_Forge;

            itemQueue[Protoss_Assimilator] =		    Item(s >= 44);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(vis(Protoss_Zealot) >= 5);
            itemQueue[Protoss_Forge] =				    Item(s >= 70);
            itemQueue[Protoss_Nexus] =				    Item(1 + (s >= 50));

            auto cannonCount =                          int(1 + Units::getEnemyCount(Protoss_Zealot) + Units::getEnemyCount(Protoss_Dragoon)) / 2;
            itemQueue[Protoss_Photon_Cannon] =		    Item(cannonCount * (com(Protoss_Forge) > 0));
        }
        else if (currentTransition == "Robo") {
            // https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =		                        s < 70;
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;
            desiredDetection =                          Protoss_Forge;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 44);
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

    void PvP1GateCore()
    {
        // https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)
        defaultPvP();

        firstUpgrade =		                            vis(Protoss_Dragoon) > 0 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        firstTech =			                            TechTypes::None;
        scout =				                            Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        gasLimit =			                            INT_MAX;

        // Openers
        if (currentOpener == "1Zealot") {
            zealotLimit =                               vis(Protoss_Cybernetics_Core) > 0 ? max(2, Units::getEnemyCount(Protoss_Zealot)) : 1;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 34);
        }
        else if (currentOpener == "2Zealot") {
            zealotLimit =                               2;

            itemQueue[Protoss_Nexus] =				    Item(1);
            itemQueue[Protoss_Pylon] =				    Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =			    Item(s >= 20);
            itemQueue[Protoss_Assimilator] =		    Item(s >= 32);
            itemQueue[Protoss_Cybernetics_Core] =	    Item(s >= 40);
        }

        if (s >= 60)
            zealotLimit = 0;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush())
                currentTransition = vis(Protoss_Cybernetics_Core) ? "4Gate" : "Defensive";
            else if (Strategy::getEnemyBuild() == "1GateDT" || Strategy::getEnemyBuild() == "FFE")
                currentTransition = "3GateRobo";
        }

        // Transitions
        if (currentTransition == "3GateRobo") {
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =                                s < 80;
            playPassive =		                        !Strategy::enemyFastExpand() && com(Protoss_Reaver) == 0;
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;

            itemQueue[Protoss_Robotics_Facility] =	    Item(s >= 52);
            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (2 * (s >= 58)));
        }
        else if (currentTransition == "Robo") {
            // http://liquipedia.net/starcraft/1_Gate_Reaver
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =		                        Strategy::enemyPressure() ? vis(Protoss_Reaver) < 3 : s < 70;
            playPassive =		                        !Strategy::enemyFastExpand() && com(Protoss_Reaver) == 0;
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;

            dragoonLimit =		                        INT_MAX;
            zealotLimit =		                        com(Protoss_Robotics_Facility) >= 1 ? 6 : zealotLimit;

            itemQueue[Protoss_Gateway] =				Item((s >= 20) + (s >= 50) + (s >= 62));
            itemQueue[Protoss_Robotics_Facility] =		Item(s >= 70);
        }
        else if (currentTransition == "4Gate") {
            // https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
            lockedTransition =                          s > 24;
            getOpening =                                s < 140 && Broodwar->getFrameCount() < 10000;
            playPassive =                               !firstReady();
            firstUnit =                                 None;

            desiredDetection =                          Protoss_Forge;
            gasLimit =                                  INT_MAX;
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
        else if (currentTransition == "DT") {
            // https://liquipedia.net/starcraft/2_Gate_DT_(vs._Protoss) -- is actually 1 Gate
            lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =                                s <= 52;
            playPassive =                               s <= 52;
            firstUnit =                                 Protoss_Dark_Templar;

            desiredDetection =                          Protoss_Forge;
            firstUpgrade =                              UpgradeTypes::None;
            hideTech =                                  true;
            wallNat =                                   s >= 52;

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (vis(Protoss_Templar_Archives) > 0));
            itemQueue[Protoss_Forge] =                  Item(s >= 70);
            itemQueue[Protoss_Photon_Cannon] =          Item(2 * (com(Protoss_Forge) > 0));
        }
        else if (currentTransition == "Defensive") {
            lockedTransition =                         true;

            desiredDetection =  Protoss_Forge;
            PvP2GateDefensive();
        }
    }
}