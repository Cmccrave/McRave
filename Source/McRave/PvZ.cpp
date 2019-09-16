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

        void defaultPvZ() {

            getOpening =            true;
            wallNat =               vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
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
            zealotLimit =           INT_MAX;
            dragoonLimit =          0;
            wallDefenseDesired =    0;

            desiredDetection =      Protoss_Observer;
            firstUnit =             None;
            firstUpgrade =		    vis(Protoss_Dragoon) > 0 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
            firstTech =             TechTypes::None;
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
        itemQueue[Protoss_Pylon] =                      Item((s >= 16) + (s >= 30));
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
                + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 6)
                + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

            wallDefenseDesired = cannonCount;

            // TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons        
            if (Strategy::getEnemyBuild() == "2HatchHydra") {
                cannonCount = max(cannonCount, 5);
                wallDefenseDesired = 5;
            }
            else if (Strategy::getEnemyBuild() == "3HatchHydra") {
                cannonCount = max(cannonCount, 4);
                wallDefenseDesired = 4;
            }
            else if (Strategy::getEnemyBuild() == "2HatchMuta" && Util::getTime() > Time(4,0)) {
                cannonCount = max(cannonCount, 8);
                wallDefenseDesired = 3;
            }
            else if (Strategy::getEnemyBuild() == "3HatchMuta" && Util::getTime() > Time(5, 0)) {
                cannonCount = max(cannonCount, 9);
                wallDefenseDesired = 3;
            }
            else if (Strategy::getEnemyBuild() == "4Pool") {
                cannonCount = max(cannonCount, 2 + (s >= 24));
                wallDefenseDesired = 3;
            }
        }

        // Reactions
        if (!lockedTransition) {
            if (Strategy::getEnemyBuild() == "4Pool" && currentOpener != "Forge")
                currentOpener = "Panic";
            if (Strategy::getEnemyBuild() == "2HatchHydra" || Strategy::getEnemyBuild() == "3HatchHydra")
                currentTransition = "StormRush";
            else if (Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta")
                currentTransition = "2Stargate";
        }

        // Openers
        if (currentOpener == "Forge") {
            scout =                                     vis(Protoss_Pylon) > 0;
            transitionReady =                           vis(Protoss_Gateway) >= 1;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 28));
            itemQueue[Protoss_Pylon] =                  Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Assimilator] =            Item(com(Protoss_Forge) > 0);
            itemQueue[Protoss_Gateway] =                Item(s >= 32);
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
            itemQueue[Protoss_Gateway] =                Item(vis(Protoss_Pylon) > 0);
            itemQueue[Protoss_Forge] =                  Item(s >= 60);
        }
        else if (currentOpener == "Panic") {
            scout =                                     com(Protoss_Photon_Cannon) >= 3;
            transitionReady =                           vis(Protoss_Pylon) >= 2;
            cutWorkers =                                vis(Protoss_Pylon) == 1 && vis(Protoss_Probe) >= 13;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item(1 + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) < 6 || s >= 32));
            itemQueue[Protoss_Shield_Battery] =         Item(vis(Protoss_Gateway) > 0, com(Protoss_Gateway) > 0);
            itemQueue[Protoss_Gateway] =                Item(0);
        }

        // Don't add an assimilator if we're being rushed
        if (Strategy::enemyRush())
            itemQueue[Protoss_Assimilator] =            Item(0);

        // Don't mine gas if we need cannons
        if (cannonCount > vis(Protoss_Photon_Cannon) && Broodwar->getFrameCount() < 10000)
            gasLimit = 0;

        // If we want Cannons but have no Forge
        if (cannonCount > 0 && currentOpener != "Panic") {
            if (!isAlmostComplete(Protoss_Forge)) {
                cannonCount =                           0;
                wallDefenseDesired =                    0;
                itemQueue[Protoss_Forge] =              Item(1);
            }
            else
                itemQueue[Protoss_Photon_Cannon] =      Item(min(vis(Protoss_Photon_Cannon) + 1, cannonCount));            
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "StormRush") {
                getOpening =                            s < 100;
                lockedTransition =                      total(Protoss_Citadel_of_Adun) > 0;

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
                lockedTransition =                      total(Protoss_Stargate) >= 2;

                firstUpgrade =                          UpgradeTypes::Protoss_Air_Weapons;
                firstTech =                             TechTypes::None;
                firstUnit =                             Protoss_Corsair;

                itemQueue[Protoss_Assimilator] =        Item((s >= 38) + (s >= 80));
                itemQueue[Protoss_Cybernetics_Core] =   Item(s >= 36);
                itemQueue[Protoss_Citadel_of_Adun] =    Item(0);
                itemQueue[Protoss_Templar_Archives] =   Item(0);
                itemQueue[Protoss_Stargate] =           Item((vis(Protoss_Corsair) > 0) + (isAlmostComplete(Protoss_Cybernetics_Core)));
            }
            else if (currentTransition == "5GateGoon") { // "https://liquipedia.net/starcraft/5_Gate_Ranged_Goons_(vs._Zerg)"
                getOpening =                            s < 160;
                lockedTransition =                      total(Protoss_Gateway) >= 3;

                zealotLimit =                           2;
                dragoonLimit =                          INT_MAX;

                firstUpgrade =                          UpgradeTypes::Singularity_Charge;
                firstTech =                             TechTypes::None;
                firstUnit =                             UnitTypes::None;

                itemQueue[Protoss_Cybernetics_Core] =   Item(s >= 40);
                itemQueue[Protoss_Gateway] =            Item((vis(Protoss_Cybernetics_Core) > 0) + (s >= 76) + (s >= 92) + (s >= 100));
                itemQueue[Protoss_Assimilator] =        Item(1 + (s >= 116));
            }
            else if (currentTransition == "NeoBisu") { // "https://liquipedia.net/starcraft/%2B1_Sair/Speedlot_(vs._Zerg)"
                getOpening =                            s < 100;
                lockedTransition =                      total(Protoss_Citadel_of_Adun) > 0 && total(Protoss_Stargate) > 0;

                firstUpgrade =                          UpgradeTypes::Protoss_Air_Weapons;
                firstTech =                             TechTypes::None;
                firstUnit =                             Protoss_Corsair;

                itemQueue[Protoss_Assimilator] =        Item((s >= 34) + (s >= 60));
                itemQueue[Protoss_Gateway] =            Item(1 + (vis(Protoss_Citadel_of_Adun) > 0));
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
            if (Strategy::enemyRush() && currentOpener != "Proxy")
                currentTransition = "Defensive";
            else if (Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Zerg_Sunken_Colony) >= 2)
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
                firstUnit =                                 None;

                itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42));
                itemQueue[Protoss_Forge] =                  Item(s >= 62);
                itemQueue[Protoss_Cybernetics_Core] =       Item(vis(Protoss_Photon_Cannon) >= 2);

                auto cannonCount =                          2 * (com(Protoss_Forge) > 0);
                cannonCount =                               min(vis(Protoss_Photon_Cannon) + 1, cannonCount);
                itemQueue[Protoss_Photon_Cannon] =          Item(cannonCount);
                wallDefenseDesired =                        cannonCount;
            }
            else if (currentTransition == "4Gate") {        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)"        
                firstUnit =                                 None;
                getOpening =                                s < 120;
                lockedTransition =                          true;
                firstUpgrade =                              UpgradeTypes::Singularity_Charge;
                zealotLimit =                               5;
                dragoonLimit =                              INT_MAX;
                wallNat =                                   vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
                playPassive =                               !firstReady() && (!Terrain::foundEnemy() || Strategy::enemyPressure());

                itemQueue[Protoss_Gateway] =                Item(2 + (s >= 62) + (s >= 70));
                itemQueue[Protoss_Assimilator] =            Item(s >= 52);
                itemQueue[Protoss_Cybernetics_Core] =       Item(vis(Protoss_Zealot) >= 5);
            }
            else if (currentTransition == "Defensive")
                PvZ2GateDefensive();
        }
    }

    void PvZ1GateCore()
    {
        defaultPvZ();
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
        gasLimit =                                      INT_MAX;

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyRush())
                currentTransition = "Defensive";

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
        if (currentTransition == "DT") {                // Experimental build from Best            
            firstUpgrade =                              UpgradeTypes::None;
            firstTech =                                 vis(Protoss_Dark_Templar) >= 2 ? TechTypes::Psionic_Storm : TechTypes::None;
            getOpening =                                s < 70;
            dragoonLimit =                              1;
            lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
            playPassive =                               s < 70;
            firstUnit =                                 Protoss_Dark_Templar;

            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 42));
            itemQueue[Protoss_Citadel_of_Adun] =        Item(s >= 34);
            itemQueue[Protoss_Templar_Archives] =       Item(vis(Protoss_Gateway) >= 2);
        }
        else if (currentTransition == "Defensive")
            PvZ2GateDefensive();
    }
}