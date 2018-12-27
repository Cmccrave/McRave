#pragma once
#include <BWAPI.h>
#include "..\BWEB\BWEB.h"
#include "McRave.h"
#include "UnitMath.h"
#include "PlayerInfo.h"
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
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
        double simBonus = 1.0;

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

        Player player = nullptr;
        Unit thisUnit = nullptr;
        UnitInfo * transport = nullptr;
        UnitInfo * target = nullptr;
        ResourceInfo * resource = nullptr;

        set<UnitInfo*> assignedCargo ={};

        TransportState tState = TransportState::None;
        LocalState lState = LocalState::None;
        GlobalState gState = GlobalState::None;
        SimState sState = SimState::None;
        Role role = Role::None;

        UnitType unitType = UnitTypes::None;
        UnitType buildingType = UnitTypes::None;

        Position position = Positions::Invalid;
        Position engagePosition = Positions::Invalid;
        Position destination = Positions::Invalid;
        Position simPosition = Positions::Invalid;
        Position lastPos = Positions::Invalid;
        WalkPosition walkPosition = WalkPositions::Invalid;
        WalkPosition lastWalk = WalkPositions::Invalid;
        TilePosition tilePosition = TilePositions::Invalid;
        TilePosition buildPosition = TilePositions::Invalid;
        TilePosition lastTile = TilePositions::Invalid;

        BWEB::PathFinding::Path path;
        BWEB::PathFinding::Path resourcePath;
        void updateTarget();
        void updateStuckCheck();
    public:
        UnitInfo();

        // General use
        void updateUnit();
        void createDummy(UnitType);
        double getDistance(UnitInfo unit) { return position.getDistance(unit.getPosition()); }

        // Use a command
        bool command(UnitCommandType, Position);
        bool command(UnitCommandType, UnitInfo*);

        // Roles
        McRave::Role getRole() { return role; }
        void setRole(McRave::Role newRole) { role = newRole; }

        // Simulation
        void setSimValue(double newValue) { simValue = newValue; }
        double getSimValue() { return simValue; }

        // Assigned Target
        bool hasTarget() { return target != nullptr; }
        UnitInfo &getTarget() { return *target; }
        void setTarget(UnitInfo * unit) { target = unit; }

        // Assigned Transport
        bool hasTransport() { return transport != nullptr; }
        UnitInfo &getTransport() { return *transport; }
        void setTransport(UnitInfo * unit) { transport = unit; }

        // Assigned Resource
        bool hasResource() { return resource != nullptr; }
        ResourceInfo &getResource() { return *resource; }
        void setResource(ResourceInfo * unit) { resource = unit; }

        // Assigned Cargo
        set<UnitInfo*>& getAssignedCargo() { return assignedCargo; }

        // States
        TransportState getTransportState() { return tState; }
        void setTransportState(TransportState newState) { tState = newState; }
        SimState getSimState() { return sState; }
        void setSimState(SimState newState) { sState = newState; }
        GlobalState getGlobalState() { return gState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        LocalState getLocalState() { return lState; }
        void setLocalState(LocalState newState) { lState = newState; }

        // Holding resource
        int framesHoldingResource() { return resourceHeldFrames; }

        // How many units are targeting this unit
        int getUnitsAttacking() { return beingAttackedCount; }
        void incrementBeingAttackedCount() { beingAttackedCount++; }

        // Engagement distances
        double getEngDist() { return engageDist; }
        void setEngDist(double newValue) { engageDist = newValue; }

        // Last positions
        Position getLastPosition() { return lastPos; }
        WalkPosition getLastWalk() { return lastWalk; }
        TilePosition getLastTile() { return lastTile; }
        bool sameTile() { return lastTile == tilePosition; }
        void setLastPositions();

        // Target path
        void setPath(BWEB::PathFinding::Path& newPath) { path = newPath; }
        BWEB::PathFinding::Path& getPath() { return path; }
        bool samePath() {
            return (path.getTiles().front() == target->getTilePosition() && path.getTiles().back() == tilePosition);
        }

        // Attack frame
        bool hasAttackedRecently() {
            return (Broodwar->getFrameCount() - lastAttackFrame < 50);
        }

        // Training frame
        void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }
        int getRemainingTrainFrames() { return remainingTrainFrame; }

        // Stuck
        bool isStuck() {
            return (Broodwar->getFrameCount() - lastMoveFrame > 50);
        }

        // Creation frame
        void setCreationFrame() { frameCreated = Broodwar->getFrameCount(); }
        int getFrameCreated() { return frameCreated; }

        // Starcraft Stats
        double getPercentHealth() { return percentHealth; }				// Returns the units health percentage
        double getPercentShield() { return percentShield; }
        double getPercentTotal() { return percentTotal; }
        double getVisibleGroundStrength() { return visibleGroundStrength; }		// Returns the units visible ground strength		
        double getMaxGroundStrength() { return maxGroundStrength; }			// Returns the units max ground strength		
        double getVisibleAirStrength() { return visibleAirStrength; }			// Returns the units visible air strength		
        double getMaxAirStrength() { return maxAirStrength; }				// Returns the units max air strength
        double getGroundRange() { return groundRange; }					// Returns the units ground range including upgrades
        double getGroundReach() { return groundReach; }
        double getGroundDamage() { return groundDamage; }				// Returns the units ground damage (not including most upgrades)	
        double getAirRange() { return airRange; }					// Returns the units air range including upgrades	
        double getAirReach() { return airReach; }
        double getAirDamage() { return airDamage; }					// Returns the units air damage (not including most upgrades)		
        double getSpeed() { return speed; }						// Returns the units movement speed in pixels per frame including upgrades	
        int getShields() { return shields; }
        int getHealth() { return health; }
        int getLastAttackFrame() { return lastAttackFrame; }				// Returns the frame on which isStartingAttack was last true		
        int getMinStopFrame() { return minStopFrame; }				// Returns the minimum number of frames that the unit needs after a shot before another command can be issued		
        int getLastVisibleFrame() { return lastVisibleFrame; }			// Returns the last frame since this unit was visible
        int getEnergy() { return energy; }

        // McRave Stats
        double getPriority() { return priority; }					// Returns the units priority for targeting purposes based on strength (not including value)		
        double getSimBonus() { return simBonus; }

        // Kill count
        int getKillCount() { return killCount; }
        void setKillCount(int newValue) { killCount = newValue; }

        // Debug circles
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
        Position getSimPosition() { return simPosition; }
        Unit unit() { return thisUnit; }
        UnitType getType() { return unitType; }
        UnitType getBuildingType() { return buildingType; }
        Player getPlayer() { return player; }

        Position getPosition() { return position; }
        Position getEngagePosition() { return engagePosition; }
        Position getDestination() { return destination; }
        WalkPosition getWalkPosition() { return walkPosition; }
        TilePosition getTilePosition() { return tilePosition; }
        TilePosition getBuildPosition() { return buildPosition; }

        void setSimBonus(double newValue) { simBonus = newValue; }
        void setLastAttackFrame(int newValue) { lastAttackFrame = newValue; }

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
    };
}
