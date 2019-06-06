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

        void defaultPvT() {
            hideTech =          false;
            playPassive =       false;
            fastExpand =        false;
            wallMain =          false;
            wallNat =           vis(Protoss_Nexus) >= 2;

            desiredDetection =  Protoss_Observer;
            firstUnit =         None;

            firstUpgrade =      UpgradeTypes::Singularity_Charge;
            firstTech =         TechTypes::None;
            scout =             vis(Protoss_Cybernetics_Core) > 0;
            gasLimit =          INT_MAX;
            zealotLimit =       0;
            dragoonLimit =      INT_MAX;
        }
    }

    void PvT2GateDefensive() {
        gasLimit =                                      com(Protoss_Cybernetics_Core) > 0 && s >= 50 ? INT_MAX : 0;
        getOpening =                                    vis(Protoss_Dark_Templar) == 0;
        playPassive =                                   false;
        firstUpgrade =                                  UpgradeTypes::None;
        firstTech =                                     TechTypes::None;
        firstUnit =                                     s >= 80 ? Protoss_Dark_Templar : None;
        fastExpand =                                    false;

        zealotLimit =                                   INT_MAX;
        dragoonLimit =                                  s >= 50 ? INT_MAX : 0;

        itemQueue[Protoss_Nexus] =                      Item(1);
        itemQueue[Protoss_Pylon] =                      Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =                    Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =                Item(s >= 40);
        itemQueue[Protoss_Shield_Battery] =             Item(vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
        itemQueue[Protoss_Cybernetics_Core] =           Item(s >= 58);
    }

    void PvT2Gate()
    {
        // https://liquipedia.net/starcraft/10/15_Gates_(vs._Terran)
        defaultPvT();
        playPassive =                                   false;
        proxy =                                         currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        wallNat =                                       vis(Protoss_Nexus) >= 2 || currentOpener == "Natural";
        scout =                                         currentOpener != "Proxy" && Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        rush =                                          currentOpener == "Proxy";

        // Openers
        if (currentOpener == "Proxy") {
            itemQueue[Protoss_Pylon] =                  Item((s >= 12) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
        }
        else if (currentOpener == "Main") {
            itemQueue[Protoss_Pylon] =                  Item((s >= 14) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 30));
        }

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyFastExpand())
                currentTransition = "DT";
            else if (Strategy::enemyRush() && currentOpener != "Proxy")
                currentTransition = "Defensive";
        }

        // Transitions
        if (currentTransition == "DT") {
            lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =                                s < 70;
            firstUnit =                                 vis(Protoss_Dragoon) >= 3 ? Protoss_Dark_Templar : UnitTypes::None;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Robo") {
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =                                s < 70;
            firstUnit =                                 com(Protoss_Dragoon) >= 3 ? Protoss_Reaver : UnitTypes::None;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Expand") {
            lockedTransition =                          vis(Protoss_Nexus) >= 2;
            getOpening =                                s < 100;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 50));
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "DoubleExpand") {
            lockedTransition =                          vis(Protoss_Nexus) >= 3;
            getOpening =                                s < 120;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 50) + (s >= 50));
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Defensive")
            PvT2GateDefensive();
    }

    void PvT1GateCore()
    {
        // https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)
        defaultPvT();
        firstUpgrade =                                  UpgradeTypes::Singularity_Charge;
        firstTech =                                     TechTypes::None;
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyRush())
                currentTransition = "Defensive";
            else if (Strategy::enemyFastExpand())
                currentTransition = "DT";
        }

        // Openers
        if (currentOpener == "0Zealot") {
            // NZCore
            zealotLimit = 0;
            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item(s >= 20);
            itemQueue[Protoss_Assimilator] =            Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentOpener == "1Zealot") {
            // ZCore
            zealotLimit = 1;
            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 32));
            itemQueue[Protoss_Gateway] =                Item(s >= 20);
            itemQueue[Protoss_Assimilator] =            Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 36);
        }

        // Transitions
        if (currentTransition == "Robo") {
            // http://liquipedia.net/starcraft/1_Gate_Reaver
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            getOpening =                                s < 60;
            hideTech =                                  com(Protoss_Reaver) == 0;
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 74));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 60) + (s >= 62));
            itemQueue[Protoss_Robotics_Facility] =      Item(s >= 52);
        }
        else if (currentTransition == "4Gate") {
            // https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)
            lockedTransition =                          vis(Protoss_Gateway) >= 4;
            getOpening =                                s < 80;
            firstUnit =                                 UnitTypes::None;

            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 30) + (2 * (s >= 62)));
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "DT") {
            // https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)
            lockedTransition =                          vis(Protoss_Citadel_of_Adun) > 0;
            getOpening =                                vis(Protoss_Dark_Templar) <= 2 && s <= 80;
            hideTech =                                  com(Protoss_Dark_Templar) < 1;
            firstUnit =                                 Protoss_Dark_Templar;
            firstUpgrade =                              vis(Protoss_Dark_Templar) >= 2 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;

            itemQueue[Protoss_Nexus] =                  Item(1 + (vis(Protoss_Dark_Templar) > 0));
            itemQueue[Protoss_Citadel_of_Adun] =        Item(s >= 36);
            itemQueue[Protoss_Templar_Archives] =       Item(s >= 48);
        }
        else if (currentTransition == "Defensive")
            PvT2GateDefensive();
    }

    void PvTNexusGate()
    {
        // http://liquipedia.net/starcraft/12_Nexus
        defaultPvT();
        fastExpand =                                    true;
        playPassive =                                   Units::getEnemyCount(Terran_Siege_Tank_Tank_Mode) == 0 && Units::getEnemyCount(Terran_Siege_Tank_Siege_Mode) == 0 && !Strategy::enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
        firstUpgrade =                                  vis(Protoss_Dragoon) >= 1 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        cutWorkers =                                    s >= 44 && s < 48;
        gasLimit =                                      goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;
        zealotLimit =                                   currentOpener == "Zealot" ? 1 : 0;

        // Openers
        if (currentOpener == "Zealot") {
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 48));
            itemQueue[Protoss_Assimilator] =            Item((s >= 30) + (s >= 86));
            itemQueue[Protoss_Gateway] =                Item((s >= 28) + (s >= 34) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =       Item(vis(Protoss_Gateway) >= 2);
        }
        else if (currentOpener == "Dragoon") {
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 48));
            itemQueue[Protoss_Assimilator] =            Item((s >= 28) + (s >= 86));
            itemQueue[Protoss_Gateway] =                Item((s >= 26) + (s >= 32) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 30);
        }

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyFastExpand() || Strategy::getEnemyBuild() == "SiegeExpand")
                currentTransition = s < 50 ? "DoubleExpand" : "ReaverCarrier";
            else if (!Strategy::enemyFastExpand() && currentTransition == "DoubleExpand")
                currentTransition = "Standard";
        }

        // Transitions
        if (currentTransition == "DoubleExpand") {
            lockedTransition =                          vis(Protoss_Nexus) >= 3;
            getOpening =                                s < 140;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24) + (com(Protoss_Cybernetics_Core) > 0));
            itemQueue[Protoss_Assimilator] =            Item(s >= 28);
        }
        else if (currentTransition == "Standard") {
            getOpening =                                s < 90;
            firstUnit =                                 None;
        }
        else if (currentTransition == "ReaverCarrier") {
            getOpening =                                s < 120;
            lockedTransition =                          vis(Protoss_Robotics_Facility) > 0;
            firstUnit =                                 com(Protoss_Reaver) > 0 ? Protoss_Carrier : Protoss_Reaver;

            itemQueue[Protoss_Gateway] =                Item((vis(Protoss_Pylon) > 1) + (vis(Protoss_Nexus) > 1) + (s >= 70) + (s >= 80));
        }
    }

    void PvTGateNexus()
    {
        defaultPvT();
        fastExpand =                                    true;
        playPassive =                                   Units::getEnemyCount(Terran_Siege_Tank_Tank_Mode) == 0 && Units::getEnemyCount(Terran_Siege_Tank_Siege_Mode) == 0 && !Strategy::enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
        firstUpgrade =                                  UpgradeTypes::Singularity_Charge;
        firstTech =                                     TechTypes::None;
        scout =                                         Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
        wallNat =                                       com(Protoss_Nexus) >= 2 ? true : false;
        gasLimit =                                      goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;
        dragoonLimit =                                  vis(Protoss_Nexus) >= 2 ? INT_MAX : 1;

        // Reactions
        if (!lockedTransition) {

            // Change Transition
            if (Strategy::enemyFastExpand() || Strategy::getEnemyBuild() == "TSiegeExpand")
                currentTransition = "DoubleExpand";
            else if ((!Strategy::enemyFastExpand() && Terrain::foundEnemy() && currentTransition == "DoubleExpand") || Strategy::enemyPressure())
                currentTransition = "Standard";

            // Change Build
            if (s < 42 && Strategy::enemyRush()) {
                currentBuild = "2Gate";
                currentOpener = "Main";
                currentTransition = "DT";
            }
        }

        // Openers - 1Gate / 2Gate
        if (currentOpener == "1Gate") {
            // 1 Gate - "http://liquipedia.net/starcraft/21_Nexus"
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (vis(Protoss_Nexus) >= 2) + (s >= 76));
        }
        else if (currentOpener == "2Gate") {
            // 2 Gate - "https://liquipedia.net/starcraft/2_Gate_Range_Expand"
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 40));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 36) + (s >= 76));
        }

        // Transitions - DoubleExpand / Standard / Carrier
        if (currentTransition == "DoubleExpand") {
            getOpening =                                s < 140;
            playPassive =                               s < 100;
            lockedTransition =                          vis(Protoss_Nexus) >= 3;
            gasLimit =                                  s >= 120 ? INT_MAX : gasLimit;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42) + (s >= 70));
            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Standard") {
            getOpening =                                s < 80;
            firstUnit =                                 com(Protoss_Nexus) >= 2 ? Protoss_Observer : UnitTypes::None;
            lockedTransition =                          false;

            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Carrier") {
            getOpening =                                s < 80;
            firstUnit =                                 com(Protoss_Nexus) >= 2 ? Protoss_Carrier : UnitTypes::None;
            lockedTransition =                          com(Protoss_Nexus) >= 2;
            gasLimit =                                  INT_MAX;

            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (s >= 50));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
    }
}