#include "McRave.h"
#include "BuildOrder.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Terran
{
    void RaxFact()
    {
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

//    void BuildOrderManager::TSparks()
//    {
//        firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
//        firstTech = TechTypes::Stim_Packs;
//        inOpeningBook = s < 60;
//        bioBuild = true;
//        scout = vis(Terran_Barracks) > 0;
//        gasLimit = 3 - (2 * int(vis(Terran_Engineering_Bay) > 0));
//
//        buildQueue[Terran_Supply_Depot] =        Item(s >= 18);
//        buildQueue[Terran_Barracks] =            Item((vis(Terran_Supply_Depot) > 0) + (s >= 24) + (s >= 42));
//        buildQueue[Terran_Refinery] =            Item(s >= 30);
//        buildQueue[Terran_Academy] =                Item(s >= 48);
//        buildQueue[Terran_Engineering_Bay] =        Item(s >= 36);
//        buildQueue[Terran_Comsat_Station] =        Item(vis(Terran_Academy) > 0);
//    }
//
//    void BuildOrderManager::T2PortWraith()
//    {
//        firstUpgrade = UpgradeTypes::None;
//        firstTech = TechTypes::Cloaking_Field;
//        inOpeningBook = s < 80;
//        fastExpand = true;
//        scout = vis(Terran_Barracks) > 0;
//
//        if (BWEB::Walls::getWall(BWEB::Map::getNaturalArea()))
//            wallNat = true;
//        else if (BWEB::Walls::getWall(BWEB::Map::getMainArea()))
//            wallMain = true;
//
//        buildQueue[Terran_Supply_Depot] =        Item((s >= 16) + (s >= 26), (s >= 18) + (s >= 26));
//        buildQueue[Terran_Barracks] =            Item(com(Terran_Supply_Depot) > 0);
//        buildQueue[Terran_Refinery] =            Item(s >= 24);
//        buildQueue[Terran_Factory] =                Item(s >= 32);
//        buildQueue[Terran_Starport] =            Item(2 * (s >= 44));
//        buildQueue[Terran_Command_Center] =        Item(1 + (s >= 70));
//    }
//
//    void BuildOrderManager::T1RaxFE()
//    {
//        bioBuild = true;
//        fastExpand = true;
//        firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
//        firstTech = TechTypes::Stim_Packs;
//        inOpeningBook = s < 86;
//        scout = vis(Terran_Barracks) > 0;
//        gasLimit = 1 + (vis(Terran_Command_Center) > 1);
//
//        if (BWEB::Walls::getWall(BWEB::Map::getNaturalArea()))
//            wallNat = true;
//        else if (BWEB::Walls::getWall(BWEB::Map::getMainArea()))
//            wallMain = true;
//
//        buildQueue[Terran_Supply_Depot] =        Item((s >= 16) + (s >= 26), (s >= 18) + (s >= 26));
//        buildQueue[Terran_Barracks] =            Item((com(Terran_Supply_Depot) > 0) + (s >= 64) + (s >= 70) + (2 * (s >= 80)));
//        buildQueue[Terran_Command_Center] =        Item(1 + (s >= 36));
//        buildQueue[Terran_Refinery] =            Item(s >= 46);
//        buildQueue[Terran_Engineering_Bay] =        Item(s >= 50);
//        buildQueue[Terran_Academy] =                Item(s >= 60);
//        buildQueue[Terran_Comsat_Station] =        Item(s >= 80);
//    }
//
//    void BuildOrderManager::T2RaxFE()
//    {
//        bioBuild = true;
//        fastExpand = true;
//        firstUpgrade = UpgradeTypes::Terran_Infantry_Weapons;
//        firstTech = TechTypes::Stim_Packs;
//        inOpeningBook = s < 86;
//        scout = vis(Terran_Barracks) > 0;
//        gasLimit = 1 + (com(Terran_Command_Center) > 1);
//        wallNat = true;
//
//        buildQueue[Terran_Supply_Depot] =        Item((s >= 16) + (vis(Terran_Bunker) > 0), (s >= 18) + (vis(Terran_Bunker) > 0));
//        buildQueue[Terran_Bunker] =                Item((com(Terran_Barracks) > 0));
//        buildQueue[Terran_Barracks] =            Item((com(Terran_Supply_Depot) > 0) + (s >= 26) + (s >= 70) + (2 * (s >= 80)));
//        buildQueue[Terran_Refinery] =            Item(s >= 50);
//        buildQueue[Terran_Engineering_Bay] =        Item(s >= 44);
//        buildQueue[Terran_Academy] =                Item(s >= 60);
//        buildQueue[Terran_Comsat_Station] =        Item(s >= 70);
//        buildQueue[Terran_Command_Center] =        Item(1 + (s >= 80));
//    }
//
//    void BuildOrderManager::T1FactFE()
//    {
//        fastExpand = true;
//        firstUpgrade = UpgradeTypes::None;
//        firstTech = TechTypes::Tank_Siege_Mode;
//        inOpeningBook = s < 80;
//        scout = vis(Terran_Barracks) > 0;
//        wallNat = true;
//
//        buildQueue[Terran_Supply_Depot] =        Item((s >= 16) + (s >= 34), (s >= 18) + (s >= 34));
//        buildQueue[Terran_Bunker] =                Item(s >= 30);
//        buildQueue[Terran_Barracks] =            Item(com(Terran_Supply_Depot) > 0);
//        buildQueue[Terran_Refinery] =            Item(s >= 28);
//        buildQueue[Terran_Factory] =                Item((s >= 32) + (s >= 64));
//        buildQueue[Terran_Machine_Shop] =        Item(s >= 40);
//        buildQueue[Terran_Command_Center] =        Item(1 + (s >= 56));
//    }
//
//    void BuildOrderManager::TNukeMemes()
//    {
//        firstUpgrade = UpgradeTypes::None;
//        firstTech = TechTypes::Personnel_Cloaking;
//        inOpeningBook = vis(Terran_Covert_Ops) <= 0;
//        bioBuild = true;
//        scout = vis(Terran_Barracks) > 0;
//
//        buildQueue[Terran_Supply_Depot] =        Item(s >= 20);
//        buildQueue[Terran_Barracks] =            Item((s >= 18) + (s >= 22) + (s >= 46));
//        buildQueue[Terran_Refinery] =            Item(s >= 40);
//        buildQueue[Terran_Academy] =                Item(s >= 42);
//        buildQueue[Terran_Factory] =                Item(s >= 50);
//        buildQueue[Terran_Starport] =            Item(com(Terran_Factory) > 0);
//        buildQueue[Terran_Science_Facility] =    Item(com(Terran_Starport) > 0);
//        buildQueue[Terran_Covert_Ops] =            Item(com(Terran_Science_Facility) > 0);
//    }
//
//    void BuildOrderManager::TBCMemes()
//    {
//        firstUpgrade = UpgradeTypes::None;
//        firstTech = TechTypes::Tank_Siege_Mode;
//        inOpeningBook = s < 80;
//        fastExpand = true;
//        scout = vis(Terran_Barracks) > 0;
//
//        techUnit = Terran_Battlecruiser;
//        techList.insert(Terran_Battlecruiser);
//        unlockedType.insert(Terran_Battlecruiser);
//
//        if (BWEB::Walls::getWall(BWEB::Map::getNaturalArea()))
//            wallNat = true;
//
//        buildQueue[Terran_Supply_Depot] =        Item((s >= 16) + (s >= 32), (s >= 18) + (s >= 32));
//        buildQueue[Terran_Bunker] =                Item(s >= 26);
//        buildQueue[Terran_Barracks] =            Item(com(Terran_Supply_Depot) > 0);
//        buildQueue[Terran_Refinery] =            Item(s >= 28);
//        buildQueue[Terran_Factory] =                Item(s >= 32);
//        buildQueue[Terran_Command_Center] =        Item(1 + (s >= 70));
//    }
//}