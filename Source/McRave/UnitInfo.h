#pragma once
#include <BWAPI.h>
#include "BWEB.h"

namespace McRave {

    class UnitInfo {
        double visibleGroundStrength = 0.0;
        double visibleAirStrength = 0.0;
        double maxGroundStrength = 0.0;
        double maxAirStrength = 0.0;
        double priority = 0.0;
        double percentHealth = 0.0;
        double percentShield = 0.0;
        double percentTotal = 0.0;
        double groundRange = 0.0;
        double groundReach = 0.0;
        double groundDamage = 0.0;
        double airRange = 0.0;
        double airReach = 0.0;
        double airDamage = 0.0;
        double speed = 0.0;
        double engageDist = 0.0;
        double simValue = 0.0;

        int lastAttackFrame = 0;
        int lastVisibleFrame = 0;
        int lastMoveFrame = 0;
        int lastThreateningFrame = 0;
        int resourceHeldFrames = 0;
        int remainingTrainFrame = 0;
        int frameCreated = 0;
        int shields = 0;
        int health = 0;
        int minStopFrame = 0;
        int energy = 0;
        int killCount = 0;

        bool burrowed = false;
        bool flying = false;

        BWAPI::Player player = nullptr;
        BWAPI::Unit thisUnit = nullptr;
        std::shared_ptr<UnitInfo> transport;
        std::shared_ptr<UnitInfo> target;
        std::shared_ptr<ResourceInfo> resource;

        std::set<std::shared_ptr<UnitInfo>> assignedCargo;
        std::set<std::shared_ptr<UnitInfo>> targetedBy;

        TransportState tState = TransportState::None;
        LocalState lState = LocalState::None;
        GlobalState gState = GlobalState::None;
        SimState sState = SimState::None;
        Role role = Role::None;

        BWAPI::UnitType unitType = BWAPI::UnitTypes::None;
        BWAPI::UnitType buildingType = BWAPI::UnitTypes::None;

        BWAPI::Position position = BWAPI::Positions::Invalid;
        BWAPI::Position engagePosition = BWAPI::Positions::Invalid;
        BWAPI::Position destination = BWAPI::Positions::Invalid;
        BWAPI::Position simPosition = BWAPI::Positions::Invalid;
        BWAPI::Position lastPos = BWAPI::Positions::Invalid;
        BWAPI::Position goal = BWAPI::Positions::Invalid;
        BWAPI::WalkPosition walkPosition = BWAPI::WalkPositions::Invalid;
        BWAPI::WalkPosition lastWalk = BWAPI::WalkPositions::Invalid;
        BWAPI::TilePosition tilePosition = BWAPI::TilePositions::Invalid;
        BWAPI::TilePosition buildPosition = BWAPI::TilePositions::Invalid;
        BWAPI::TilePosition lastTile = BWAPI::TilePositions::Invalid;

        BWEB::PathFinding::Path path;
        BWEB::PathFinding::Path resourcePath;
        BWEM::CPPath quickPath;
        
        void updateTarget();
        void updateStuckCheck();
    public:
        UnitInfo();

        std::set<std::shared_ptr<UnitInfo>>& getAssignedCargo() { return assignedCargo; }
        std::set<std::shared_ptr<UnitInfo>>& getTargetedBy() { return targetedBy; }
        TransportState getTransportState() { return tState; }
        SimState getSimState() { return sState; }
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }

        bool samePath() {
            return (path.getTiles().front() == target->getTilePosition() && path.getTiles().back() == tilePosition);
        }
        bool hasAttackedRecently() {            
            return (BWAPI::Broodwar->getFrameCount() - lastAttackFrame < 50);
        }
        bool isStuck() {
            return (BWAPI::Broodwar->getFrameCount() - lastMoveFrame > 50);
        }
        bool targetsFriendly() {
            return unitType == BWAPI::UnitTypes::Terran_Medic || unitType == BWAPI::UnitTypes::Terran_Science_Vessel || unitType == BWAPI::UnitTypes::Zerg_Defiler;
        }
        bool isSuicidal() {
            return unitType == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || unitType == BWAPI::UnitTypes::Zerg_Scourge || unitType == BWAPI::UnitTypes::Zerg_Infested_Terran;
        }
        bool isLightAir() {
            return unitType == BWAPI::UnitTypes::Protoss_Corsair || unitType == BWAPI::UnitTypes::Zerg_Mutalisk || unitType == BWAPI::UnitTypes::Terran_Wraith;
        }
        bool isCapitalShip() {
            return unitType == BWAPI::UnitTypes::Protoss_Carrier || unitType == BWAPI::UnitTypes::Terran_Battlecruiser || unitType == BWAPI::UnitTypes::Zerg_Guardian;
        }
        bool isHovering() {
            return unitType.isWorker() || unitType == BWAPI::UnitTypes::Protoss_Archon || unitType == BWAPI::UnitTypes::Protoss_Dark_Archon || unitType == BWAPI::UnitTypes::Terran_Vulture;
        }

        double getPercentHealth() { return percentHealth; }
        double getPercentShield() { return percentShield; }
        double getPercentTotal() { return percentTotal; }
        double getVisibleGroundStrength() { return visibleGroundStrength; }
        double getMaxGroundStrength() { return maxGroundStrength; }
        double getVisibleAirStrength() { return visibleAirStrength; }
        double getMaxAirStrength() { return maxAirStrength; }
        double getGroundRange() { return groundRange; }
        double getGroundReach() { return groundReach; }
        double getGroundDamage() { return groundDamage; }
        double getAirRange() { return airRange; }
        double getAirReach() { return airReach; }
        double getAirDamage() { return airDamage; }
        double getSpeed() { return speed; }
        double getPriority() { return priority; }
        double getEngDist() { return engageDist; }
        double getSimValue() { return simValue; }
        double getDistance(UnitInfo unit) { return position.getDistance(unit.getPosition()); }
        int getShields() { return shields; }
        int getHealth() { return health; }
        int getLastAttackFrame() { return lastAttackFrame; }
        int getMinStopFrame() { return minStopFrame; }
        int getLastVisibleFrame() { return lastVisibleFrame; }
        int getEnergy() { return energy; }
        int getFrameCreated() { return frameCreated; }
        int getRemainingTrainFrames() { return remainingTrainFrame; }
        int getKillCount() { return killCount; }
        int framesHoldingResource() { return resourceHeldFrames; }

        void circleRed() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Red); }
        void circleOrange() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Orange); }
        void circleYellow() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Yellow); }
        void circleGreen() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Green); }
        void circleBlue() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Blue); }
        void circlePurple() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Purple); }
        void circleBlack() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Black); }

        bool isThreatening();
        bool isHidden();
		bool canStartAttack();

        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool sameTile() { return lastTile == tilePosition; }

        bool hasResource() { return resource != nullptr; }
        bool hasTransport() { return transport != nullptr; }
        bool hasTarget() { return target != nullptr; }
        bool command(BWAPI::UnitCommandType, BWAPI::Position, bool);
        bool command(BWAPI::UnitCommandType, UnitInfo&);

        ResourceInfo &getResource() { return *resource; }
        UnitInfo &getTransport() { return *transport; }
        UnitInfo &getTarget() { return *target; }
        Role getRole() { return role; }

        BWAPI::Unit unit() { return thisUnit; }
        BWAPI::UnitType getType() { return unitType; }
        BWAPI::UnitType getBuildingType() { return buildingType; }
        BWAPI::Player getPlayer() { return player; }

        BWAPI::Position getPosition() { return position; }
        BWAPI::Position getEngagePosition() { return engagePosition; }
        BWAPI::Position getDestination() { return destination; }
        BWAPI::Position getLastPosition() { return lastPos; }
        BWAPI::Position getSimPosition() { return simPosition; }
        BWAPI::Position getGoal() { return goal; }
        BWAPI::WalkPosition getWalkPosition() { return walkPosition; }
        BWAPI::WalkPosition getLastWalk() { return lastWalk; }
        BWAPI::TilePosition getTilePosition() { return tilePosition; }
        BWAPI::TilePosition getBuildPosition() { return buildPosition; }
        BWAPI::TilePosition getLastTile() { return lastTile; }

        BWEB::PathFinding::Path& getPath() { return path; }
        BWEM::CPPath& getQuickPath() { return quickPath; }

        void updateUnit();
        void createDummy(BWAPI::UnitType);

        void setEngDist(double newValue) { engageDist = newValue; }
        void setSimValue(double newValue) { simValue = newValue; }

        void setLastAttackFrame(int newValue) { lastAttackFrame = newValue; }
        void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }
        void setCreationFrame() { frameCreated = BWAPI::Broodwar->getFrameCount(); }
        void setKillCount(int newValue) { killCount = newValue; }

        void setTransportState(TransportState newState) { tState = newState; }
        void setSimState(SimState newState) { sState = newState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        void setLocalState(LocalState newState) { lState = newState; }

        void setResource(const std::shared_ptr<ResourceInfo>& unit) { resource = unit; }
        void setTransport(const std::shared_ptr<UnitInfo>& unit) { transport = unit; }
        void setTarget(const std::shared_ptr<UnitInfo>& unit) { target = unit; }
        void setRole(Role newRole) { role = newRole; }

        void setUnit(BWAPI::Unit newUnit) { thisUnit = newUnit; }
        void setType(BWAPI::UnitType newType) { unitType = newType; }
        void setBuildingType(BWAPI::UnitType newType) { buildingType = newType; }
        void setPlayer(BWAPI::Player newPlayer) { player = newPlayer; }

        void setSimPosition(BWAPI::Position newPosition) { simPosition = newPosition; }
        void setPosition(BWAPI::Position newPosition) { position = newPosition; }
        void setEngagePosition(BWAPI::Position newPosition) { engagePosition = newPosition; }
        void setDestination(BWAPI::Position newPosition) { destination = newPosition; }
        void setGoal(BWAPI::Position newPosition) { goal = newPosition; }
        void setWalkPosition(BWAPI::WalkPosition newPosition) { walkPosition = newPosition; }
        void setTilePosition(BWAPI::TilePosition newPosition) { tilePosition = newPosition; }
        void setBuildPosition(BWAPI::TilePosition newPosition) { buildPosition = newPosition; }

        void setPath(BWEB::PathFinding::Path& newPath) { path = newPath; }
        void setQuickPath(BWEM::CPPath newPath) { quickPath = newPath; }

        void setLastPositions();

        bool operator== (UnitInfo& p) {
            return thisUnit == p.unit();
        }

        bool operator!= (UnitInfo& p) {
            return thisUnit != p.unit();
        }

        bool operator< (UnitInfo& p) {
            return thisUnit < p.unit();
        }
    };
}
