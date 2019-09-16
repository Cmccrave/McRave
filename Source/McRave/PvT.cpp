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
            getOpening =            true;
            wallNat =               com(Protoss_Pylon) >= 3 || currentOpener == "Natural";
            wallMain =              false;
            scout =                 vis(Protoss_Cybernetics_Core) > 0;
            fastExpand =            false;
            proxy =                 false;
            hideTech =              false;
            playPassive =           false;
            rush =                  false;
            cutWorkers =            false;
            transitionReady =       false;

            gasLimit =              INT_MAX;
            zealotLimit =           0;
            dragoonLimit =          INT_MAX;
            wallDefenseDesired =    0;
     
            desiredDetection =      Protoss_Observer;
            firstUpgrade =          UpgradeTypes::Singularity_Charge;
            firstTech =             TechTypes::None;
        }
    }

    void PvT2GateDefensive() {
        gasLimit =                                      com(Protoss_Cybernetics_Core) > 0 && s >= 50 ? INT_MAX : 0;
        getOpening =                                    s < 80;
        playPassive =                                   false;
        firstUpgrade =                                  UpgradeTypes::None;
        firstTech =                                     TechTypes::None;
        firstUnit =                                     None;
        fastExpand =                                    false;

        zealotLimit =                                   s >= 50 ? 0 : INT_MAX;
        dragoonLimit =                                  s >= 50 ? INT_MAX : 0;

        itemQueue[Protoss_Nexus] =                      Item(1);
        itemQueue[Protoss_Pylon] =                      Item((s >= 16) + (s >= 30));
        itemQueue[Protoss_Gateway] =                    Item((s >= 20) + (s >= 24) + (s >= 66));
        itemQueue[Protoss_Assimilator] =                Item(s >= 40);
        itemQueue[Protoss_Shield_Battery] =             Item(vis(Protoss_Zealot) >= 2 && vis(Protoss_Pylon) >= 2);
        itemQueue[Protoss_Cybernetics_Core] =           Item(s >= 58);
    }

    void PvT2Gate()
    {
        // "https://liquipedia.net/starcraft/10/15_Gates_(vs._Terran)"
        defaultPvT();
        proxy =                                         currentOpener == "Proxy" && vis(Protoss_Gateway) < 2 && Broodwar->getFrameCount() < 5000;
        scout =                                         currentOpener != "Proxy" && Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) >= 1 : vis(Protoss_Gateway) >= 2;
        rush =                                          currentOpener == "Proxy";
        playPassive =                                   Strategy::enemyPressure() && Util::getTime() < Time(6, 0);

        // Openers
        if (currentOpener == "Proxy") {
            itemQueue[Protoss_Pylon] =                  Item((s >= 12) + (s >= 30), (s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((vis(Protoss_Pylon) > 0 && s >= 18) + (vis(Protoss_Gateway) > 0), 2 * (s >= 18));
        }
        else if (currentOpener == "Main") {
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 30));
        }

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyFastExpand())
                currentTransition = "DT";
            else if (Strategy::enemyRush() && currentOpener != "Proxy")
                currentTransition = "Defensive";
        }

        // Transitions
        if (currentTransition == "DT") {
            lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
            getOpening =                                s < 70;
            firstUnit =                                 Protoss_Dark_Templar;
            hideTech =                                  true;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
            itemQueue[Protoss_Citadel_of_Adun] =        Item(vis(Protoss_Dragoon) >= 3);
            itemQueue[Protoss_Templar_Archives] =       Item(isAlmostComplete(Protoss_Citadel_of_Adun));
        }
        else if (currentTransition == "Robo") {
            lockedTransition =                          total(Protoss_Robotics_Facility) > 0;
            getOpening =                                s < 70;
            firstUnit =                                 Protoss_Reaver;

            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
            itemQueue[Protoss_Robotics_Facility] =      Item(vis(Protoss_Dragoon) >= 3);
        }
        else if (currentTransition == "Expand") {
            lockedTransition =                          total(Protoss_Nexus) >= 2;
            getOpening =                                s < 100;
            firstUnit =                                 None;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 50));
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "DoubleExpand") {
            lockedTransition =                          total(Protoss_Nexus) >= 3;
            getOpening =                                s < 120;
            gasLimit =                                  3;
            firstUnit =                                 None;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 50) + (s >= 50));
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Defensive")
            PvT2GateDefensive();
    }

    void PvT1GateCore()
    {
        // "https://liquipedia.net/starcraft/1_Gate_Core_(vs._Terran)"
        defaultPvT();
        firstUpgrade =                                  UpgradeTypes::Singularity_Charge;
        firstTech =                                     TechTypes::None;
        scout =                                         Broodwar->getStartLocations().size() >= 3 ? vis(Protoss_Gateway) > 0 : vis(Protoss_Pylon) > 0;

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyRush())
                currentTransition = "Defensive";
            else if (Strategy::enemyFastExpand())
                currentTransition = "DT";
        }

        // Openers
        if (currentOpener == "0Zealot") {               // NZCore            
            zealotLimit = 0;
            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item(s >= 20);
            itemQueue[Protoss_Assimilator] =            Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentOpener == "1Zealot") {          // ZCore            
            zealotLimit = 1;
            itemQueue[Protoss_Nexus] =                  Item(1);
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 32));
            itemQueue[Protoss_Gateway] =                Item(s >= 20);
            itemQueue[Protoss_Assimilator] =            Item(s >= 24);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 36);
        }

        // Transitions
        if (currentTransition == "Robo") {              // "http://liquipedia.net/starcraft/1_Gate_Reaver"            
            lockedTransition =                          total(Protoss_Robotics_Facility) > 0;
            getOpening =                                s < 60;
            hideTech =                                  com(Protoss_Reaver) <= 0;
            firstUnit =                                 Strategy::enemyPressure() ? Protoss_Reaver : Protoss_Observer;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 74));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 60) + (s >= 62));
            itemQueue[Protoss_Robotics_Facility] =      Item(s >= 52);
        }
        else if (currentTransition == "4Gate") {        // "https://liquipedia.net/starcraft/4_Gate_Goon_(vs._Protoss)"            
            lockedTransition =                          total(Protoss_Gateway) >= 4;
            getOpening =                                s < 80;
            firstUnit =                                 None;

            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 30) + (2 * (s >= 62)));
            itemQueue[Protoss_Assimilator] =            Item(s >= 22);
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "DT") {           // "https://liquipedia.net/starcraft/DT_Fast_Expand_(vs._Terran)"            
            lockedTransition =                          total(Protoss_Citadel_of_Adun) > 0;
            getOpening =                                s <= 80;
            hideTech =                                  com(Protoss_Dark_Templar) <= 0;
            firstUnit =                                 Protoss_Dark_Templar;
            firstUpgrade =                              vis(Protoss_Dark_Templar) >= 2 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
            playPassive =                               Strategy::enemyPressure() ? vis(Protoss_Observer) == 0 : false;

            itemQueue[Protoss_Gateway] =			    Item((s >= 20) + (vis(Protoss_Templar_Archives) > 0));
            itemQueue[Protoss_Nexus] =                  Item(1 + (vis(Protoss_Dark_Templar) > 0));
            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (vis(Protoss_Nexus) >= 2));
            itemQueue[Protoss_Citadel_of_Adun] =        Item(s >= 36);
            itemQueue[Protoss_Templar_Archives] =       Item(s >= 48);
        }
        else if (currentTransition == "Defensive")
            PvT2GateDefensive();
    }

    void PvTNexusGate()
    {
        // "http://liquipedia.net/starcraft/12_Nexus"
        defaultPvT();
        fastExpand =                                    true;
        playPassive =                                   Strategy::enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady() && vis(Protoss_Dragoon) < 4;
        firstUpgrade =                                  vis(Protoss_Dragoon) >= 1 ? UpgradeTypes::Singularity_Charge : UpgradeTypes::None;
        cutWorkers =                                    s >= 44 && s < 48;
        gasLimit =                                      goonRange() && com(Protoss_Nexus) < 2 ? 2 : INT_MAX;
        zealotLimit =                                   currentOpener == "Zealot" ? 1 : 0;
        scout =                                         vis(Protoss_Pylon) > 0;

        // Openers
        if (currentOpener == "Zealot") {
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 44));
            itemQueue[Protoss_Assimilator] =            Item(s >= 30);
            itemQueue[Protoss_Gateway] =                Item((s >= 28) + (s >= 34));
            itemQueue[Protoss_Cybernetics_Core] =       Item(vis(Protoss_Gateway) >= 2);

            transitionReady = vis(Protoss_Gateway) >= 2;
        }
        else if (currentOpener == "Dragoon") {
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 44));
            itemQueue[Protoss_Assimilator] =            Item(vis(Protoss_Gateway) >= 1);
            itemQueue[Protoss_Gateway] =                Item((s >= 26) + (s >= 32));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 30);

            transitionReady = vis(Protoss_Gateway) >= 2;
        }

        // Reactions
        if (!lockedTransition) {
            if (Strategy::enemyFastExpand() || Strategy::getEnemyBuild() == "SiegeExpand")
                currentTransition = s < 50 ? "DoubleExpand" : "ReaverCarrier";
            else if (Terrain::getEnemyStartingPosition().isValid() && !Strategy::enemyFastExpand() && currentTransition == "DoubleExpand")
                currentTransition = "Standard";
            else if (Strategy::enemyPressure())
                currentTransition = "Standard";
        }

        // Transitions
        if (transitionReady) {
            if (currentTransition == "DoubleExpand") {
                lockedTransition =                          total(Protoss_Nexus) >= 3;
                getOpening =                                s < 120;
                firstUnit =                                 None;

                itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 24) + (com(Protoss_Cybernetics_Core) > 0));
                itemQueue[Protoss_Assimilator] =            Item(s >= 26);
            }
            else if (currentTransition == "Standard") {
                getOpening =                                s < 100;
                firstUnit =                                 None;

                itemQueue[Protoss_Assimilator] =            Item(1 + (s >= 86));
                itemQueue[Protoss_Gateway] =                Item(2 + (s >= 80));
            }
            else if (currentTransition == "ReaverCarrier") {
                getOpening =                                s < 120;
                lockedTransition =                          total(Protoss_Stargate) > 0;
                firstUnit =                                 Players::getCurrentCount(PlayerState::Enemy, Terran_Medic) > 0 && vis(Protoss_Reaver) == 0 ? Protoss_Reaver : Protoss_Carrier;

                itemQueue[Protoss_Assimilator] =            Item(1 + (s >= 56));

                if (firstUnit == Protoss_Carrier) {
                    itemQueue[Protoss_Stargate] =               Item((s >= 80) + (vis(Protoss_Carrier) > 0));
                    itemQueue[Protoss_Fleet_Beacon] =           Item(isAlmostComplete(Protoss_Stargate));
                }

                if (firstUnit == Protoss_Reaver)
                    itemQueue[Protoss_Robotics_Facility] =  Item(s >= 58);
            }
        }
    }

    void PvTGateNexus()
    {
        defaultPvT();
        fastExpand =                                    true;
        playPassive =                                   Strategy::enemyPressure() ? vis(Protoss_Dragoon) < 12 : !firstReady();
        firstUpgrade =                                  UpgradeTypes::Singularity_Charge;
        firstTech =                                     TechTypes::None;
        scout =                                         Broodwar->getStartLocations().size() == 4 ? vis(Protoss_Pylon) > 0 : vis(Protoss_Pylon) > 0;
        zealotLimit =                                   0;

        // Reactions
        if (!lockedTransition) {
            if (currentTransition != "Carrier" && (Strategy::enemyFastExpand() || Strategy::getEnemyBuild() == "TSiegeExpand"))
                currentTransition = "DoubleExpand";
            else if ((!Strategy::enemyFastExpand() && Terrain::foundEnemy() && currentTransition == "DoubleExpand") || Strategy::enemyPressure())
                currentTransition = "Standard";

            if (s < 42 && Strategy::enemyRush()) {
                currentBuild = "2Gate";
                currentOpener = "Main";
                currentTransition = "DT";
            }
        }

        // Openers
        if (currentOpener == "1Gate") {                 // "http://liquipedia.net/starcraft/21_Nexus"            
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (vis(Protoss_Nexus) >= 2) + (s >= 76));

            gasLimit =                                  goonRange() && vis(Protoss_Nexus) < 2 ? 2 : INT_MAX;
            dragoonLimit =                              Util::getTime() > Time(4, 0) || vis(Protoss_Nexus) >= 2 ? INT_MAX : 1;
            cutWorkers =                                vis(Protoss_Probe) >= 19 && s < 46;
        }
        else if (currentOpener == "2Gate") {            // "https://liquipedia.net/starcraft/2_Gate_Range_Expand"
            
            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 40));
            itemQueue[Protoss_Pylon] =                  Item((s >= 16) + (s >= 30));
            itemQueue[Protoss_Gateway] =                Item((s >= 20) + (s >= 36) + (s >= 76));

            gasLimit =                                  goonRange() && vis(Protoss_Pylon) < 3 ? 2 : INT_MAX;
            dragoonLimit =                              Util::getTime() > Time(4, 0) || vis(Protoss_Nexus) >= 2 ? INT_MAX : 0;
            cutWorkers =                                vis(Protoss_Probe) >= 20 && s < 48;
        }

        // Transitions
        if (currentTransition == "DoubleExpand") {
            firstUnit =                                 None;
            getOpening =                                s < 140;
            playPassive =                               s < 100;
            lockedTransition =                          total(Protoss_Nexus) >= 3;

            itemQueue[Protoss_Nexus] =                  Item(1 + (s >= 42) + (s >= 70));
            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Standard") {
            getOpening =                                s < 80;
            firstUnit =                                 Protoss_Observer;
            lockedTransition =                          false;

            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (s >= 80));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
        }
        else if (currentTransition == "Carrier") {
            getOpening =                                s < 160;
            firstUnit =                                 Protoss_Carrier;
            lockedTransition =                          total(Protoss_Stargate) > 0;

            itemQueue[Protoss_Assimilator] =            Item((s >= 24) + (s >= 60));
            itemQueue[Protoss_Cybernetics_Core] =       Item(s >= 26);
            itemQueue[Protoss_Stargate] =               Item(vis(Protoss_Dragoon) >= 3);
            itemQueue[Protoss_Fleet_Beacon] =           Item(isAlmostComplete(Protoss_Stargate));
        }
    }
}