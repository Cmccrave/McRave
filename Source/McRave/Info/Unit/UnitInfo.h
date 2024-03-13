#pragma once
#include <BWAPI.h>
#include "BWEB.h"
#include "UnitMath.h"

namespace McRave {

    class UnitInfo : public std::enable_shared_from_this<UnitInfo>
    {
    #pragma region UnitData

        UnitData data;

        double engageDist = 0.0;
        double simValue = 0.0;
        double engageRadius = 0.0;
        double retreatRadius = 0.0;
        double currentSpeed = 0.0;

        int lastAttackFrame = -999;
        int lastRepairFrame = -999;
        int lastVisibleFrame = -999;
        int lastMoveFrame = -999;
        int lastStuckFrame = 0;
        int lastThreateningFrame = 0;
        int lastStimFrame = 0;
        int threateningFrames = 0;
        int resourceHeldFrames = -999;
        int remainingTrainFrame = -999;
        int startedFrame = -999;
        int completeFrame = -999;
        int arriveFrame = -999;

        bool proxy = false;
        bool completed = false;
        bool burrowed = false;
        bool flying = false;
        bool threatening = false;
        bool hidden = false;
        bool nearSplash = false;
        bool nearSuicide = false;
        bool nearHidden = false;
        bool targetedBySplash = false;
        bool targetedBySuicide = false;
        bool targetedByHidden = false;
        bool markedForDeath = false;
        bool invincible = false;
        std::weak_ptr<UnitInfo> transport;
        std::weak_ptr<UnitInfo> target;
        std::weak_ptr<UnitInfo> commander;
        std::weak_ptr<UnitInfo> simTarget;
        std::weak_ptr<ResourceInfo> resource;

        std::vector<std::weak_ptr<UnitInfo>> assignedCargo;
        std::vector<std::weak_ptr<UnitInfo>> unitsTargetingThis;
        std::vector<std::weak_ptr<UnitInfo>> unitsInReachOfThis;
        std::vector<std::weak_ptr<UnitInfo>> unitsInRangeOfThis;
        TransportState tState = TransportState::None;
        LocalState lState = LocalState::None;
        GlobalState gState = GlobalState::None;
        SimState sState = SimState::None;
        Role role = Role::None;
        GoalType gType = GoalType::None;
        BWAPI::Player player = nullptr;
        BWAPI::Unit bwUnit = nullptr;
        BWAPI::UnitType type = BWAPI::UnitTypes::None;
        BWAPI::UnitType buildingType = BWAPI::UnitTypes::None;

        BWAPI::Position position = BWAPI::Positions::Invalid;
        BWAPI::Position engagePosition = BWAPI::Positions::Invalid;
        BWAPI::Position destination = BWAPI::Positions::Invalid;
        BWAPI::Position formation = BWAPI::Positions::Invalid;
        BWAPI::Position navigation = BWAPI::Positions::Invalid;
        BWAPI::Position lastPos = BWAPI::Positions::Invalid;
        BWAPI::Position goal = BWAPI::Positions::Invalid;
        BWAPI::Position commandPosition = BWAPI::Positions::Invalid;
        BWAPI::Position surroundPosition = BWAPI::Positions::Invalid;
        BWAPI::Position interceptPosition = BWAPI::Positions::Invalid;
        BWAPI::WalkPosition walkPosition = BWAPI::WalkPositions::Invalid;
        BWAPI::WalkPosition lastWalk = BWAPI::WalkPositions::Invalid;
        BWAPI::TilePosition tilePosition = BWAPI::TilePositions::Invalid;
        BWAPI::TilePosition buildPosition = BWAPI::TilePositions::Invalid;
        BWAPI::TilePosition lastTile = BWAPI::TilePositions::Invalid;
        BWAPI::UnitCommandType commandType = BWAPI::UnitCommandTypes::None;

        std::map<int, BWAPI::Position> positionHistory;
        std::map<int, BWAPI::UnitCommandType> commandHistory;
        std::map<int, std::pair<BWAPI::Order, BWAPI::Position>> orderHistory;
        BWEB::Path destinationPath;
        void updateHistory();
        void updateStatistics();
        void checkStuck();
        void checkHidden();
        void checkThreatening();
        void checkProxy();
        void checkCompletion();
    public:

        // HACK: Hacky stuff that was added quickly for testing
        bool movedFlag = false;
        bool saveUnit = false;
        bool stunned = false;

        bool isValid() { return unit() && unit()->exists(); }
        bool isAvailable() { return !unit()->isLockedDown() && !unit()->isMaelstrommed() && !unit()->isStasised() && unit()->isCompleted(); }
        UnitInfo();

        UnitInfo(BWAPI::Unit u) {
            bwUnit = u;
        }

        BWAPI::Position retreatPos = BWAPI::Positions::Invalid;
        BWAPI::Position marchPos = BWAPI::Positions::Invalid;

        bool hasResource() { return !resource.expired(); }
        bool hasTransport() { return !transport.expired(); }
        bool hasTarget() { return !target.expired(); }
        bool hasCommander() { return !commander.expired(); }
        bool hasSimTarget() { return !simTarget.expired(); }
        bool hasAttackedRecently() { return (BWAPI::Broodwar->getFrameCount() - lastAttackFrame < 120); }
        bool hasRepairedRecently() { return (BWAPI::Broodwar->getFrameCount() - lastRepairFrame < 120); }
        bool targetsFriendly() { return type == BWAPI::UnitTypes::Terran_Medic || type == BWAPI::UnitTypes::Terran_Science_Vessel || (type == BWAPI::UnitTypes::Zerg_Defiler && data.energy < 100); }

        bool isSuicidal() { return type == BWAPI::UnitTypes::Protoss_Scarab || type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || type == BWAPI::UnitTypes::Zerg_Scourge || type == BWAPI::UnitTypes::Zerg_Infested_Terran; }
        bool isSplasher() { return type == BWAPI::UnitTypes::Protoss_Reaver || type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || type == BWAPI::UnitTypes::Protoss_Archon || type == BWAPI::UnitTypes::Protoss_Corsair || type == BWAPI::UnitTypes::Terran_Valkyrie || type == BWAPI::UnitTypes::Zerg_Devourer; }
        bool isLightAir() { return type == BWAPI::UnitTypes::Protoss_Corsair || type == BWAPI::UnitTypes::Protoss_Scout || type == BWAPI::UnitTypes::Zerg_Mutalisk || type == BWAPI::UnitTypes::Terran_Wraith; }
        bool isCapitalShip() { return type == BWAPI::UnitTypes::Protoss_Carrier || type == BWAPI::UnitTypes::Terran_Battlecruiser || type == BWAPI::UnitTypes::Zerg_Guardian; }
        bool isHovering() { return type.isWorker() || type == BWAPI::UnitTypes::Protoss_Archon || type == BWAPI::UnitTypes::Protoss_Dark_Archon || type == BWAPI::UnitTypes::Terran_Vulture; }
        bool isTransport() { return type == BWAPI::UnitTypes::Protoss_Shuttle || type == BWAPI::UnitTypes::Terran_Dropship || type == BWAPI::UnitTypes::Zerg_Overlord; }
        bool isSpellcaster() { return type == BWAPI::UnitTypes::Protoss_High_Templar || type == BWAPI::UnitTypes::Protoss_Dark_Archon || type == BWAPI::UnitTypes::Terran_Medic || type == BWAPI::UnitTypes::Terran_Science_Vessel || type == BWAPI::UnitTypes::Zerg_Defiler; }
        bool isSiegeTank() { return type == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode || type == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode; }
        bool isNearMapEdge() { return tilePosition.x < 2 || tilePosition.x > BWAPI::Broodwar->mapWidth() - 2 || tilePosition.y < 2 || tilePosition.y > BWAPI::Broodwar->mapHeight() - 2; }
        bool isCompleted() { return completed; }
        bool isStimmed() { return BWAPI::Broodwar->getFrameCount() - lastStimFrame < 300; }
        bool isStuck() { return BWAPI::Broodwar->getFrameCount() - lastMoveFrame > 10; }
        bool isInvincible() { return invincible; }
        bool wasStuckRecently() { return BWAPI::Broodwar->getFrameCount() - lastStuckFrame < 240; }

        bool isHealthy();
        bool isRequestingPickup();
        bool isWithinReach(UnitInfo&);
        bool isWithinRange(UnitInfo&);
        bool isWithinAngle(UnitInfo&);
        bool isWithinBuildRange();
        bool isWithinGatherRange();
        bool canStartAttack();
        bool canStartCast(BWAPI::TechType, BWAPI::Position);
        bool canStartGather();
        bool canAttackGround();
        bool canAttackAir();
        bool canOneShot(UnitInfo&);
        bool canTwoShot(UnitInfo&);

        // General commands that verify we aren't spamming the same command and sticking the unit
        void setCommand(BWAPI::UnitCommandType, BWAPI::Position);
        void setCommand(BWAPI::UnitCommandType, UnitInfo&);
        BWAPI::Position getCommandPosition() { return commandPosition; }
        BWAPI::UnitCommandType getCommandType() { return commandType; }

        // Debug text
        void setCommandText(std::string);
        void setDestinationText(std::string);

        // Information about frame timings
        int frameStartedWhen() {
            return startedFrame;
        }
        int frameCompletesWhen() {
            return completeFrame;
        }
        int frameArrivesWhen() {
            return arriveFrame;
        }
        Time timeStartedWhen() {
            int started = frameStartedWhen();
            return Time(started);
        }
        Time timeCompletesWhen() {
            int completes = frameCompletesWhen();
            return Time(completes);
        }
        Time timeArrivesWhen() {
            int arrival = frameArrivesWhen();
            return Time(arrival);
        }

        bool attemptingRunby();
        bool attemptingSurround();
        bool attemptingHarass();
        bool attemptingRegroup();

        void update();
        void verifyPaths();

        std::vector<std::weak_ptr<UnitInfo>>& getAssignedCargo() { return assignedCargo; }
        std::vector<std::weak_ptr<UnitInfo>>& getUnitsTargetingThis() { return unitsTargetingThis; }
        std::vector<std::weak_ptr<UnitInfo>>& getUnitsInReachOfThis() { return unitsInReachOfThis; }
        std::vector<std::weak_ptr<UnitInfo>>& getUnitsInRangeOfThis() { return unitsInRangeOfThis; }
        std::map<int, BWAPI::Position>& getPositionHistory() { return positionHistory; }
        std::map<int, BWAPI::UnitCommandType>& getCommandHistory() { return commandHistory; }
        std::map<int, std::pair<BWAPI::Order, BWAPI::Position>>& getOrderHistory() { return orderHistory; }

        TransportState getTransportState() { return tState; }
        SimState getSimState() { return sState; }
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }

        std::weak_ptr<ResourceInfo> getResource() { return resource; }
        std::weak_ptr<UnitInfo> getTransport() { return transport; }
        std::weak_ptr<UnitInfo> getTarget() { return target; }
        std::weak_ptr<UnitInfo> getCommander() { return commander; }
        std::weak_ptr<UnitInfo> getSimTarget() { return simTarget; }

        Role getRole() { return role; }
        GoalType getGoalType() { return gType; }
        BWAPI::Unit& unit() { return bwUnit; }
        BWAPI::UnitType getType() { return type; }
        BWAPI::UnitType getBuildType() { return buildingType; }
        BWAPI::Player getPlayer() { return player; }
        BWAPI::Position getPosition() { return position; }
        BWAPI::Position getEngagePosition() { return engagePosition; }
        BWAPI::Position getDestination() { return destination; }
        BWAPI::Position getFormation() { return formation; }
        BWAPI::Position getNavigation() { return navigation; }
        BWAPI::Position getGoal() { return goal; }
        BWAPI::Position getInterceptPosition() { return interceptPosition; }
        BWAPI::Position getSurroundPosition() { return surroundPosition; }
        BWAPI::WalkPosition getWalkPosition() { return walkPosition; }
        BWAPI::TilePosition getTilePosition() { return tilePosition; }
        BWAPI::TilePosition getBuildPosition() { return buildPosition; }
        BWAPI::TilePosition getLastTile() { return lastTile; }
        BWEB::Path& getDestinationPath() { return destinationPath; }
        double getPercentTotal() { return data.percentTotal; }
        double getVisibleGroundStrength() { return data.visibleGroundStrength; }
        double getMaxGroundStrength() { return data.maxGroundStrength; }
        double getVisibleAirStrength() { return data.visibleAirStrength; }
        double getMaxAirStrength() { return data.maxAirStrength; }
        double getGroundRange() { return data.groundRange; }
        double getGroundReach() { return data.groundReach; }
        double getGroundDamage() { return data.groundDamage; }
        double getAirRange() { return data.airRange; }
        double getAirReach() { return data.airReach; }
        double getAirDamage() { return data.airDamage; }
        double getSpeed() { return data.speed; }
        double getCurrentSpeed() { return currentSpeed; }
        double getPriority() { return data.priority; }
        double getEngDist() { return engageDist; }
        double getSimValue() { return simValue; }
        double getEngageRadius() { return engageRadius; }
        double getRetreatRadius() { return retreatRadius; }
        int getShields() { return data.shields; }
        int getHealth() { return data.health; }
        int getLastAttackFrame() { return lastAttackFrame; }
        int getLastVisibleFrame() { return lastVisibleFrame; }
        int getEnergy() { return data.energy; }
        int getRemainingTrainFrames() { return remainingTrainFrame; }
        int framesHoldingResource() { return resourceHeldFrames; }
        int getWalkWidth() { return data.walkWidth; }
        int getWalkHeight() { return data.walkHeight; }
        bool isMarkedForDeath() { return markedForDeath; }
        bool isProxy() { return proxy; }
        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool isThreatening() { return threatening; }
        bool isHidden() { return hidden; }
        bool isNearSplash() { return nearSplash; }
        bool isNearSuicide() { return nearSuicide; }
        bool isNearHidden() { return nearHidden; }
        bool isTargetedBySplash() { return targetedBySplash; }
        bool isTargetedBySuicide() { return targetedBySuicide; }
        bool isTargetedByHidden() { return targetedByHidden; }

        void setAssumedLocation(BWAPI::Position p, BWAPI::WalkPosition w, BWAPI::TilePosition t) {
            position = p;
            walkPosition = w;
            tilePosition = t;
        }
        void setMarkForDeath(bool newValue) { markedForDeath = newValue; }
        void setEngDist(double newValue) { engageDist = newValue; }
        void setSimValue(double newValue) { simValue = newValue; }
        void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }
        void setTransportState(TransportState newState) { tState = newState; }
        void setSimState(SimState newState) { sState = newState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        void setLocalState(LocalState newState) { lState = newState; }
        void setResource(ResourceInfo* unit) { unit ? resource = unit->weak_from_this() : resource.reset(); }
        void setTransport(UnitInfo* unit) { unit ? transport = unit->weak_from_this() : transport.reset(); }
        void setTarget(UnitInfo* unit) { unit ? target = unit->weak_from_this() : target.reset(); }
        void setCommander(UnitInfo* unit) { unit ? commander = unit->weak_from_this() : commander.reset(); }
        void setSimTarget(UnitInfo* unit) { unit ? simTarget = unit->weak_from_this() : simTarget.reset(); }
        void setRole(Role newRole) { role = newRole; }
        void setGoalType(GoalType newGoalType) { gType = newGoalType; }
        void setBuildingType(BWAPI::UnitType newType) { buildingType = newType; }
        void setEngagePosition(BWAPI::Position newPosition) { engagePosition = newPosition; }
        void setDestination(BWAPI::Position newPosition) { destination = newPosition; }
        void setFormation(BWAPI::Position newPosition) { formation = newPosition; }
        void setNavigation(BWAPI::Position newPosition) { navigation = newPosition; }
        void setGoal(BWAPI::Position newPosition) { goal = newPosition; }
        void setBuildPosition(BWAPI::TilePosition newPosition) { buildPosition = newPosition; }
        void setDestinationPath(BWEB::Path& newPath) { destinationPath = newPath; }

        void setInterceptPosition(BWAPI::Position p) { interceptPosition = p; }
        void setSurroundPosition(BWAPI::Position p) { surroundPosition = p; }

        void circle(BWAPI::Color color);
        void box(BWAPI::Color color);

        bool operator== (UnitInfo& p) {
            return bwUnit == p.unit();
        }

        bool operator!= (UnitInfo& p) {
            return bwUnit != p.unit();
        }

        bool operator< (UnitInfo& p) {
            return bwUnit < p.unit();
        }

        bool operator== (std::weak_ptr<UnitInfo>(unit)) {
            return bwUnit == unit.lock()->unit();
        }

        bool operator!= (std::weak_ptr<UnitInfo>(unit)) {
            return bwUnit != unit.lock()->unit();
        }

        bool operator< (std::weak_ptr<UnitInfo>(unit)) {
            return bwUnit < unit.lock()->unit();
        }
    };

    inline bool operator== (std::weak_ptr<UnitInfo>(lunit), std::weak_ptr<UnitInfo>(runit)) {
        return lunit.lock()->unit() == runit.lock()->unit();
    }

    inline bool operator< (std::weak_ptr<UnitInfo>(lunit), std::weak_ptr<UnitInfo>(runit)) {
        return lunit.lock()->unit() < runit.lock()->unit();
    }
}
