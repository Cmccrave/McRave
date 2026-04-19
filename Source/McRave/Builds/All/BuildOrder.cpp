#include "BuildOrder.h"

#include <fstream>

#include "Builds/Protoss/ProtossBuildOrder.h"
#include "Builds/Terran//TerranBuildOrder.h"
#include "Builds/Zerg/ZergBuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Main/Common.h"
#include "Main/Visuals.h"
#include "Map/Terrain/Terrain.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder {
    namespace {

        void updateBuild()
        {
            // Set s for better build readability - TODO: better build order management
            buildQueue.clear();
            upgradeQueue.clear();
            techQueue.clear();
            armyComposition.clear();

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
    } // namespace

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

    bool atPercent(UnitType t, double percent)
    {
        if (com(t) > 0)
            return true;

        // Estimate how long until a building finishes based on how far it is from the nearest worker
        auto closestBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == t; });
        auto closestWorker   = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == Broodwar->self()->getRace().getWorker(); });

        if (closestBuilding && closestWorker)
            return double(t.buildTime() - closestBuilding->unit()->getRemainingBuildTime()) / double(t.buildTime()) >= percent;
        return false;
    }

    bool atPercent(TechType t, double percent)
    {
        if (Broodwar->self()->hasResearched(t))
            return true;

        // Estimate how long until a building finishes based on how far it is from the nearest worker
        auto closestBuilding = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == t.whatResearches() && u->unit()->isResearching(); });
        return closestBuilding != nullptr;
    }

    bool techComplete()
    {
        for (auto &type : focusUnits) {

            // Trigger detection completion early
            if (Spy::enemyInvis() && type == Protoss_Observer)
                return total(Protoss_Robotics_Facility) > 0;

            // When caster units are ready
            if (type == Protoss_High_Templar)
                return total(type) > 0 && Players::hasResearched(PlayerState::Self, TechTypes::Psionic_Storm);
            if (type == Zerg_Defiler)
                return total(type) > 0 && Players::hasResearched(PlayerState::Self, TechTypes::Consume);

            // When timing attack finishes
            if (type == Protoss_Dark_Templar)
                return total(type) >= 4;
            if (type == Zerg_Mutalisk || type == Zerg_Hydralisk)
                return total(type) >= 6;
            if (type == Zerg_Lurker)
                return Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect) || Spy::Terran::enemyMech();

            // Default to any completion
            if (total(type) == 0)
                return false;
        }
        return true;
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

    bool unlockReady(UnitType type)
    {
        // P
        if (type == Protoss_Probe)
            return true;
        if (type == Protoss_Zealot)
            return com(Protoss_Gateway) > 0;
        if (type == Protoss_Dragoon)
            return com(Protoss_Gateway) > 0 && com(Protoss_Cybernetics_Core) > 0;
        if (type == Protoss_High_Templar || type == Protoss_Dark_Templar || type == Protoss_Archon || type == Protoss_Dark_Archon)
            return com(Protoss_Gateway) > 0 && com(Protoss_Templar_Archives) > 0;
        if (type == Protoss_Corsair || type == Protoss_Scout)
            return com(Protoss_Stargate) > 0;
        if (type == Protoss_Carrier)
            return com(Protoss_Stargate) > 0 && com(Protoss_Fleet_Beacon) > 0;
        if (type == Protoss_Arbiter)
            return com(Protoss_Stargate) > 0 && com(Protoss_Arbiter_Tribunal) > 0;
        if (type == Protoss_Shuttle)
            return com(Protoss_Robotics_Facility) > 0;
        if (type == Protoss_Reaver)
            return com(Protoss_Robotics_Facility) > 0 && com(Protoss_Robotics_Support_Bay) > 0;
        if (type == Protoss_Observer)
            return com(Protoss_Robotics_Facility) > 0 && (Protoss_Observatory) > 0;

        // T
        if (type == Terran_SCV)
            return true;
        if (type == Terran_Marine)
            return com(Terran_Barracks) > 0;
        if (type == Terran_Firebat || type == Terran_Medic)
            return com(Terran_Academy) > 0;
        if (type == Terran_Vulture)
            return com(Terran_Factory) > 0;
        if (type == Terran_Siege_Tank_Tank_Mode)
            return com(Terran_Machine_Shop) > 0;
        if (type == Terran_Goliath)
            return com(Terran_Armory) > 0;
        if (type == Terran_Wraith)
            return com(Terran_Starport) > 0;
        if (type == Terran_Dropship)
            return com(Terran_Starport) > 0 && com(Terran_Control_Tower) > 0;
        if (type == Terran_Valkyrie)
            return com(Terran_Starport) > 0 && com(Terran_Control_Tower) > 0 && com(Terran_Armory) > 0;
        if (type == Terran_Science_Vessel)
            return com(Terran_Starport) > 0 && com(Terran_Control_Tower) > 0 && com(Terran_Science_Facility) > 0;
        if (type == Terran_Battlecruiser)
            return com(Terran_Starport) > 0 && com(Terran_Control_Tower) > 0 && com(Terran_Physics_Lab) > 0;

        // Z
        if (type == Zerg_Drone)
            return true;
        if (type == Zerg_Zergling)
            return com(Zerg_Spawning_Pool) > 0;
        if (type == Zerg_Mutalisk)
            return com(Zerg_Spire) > 0;
        if (type == Zerg_Devourer || type == Zerg_Guardian)
            return com(Zerg_Greater_Spire) > 0;
        if (type == Zerg_Hydralisk)
            return com(Zerg_Hydralisk_Den) > 0;
        if (type == Zerg_Lurker)
            return com(Zerg_Hydralisk_Den) > 0 && com(Zerg_Lair) > 0 && Broodwar->self()->hasResearched(TechTypes::Lurker_Aspect);
        if (type == Zerg_Queen)
            return com(Zerg_Queens_Nest);
        if (type == Zerg_Ultralisk)
            return com(Zerg_Ultralisk_Cavern) > 0;
        if (type == Zerg_Defiler)
            return com(Zerg_Defiler_Mound) > 0;

        LOG_ONCE("Type not checked for training: ", type.c_str());
        return false;
    }

    void getNewTech()
    {
        // First one always gets inserted
        if (focusUnit != None && !isFocusUnit(focusUnit)) {
            focusUnits.insert(focusUnit);
            LOG("focusing existing ", focusUnit.c_str());
            return;
        }

        if (!getTech || !techComplete())
            return;

        // Select next tech based on the order
        for (auto &type : unitOrder) {
            if (!isFocusUnit(type)) {
                getTech = false;
                focusUnits.insert(type);
                LOG("focusing new ", type.c_str());
                return;
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
        std::function<void(UnitType)> addBuildableRequisites = [&](auto &type) {
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

    void setLearnedBuild(string_view newBuild, string_view newOpener, string_view newTransition)
    {
        currentBuild      = newBuild;
        currentOpener     = newOpener;
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

    map<UnitType, int> &getBuildQueue() { return buildQueue; }
    map<UpgradeType, int> &getUpgradeQueue() { return upgradeQueue; }
    map<TechType, int> &getTechQueue() { return techQueue; }
    map<UnitType, double> getArmyComposition() { return armyComposition; }

    set<UnitType> &getUnlockedList() { return unlockedType; }
    int gasWorkerLimit() { return gasLimit; }
    int getUnitReservation(UnitType type)
    {
        if (unitReservations.find(type) == unitReservations.end())
            return 0;
        return unitReservations[type];
    }

    bool isRush(UnitType type)
    {
        if (type != None)
            return unitRush[type];
        return rush;
    }

    bool isPressure(UnitType type)
    {
        if (type != None)
            return unitPressure[type];
        return pressure;
    }

    bool isAllIn() { return activeAllin.isActive(); }

    bool isPreparingAllIn() { return activeAllin.isPreparing(); }

    bool isUnitUnlocked(UnitType unit) { return unlockedType.find(unit) != unlockedType.end(); }

    int gasMax() { return Resources::getGasCount() * 3; }

    bool isOpener() { return inOpening; }
    bool takeNatural() { return wantNatural; }
    bool takeThird() { return wantThird; }
    bool isWallNat() { return wallNat; }
    bool isWallMain() { return wallMain; }
    bool isWallThird() { return wallThird; }
    bool isProxy() { return proxy; }
    bool isHideTech() { return hideTech; }
    bool isGasTrick() { return gasTrick; }
    bool isPlanEarly() { return planEarly; }
    bool shouldScout() { return scout; }
    bool shouldExpand() { return expandDesired; }
    bool shouldRamp() { return rampDesired; }
    bool mineralThirdDesired() { return mineralThird; }
    string getCurrentBuild() { return currentBuild; }
    string getCurrentOpener() { return currentOpener; }
    string getCurrentTransition() { return currentTransition; }
} // namespace McRave::BuildOrder