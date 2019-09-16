#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Terran {

    void opener()
    {
        if (currentBuild == "RaxFact")
            RaxFact();
    }

    void tech()
    {
        getNewTech();
        checkNewTech();
        checkAllTech();
        checkExoticTech();
    }

    void situational()
    {

        auto satVal = 2;
        auto prodVal = vis(Terran_Barracks) + vis(Terran_Factory) + vis(Terran_Starport);
        auto baseVal = vis(Terran_Command_Center);
        auto techVal = techList.size();

        productionSat = (prodVal >= satVal * baseVal);
        techSat = (techVal > baseVal);

        if (techComplete())
            techUnit = None; // If we have our tech unit, set to none	
        if (Strategy::needDetection() || (!getOpening && !getTech && !techSat /*&& productionSat*/ && techUnit == None))
            getTech = true; // If production is saturated and none are idle or we need detection, choose a tech

        productionSat = (com(Terran_Factory) >= min(12, (2 * vis(Terran_Command_Center))));
        unlockedType.insert(Terran_Goliath);
        unlockedType.insert(Terran_Vulture);

        if (Players::getSupply(PlayerState::Self) >= 80)
            unlockedType.insert(Terran_Siege_Tank_Tank_Mode);

        if (!getOpening || com(Terran_Marine) >= 4)
            unlockedType.erase(Terran_Marine);
        else
            unlockedType.insert(Terran_Marine);


        // Control Tower
        if (com(Terran_Starport) >= 1)
            itemQueue[Terran_Control_Tower] = Item(com(Terran_Starport));

        // Machine Shop
        if (com(Terran_Factory) >= 3)
            itemQueue[Terran_Machine_Shop] = Item(max(1, com(Terran_Factory) - com(Terran_Command_Center) - 1));

        // Supply Depot logic
        if (vis(Terran_Supply_Depot) > (int)fastExpand) {
            int count = min(22, (int)floor((Players::getSupply(PlayerState::Self) / max(15, (16 - vis(Terran_Supply_Depot) - vis(Terran_Command_Center))))));
            itemQueue[Terran_Supply_Depot] = Item(count);
        }

        // Expansion logic
        if (shouldExpand())
            itemQueue[Terran_Command_Center] = Item(com(Terran_Command_Center) + 1);

        // Bunker logic
        if (Strategy::enemyRush() && !wallMain)
            itemQueue[Terran_Bunker] = Item(1);

        if (!getOpening)
        {
            // Refinery logic
            if (shouldAddGas())
                itemQueue[Terran_Refinery] = Item(Resources::getGasCount());

            // Armory logic TODO
            itemQueue[Terran_Armory] = Item((Players::getSupply(PlayerState::Self) > 160) + (Players::getSupply(PlayerState::Self) > 200));

            // Academy logic
            if (Strategy::needDetection()) {
                itemQueue[Terran_Academy] = Item(1);
                itemQueue[Terran_Comsat_Station] = Item(2);
            }

            if (com(Terran_Science_Facility) > 0)
                itemQueue[Terran_Physics_Lab] = Item(1);

            // Engineering Bay logic
            if (Players::getSupply(PlayerState::Self) > 200)
                itemQueue[Terran_Engineering_Bay] = Item(1);

            // Missle Turret logic
            if (Players::vZ() && com(Terran_Engineering_Bay) > 0)
                itemQueue[Terran_Missile_Turret] = Item(com(Terran_Command_Center) * 2);
        }
    }
}