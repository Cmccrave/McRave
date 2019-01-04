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
        int resourceHeldFrames = 0;
        int remainingTrainFrame = 0;
        int frameCreated = 0;
        int shields = 0;
        int health = 0;
        int minStopFrame = 0;
        int energy = 0;
        int killCount = 0;
        int beingAttackedCount = 0;

        bool burrowed = false;
        bool flying = false;

        BWAPI::Player player = nullptr;
        BWAPI::Unit thisUnit = nullptr;
        UnitInfo * transport = nullptr;
        UnitInfo * target = nullptr;
        ResourceInfo * resource = nullptr;

        std::set<UnitInfo*> assignedCargo ={};

        TransportState tState = TransportState::None;
        LocalState lState = LocalState::None;
        GlobalState gState = GlobalState::None;
        SimState sState = SimState::None;
        Role role = Role::None;

        BWAPI::UnitType unitType = UnitTypes::None;
        BWAPI::UnitType buildingType = UnitTypes::None;

        BWAPI::Position position = Positions::Invalid;
        BWAPI::Position engagePosition = Positions::Invalid;
        BWAPI::Position destination = Positions::Invalid;
        BWAPI::Position simPosition = Positions::Invalid;
        BWAPI::Position lastPos = Positions::Invalid;
        BWAPI::WalkPosition walkPosition = WalkPositions::Invalid;
        BWAPI::WalkPosition lastWalk = WalkPositions::Invalid;
        BWAPI::TilePosition tilePosition = TilePositions::Invalid;
        BWAPI::TilePosition buildPosition = TilePositions::Invalid;
        BWAPI::TilePosition lastTile = TilePositions::Invalid;

        BWEB::PathFinding::Path path;
        BWEB::PathFinding::Path resourcePath;
        void updateTarget();
        void updateStuckCheck();
    public:
        UnitInfo();

        set<UnitInfo*>& getAssignedCargo() { return assignedCargo; }
        TransportState getTransportState() { return tState; }
        SimState getSimState() { return sState; }
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }

        bool samePath() {
            return (path.getTiles().front() == target->getTilePosition() && path.getTiles().back() == tilePosition);
        }
        bool hasAttackedRecently() {
            return (Broodwar->getFrameCount() - lastAttackFrame < 50);
        }
        bool isStuck() {
            return (Broodwar->getFrameCount() - lastMoveFrame > 50);
        }
        bool targetsFriendly() {
            return unitType == UnitTypes::Terran_Medic || unitType == UnitTypes::Terran_Science_Vessel || unitType == UnitTypes::Zerg_Defiler;
        }
        bool isSuicidal(){
            return unitType == UnitTypes::Terran_Vulture_Spider_Mine || unitType == UnitTypes::Zerg_Scourge || unitType == UnitTypes::Zerg_Infested_Terran;
        }
        bool isLightAir() {
            return unitType == UnitTypes::Protoss_Corsair || unitType == UnitTypes::Zerg_Mutalisk  || unitType == UnitTypes::Terran_Wraith;
        }
        bool isCapitalShip() {
            return unitType == UnitTypes::Protoss_Carrier || unitType == UnitTypes::Terran_Battlecruiser || unitType == UnitTypes::Zerg_Guardian;
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
        int getUnitsAttacking() { return beingAttackedCount; }
        int framesHoldingResource() { return resourceHeldFrames; }

        void circleRed() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Red); }
        void circleOrange() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Orange); }
        void circleYellow() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Yellow); }
        void circleGreen() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Green); }
        void circleBlue() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Blue); }
        void circlePurple() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Purple); }
        void circleBlack() { Broodwar->drawCircleMap(position, unitType.width(), Colors::Black); }

        bool isHovering() { return unitType.isWorker() || unitType == UnitTypes::Protoss_Archon || unitType == UnitTypes::Protoss_Dark_Archon || unitType == UnitTypes::Terran_Vulture; }
        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool sameTile() { return lastTile == tilePosition; }

        bool hasResource() { return resource != nullptr; }
        bool hasTransport() { return transport != nullptr; }
        bool hasTarget() { return target != nullptr; }
        bool command(BWAPI::UnitCommandType, BWAPI::Position);
        bool command(BWAPI::UnitCommandType, UnitInfo*);

        ResourceInfo &getResource() { return *resource; }
        UnitInfo &getTransport() { return *transport; }
        UnitInfo &getTarget() { return *target; }
        McRave::Role getRole() { return role; }

        Unit unit() { return thisUnit; }
        UnitType getType() { return unitType; }
        UnitType getBuildingType() { return buildingType; }
        Player getPlayer() { return player; }

        Position getPosition() { return position; }
        Position getEngagePosition() { return engagePosition; }
        Position getDestination() { return destination; }
        Position getLastPosition() { return lastPos; }
        Position getSimPosition() { return simPosition; }
        WalkPosition getWalkPosition() { return walkPosition; }
        WalkPosition getLastWalk() { return lastWalk; }
        TilePosition getTilePosition() { return tilePosition; }
        TilePosition getBuildPosition() { return buildPosition; }
        TilePosition getLastTile() { return lastTile; }

        BWEB::PathFinding::Path& getPath() { return path; }

        void updateUnit();
        void createDummy(UnitType);

        void setEngDist(double newValue) { engageDist = newValue; }
        void setSimValue(double newValue) { simValue = newValue; }

        void setLastAttackFrame(int newValue) { lastAttackFrame = newValue; }
        void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }
        void setCreationFrame() { frameCreated = Broodwar->getFrameCount(); }
        void setKillCount(int newValue) { killCount = newValue; }

        void setTransportState(TransportState newState) { tState = newState; }
        void setSimState(SimState newState) { sState = newState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        void setLocalState(LocalState newState) { lState = newState; }

        void setResource(ResourceInfo * unit) { resource = unit; }
        void setTransport(UnitInfo * unit) { transport = unit; }
        void setTarget(UnitInfo * unit) { target = unit; }
        void setRole(McRave::Role newRole) { role = newRole; }

        void setUnit(Unit newUnit) { thisUnit = newUnit; }
        void setType(UnitType newType) { unitType = newType; }
        void setBuildingType(UnitType newType) { buildingType = newType; }
        void setPlayer(Player newPlayer) { player = newPlayer; }

        void setSimPosition(Position newPosition) { simPosition = newPosition; }
        void setPosition(Position newPosition) { position = newPosition; }
        void setEngagePosition(Position newPosition) { engagePosition = newPosition; }
        void setDestination(Position newPosition) { destination = newPosition; }
        void setWalkPosition(WalkPosition newPosition) { walkPosition = newPosition; }
        void setTilePosition(TilePosition newPosition) { tilePosition = newPosition; }
        void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }

        void setPath(BWEB::PathFinding::Path& newPath) { path = newPath; }
        void setLastPositions();
        void incrementBeingAttackedCount() { beingAttackedCount++; }
    };
}
