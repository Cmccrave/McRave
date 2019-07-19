#include "McRave.h"
#include "BuildOrder.h"
#include <fstream>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder
{
    namespace {

        void updateBuild()
        {
            // Set s for better build readability - TODO: better build order management
            s = Players::getSupply(PlayerState::Self);
            startCount = Broodwar->getStartLocations().size();

            // TODO: Check if we own a <race> unit - have a build order allowed PER race for FFA weirdness and maybe mind control shenanigans
            if (Broodwar->self()->getRace() == Races::Protoss) {
                Protoss::opener();
                Protoss::tech();
                Protoss::situational();
                Protoss::unlocks();
            }
            if (Broodwar->self()->getRace() == Races::Terran) {
                Terran::opener();
                Terran::tech();
                Terran::situational();
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                Zerg::opener();
                Zerg::tech();
                Zerg::situational();
                Zerg::unlocks();
            }
        }
    }

    bool isAlmostComplete(UnitType t) {
        if (com(t) > 0)
            return true;

        // Estimate how long until a building finishes based on how far it is from the nearest worker
        auto closestBuilding = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Self, [&](auto &u) {
            return u.getType() == t;
        });
        auto closestWorker = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Self, [&](auto &u) {
            return u.getType() == Broodwar->self()->getRace().getWorker();
        });

        if (closestBuilding && closestWorker)
            return (BWEB::Map::getGroundDistance(closestBuilding->getPosition(), BWEB::Map::getMainPosition()) / closestWorker->getSpeed()) > closestBuilding->unit()->getRemainingBuildTime();
        return false;
    }

    bool techComplete()
    {
        // When 1 unit finishes
        if (techUnit == Protoss_Scout || techUnit == Protoss_Corsair || techUnit == Protoss_Reaver || techUnit == Protoss_Observer || techUnit == Terran_Science_Vessel)
            return Broodwar->self()->completedUnitCount(techUnit) > 0;

        // When 2 units are visible
        if (techUnit == Protoss_High_Templar)
            return Broodwar->self()->visibleUnitCount(techUnit) >= 2;

        // When 2 units finish
        if (techUnit == Protoss_Dark_Templar)
            return Broodwar->self()->completedUnitCount(techUnit) >= 2;

        // When 1 unit is visible
        return Broodwar->self()->visibleUnitCount(techUnit) > 0;
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateBuild();
        Visuals::endPerfTest("BuildOrder");
    }

    bool shouldExpand()
    {
        UnitType baseType = Broodwar->self()->getRace().getResourceDepot();

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Broodwar->self()->minerals() > 400 + (50 * com(baseType)))
                return true;
            else if (techUnit == None && Resources::isMinSaturated() && techSat && productionSat)
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Terran) {
            if (Broodwar->self()->minerals() > 400 + (100 * com(baseType)))
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Broodwar->self()->minerals() - Production::getReservedMineral() - Buildings::getQueuedMineral() >= 300)
                return true;
        }
        return false;
    }

    bool shouldAddProduction()
    {
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Broodwar->self()->minerals() >= 300 && vis(Zerg_Larva) < 3)
                return true;
        }
        else {
            if (!productionSat && Broodwar->self()->minerals() >= 150)
                return true;
        }
        return false;
    }

    bool shouldAddGas()
    {
        auto workerCount = Broodwar->self()->completedUnitCount(Broodwar->self()->getRace().getWorker());
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Resources::isGasSaturated() && Broodwar->self()->minerals() - Production::getReservedMineral() - Buildings::getQueuedMineral() > 300)
                return true;
            if (vis(Zerg_Extractor) == 0)
                return true;
        }

        else if (Broodwar->self()->getRace() == Races::Protoss)
            return vis(Protoss_Assimilator) != 1 || workerCount >= 34 || Broodwar->self()->minerals() > 600;

        else if (Broodwar->self()->getRace() == Races::Terran)
            return true;
        return false;
    }

    int buildCount(UnitType unit)
    {
        if (itemQueue.find(unit) != itemQueue.end())
            return itemQueue[unit].getActualCount();
        return 0;
    }

    bool firstReady()
    {
        if (firstTech != TechTypes::None && Broodwar->self()->hasResearched(firstTech))
            return true;
        else if (firstUpgrade != UpgradeTypes::None && Broodwar->self()->getUpgradeLevel(firstUpgrade) > 0)
            return true;
        else if (firstTech == TechTypes::None && firstUpgrade == UpgradeTypes::None)
            return true;
        return false;
    }

    bool unlockReady(UnitType type) {
        bool ready = false;

        if (type == Protoss_High_Templar || type == Protoss_Dark_Templar || type == Protoss_Archon || type == Protoss_Dark_Archon)
            ready = vis(Protoss_Citadel_of_Adun) > 0;
        if (type == Protoss_Corsair || type == Protoss_Scout)
            ready = vis(Protoss_Stargate) > 0;
        if (type == Protoss_Reaver || type == Protoss_Observer)
            ready = vis(Protoss_Robotics_Facility) > 0;
        if (type == Protoss_Carrier)
            ready = vis(Protoss_Fleet_Beacon) > 0;
        if (type == Protoss_Arbiter)
            ready = vis(Protoss_Arbiter_Tribunal) > 0;

        return ready;
    }

    void getNewTech()
    {
        if (!getTech)
            return;

        // Choose a tech based on highest unit score
        double highest = 0.0;
        for (auto &[tech,score] : Strategy::getUnitScores()) {
            if (tech == Protoss_Dragoon
                || tech == Protoss_Zealot
                || tech == Protoss_Shuttle
                || isTechUnit(tech))
                continue;

            if (score > highest) {
                highest = score;
                techUnit = tech;
            }
        }
    }

    void checkNewTech()
    {
        auto canGetTech = (Broodwar->self()->getRace() == Races::Protoss && com(Protoss_Cybernetics_Core) > 0)
            || (Broodwar->self()->getRace() == Races::Zerg && com(Zerg_Spawning_Pool) > 0);

        const auto alreadyUnlocked = [&](UnitType type) {
            if (unlockedType.find(type) != unlockedType.end() && techList.find(type) != techList.end())
                return true;
            return false;
        };

        // No longer need to choose a tech
        if (techUnit != None) {
            getTech = false;
            techList.insert(techUnit);
            unlockedType.insert(techUnit);
        }

        // Multi-unlock
        if (techUnit == Protoss_Arbiter || techUnit == Protoss_High_Templar) {
            unlockedType.insert(Protoss_Dark_Templar);
            techList.insert(Protoss_Dark_Templar);
        }
        else if (techUnit == Zerg_Mutalisk && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
            techList.insert(Zerg_Scourge);
            unlockedType.insert(Zerg_Scourge);
        }
        else if (techUnit == Zerg_Lurker) {
            techList.insert(Zerg_Hydralisk);
            unlockedType.insert(Zerg_Hydralisk);
        }

        // Add Observers if we have a Reaver
        if (vis(Protoss_Reaver) >= 1) {
            techList.insert(Protoss_Observer);
            unlockedType.insert(Protoss_Observer);
        }

        // Add Reavers if we have a Observer
        if (Players::vP() && vis(Protoss_Observer) >= 1 && currentTransition != "4Gate") {
            techList.insert(Protoss_Reaver);
            unlockedType.insert(Protoss_Reaver);
        }

        // Add Shuttles if we have Reavers/HT
        if (com(Protoss_Robotics_Facility) > 0 && (isTechUnit(Protoss_Reaver) || isTechUnit(Protoss_High_Templar) || (Players::vP() && !Strategy::needDetection() && isTechUnit(Protoss_Observer)))) {
            unlockedType.insert(Protoss_Shuttle);
            techList.insert(Protoss_Shuttle);
        }

        //// Add HT or Arbiter if enemy has detection
        //if (com(Protoss_Dark_Templar) >= 1) {
        //    auto hasDetection = Units::getEnemyCount(Protoss_Observer) > 0
        //        || Units::getEnemyCount(Protoss_Photon_Cannon) > 0
        //        || Units::getEnemyCount(Terran_Science_Vessel) > 0
        //        || Units::getEnemyCount(Terran_Missile_Turret) > 0 
        //        || Units::getEnemyCount(Terran_Vulture_Spider_Mine) > 0
        //        || Units::getEnemyCount(Zerg_Overlord) > 0;

        //    auto substitute = Players::vT() ? Protoss_Arbiter : Protoss_High_Templar;

        //    if (!Strategy::enemyPressure()) {
        //        unlockedType.insert(substitute);
        //        techList.insert(substitute);
        //    }
        //}
    }

    void checkAllTech()
    {
        bool moreToAdd;
        set<UnitType> toCheck;

        // For every unit in our tech list, ensure we are building the required buildings
        for (auto &type : techList) {
            toCheck.insert(type);
            toCheck.insert(type.whatBuilds().first);
        }

        // Iterate all required branches of buildings that are required for this tech unit
        do {
            moreToAdd = false;
            for (auto &check : toCheck) {
                for (auto &pair : check.requiredUnits()) {
                    UnitType type(pair.first);
                    if (Broodwar->self()->completedUnitCount(type) == 0 && toCheck.find(type) == toCheck.end()) {
                        toCheck.insert(type);
                        moreToAdd = true;
                    }
                }
            }
        } while (moreToAdd);

        // For each building we need to check, add to our queue whatever is possible to build based on its required branch
        for (auto &check : toCheck) {

            if (!check.isBuilding())
                continue;

            bool canAdd = true;
            for (auto &pair : check.requiredUnits()) {
                UnitType type(pair.first);
                if (type.isBuilding() && !isAlmostComplete(type)) {
                    canAdd = false;
                }
            }

            // HACK: Our check doesn't look for required buildings for tech needed for Lurkers
            if (check == Zerg_Lurker)
                itemQueue[Zerg_Lair] = Item(1);

            // Add extra production - TODO: move to shouldAddProduction
            int s = Players::getSupply(PlayerState::Self);
            if (canAdd && buildCount(check) <= 1) {
                if (check == Protoss_Stargate) {
                    if ((s >= 250 && techList.find(Protoss_Corsair) != techList.end())
                        || (s >= 300 && techList.find(Protoss_Arbiter) != techList.end())
                        || (s >= 100 && techList.find(Protoss_Carrier) != techList.end()))
                        itemQueue[check] = Item(2);
                    else
                        itemQueue[check] = Item(1);
                }
                else if (check != Protoss_Gateway)
                    itemQueue[check] = Item(1);

            }
        }
    }

    void checkExoticTech()
    {
        // Corsair/Scout upgrades
        if ((techList.find(Protoss_Scout) != techList.end() || techList.find(Protoss_Corsair) != techList.end()) && Players::getSupply(PlayerState::Self) >= 300)
            itemQueue[Protoss_Fleet_Beacon] = Item(1);

        // Hive upgrades
        if (Broodwar->self()->getRace() == Races::Zerg && Players::getSupply(PlayerState::Self) >= 200) {
            itemQueue[Zerg_Queens_Nest] = Item(1);
            itemQueue[Zerg_Hive] = Item(com(Zerg_Queens_Nest) >= 1);
            itemQueue[Zerg_Lair] = Item(com(Zerg_Queens_Nest) < 1);
        }
    }

    void setLearnedBuild(string newBuild, string newOpener, string newTransition) {
        currentBuild = newBuild;
        currentOpener = newOpener;
        currentTransition = newTransition;
    }

    map<BWAPI::UnitType, Item>& getItemQueue() { return itemQueue; }
    UnitType getTechUnit() { return techUnit; }
    UpgradeType getFirstUpgrade() { return firstUpgrade; }
    TechType getFirstTech() { return firstTech; }
    set <UnitType>& getTechList() { return  techList; }
    set <UnitType>& getUnlockedList() { return  unlockedType; }
    int gasWorkerLimit() { return gasLimit; }
    bool isWorkerCut() { return cutWorkers; }
    bool isUnitUnlocked(UnitType unit) { return unlockedType.find(unit) != unlockedType.end(); }
    bool isTechUnit(UnitType unit) { return techList.find(unit) != techList.end(); }
    bool isOpener() { return getOpening; }
    bool isFastExpand() { return fastExpand; }
    bool shouldScout() { return scout; }
    bool isWallNat() { return wallNat; }
    bool isWallMain() { return wallMain; }
    bool isProxy() { return proxy; }
    bool isHideTech() { return hideTech; }
    bool isPlayPassive() { return playPassive; }
    bool isRush() { return rush; }
    bool isGasTrick() { return gasTrick; }
    string getCurrentBuild() { return currentBuild; }
    string getCurrentOpener() { return currentOpener; }
    string getCurrentTransition() { return currentTransition; }
}