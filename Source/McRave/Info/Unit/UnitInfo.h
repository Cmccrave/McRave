#pragma once
#include <BWAPI.h>

#include "BWEB.h"
#include "Info/Player/Players.h"
#include "Info/Resource/ResourceInfo.h"
#include "Info/Unit/UnitFrames.h"
#include "Info/Unit/UnitMath.h"
#include "Main/Common.h"
#include "Main/Helpers.h"
#include "UnitData.h"
#include "UnitHelpers.h"
#include "UnitHistory.h"
#include "UnitMath.h"

namespace McRave {

    class UnitInfo : public std::enable_shared_from_this<UnitInfo>, //
                     public UnitFrames,                             //
                     public UnitData,                               //
                     public UnitHelpers,                            //
                     public UnitHistory                             //
    {

        double engageDist    = 0.0;
        double simValue      = 0.0;
        double engageRadius  = 0.0;
        double retreatRadius = 0.0;
        double currentSpeed  = 0.0;

        bool cloaked           = false;
        bool stunned           = false;
        bool proxy             = false;
        bool completed         = false;
        bool burrowed          = false;
        bool flying            = false;
        bool threatening       = false;
        bool hidden            = false;
        bool nearSplash        = false;
        bool nearSuicide       = false;
        bool nearHidden        = false;
        bool targetedBySplash  = false;
        bool targetedBySuicide = false;
        bool targetedByHidden  = false;
        bool markedForDeath    = false;
        bool invincible        = false;
        std::weak_ptr<UnitInfo> transport;
        std::weak_ptr<UnitInfo> target_;
        std::weak_ptr<UnitInfo> commander;
        std::weak_ptr<UnitInfo> simTarget;
        std::weak_ptr<ResourceInfo> resource;

        std::vector<std::weak_ptr<UnitInfo>> assignedCargo;

        std::vector<std::weak_ptr<UnitInfo>> unitsTargetingThis;
        std::vector<std::weak_ptr<UnitInfo>> unitsInReachOfThis;
        std::vector<std::weak_ptr<UnitInfo>> unitsInRangeOfThis;
        std::set<BWAPI::UnitType> typesTargetingThis;
        std::set<BWAPI::UnitType> typesReachingThis;
        std::set<BWAPI::UnitType> typesRangingThis;

        TransportState tState = TransportState::None;
        LocalState lState     = LocalState::None;
        GlobalState gState    = GlobalState::None;
        SimState sState       = SimState::None;
        Role role             = Role::None;
        GoalType gType        = GoalType::None;
        Player player         = nullptr;
        Unit bwUnit           = nullptr;
        UnitType type         = UnitTypes::None;
        UnitType buildingType = UnitTypes::None;

        Position position           = Positions::Invalid;
        Position engagePosition     = Positions::Invalid;
        Position destination        = Positions::Invalid;
        Position formation          = Positions::Invalid;
        Position navigation         = Positions::Invalid;
        Position goal               = Positions::Invalid;
        Position commandPosition    = Positions::Invalid;
        Position surroundPosition   = Positions::Invalid;
        Position interceptPosition  = Positions::Invalid;
        Position trapPosition       = Positions::Invalid;
        Position facingPosition     = Positions::Invalid;
        WalkPosition walkPosition   = WalkPositions::Invalid;
        TilePosition tilePosition   = TilePositions::Invalid;
        TilePosition buildPosition  = TilePositions::Invalid;
        UnitCommandType commandType = UnitCommandTypes::None;

        BWEB::Path marchPath;
        BWEB::Path retreatPath;
        void updateHistory();
        void updateStatistics();
        void updateEvents();
        void checkStuck();
        void checkHidden();
        void checkThreatening();
        void checkProxy();
        void checkCompletion();

    public:
        UnitInfo();

        UnitInfo(Unit u) { bwUnit = u; }

        void update();

        // HACK: Hacky stuff that was added quickly for testing
        bool movedFlag = false;
        bool saveUnit  = false;
        bool inDanger  = false;
        bool sacrifice = false;

        int debugNum = 0;
        bool debugFlag = false;

        int lastCommandFrame     = 0;
        int nextCommandFrame     = 0;
        int lastThreateningFrame = -999;
        int framesVisible        = -999;
        int framesCommitted      = 0;
        bool sharedCommand       = false;

        bool isValid() { return unit() && unit()->exists(); }
        bool isAvailable() { return !unit()->isLockedDown() && !unit()->isMaelstrommed() && !unit()->isStasised() && unit()->isCompleted(); }

        Position retreatPos = Positions::Invalid;
        Position marchPos   = Positions::Invalid;

        bool hasResource() { return !resource.expired(); }
        bool hasTransport() { return !transport.expired(); }
        bool hasTarget() { return !target_.expired(); }
        bool hasCommander() { return !commander.expired(); }
        bool hasSimTarget() { return !simTarget.expired(); }

        void addTargeter(UnitInfo &targeter)
        {
            unitsTargetingThis.push_back(targeter.weak_from_this());
            typesTargetingThis.insert(targeter.getType());
        }

        bool hasSameMarchPath(Position source, Position target) { return marchPath.getSource() == TilePosition(source) && marchPath.getTarget() == TilePosition(target); }
        bool hasSameRetreatPath(Position source, Position target) { return retreatPath.getSource() == TilePosition(source) && retreatPath.getTarget() == TilePosition(target); }
        bool targetsFriendly() { return (type == UnitTypes::Terran_Medic && getEnergy() > 0) || type == UnitTypes::Terran_Science_Vessel || (type == UnitTypes::Zerg_Defiler && getEnergy() < 100); }

        bool isSuicidal() { return type == UnitTypes::Protoss_Scarab || type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Zerg_Scourge || type == UnitTypes::Zerg_Infested_Terran; }
        bool isSplasher()
        {
            return type == UnitTypes::Protoss_Reaver || type == UnitTypes::Protoss_High_Templar || type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Protoss_Archon ||
                   type == UnitTypes::Protoss_Corsair || type == UnitTypes::Terran_Valkyrie || type == UnitTypes::Zerg_Devourer;
        }
        bool isLightAir() { return type == UnitTypes::Protoss_Corsair || type == UnitTypes::Protoss_Scout || type == UnitTypes::Zerg_Mutalisk || type == UnitTypes::Terran_Wraith; }
        bool isToken() { return type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Protoss_Scarab || type == UnitTypes::Protoss_Interceptor; }
        bool isCapitalShip() { return type == UnitTypes::Protoss_Carrier || type == UnitTypes::Terran_Battlecruiser || type == UnitTypes::Zerg_Guardian; }
        bool isHovering() { return type.isWorker() || type == UnitTypes::Protoss_Archon || type == UnitTypes::Protoss_Dark_Archon || type == UnitTypes::Terran_Vulture; }
        bool isTransport() { return type == UnitTypes::Protoss_Shuttle || type == UnitTypes::Terran_Dropship || type == UnitTypes::Zerg_Overlord; }
        bool isSpellcaster()
        {
            return type == UnitTypes::Protoss_High_Templar || type == UnitTypes::Protoss_Dark_Archon || type == UnitTypes::Terran_Medic || type == UnitTypes::Terran_Science_Vessel ||
                   type == UnitTypes::Zerg_Defiler;
        }
        bool isSiegeTank() { return type == UnitTypes::Terran_Siege_Tank_Siege_Mode || type == UnitTypes::Terran_Siege_Tank_Tank_Mode; }

        bool isCommandable();

        bool canMirrorCommander(UnitInfo &otherUnit)
        {
            return gState != GlobalState::Retreat && !unit()->isIrradiated() && !isNearSuicide() && !attemptingRegroup() && !attemptingAvoidance() &&
                   (getType() == otherUnit.getType() || lState != LocalState::Attack);
        }

        bool isHealthy();
        bool isRequestingPickup();
        bool isWithinEngage(UnitInfo &);
        bool isWithinReach(UnitInfo &);
        bool isWithinRange(UnitInfo &);
        bool isWithinAngle(UnitInfo &);
        bool isWithinBuildRange();
        bool isWithinGatherRange();
        bool canStartAttack();
        bool canStartCast(TechType, Position);
        bool canStartCast(TechType, UnitInfo &);
        bool canStartGather();
        bool canAttackGround();
        bool canAttackAir();
        bool canAttack(UnitInfo &);

        double getDpsAgainst(UnitInfo &);

        // General commands that verify we aren't spamming the same command and sticking the unit
        void setCommand(UnitCommandType, Position);
        void setCommand(UnitCommandType, UnitInfo &);
        void setCommand(UnitCommandType);
        void setCommand(TechType, Position);
        void setCommand(TechType, UnitInfo &);
        void setCommand(TechType);
        Position getCommandPosition() { return commandPosition; }
        UnitCommandType getCommandType() { return commandType; }

        // Debug text
        std::string commandText;

        bool attemptingRunby();
        bool attemptingSurround();
        bool attemptingTrap();
        bool attemptingIntercept();
        bool attemptingHarass();
        bool attemptingRegroup();
        bool attemptingAvoidance();

        std::vector<std::weak_ptr<UnitInfo>> &getAssignedCargo() { return assignedCargo; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsTargetingThis() { return unitsTargetingThis; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInReachOfThis() { return unitsInReachOfThis; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInRangeOfThis() { return unitsInRangeOfThis; }
        std::map<int, Position> &getPositionHistory() { return positionHistory; }
        std::map<int, UnitCommandType> &getCommandHistory() { return commandHistory; }
        std::map<int, std::pair<Order, Position>> &getOrderHistory() { return orderHistory; }

        TransportState getTransportState() { return tState; }
        SimState getSimState() { return sState; }
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }

        std::weak_ptr<ResourceInfo> getResource() { return resource; }
        std::weak_ptr<UnitInfo> getTransport() { return transport; }
        std::weak_ptr<UnitInfo> getTarget() { return target_; }
        std::weak_ptr<UnitInfo> getCommander() { return commander; }
        std::weak_ptr<UnitInfo> getSimTarget() { return simTarget; }

        Role getRole() { return role; }
        GoalType getGoalType() { return gType; }
        Unit &unit() { return bwUnit; }
        UnitType getType() { return type; }
        UnitType getBuildType() { return buildingType; }
        Player getPlayer() { return player; }
        Position getPosition() { return position; }
        Position getEngagePosition() { return engagePosition; }
        Position getDestination() { return destination; }
        Position getFormation() { return formation; }
        Position getNavigation() { return navigation; }
        Position getGoal() { return goal; }
        Position getInterceptPosition() { return interceptPosition; }
        Position getSurroundPosition() { return surroundPosition; }
        Position getTrapPosition() { return trapPosition; }
        Position getFacingPosition() { return facingPosition; }
        WalkPosition getWalkPosition() { return walkPosition; }
        TilePosition getTilePosition() { return tilePosition; }
        TilePosition getBuildPosition() { return buildPosition; }
        BWEB::Path &getMarchPath() { return marchPath; }
        BWEB::Path &getRetreatPath() { return retreatPath; }

        double getCurrentSpeed() { return currentSpeed; }
        double getEngDist() { return engageDist; }
        double getSimValue() { return simValue; }
        double getEngageRadius() { return engageRadius; }
        double getRetreatRadius() { return retreatRadius; }

        bool isMarkedForDeath() { return markedForDeath; }
        bool isProxy() { return proxy; }
        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool isThreatening() { return threatening; }
        bool isStunned() { return stunned; }
        bool isHidden() { return hidden; }
        bool isCloaked() { return cloaked; }
        bool isCompleted() { return completed; }
        bool isInvincible() { return invincible; }
        bool isNearSplash() { return nearSplash; }
        bool isNearSuicide() { return nearSuicide; }
        bool isNearHidden() { return nearHidden; }
        bool isTargetedBySplash() { return targetedBySplash; }
        bool isTargetedBySuicide() { return targetedBySuicide; }
        bool isTargetedByHidden() { return targetedByHidden; }

        bool isTargetedByType(BWAPI::UnitType type);
        bool isReachedByType(BWAPI::UnitType type);
        bool isRangedByType(BWAPI::UnitType type);

        void setAssumedLocation(Position p, WalkPosition w, TilePosition t)
        {
            position     = p;
            walkPosition = w;
            tilePosition = t;
        }
        void setMarkForDeath(bool newValue) { markedForDeath = newValue; }
        void setEngDist(double newValue) { engageDist = newValue; }
        void setSimValue(double newValue) { simValue = newValue; }
        void setTransportState(TransportState newState) { tState = newState; }
        void setSimState(SimState newState) { sState = newState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        void setLocalState(LocalState newState) { lState = newState; }
        void setResource(ResourceInfo *unit);
        void setTransport(UnitInfo *unit) { unit ? transport = unit->weak_from_this() : transport.reset(); }
        void setTarget(UnitInfo *unit) { unit ? target_ = unit->weak_from_this() : target_.reset(); }
        void setCommander(UnitInfo *unit) { unit ? commander = unit->weak_from_this() : commander.reset(); }
        void setSimTarget(UnitInfo *unit) { unit ? simTarget = unit->weak_from_this() : simTarget.reset(); }
        void setRole(Role newRole) { role = newRole; }
        void setGoalType(GoalType newGoalType) { gType = newGoalType; }
        void setBuildingType(UnitType newType) { buildingType = newType; }
        void setEngagePosition(Position newPosition) { engagePosition = newPosition; }
        void setDestination(Position newPosition) { destination = newPosition; }
        void setFormation(Position newPosition) { formation = newPosition; }
        void setNavigation(Position newPosition) { navigation = newPosition; }
        void setGoal(Position newPosition) { goal = newPosition; }
        void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
        void setMarchPath(BWEB::Path &newPath) { marchPath = newPath; }
        void setRetreatPath(BWEB::Path &newPath) { retreatPath = newPath; }

        void setInterceptPosition(Position p) { interceptPosition = p; }
        void setSurroundPosition(Position p) { surroundPosition = p; }
        void setTrapPosition(Position p) { trapPosition = p; }

        void circle(Color color);
        void box(Color color);

        bool operator==(const UnitInfo &other) const { return bwUnit == other.bwUnit; }

        bool operator!=(const UnitInfo &other) const { return !(*this == other); }

        bool operator<(const UnitInfo &other) const { return bwUnit < other.bwUnit; }

        bool operator<(const std::weak_ptr<UnitInfo> &other) const
        {
            if (auto ptr = other.lock()) {
                return bwUnit < ptr->bwUnit;
            }
            return false;
        }
    };

    inline bool operator==(const std::weak_ptr<UnitInfo> &a, const std::weak_ptr<UnitInfo> &b) { return !a.owner_before(b) && !b.owner_before(a); }

    inline bool operator!=(const std::weak_ptr<UnitInfo> &a, const std::weak_ptr<UnitInfo> &b) { return a.owner_before(b) || b.owner_before(a); }

    inline bool operator<(std::weak_ptr<UnitInfo>(lunit), std::weak_ptr<UnitInfo>(runit)) { return lunit.lock()->unit() < runit.lock()->unit(); }
} // namespace McRave
