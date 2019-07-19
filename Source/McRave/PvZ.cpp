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

        bool mineral(int value) {
            return Broodwar->self()->minerals() >= value;
        }

        bool gas(int value) {
            return Broodwar->self()->gas() >= value;
        }

        void defaultPvZ() {
            hideTech =                                  false;
            playPassive =                               false;
            fastExpand =                                false;
            wallNat =                                   false;
            wallMain =                                  false;

            desiredDetection =                          UnitTypes::Protoss_Observer;
            firstUnit =                                 UnitTypes::None;

            firstUpgrade =                              UpgradeTypes::Protoss_Ground_Weapons;
            firstTech =                                 TechTypes::None;
            scout =                                     vis(Protoss_Pylon) > 0;
            gasLimit =                                  INT_MAX;
            zealotLimit =                               INT_MAX;
            dragoonLimit =                              0;
        }
    }

    void PvZ2GateDefensive() {
        gasLimit =                                      (com(Protoss_Cybernetics_Core) && s >= 50) ? INT_MAX : 0;
        getOpening =                                    vis(Protoss_Corsair) == 0;
        playPassive =                                   s < 60;
        firstUpgrade =                                  UpgradeTypes::None;
        firstTech =                                     TechTypes::None;
        cutWorkers =                                    Production::hasIdleProduction();

        zealotLimit =                                   INT_MAX;
        dragoonLimit =                                  vis(Protoss_Templar_Archives) > 0 ? INT_MAX : 0;
        firstUnit =                                     Protoss_Corsair;

        itemQueue[Protoss_Nexus] =                      Item(1);
        itemQueue[Protoss_Pylon] =                      Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =                    Item((s >= 20) + (vis(Protoss_Zealot) > 0) + (s >= 66));
        itemQueue[Protoss_Assimilator] =                Item(s >= 40);
        itemQueue[Protoss_Cybernetics_Core] =           Item(s >= 58);
        itemQueue[Protoss_Stargate] =                   Item(isAlmostComplete(Protoss_Cybernetics_Core));
    }

    void PvZFFE()
    {
        defaultPvZ();

        fastExpand =                                    true;
        wallNat =                                       true;
        cutWorkers =                                    Strategy::enemyRush() ? vis(Protoss_Photon_Cannon) < 2 : false;

        int cannonCount = 0;

        if (s < 160) {
            cannonCount = int(isAlmostComplete(Protoss_Forge))
                + (Units::getEnemyCount(Zerg_Zergling) >= 6)
                + (Units::getEnemyCount(Zerg_Zergling) >= 12)
                + (Units::getEnemyCount(Zerg_Zergling) >= 24)
                + (Units::getEnemyCount(Zerg_Hydralisk) / 2);

            // TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons        
            if (Strategy::getEnemyBuild() == "2HatchHydra")
                cannonCount = max(cannonCount, 5);
            else if (Strategy::getEnemyBuild() == "3HatchHydra")
                cannonCount = max(cannonCount, 4);
            else if (Strategy::getEnemyBuild() == "2HatchMuta" && Broodwar->getFrameCount() >= 7200)
                cannonCount = max(cannonCount, 3);
            else if (Strategy::getEnemyBuild() == "3HatchMuta" && Broodwar->getFrameCount() >= 7500)
                cannonCount = max(cannonCount, 4);
            else if (Strategy::getEnemyBuild() == "4Pool")
                cannonCount = max(cannonCount, 2 + (s >= 24));

            if (cannonCount > 0) {
                for (auto &[_, station] : Stations::getMyStations()) {
                    if (!Stations::needPower(*station) && Strategy::getEnemyBuild() == "2HatchMuta" && Broodwar->getFrameCount() >= 7200) {
                        cannonCount+=2;
                    }
                    if (!Stations::needPower(*station) && Strategy::getEnemyBuild() == "3HatchMuta" && Broodwar->getFrameCount() >= 7500) {
                        cannonCount+=2;
                    }
                }
            }

            // Only queue one at a time to prevent idle probes
            cannonCount =                                   min(vis(Protoss_Photon_Cannon) + 1, cannonCount);
        }


        // Reactions
        if (!lockedTransition) {

            // Change Opener
            if (Strategy::getEnemyBuild() == "4Pool" && currentOpener != "Forge")
                currentOpener = "Panic";

            // Change Transition
            if (Strategy::getEnemyBuild() == "2HatchHydra" || Strategy::getEnemyBuild() == "3HatchHydra")
                currentTransition = "StormRush";
            else if (Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta")
                currentTransition = "2Stargate";
        }

        // Openers
        if (currentOpener == "Forge") {
            scout =                                     vis(Protoss_Pylon) > 0;
            transitionReady =                           vis(Protoss_Gateway) >= 2;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 28));
            itemQueue[Protoss_Pylon] =                  Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Assimilator] =            Item(com(Protoss_Forge) > 0 && vis(Protoss_Gateway) > 0);
            itemQueue[Protoss_Gateway] =                Item((s >= 32) + (s >= 40));
            itemQueue[Protoss_Forge] =                  Item(s >= 20);
        }
        else if (currentOpener == "Nexus") {
            scout =                                     vis(Protoss_Pylon) > 0;
            transitionReady =                           vis(Protoss_Assimilator) >= 1;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =                  Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Assimilator] =            Item(vis(Protoss_Gateway) >= 1);
            itemQueue[Protoss_Gateway] =                Item(vis(Protoss_Forge) > 0);
            itemQueue[Protoss_Forge] =                  Item(vis(Protoss_Nexus) >= 2);
        }
        else if (currentOpener == "Gate") {
            scout =                                     vis(Protoss_Gateway) > 0;
            transitionReady =                           vis(Protoss_Nexus) >= 2;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42));
            itemQueue[Protoss_Pylon] =                  Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((vis(Protoss_Pylon) > 0) + (s >= 60));
            itemQueue[Protoss_Forge] =                  Item(s >= 60);
        }
        else if (currentOpener == "Panic") {
            scout =                                     com(Protoss_Photon_Cannon) >= 3;
            transitionReady =                           vis(Protoss_Pylon) >= 2;
            cutWorkers =                                vis(Protoss_Pylon) == 1 && vis(Protoss_Probe) >= 13;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item(1 + (Units::getEnemyCount(Zerg_Zergling) < 6 || s >= 32));
            itemQueue[Protoss_Shield_Battery] =         Item(vis(Protoss_Gateway) > 0, com(Protoss_Gateway) > 0);
            itemQueue[Protoss_Gateway] =                Item(0);
        }

        // HACK: Don't add an assimilator if we're being rushed
        if (Strategy::enemyRush())
            itemQueue[Protoss_Assimilator] =            Item(0);

        // If we want Cannons but have no Forge
        if (cannonCount > 0 && currentOpener != "Panic") {
            if (!isAlmostComplete(Protoss_Forge)) {
                cannonCount =                           0;
                itemQueue[Protoss_Forge] =              Item(1);
            }
            else
                itemQueue[Protoss_Photon_Cannon] =      Item(cannonCount);
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "StormRush") {
                getOpening =                            s < 100;
                lockedTransition =                      vis(Protoss_Citadel_of_Adun) > 0;

                firstUpgrade =                          UpgradeTypes::None;
                firstTech =                             TechTypes::Psionic_Storm;
                firstUnit =                             Protoss_High_Templar;

                itemQueue[Protoss_Assimilator] =        Item((s >= 38) + (s >= 60));
                itemQueue[Protoss_Cybernetics_Core] =   Item((s >= 42));
                itemQueue[Protoss_Citadel_of_Adun] =    Item(isAlmostComplete(Protoss_Cybernetics_Core));
                itemQueue[Protoss_Templar_Archives] =   Item(isAlmostComplete(Protoss_Citadel_of_Adun));
                itemQueue[Protoss_Stargate] =           Item(0);
            }
            else if (currentTransition == "2Stargate") {
                getOpening =                            s < 100;
                lockedTransition =                      vis(Protoss_Stargate) >= 2;

                firstUpgrade =                          UpgradeTypes::Protoss_Air_Weapons;
                firstTech =                             TechTypes::None;
                firstUnit =                             Protoss_Corsair;

                itemQueue[Protoss_Assimilator] =        Item((s >= 38) + (s >= 60));
                itemQueue[Protoss_Cybernetics_Core] =   Item(s >= 40);
                itemQueue[Protoss_Citadel_of_Adun] =    Item(0);
                itemQueue[Protoss_Templar_Archives] =   Item(0);
                itemQueue[Protoss_Stargate] =           Item((vis(Protoss_Corsair) > 0) + (isAlmostComplete(Protoss_Cybernetics_Core)));
            }
            else if (currentTransition == "5GateGoon") {
                getOpening =                            s < 160;
                lockedTransition =                      vis(Protoss_Gateway) >= 3;

                zealotLimit =                           2;
                dragoonLimit =                          INT_MAX;

                firstUpgrade =                          UpgradeTypes::Singularity_Charge;
                firstTech =                             TechTypes::None;
                firstUnit =                             UnitTypes::None;

                itemQueue[Protoss_Cybernetics_Core] =   Item(s >= 40);
                itemQueue[Protoss_Gateway] =            Item((vis(Protoss_Cybernetics_Core) > 0) + (s >= 76) + (s >= 92) + (s >= 100));
                itemQueue[Protoss_Assimilator] =        Item(1 + (s >= 116));
            }
            else if (currentTransition == "NeoBisu") {
                getOpening =                            s < 100;
                lockedTransition =                      vis(Protoss_Citadel_of_Adun) > 0 && vis(Protoss_Stargate) > 0;

                firstUpgrade =                          UpgradeTypes::Protoss_Air_Weapons;
                firstTech =                             TechTypes::None;
                firstUnit =                             Protoss_Corsair;

                itemQueue[Protoss_Assimilator] =        Item((s >= 34) + (s >= 60));
                itemQueue[Protoss_Cybernetics_Core] =   Item(s >= 36);
                itemQueue[Protoss_Citadel_of_Adun] =    Item(vis(Protoss_Assimilator) >= 2);
                itemQueue[Protoss_Stargate] =           Item(com(Protoss_Cybernetics_Core) >= 1);
                itemQueue[Protoss_Templar_Archives] =   Item(Broodwar->self()->isUpgrading(UpgradeTypes::Leg_Enhancements) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements));

                if (vis(Protoss_Templar_Archives) > 0) {
                    techList.insert(Protoss_High_Templar);
                    unlockedType.insert(Protoss_High_Templar);
                    techList.insert(Protoss_Dark_Templar);
                    unlockedType.insert(Protoss_Dark_Templar);
                }
            }
        }
    }

    void PvZ2Gate()
    {
        defaultPvZ();
        proxy =                                         currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        wallNat =                                       vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
        scout =                                         currentOpener != "Proxy" && Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        rush =                                          currentOpener == "Proxy";
        transitionReady =                               vis(Protoss_Gateway) >= 2;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush() && currentOpener != "Proxy")
                currentTransition = "Defensive";
            else if (Units::getEnemyCount(UnitTypes::Zerg_Sunken_Colony) >= 2)
                currentTransition = "Expand";
        }

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

        // Builds
        if (transitionReady) {
            if (currentTransition == "Expand") {
                getOpening =                                s < 90;
                lockedTransition =                          vis(Protoss_Nexus) >= 2;
                wallNat =                                   vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";

                itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42));
                itemQueue[Protoss_Forge] =                  Item(s >= 62);
                itemQueue[Protoss_Cybernetics_Core] =       Item(vis(Protoss_Photon_Cannon) >= 2);
                itemQueue[Protoss_Photon_Cannon] =          Item(2 * (com(Protoss_Forge) > 0));
            }
            else if (currentTransition == "Defensive") {
                getOpening =                                s < 80;
                lockedTransition =                          true;
                wallNat =                                   vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
                gasLimit =                                  1;

                itemQueue[Protoss_Nexus] =                  Item(1);
                itemQueue[Protoss_Gateway] =                Item(2 + (s >= 62) + (s >= 70));
                itemQueue[Protoss_Assimilator] =            Item(s >= 64);
                itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 66);
            }
            else if (currentTransition == "4Gate") {
                // https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
                getOpening =                                s < 120;
                lockedTransition =                          true;
                firstUpgrade =                              UpgradeTypes::Singularity_Charge;
                zealotLimit =                               5;
                dragoonLimit =                              INT_MAX;
                wallNat =                                   vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
                playPassive =                               !firstReady() && !Terrain::foundEnemy();

                itemQueue[Protoss_Gateway] =                Item(2 + (s >= 62) + (s >= 70));
                itemQueue[Protoss_Assimilator] =            Item(s >= 52);
                itemQueue[Protoss_Cybernetics_Core] =       Item(vis(Protoss_Zealot) >= 5);
            }
        }
    }

    void PvZ1GateCore()
    {
        defaultPvZ();
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        gasLimit =                                      INT_MAX;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush())
                currentTransition = "Defensive";

            // Change Build
            if (Strategy::enemyPressure() || Strategy::enemyGasSteal()) {
                currentBuild = "2Gate";
                currentOpener = "Main";
                currentTransition = "4Gate";
            }
        }

        // Openers
        if (currentOpener == "1Zealot") {
            zealotLimit =                               vis(Protoss_Cybernetics_Core) > 0 ? INT_MAX : 1;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 32));
            itemQueue[Protoss_Gateway] =                Item(s >= 20);
            itemQueue[Protoss_Assimilator] =            Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 34);
        }
        else if (currentOpener == "2Zealot") {
            zealotLimit =                               vis(Protoss_Cybernetics_Core) > 0 ? INT_MAX : 2;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 24));
            itemQueue[Protoss_Gateway] =                Item(s >= 20);
            itemQueue[Protoss_Assimilator] =            Item(s >= 32);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 40);
        }

        // Builds
        if (currentTransition == "DT") {
            // Experimental build from Best
            firstUpgrade =                              UpgradeTypes::None;
            firstTech =                                 vis(Protoss_Dark_Templar) >= 2 ? TechTypes::Psionic_Storm : TechTypes::None;
            getOpening =                                s < 70;
            dragoonLimit =                              1;
            lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
            playPassive =                               s < 70;
            firstUnit =                                 Protoss_Dark_Templar;

            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 42));
            itemQueue[Protoss_Citadel_of_Adun] =        Item(s >= 34);
            itemQueue[Protoss_Templar_Archives] =       Item(vis(Protoss_Gateway) >= 2);
        }
        else if (currentTransition == "Robo") {
            //getOpening =        s < 70;
            //firstUpgrade =        UpgradeTypes::Protoss_Air_Weapons;
            //firstTech =            TechTypes::None;
            //dragoonLimit =        1;
            //playPassive =        com(Protoss_Stargate) == 0;
            //firstUnit =         Protoss_Reaver;
            //lockedTransition =  vis(Protoss_Stargate) > 0;

            //itemQueue[Protoss_Gateway] =            Item((s >= 20) + vis(Protoss_Reaver) > 0);
        }
        else if (currentTransition == "Defensive")
            PvZ2GateDefensive();
    }
}