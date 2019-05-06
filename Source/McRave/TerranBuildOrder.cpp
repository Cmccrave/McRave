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
        // Some temp stuff
        bool bioUnlocked, mechUnlocked, airUnlocked;
        bioUnlocked = mechUnlocked = airUnlocked = false;

        satVal = 2;
        prodVal = vis(UnitTypes::Terran_Barracks) + vis(UnitTypes::Terran_Factory) + vis(UnitTypes::Terran_Starport);
        baseVal = vis(UnitTypes::Terran_Command_Center);

        techVal = techList.size();
        productionSat = (prodVal >= satVal * baseVal);
        techSat = (techVal > baseVal);

        if (techComplete())
            techUnit = UnitTypes::None; // If we have our tech unit, set to none	
        if (Strategy::needDetection() || (!getOpening && !getTech && !techSat /*&& productionSat*/ && techUnit == UnitTypes::None))
            getTech = true; // If production is saturated and none are idle or we need detection, choose a tech

        // Unlocked type logic
        if (currentBuild == "TSparks")
        {
            bioUnlocked = true;
            productionSat = (com(UnitTypes::Terran_Barracks) >= min(12, (3 * vis(UnitTypes::Terran_Command_Center))));
            unlockedType.insert(UnitTypes::Terran_Marine);
            unlockedType.insert(UnitTypes::Terran_Medic);
        }
        else if (currentBuild == "T2PortWraith")
        {
            airUnlocked = true;
            mechUnlocked = true;
            unlockedType.insert(UnitTypes::Terran_Wraith);
            unlockedType.insert(UnitTypes::Terran_Vulture);
            unlockedType.insert(UnitTypes::Terran_Valkyrie);
            techList.insert(UnitTypes::Terran_Wraith);
            techList.insert(UnitTypes::Terran_Valkyrie);

            if (!getOpening || com(UnitTypes::Terran_Marine) >= 4)
                unlockedType.erase(UnitTypes::Terran_Marine);
            else
                unlockedType.insert(UnitTypes::Terran_Marine);

            // Machine Shop
            if (com(UnitTypes::Terran_Factory) >= 1)
                itemQueue[UnitTypes::Terran_Machine_Shop] = Item(max(1, com(UnitTypes::Terran_Factory) - (com(UnitTypes::Terran_Command_Center))));

            // Control Tower
            if (com(UnitTypes::Terran_Starport) >= 1)
                itemQueue[UnitTypes::Terran_Control_Tower] = Item(com(UnitTypes::Terran_Starport));
        }
        else if (currentBuild == "T1RaxFE" || currentBuild == "T2RaxFE")
        {
            bioUnlocked = true;
            unlockedType.insert(UnitTypes::Terran_Marine);
            unlockedType.insert(UnitTypes::Terran_Medic);
            productionSat = (com(UnitTypes::Terran_Barracks) >= min(12, (3 * vis(UnitTypes::Terran_Command_Center))));
        }

        else if (currentBuild == "TNukeMemes")
        {
            bioUnlocked = true;
            unlockedType.insert(UnitTypes::Terran_Marine);
            unlockedType.insert(UnitTypes::Terran_Ghost);
            unlockedType.insert(UnitTypes::Terran_Nuclear_Missile);
            techList.insert(UnitTypes::Terran_Ghost);
            techList.insert(UnitTypes::Terran_Nuclear_Missile);

            if (com(UnitTypes::Terran_Covert_Ops) > 0)
                itemQueue[UnitTypes::Terran_Nuclear_Silo] = Item(com(UnitTypes::Terran_Command_Center));
        }
        else if (currentBuild == "TBCMemes") {
            unlockedType.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);
            unlockedType.insert(UnitTypes::Terran_Vulture);
            unlockedType.insert(UnitTypes::Terran_Goliath);

            if (!getOpening || com(UnitTypes::Terran_Marine) >= 10)
                unlockedType.erase(UnitTypes::Terran_Marine);
            else
                unlockedType.insert(UnitTypes::Terran_Marine);

            mechUnlocked = true;
        }
        else
        {
            mechUnlocked = true;
            productionSat = (com(UnitTypes::Terran_Factory) >= min(12, (2 * vis(UnitTypes::Terran_Command_Center))));
            unlockedType.insert(UnitTypes::Terran_Goliath);
            unlockedType.insert(UnitTypes::Terran_Vulture);

            if (Units::getSupply() >= 80)
                unlockedType.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);

            if (!getOpening || com(UnitTypes::Terran_Marine) >= 4)
                unlockedType.erase(UnitTypes::Terran_Marine);
            else
                unlockedType.insert(UnitTypes::Terran_Marine);
        }

        // Control Tower
        if (com(UnitTypes::Terran_Starport) >= 1)
            itemQueue[UnitTypes::Terran_Control_Tower] = Item(com(UnitTypes::Terran_Starport));

        // Machine Shop
        if (com(UnitTypes::Terran_Factory) >= 3)
            itemQueue[UnitTypes::Terran_Machine_Shop] = Item(max(1, com(UnitTypes::Terran_Factory) - com(UnitTypes::Terran_Command_Center) - 1));

        // Supply Depot logic
        if (vis(UnitTypes::Terran_Supply_Depot) > (int)fastExpand) {
            int count = min(22, (int)floor((Units::getSupply() / max(15, (16 - vis(UnitTypes::Terran_Supply_Depot) - vis(UnitTypes::Terran_Command_Center))))));
            itemQueue[UnitTypes::Terran_Supply_Depot] = Item(count);
        }

        // Expansion logic
        if (shouldExpand())
            itemQueue[UnitTypes::Terran_Command_Center] = Item(com(UnitTypes::Terran_Command_Center) + 1);

        // Bunker logic
        if (Strategy::enemyRush() && !wallMain)
            itemQueue[UnitTypes::Terran_Bunker] = Item(1);

        if (!getOpening)
        {
            // Refinery logic
            if (shouldAddGas())
                itemQueue[UnitTypes::Terran_Refinery] = Item(Resources::getGasCount());

            // Armory logic TODO
            itemQueue[UnitTypes::Terran_Armory] = Item((Units::getSupply() > 160) + (Units::getSupply() > 200));

            // Academy logic
            if (Strategy::needDetection()) {
                itemQueue[UnitTypes::Terran_Academy] = Item(1);
                itemQueue[UnitTypes::Terran_Comsat_Station] = Item(2);
            }

            if (com(UnitTypes::Terran_Science_Facility) > 0)
                itemQueue[UnitTypes::Terran_Physics_Lab] = Item(1);

            // Engineering Bay logic
            if (Units::getSupply() > 200)
                itemQueue[UnitTypes::Terran_Engineering_Bay] = Item(1);

            // Barracks logic
            if (bioUnlocked && shouldAddProduction())
                itemQueue[UnitTypes::Terran_Barracks] = Item(min(com(UnitTypes::Terran_Command_Center) * 3, vis(UnitTypes::Terran_Barracks) + 1));

            // Factory logic
            if (mechUnlocked && shouldAddProduction())
                itemQueue[UnitTypes::Terran_Factory] = Item(min(com(UnitTypes::Terran_Command_Center) * 2, vis(UnitTypes::Terran_Factory) + 1));

            // Starport logic
            if (airUnlocked && shouldAddProduction())
                itemQueue[UnitTypes::Terran_Starport] = Item(min(com(UnitTypes::Terran_Command_Center) * 2, vis(UnitTypes::Terran_Starport) + 1));

            // Missle Turret logic
            if (Players::getNumberZerg() > 0 && com(UnitTypes::Terran_Engineering_Bay) > 0)
                itemQueue[UnitTypes::Terran_Missile_Turret] = Item(com(UnitTypes::Terran_Command_Center) * 2);
        }
    }
}