#include "McRave.h"
#include "BuildOrder.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Terran
{

    void StupidBunkerExploitShit()
    {
        wallNat = true;

        buildQueue[Terran_Command_Center] =      1 + (s >= 40);
        buildQueue[Terran_Supply_Depot] =        s >= 18;
        buildQueue[Terran_Barracks] =            (s >= 20) + (s >= 45);
        buildQueue[Terran_Refinery] =            (s >= 24) + (s >= 60);
        buildQueue[Terran_Bunker] =              12 * (s >= 30);
        buildQueue[Terran_Factory] =             (s >= 30);
        buildQueue[Terran_Starport] =            3 * (s >= 60);

        if (s >= 60) {
            techList.insert(Terran_Battlecruiser);
            unlockedType.insert(Terran_Battlecruiser);
            armyComposition[Terran_Battlecruiser] = 1.00;
        }
    }

    void RaxFact()
    {
        StupidBunkerExploitShit();
        return;

        if (currentTransition == "2Fact") {
            firstUpgrade = UpgradeTypes::Ion_Thrusters;
            firstTech = TechTypes::None;
            inOpeningBook = s < 70;
            scout = s >= 20 && vis(Terran_Supply_Depot) > 0;
            wallMain = true;
            gasLimit = INT_MAX;

            buildQueue[Terran_Supply_Depot] =        s >= 18;
            buildQueue[Terran_Barracks] =            s >= 20;
            buildQueue[Terran_Refinery] =            s >= 24;
            buildQueue[Terran_Factory] =            (s >= 30) + (s >= 36) + (s >= 46);
            buildQueue[Terran_Machine_Shop] =       (s >= 30) + (com(Terran_Factory) >= 2);
        }
    }
}