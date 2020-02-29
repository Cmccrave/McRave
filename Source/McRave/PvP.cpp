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

        bool enemyMaybeDT() {
            return (Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) > 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) <= 2) || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 3 || Strategy::needDetection();
        }

        void defaultPvP() {

            inOpeningBook =                                 true;
            wallNat =                                       vis(Protoss_Nexus) >= 2;
            wallMain =                                      false;
            scout =                                         vis(Protoss_Gateway) > 0;
            fastExpand =                                    false;
            proxy =                                         false;
            hideTech =                                      false;
            playPassive =                                   false;
            rush =                                          false;
            cutWorkers =                                    false;
            transitionReady =                               false;

            gasLimit =                                      INT_MAX;
            zealotLimit =                                   1;
            dragoonLimit =                                  INT_MAX;

            desiredDetection =                              Protoss_Observer;
            firstUpgrade =                                  vis(Protoss_Dragoon) > 0 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
            firstTech =                                     TechTypes::None;

            armyComposition[Protoss_Zealot] =               0.10;
            armyComposition[Protoss_Dragoon] =              0.90;
        }
    }

    void PvP2GateDefensive() {

        lockedTransition =                                  true;

        // Make a tech decision before 3:30
        if (Util::getTime() < Time(3, 30))
            firstUnit =                                     (Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Players::getCurrentCount(PlayerState::Enemy, Protoss_Assimilator) > 0) ? Protoss_Reaver : None;

        gasLimit =                                          (2 * (vis(Protoss_Cybernetics_Core) > 0 && s >= 46)) + (com(Protoss_Cybernetics_Core) > 0);
        inOpeningBook =                                     s < 100;
        playPassive    =                                    enemyMoreZealots() && s < 100 && com(Protoss_Dragoon) < 4;
        firstUpgrade =                                      com(Protoss_Dragoon) >= 2 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        firstTech =                                         TechTypes::None;
        fastExpand =                                        false;
        rush =                                              false;

        zealotLimit =                                       s > 80 ? 0 : INT_MAX;
        dragoonLimit =                                      s > 60 ? INT_MAX : 0;

        desiredDetection =                                  Protoss_Forge;
        cutWorkers =                                        Util::getTime() < Time(3, 30) && enemyMoreZealots() && Production::hasIdleProduction();

        buildQueue[Protoss_Nexus] =                         1;
        buildQueue[Protoss_Pylon] =                         (s >= 14) + (s >= 30), (s >= 16) + (s >= 30);
        buildQueue[Protoss_Gateway] =                       (s >= 20) + (s >= 24) + (s >= 66);
        buildQueue[Protoss_Assimilator] =                   vis(Protoss_Zealot) >= 5;
        buildQueue[Protoss_Cybernetics_Core] =              vis(Protoss_Assimilator) >= 1;
        buildQueue[Protoss_Shield_Battery] =                vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;

        if (firstUnit == Protoss_Reaver)
            buildQueue[Protoss_Robotics_Facility] =         com(Protoss_Dragoon) >= 2;

        // Army Composition
        armyComposition[Protoss_Zealot] =                   0.05;
        armyComposition[Protoss_Reaver] =                   0.10;
        armyComposition[Protoss_Observer] =                 0.05;
        armyComposition[Protoss_Shuttle] =                  0.05;
        armyComposition[Protoss_Dragoon] =                  0.75;
    }

    void PvP2Gate()
    {
        // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)"
        defaultPvP();
        zealotLimit =                                       s <= 80 ? INT_MAX : 0;
        proxy =                                             currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        scout =                                             currentOpener != "Proxy" && startCount >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        rush =                                              !Strategy::enemyRush() && Util::getTime() < Time(5, 0) && (Util::getTime() > Time(3, 30) || com(Protoss_Zealot) >= 3);
        transitionReady =                                   vis(Protoss_Gateway) >= 2;
        playPassive =                                       (!Strategy::enemyFastExpand() && Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) >= 4 && Broodwar->getFrameCount() < 15000) || (Util::getTime() < Time(3, 30) && com(Protoss_Zealot) < 3);
        desiredDetection =                                  Protoss_Forge;
        gasLimit =                                          vis(Protoss_Cybernetics_Core) > 0 ? INT_MAX : 0;

        if (Strategy::enemyRush())
            buildQueue[Protoss_Shield_Battery] =            vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;

        // Reactions
        if (!lockedTransition) {

            // If we should do a robo transition instead
            if (Strategy::getEnemyBuild() == "FFE" || Strategy::getEnemyBuild() == "1GateDT")
                currentTransition = "Robo";
            else if (Strategy::getEnemyBuild() == "CannonRush")
                currentTransition = "Robo";
            else if (Strategy::enemyPressure())
                currentTransition = "DT";
        }

        // Openers
        if (currentOpener == "Proxy") {                     // 9/9            
            buildQueue[Protoss_Pylon] =                     (s >= 12) + (s >= 26);
            buildQueue[Protoss_Gateway] =                   (vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0);
        }
        else if (currentOpener == "Natural") {
            if (startCount >= 3) {                          // 9/10
                buildQueue[Protoss_Pylon] =                 (s >= 14) + (s >= 26);
                buildQueue[Protoss_Gateway] =               (vis(Protoss_Pylon) > 0 && s >= 18) + (s >= 20);
            }
            else {                                          // 9/9
                buildQueue[Protoss_Pylon] =                 (s >= 14) + (s >= 26);
                buildQueue[Protoss_Gateway] =               (vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0);
            }
        }
        else if (currentOpener == "Main") {
            if (startCount >= 3) {                          // 10/12
                buildQueue[Protoss_Pylon] =                 (s >= 16) + (s >= 32);
                buildQueue[Protoss_Gateway] =               (s >= 20) + (s >= 24);
            }
            else {                                          // 9/10
                buildQueue[Protoss_Pylon] =                 (s >= 16) + (s >= 26);
                buildQueue[Protoss_Gateway] =               (vis(Protoss_Pylon) > 0 && s >= 18) + (s >= 20);
            }
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "DT") {
                lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
                inOpeningBook =                             s < 80;
                firstUpgrade =                              UpgradeTypes::None;
                firstUnit =                                 Protoss_Dark_Templar;
                wallNat =                                   (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
                hideTech =                                  com(Protoss_Dark_Templar) <= 0;

                // Build
                buildQueue[Protoss_Gateway] =               2 + (com(Protoss_Cybernetics_Core) > 0);
                buildQueue[Protoss_Nexus] =                 1;
                buildQueue[Protoss_Assimilator] =           total(Protoss_Zealot) >= 5;
                buildQueue[Protoss_Cybernetics_Core] =      (total(Protoss_Zealot) >= 3 && vis(Protoss_Assimilator) >= 1);
                buildQueue[Protoss_Citadel_of_Adun] =       atPercent(Protoss_Cybernetics_Core, 1.00);
                buildQueue[Protoss_Templar_Archives] =      atPercent(Protoss_Citadel_of_Adun, 1.00);
                buildQueue[Protoss_Forge] =                 s >= 70;

                // Army Composition
                armyComposition[Protoss_Zealot] =           0.05;
                armyComposition[Protoss_Dark_Templar] =     0.05;
                armyComposition[Protoss_Dragoon] =          0.90;
            }
            else if (currentTransition == "Expand") {       // "https://liquipedia.net/starcraft/2_Gate_(vs._Protoss)#10.2F12_Gateway_Expand"            
                lockedTransition =                          total(Protoss_Nexus) >= 2;
                inOpeningBook =                             s < 80;
                firstUnit =                                 None;
                wallNat =                                   currentOpener == "Natural" || s >= 56;

                // Build
                buildQueue[Protoss_Gateway] =               2 + (com(Protoss_Cybernetics_Core) > 0);
                buildQueue[Protoss_Assimilator] =           total(Protoss_Zealot) >= 5;
                buildQueue[Protoss_Cybernetics_Core] =      (total(Protoss_Zealot) >= 3 && vis(Protoss_Assimilator) >= 1);
                buildQueue[Protoss_Forge] =                 s >= 70;
                buildQueue[Protoss_Nexus] =                 1 + (vis(Protoss_Zealot) >= 3);

                // Army Composition
                armyComposition[Protoss_Zealot] =           0.10;
                armyComposition[Protoss_Dragoon] =          0.90;
            }
            else if (currentTransition == "Robo") {         // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"            
                lockedTransition =                          total(Protoss_Robotics_Facility) > 0;
                inOpeningBook =                             s < 130;
                firstUnit =                                 enemyMaybeDT() ? Protoss_Observer : Protoss_Reaver;

                // Build
                buildQueue[Protoss_Gateway] =               2 + (com(Protoss_Cybernetics_Core) > 0);
                buildQueue[Protoss_Nexus] =                 1 + (s >= 130);
                buildQueue[Protoss_Assimilator] =           total(Protoss_Zealot) >= 5;
                buildQueue[Protoss_Cybernetics_Core] =      (total(Protoss_Zealot) >= 3 && vis(Protoss_Assimilator) >= 1);
                buildQueue[Protoss_Robotics_Facility] =     com(Protoss_Dragoon) >= 2;

                // Army Composition
                armyComposition[Protoss_Zealot] =           0.05;
                armyComposition[Protoss_Reaver] =           0.10;
                armyComposition[Protoss_Observer] =         0.05;
                armyComposition[Protoss_Shuttle] =          0.05;
                armyComposition[Protoss_Dragoon] =          0.75;
            }
        }
    }

    void PvP1GateCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Protoss)"
        defaultPvP();
        rush = !enemyMoreZealots() && com(Protoss_Dragoon) == 0;

        // Reactions
        if (!lockedTransition) {

            // If enemy is rushing us
            if (Strategy::enemyRush() && vis(Protoss_Cybernetics_Core) == 0) {
                currentBuild = "2Gate";
                currentOpener = "Main";
                currentTransition = "Robo";
            }
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
                currentTransition = "3Gate";
        }

        // Openers
        if (currentOpener == "0Zealot") {                   // NZCore
            zealotLimit =                                   0;
            scout =                                         vis(Protoss_Pylon) > 0;
            transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

            buildQueue[Protoss_Nexus] =                     1;
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 30);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 26;
        }
        else if (currentOpener == "1Zealot") {              // ZCore
            zealotLimit =                                   s >= 60 ? 0 : 1;
            scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
            transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

            buildQueue[Protoss_Nexus] =                     1;
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 34;
        }
        else if (currentOpener == "2Zealot") {              // ZCoreZ
            zealotLimit =                                   s >= 60 ? 0 : 1 + (vis(Protoss_Cybernetics_Core) > 0);
            scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;
            transitionReady =                               vis(Protoss_Cybernetics_Core) > 0;

            buildQueue[Protoss_Nexus] =                     1;
            buildQueue[Protoss_Pylon] =                     (s >= 16) + (s >= 32);
            buildQueue[Protoss_Gateway] =                   s >= 20;
            buildQueue[Protoss_Assimilator] =               s >= 24;
            buildQueue[Protoss_Cybernetics_Core] =          s >= 34;
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "Robo") {              // "https://liquipedia.net/starcraft/2_Gate_Reaver_(vs._Protoss)"   
                firstUnit =                                 enemyMaybeDT() ? Protoss_Observer : Protoss_Reaver;
                lockedTransition =                          total(Protoss_Robotics_Facility) > 0;
                inOpeningBook =                             Terrain::isNarrowNatural() ? Util::getTime() < Time(6, 30) : Util::getTime() < Time(7, 30);
                playPassive =                               Terrain::isNarrowNatural() ? Util::getTime() < Time(8, 45) : Util::getTime() < Time(8, 00);
                wallNat =                                   Terrain::isNarrowNatural() ? Util::getTime() > Time(4, 30) : Util::getTime() > Time(5, 00);

                // Build
                buildQueue[Protoss_Gateway] =               (s >= 20) + (vis(Protoss_Robotics_Facility) > 0);
                buildQueue[Protoss_Robotics_Facility] =     s >= 50;
                buildQueue[Protoss_Shield_Battery] =        (s >= 80) + (s >= 100);

                // Army Composition
                armyComposition[Protoss_Zealot] =           0.05;
                armyComposition[Protoss_Reaver] =           0.10;
                armyComposition[Protoss_Observer] =         0.05;
                armyComposition[Protoss_Shuttle] =          0.05;
                armyComposition[Protoss_Dragoon] =          0.75;
            }
            else if (currentTransition == "3Gate") {        // -nolink-
                firstUnit =                                 None;
                lockedTransition =                          total(Protoss_Gateway) >= 3;
                inOpeningBook =                             Strategy::enemyPressure() ? Broodwar->getFrameCount() < 9000 : Broodwar->getFrameCount() < 8000;
                playPassive =                               Strategy::enemyPressure() ? Broodwar->getFrameCount() < 13000 : false;
                gasLimit =                                  vis(Protoss_Gateway) >= 2 && com(Protoss_Gateway) < 3 ? 2 : INT_MAX;
                wallNat =                                   Util::getTime() > Time(4, 30);

                // Build
                buildQueue[Protoss_Gateway] =               (s >= 20) + (2 * (s >= 58));

                // Army Composition
                armyComposition[Protoss_Zealot] = 0.05;
                armyComposition[Protoss_Dragoon] = 0.95;
            }
            else if (currentTransition == "4Gate") {        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)" 
                firstUnit =                                 None;
                lockedTransition =                          total(Protoss_Gateway) >= 3;
                inOpeningBook =                             s < 140 && com(Protoss_Dragoon) < Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) + 8;
                playPassive =                               !firstReady();
                desiredDetection =                          Protoss_Forge;

                // Build
                if (Strategy::enemyRush()) {
                    zealotLimit =                           s < 60 ? INT_MAX : 0;
                    gasLimit =                              vis(Protoss_Dragoon) > 2 ? 2 : 1;
                    playPassive =                           com(Protoss_Dragoon) < 2;
                    buildQueue[Protoss_Shield_Battery] =    enemyMoreZealots() && vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2;
                    buildQueue[Protoss_Gateway] =           (s >= 20) + (vis(Protoss_Pylon) >= 3) + (2 * (s >= 62));
                    buildQueue[Protoss_Cybernetics_Core] =  s >= 34;
                }
                else {
                    buildQueue[Protoss_Shield_Battery] =    0;
                    buildQueue[Protoss_Gateway] =           (s >= 20) + (s >= 54) + (2 * (s >= 62));
                    buildQueue[Protoss_Cybernetics_Core] =  s >= 34;
                }

                // Army Composition
                armyComposition[Protoss_Zealot] = 0.05;
                armyComposition[Protoss_Dragoon] = 0.95;
            }
            else if (currentTransition == "DT") {           // "https://liquipedia.net/starcraft/2_Gate_DT_(vs._Protoss)"
                firstUnit =                                 Protoss_Dark_Templar;
                lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
                inOpeningBook =                             s < 90;
                playPassive =                               Broodwar->getFrameCount() < 13500;
                wallNat =                                   (com(Protoss_Forge) > 0 && com(Protoss_Dark_Templar) > 0);
                desiredDetection =                          Protoss_Forge;
                firstUpgrade =                              UpgradeTypes::None;
                hideTech =                                  com(Protoss_Dark_Templar) <= 0;
                zealotLimit =                               vis(Protoss_Photon_Cannon) >= 2 && s < 60 ? INT_MAX : zealotLimit;

                // Build
                buildQueue[Protoss_Gateway] =               (s >= 20) + (vis(Protoss_Templar_Archives) > 0);
                buildQueue[Protoss_Forge] =                 s >= 70;
                buildQueue[Protoss_Photon_Cannon] =         2 * (com(Protoss_Forge) > 0);
                buildQueue[Protoss_Citadel_of_Adun] =       vis(Protoss_Dragoon) > 0;
                buildQueue[Protoss_Templar_Archives] =      atPercent(Protoss_Citadel_of_Adun, 1.00);

                // Army Composition
                armyComposition[Protoss_Zealot] =           0.05;
                armyComposition[Protoss_Dark_Templar] =     0.05;
                armyComposition[Protoss_Dragoon] =          0.90;
            }
        }
    }
}