#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Units {

    namespace {
        map <Unit, UnitInfo> enemyUnits;
        map <Unit, UnitInfo> myUnits;
        map <Unit, UnitInfo> allyUnits;
        map <Unit, UnitInfo> neutrals;
        map <UnitSizeType, int> allySizes;
        map <UnitSizeType, int> enemySizes;
        map <UnitType, int> enemyTypes;
        map <UnitType, int> myVisibleTypes;
        map <UnitType, int> myCompleteTypes;
        map <Role, int> myRoles;
        set<Unit> splashTargets;
        double immThreat, proxThreat;
        double globalAllyGroundStrength, globalEnemyGroundStrength;
        double globalAllyAirStrength, globalEnemyAirStrength;
        double allyDefense;
        int supply = 8;
        int scoutDeadFrame = 0;
        Position armyCenter;

        void updateUnitSizes()
        {
            allySizes.clear();
            enemySizes.clear();

            for (auto &u : myUnits) {
                auto &unit = u.second;
                if (unit.getRole() == Role::Combat)
                    allySizes[unit.getType().size()]++;
            }

            for (auto &u : enemyUnits) {
                auto &unit = u.second;
                if (!unit.getType().isBuilding() && !unit.getType().isWorker())
                    enemySizes[unit.getType().size()]++;
            }
        }

        void updateRole(UnitInfo& unit)
        {
            // Don't assign a role to uncompleted units
            if (!unit.unit()->isCompleted() && !unit.getType().isBuilding() && unit.getType() != UnitTypes::Zerg_Egg) {
                unit.setRole(Role::None);
                return;
            }

            // Store old role to update counters after
            auto oldRole = unit.getRole();

            // Update default role
            if (unit.getRole() == Role::None) {
                if (unit.getType().isWorker())
                    unit.setRole(Role::Worker);
                else if ((unit.getType().isBuilding() && unit.getGroundDamage() == 0.0 && unit.getAirDamage() == 0.0) || unit.getType() == UnitTypes::Zerg_Larva || unit.getType() == UnitTypes::Zerg_Egg)
                    unit.setRole(Role::Production);
                else if (unit.getType().isBuilding() && unit.getGroundDamage() != 0.0 && unit.getAirDamage() != 0.0)
                    unit.setRole(Role::Defender);
                else if (unit.getType().spaceProvided() > 0)
                    unit.setRole(Role::Transport);
                else
                    unit.setRole(Role::Combat);
            }

            // Check if workers should fight or work
            if (unit.getType().isWorker()) {
                if (unit.getRole() == Role::Worker && (Util::reactivePullWorker(unit) || Util::proactivePullWorker(unit) || Util::pullRepairWorker(unit)))
                    unit.setRole(Role::Combat);
                else if (unit.getRole() == Role::Combat && !Util::reactivePullWorker(unit) && !Util::proactivePullWorker(unit) && !Util::pullRepairWorker(unit))
                    unit.setRole(Role::Worker);
            }

            // Check if an overlord should scout or support
            if (unit.getType() == UnitTypes::Zerg_Overlord) {
                if (unit.getRole() == Role::None || getMyRoleCount(Role::Scout) < getMyRoleCount(Role::Support) + 1)
                    unit.setRole(Role::Scout);
                else if (getMyRoleCount(Role::Support) < getMyRoleCount(Role::Scout) + 1)
                    unit.setRole(Role::Support);
            }

            // Check if we should scout - TODO: scout count from scout manager
            if (BWEB::Map::getNaturalChoke() && BuildOrder::shouldScout() && getMyRoleCount(Role::Scout) < 1 && Broodwar->getFrameCount() - scoutDeadFrame > 500) {
                auto type = Broodwar->self()->getRace().getWorker();
                auto scout = Util::getClosestUnit(Position(BWEB::Map::getNaturalChoke()->Center()), PlayerState::Self, [&](auto &u) {
                    return u.getType().isWorker();
                });

                if (scout == &unit) {

                    if (unit.hasResource())
                        unit.getResource().setGathererCount(scout->getResource().getGathererCount() - 1);

                    unit.setRole(Role::Scout);
                    unit.setResource(nullptr);
                    unit.setBuildingType(UnitTypes::None);
                    unit.setBuildPosition(TilePositions::Invalid);
                }
            }

            // Check if a worker morphed into a building
            if (unit.getRole() == Role::Worker && unit.getType().isBuilding()) {
                if (unit.getType().isBuilding() && unit.getGroundDamage() == 0.0 && unit.getAirDamage() == 0.0)
                    unit.setRole(Role::Production);
                else
                    unit.setRole(Role::Combat);
            }

            // Detectors and Support roles
            if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == UnitTypes::Protoss_Arbiter)
                unit.setRole(Role::Support);

            // Increment new role counter, decrement old role counter
            auto newRole = unit.getRole();
            if (oldRole != newRole) {
                if (oldRole != Role::None)
                    myRoles[oldRole] --;
                if (newRole != Role::None)
                    myRoles[newRole] ++;
            }
        }

        void updateUnits()
        {
            // Reset calculations and caches
            globalEnemyGroundStrength = 0.0;
            globalEnemyAirStrength = 0.0;
            globalAllyGroundStrength = 0.0;
            globalAllyAirStrength = 0.0;
            immThreat = 0.0;
            proxThreat = 0.0;
            allyDefense = 0.0;
            splashTargets.clear();
            enemyTypes.clear();
            myVisibleTypes.clear();
            myCompleteTypes.clear();
            supply = 0;

            // Update Enemy Units
            for (auto &u : enemyUnits) {
                UnitInfo &unit = u.second;

                // If this is a flying building that we haven't recognized as being a flyer, remove overlap tiles
                auto flyingBuilding = unit.unit()->exists() && !unit.isFlying() && (unit.unit()->getOrder() == Orders::LiftingOff || unit.unit()->getOrder() == Orders::BuildingLiftOff || unit.unit()->isFlying());

                if (flyingBuilding && unit.getLastTile().isValid()) {
                    for (int x = unit.getLastTile().x; x < unit.getLastTile().x + unit.getType().tileWidth(); x++) {
                        for (int y = unit.getLastTile().y; y < unit.getLastTile().y + unit.getType().tileHeight(); y++) {
                            TilePosition t(x, y);
                            if (!t.isValid())
                                continue;

                            BWEB::Map::removeUsed(t, 1, 1);
                        }
                    }
                }

                // If unit is visible, update it
                if (unit.unit()->exists()) {
                    unit.updateUnit();

                    if (unit.hasTarget() && (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Protoss_Scarab))
                        splashTargets.insert(unit.getTarget().unit());

                    if (unit.getType().isBuilding() && !unit.isFlying() && unit.unit()->exists() && !BWEB::Map::isUsed(unit.getTilePosition()))
                        BWEB::Map::addUsed(unit.getTilePosition(), unit.getType().tileWidth(), unit.getType().tileHeight());
                }

                // Must see a 3x3 grid of Tiles to set a unit to invalid position
                if (!unit.unit()->exists() && (!unit.isBurrowed() || Command::overlapsAllyDetection(unit.getPosition()) || (unit.getWalkPosition().isValid() && Grids::getAGroundCluster(unit.getWalkPosition()) > 0)) && unit.getPosition().isValid()) {
                    bool move = true;
                    for (int x = unit.getTilePosition().x - 1; x < unit.getTilePosition().x + 1; x++) {
                        for (int y = unit.getTilePosition().y - 1; y < unit.getTilePosition().y + 1; y++) {
                            TilePosition t(x, y);
                            if (t.isValid() && !Broodwar->isVisible(t))
                                move = false;
                        }
                    }
                    if (move) {
                        unit.setPosition(Positions::Invalid);
                        unit.setTilePosition(TilePositions::Invalid);
                        unit.setWalkPosition(WalkPositions::Invalid);
                    }
                }

                // If unit has a valid type, update enemy composition tracking
                if (unit.getType().isValid())
                    enemyTypes[unit.getType()] += 1;

                // If unit is not a worker or building, add it to global strength	
                if (!unit.getType().isWorker())
                    unit.getType().isFlyer() ? globalEnemyAirStrength += unit.getVisibleAirStrength() : globalEnemyGroundStrength += unit.getVisibleGroundStrength();

                // If a unit is threatening our position
                if (unit.isThreatening() && (unit.getType().groundWeapon().damageAmount() > 0 || unit.getType() == UnitTypes::Terran_Bunker)) {
                    if (unit.getType().isBuilding())
                        immThreat += 1.50;
                    else
                        immThreat += unit.getVisibleGroundStrength();
                }
                if (unit.isThreatening())
                    unit.circleRed();
            }

            // Update myUnits
            double centerCluster = 0.0;
            for (auto &u : myUnits) {
                auto &unit = u.second;

                unit.updateUnit();
                updateRole(unit);

                if (unit.getRole() == Role::Combat) {
                    double g = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
                    if (g > centerCluster) {
                        centerCluster = g;
                        armyCenter = unit.getPosition();
                    }
                }

                auto type = unit.getType() == UnitTypes::Zerg_Egg ? unit.unit()->getBuildType() : unit.getType();
                if (unit.unit()->isCompleted()) {
                    myCompleteTypes[type] ++;
                    myVisibleTypes[type] ++;
                }
                else {
                    myVisibleTypes[type] ++;
                }

                supply += type.supplyRequired();

                // If unit is not a building and deals damage, add it to global strength	
                if (!unit.getType().isBuilding())
                    unit.getType().isFlyer() ? globalAllyAirStrength += unit.getVisibleAirStrength() : globalAllyGroundStrength += unit.getVisibleGroundStrength();
            }

            for (auto &u : neutrals) {
                auto &unit = u.second;
                if (!unit.unit() || !unit.unit()->exists())
                    continue;
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateUnitSizes();
        updateUnits();
        Visuals::endPerfTest("Units");
    }
       
    int getEnemyCount(UnitType t)
    {
        map<UnitType, int>::iterator itr = enemyTypes.find(t);
        if (itr != enemyTypes.end())
            return itr->second;
        return 0;
    }

    void storeUnit(Unit unit)
    {
        auto &info = unit->getPlayer() == Broodwar->self() ? myUnits[unit] : (unit->getPlayer() == Broodwar->enemy() ? enemyUnits[unit] : allyUnits[unit]);
        info.setUnit(unit);
        info.updateUnit();

        if (unit->getPlayer() == Broodwar->self() && unit->getType() == UnitTypes::Protoss_Pylon)
            Pylons::storePylon(unit);
    }

    void removeUnit(Unit unit)
    {
        BWEB::Map::onUnitDestroy(unit);

        for (auto &u : myUnits) {
            auto &info = u.second;
            if (info.hasTarget() && info.getTarget().unit() == unit)
                info.setTarget(nullptr);
        }

        for (auto &u : enemyUnits) {
            auto &info = u.second;
            if (info.hasTarget() && info.getTarget().unit() == unit)
                info.setTarget(nullptr);
        }

        if (myUnits.find(unit) != myUnits.end()) {
            auto &info = myUnits[unit];

            if (info.hasResource())
                info.getResource().setGathererCount(info.getResource().getGathererCount() - 1);
            if (info.getRole() != Role::None)
                myRoles[info.getRole()]--;
            if (info.getRole() == Role::Scout)
                scoutDeadFrame = Broodwar->getFrameCount();

            Transports::removeUnit(unit);
            myUnits.erase(unit);
        }
        else if (enemyUnits.find(unit) != enemyUnits.end())
            enemyUnits.erase(unit);
        else if (allyUnits.find(unit) != allyUnits.end())
            allyUnits.erase(unit);
        else if (neutrals.find(unit) != neutrals.end())
            neutrals.erase(unit);
    }

    void morphUnit(Unit unit)
    {
        if (myUnits.find(unit) != myUnits.end()) {
            auto &info = myUnits[unit];
            info.setUnit(unit);
            info.updateUnit();

            // Remove all assignments
            if (info.hasResource()) {
                info.getResource().setGathererCount(info.getResource().getGathererCount() - 1);
                info.setResource(nullptr);
            }

            info.setBuildingType(UnitTypes::None);
            info.setBuildPosition(TilePositions::Invalid);
            info.setTarget(nullptr);
        }
    }

    Position getArmyCenter() { return armyCenter; }
    set<Unit>& getSplashTargets() { return splashTargets; }
    map<Unit, UnitInfo>& getMyUnits() { return myUnits; }
    map<Unit, UnitInfo>& getEnemyUnits() { return enemyUnits; }
    map<Unit, UnitInfo>& getNeutralUnits() { return neutrals; }
    map<UnitSizeType, int>& getAllySizes() { return allySizes; }
    map<UnitSizeType, int>& getEnemySizes() { return enemySizes; }
    map<UnitType, int>& getEnemyTypes() { return enemyTypes; }
    double getImmThreat() { return immThreat; }
    double getProxThreat() { return proxThreat; }
    double getGlobalAllyGroundStrength() { return globalAllyGroundStrength; }
    double getGlobalEnemyGroundStrength() { return globalEnemyGroundStrength; }
    double getGlobalAllyAirStrength() { return globalAllyAirStrength; }
    double getGlobalEnemyAirStrength() { return globalEnemyAirStrength; }
    double getAllyDefense() { return allyDefense; }
    int getSupply() { return supply; }
    int getMyRoleCount(Role role) { return myRoles[role]; }
    int getMyVisible(UnitType type) { return myVisibleTypes[type]; }
    int getMyComplete(UnitType type) { return myCompleteTypes[type]; }
}