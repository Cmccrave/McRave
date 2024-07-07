#include "Main/McRave.h"
#include "BuildOrder.h"
#include <fstream>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "Builds/Zerg/ZergBuildOrder.h"
#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Builds/Terran//TerranBuildOrder.h"

namespace McRave::BuildOrder
{
    namespace {

        void updateBuild()
        {
            // Set s for better build readability - TODO: better build order management
            buildQueue.clear();
            upgradeQueue.clear();
            techQueue.clear();
            armyComposition.clear();

            // TODO: Dont wipe this here, use a queue instead
            focusUnit = UnitTypes::None;

            // TODO: Check if we own a <race> unit - have a build order allowed PER race for FFA weirdness and maybe mind control shenanigans
            if (Players::getSupply(PlayerState::Self, Races::Protoss) > 0) {
                s = Players::getSupply(PlayerState::Self, Races::Protoss);
                Protoss::opener();
                Protoss::tech();
                Protoss::composition();
                Protoss::situational();
                Protoss::unlocks();
            }
            if (Players::getSupply(PlayerState::Self, Races::Terran) > 0) {
                s = Players::getSupply(PlayerState::Self, Races::Terran);
                Terran::opener();
                Terran::tech();
                Terran::composition();
                Terran::situational();
                Terran::unlocks();
            }
            if (Players::getSupply(PlayerState::Self, Races::Zerg) > 0) {
                s = Players::getSupply(PlayerState::Self, Races::Zerg);
                Zerg::opener();
                Zerg::tech();
                Zerg::composition();
                Zerg::situational();
                Zerg::unlocks();
            }
        }
    }

    int getGasQueued()
    {
        int gasQueued = 0;
        for (auto &[building, count] : buildQueue) {
            int morphOffset = 0;
            if (building == Zerg_Creep_Colony)
                morphOffset = vis(Zerg_Sunken_Colony) + vis(Zerg_Spore_Colony);
            if (building == Zerg_Hatchery)
                morphOffset = vis(Zerg_Lair) + vis(Zerg_Hive);
            if (building == Zerg_Lair)
                morphOffset = vis(Zerg_Hive);

            if (count > vis(building) + morphOffset)
                gasQueued += building.gasPrice() * (count - vis(building) - morphOffset);
        }
        return gasQueued;
    }

    int getMinQueued()
    {
        int minQueued = 0;
        for (auto &[building, count] : buildQueue) {
            int morphOffset = 0;
            if (building == Zerg_Creep_Colony)
                morphOffset = vis(Zerg_Sunken_Colony) + vis(Zerg_Spore_Colony);
            if (building == Zerg_Hatchery)
                morphOffset = vis(Zerg_Lair) + vis(Zerg_Hive);
            if (building == Zerg_Lair)
                morphOffset = vis(Zerg_Hive);

            if (count > vis(building) + morphOffset)
                minQueued += building.mineralPrice() * (count - vis(building) - morphOffset);            
        }
        return minQueued;
    }

    bool atPercent(UnitType t, double percent) {
        if (com(t) > 0)
            return true;

        // Estimate how long until a building finishes based on how far it is from the nearest worker
        auto closestBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) {
            return u->getType() == t;
        });
        auto closestWorker = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) {
            return u->getType() == Broodwar->self()->getRace().getWorker();
        });

        if (closestBuilding && closestWorker)
            return double(t.buildTime() - closestBuilding->unit()->getRemainingBuildTime()) / double(t.buildTime()) >= percent;
        return false;
    }

    bool atPercent(TechType t, double percent) {
        if (Broodwar->self()->hasResearched(t))
            return true;

        // Estimate how long until a building finishes based on how far it is from the nearest worker
        auto closestBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) {
            return u->getType() == t.whatResearches() && u->unit()->isResearching();
        });
        return closestBuilding != nullptr;
    }

    bool techComplete()
    {
        if (Spy::enemyInvis() && focusUnit == Protoss_Observer)
            return vis(Protoss_Robotics_Facility) > 0;

        // When 1 unit finishes
        if (focusUnit == Protoss_Scout || focusUnit == Protoss_Corsair || focusUnit == Protoss_Reaver || focusUnit == Protoss_Observer || focusUnit == Terran_Science_Vessel)
            return com(focusUnit) > 0;

        // When 2 units are visible
        if (focusUnit == Protoss_High_Templar)
            return vis(focusUnit) >= 1 && Broodwar->self()->hasResearched(TechTypes::Psionic_Storm);

        // When 2 units finish
        if (focusUnit == Protoss_Dark_Templar)
            return total(focusUnit) >= 4;

        // When timing attack finishes
        if (focusUnit == Zerg_Mutalisk || focusUnit == Zerg_Hydralisk)
            return total(focusUnit) >= 6;
        if (focusUnit == Zerg_Lurker) {
            auto vsMech = Spy::getEnemyTransition() == "2Fact"
                || Spy::getEnemyTransition() == "1FactTanks"
                || Spy::getEnemyTransition() == "5FactGoliath";
            return Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect) || vsMech;
        }

        // When 1 unit is visible
        return vis(focusUnit) > 0;
    }

    double getCompositionPercentage(UnitType unit)
    {
        auto ptr = armyComposition.find(unit);
        if (ptr != armyComposition.end())
            return ptr->second;
        return 0.0;
    }

    int buildCount(UnitType unit)
    {
        auto ptr = buildQueue.find(unit);
        if (ptr != buildQueue.end())
            return ptr->second;
        return 0;
    }

    bool unlockReady(UnitType type) {
        bool ready = false;

        // P
        if (type == Protoss_High_Templar || type == Protoss_Dark_Templar || type == Protoss_Archon || type == Protoss_Dark_Archon)
            ready = com(Protoss_Citadel_of_Adun) > 0;
        if (type == Protoss_Corsair || type == Protoss_Scout)
            ready = com(Protoss_Stargate) > 0;
        if (type == Protoss_Reaver || type == Protoss_Observer)
            ready = com(Protoss_Robotics_Facility) > 0;
        if (type == Protoss_Carrier)
            ready = com(Protoss_Fleet_Beacon) > 0;
        if (type == Protoss_Arbiter)
            ready = com(Protoss_Arbiter_Tribunal) > 0;

        // Z
        if (type == Zerg_Mutalisk)
            ready = vis(Zerg_Spire) > 0;
        if (type == Zerg_Hydralisk)
            ready = vis(Zerg_Hydralisk_Den) > 0;
        if (type == Zerg_Lurker)
            ready = vis(Zerg_Hydralisk_Den) > 0 && vis(Zerg_Lair) > 0;

        return ready;
    }

    void getNewTech()
    {
        // If we already have a tech choice based on build, only try to unlock it and nothing else for now
        if (focusUnit != None && !isFocusUnit(focusUnit)) {
            if (unlockReady(focusUnit)) {
                getTech = false;
                focusUnits.insert(focusUnit);
                unlockedType.insert(focusUnit);
            }
            return;
        }

        if (getTech) {

            // If we already chose a tech unit
            if (focusUnit != None) {
                getTech = false;
                focusUnits.insert(focusUnit);
                unlockedType.insert(focusUnit);
                return;
            }

            // Select next tech based on the order
            for (auto &type : unitOrder) {
                if (!isFocusUnit(type)) {
                    focusUnit = type;
                    getTech = false;
                    focusUnits.insert(focusUnit);
                    unlockedType.insert(focusUnit);
                    break;
                }
            }
        }
    }

    void getTechBuildings()
    {
        const auto notCompleted = [&](UnitType type) {
            int morphOffset = 0;
            if (type == Zerg_Creep_Colony)
                morphOffset = vis(Zerg_Sunken_Colony) + vis(Zerg_Spore_Colony);
            if (type == Zerg_Hatchery)
                morphOffset = vis(Zerg_Lair) + vis(Zerg_Hive);
            if (type == Zerg_Lair)
                morphOffset = vis(Zerg_Hive);
            return (int(atPercent(type, 0.95)) + morphOffset) == 0;
        };

        // For every unit in our tech list, ensure we are building the required buildings
        vector<UnitType> toCheck;
        vector<UnitType> checkedAlready;

        // Add any buildable requisite buildings to make this unit
        std::function<void(UnitType)>addBuildableRequisites = [&](auto &type) {
            for (auto &[parent, _] : type.requiredUnits()) {
                if (parent == Zerg_Larva || parent.isWorker())
                    continue;
                if (notCompleted(parent) && find(checkedAlready.begin(), checkedAlready.end(), parent) == checkedAlready.end()) {
                    checkedAlready.push_back(type);
                    addBuildableRequisites(parent);
                    return;
                }
            }
            toCheck.push_back(type);
        };
        for (auto &type : focusUnits)
            addBuildableRequisites(type);
        reverse(toCheck.begin(), toCheck.end());

        // Some hardcoded requirements
        if (isFocusUnit(Zerg_Zergling))
            toCheck.push_back(Zerg_Hive);

        // For each building we need to check, add to our queue whatever is possible to build based on its required branch
        for (auto &check : toCheck) {
            if (!check.isBuilding())
                continue;

            // Queue tech structure
            if (buildCount(check) <= 0) {
                buildQueue[check] = 1;
                break;
            }
        }
    }

    void setLearnedBuild(string newBuild, string newOpener, string newTransition) {
        currentBuild = newBuild;
        currentOpener = newOpener;
        currentTransition = newTransition;
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateBuild();
        Visuals::endPerfTest("BuildOrder");
    }

    // Focus
    UnitType getFirstFocusUnit() { return focusUnit; }

    // getFocusTechs
    // getFocusUpgrades

    bool isFocusUnit(UnitType unit) { return focusUnits.find(unit) != focusUnits.end(); }
    // bool isFocusTech
    // bool isFocusUpgrade

    map<UnitType, int>& getBuildQueue() { return buildQueue; }
    map<UpgradeType, int>& getUpgradeQueue() { return upgradeQueue; }
    map<TechType, int>& getTechQueue() { return techQueue; }
    map<UnitType, double> getArmyComposition() { return armyComposition; }

    set<UnitType>& getUnlockedList() { return  unlockedType; }
    int gasWorkerLimit() { return gasLimit; }
    int getUnitReservation(UnitType type) { 
        if (unitReservations.find(type) == unitReservations.end())
            return 0;
        return unitReservations[type]; 
    }

    bool isAllIn() {
        return activeAllin.isActive();
    }

    bool isUnitUnlocked(UnitType unit) { return unlockedType.find(unit) != unlockedType.end(); }
    
    bool isOpener() { return inOpening; }
    bool takeNatural() { return wantNatural; }
    bool takeThird() { return wantThird; }
    bool isWallNat() { return wallNat; }
    bool isWallMain() { return wallMain; }
    bool isProxy() { return proxy; }
    bool isHideTech() { return hideTech; }
    bool isRush() { return rush; }
    bool isPressure() { return pressure; }
    bool isGasTrick() { return gasTrick; }
    bool isPlanEarly() { return planEarly; }
    bool shouldScout() { return scout; }
    bool shouldExpand() { return expandDesired; }
    bool shouldRamp() { return rampDesired; }
    bool mineralThirdDesired() { return mineralThird; }
    string getCurrentBuild() { return currentBuild; }
    string getCurrentOpener() { return currentOpener; }
    string getCurrentTransition() { return currentTransition; }
}