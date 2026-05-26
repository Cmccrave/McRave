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
        std::vector<std::weak_ptr<UnitInfo>> unitsInEngageOfThis;
        std::vector<std::weak_ptr<UnitInfo>> unitsInReachOfThis;
        std::vector<std::weak_ptr<UnitInfo>> unitsInRangeOfThis;
        std::set<BWAPI::UnitType> typesTargetingThis;
        std::set<BWAPI::UnitType> typesEngagingThis;
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

        Position position          = Positions::Invalid;
        Position engagePosition    = Positions::Invalid;
        Position destination       = Positions::Invalid;
        Position formation         = Positions::Invalid;
        Position navigation        = Positions::Invalid;
        Position goal              = Positions::Invalid;
        Position surroundPosition  = Positions::Invalid;
        Position interceptPosition = Positions::Invalid;
        Position trapPosition      = Positions::Invalid;
        Position facingPosition    = Positions::Invalid;
        Position buildPosition     = Positions::Invalid;
        Position retreatPosition   = Positions::Invalid;
        Position marchPosition     = Positions::Invalid;
        WalkPosition walkPosition  = WalkPositions::Invalid;
        TilePosition tilePosition  = TilePositions::Invalid;
        TilePosition buildLocation = TilePositions::Invalid;

        Unit commandTarget          = nullptr;
        Position commandPosition    = Positions::Invalid;
        UnitCommandType commandType = UnitCommandTypes::None;
        int commandFrame            = 0;

        struct Blocker {
            int dx, dy;
            double dist;
        };
        std::vector<Blocker> blockers;

        BWEB::Path marchPath;
        BWEB::Path retreatPath;
        void updateHistory();
        void updateEvents();
        void updateStatistics();
        void checkStuck();
        void checkHidden();
        void checkThreatening();
        void checkProxy();
        void checkCompletion();

    public:
        UnitInfo();

        UnitInfo(Unit u) { bwUnit = u; }

        void update();
        void updateOcclusion();

        // BWAPI stuff
        // TODO: eventually remove Unit() access, cache everything important
        Unit &unit() { return bwUnit; }
        UnitType getType() { return type; }
        UnitType getBuildType() { return buildingType; }
        Player getPlayer() { return player; }

        // TODO: Fix all these
        // HACK: Hacky stuff that was added quickly for testing
        bool movedFlag = false;
        bool saveUnit  = false;
        bool inDanger  = false;
        bool sacrifice = false;
        int debugNum   = 0;
        bool debugFlag = false;
        int lastQueueFrame       = 0;
        int nextQueueFrame       = 0;
        int lastThreateningFrame = -999;
        int framesVisible        = -999;
        int framesCommitted      = 0;
        bool sharedCommand       = false;

        // Returns true if the unit is completed and not morphing
        bool isCompleted() { return completed; }

        // Returns true if unit is valid and exists
        bool isValid() { return unit() && unit()->exists(); }

        // Returns true if this unit can actually take commands
        bool isAvailable() { return !unit()->isLockedDown() && !unit()->isMaelstrommed() && !unit()->isStasised() && unit()->isCompleted(); }
        
        // Returns true for suicidal types
        bool isSuicidal() { return type == UnitTypes::Protoss_Scarab || type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Zerg_Scourge || type == UnitTypes::Zerg_Infested_Terran; }

        // Returns true for splashing types
        bool isSplasher()
        {
            return type == UnitTypes::Protoss_Reaver || type == UnitTypes::Protoss_High_Templar || type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Protoss_Archon ||
                   type == UnitTypes::Protoss_Corsair || type == UnitTypes::Terran_Valkyrie || type == UnitTypes::Zerg_Devourer;
        }

        // Returns true for fast air types
        bool isLightAir()
        {
            return type == UnitTypes::Protoss_Corsair || type == UnitTypes::Protoss_Scout || type == UnitTypes::Zerg_Mutalisk || type == UnitTypes::Terran_Wraith || type == UnitTypes::Terran_Valkyrie;
        }

        // Returns true for supplyless unit types
        bool isToken() { return type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Protoss_Scarab || type == UnitTypes::Protoss_Interceptor || type == UnitTypes::Zerg_Egg || type == UnitTypes::Zerg_Larva; }

        // Returns true for large air types
        bool isCapitalShip() { return type == UnitTypes::Protoss_Carrier || type == UnitTypes::Terran_Battlecruiser || type == UnitTypes::Zerg_Guardian; }

        // Returns true for hovering types (different movement script)
        bool isHovering() { return type.isWorker() || type == UnitTypes::Protoss_Archon || type == UnitTypes::Protoss_Dark_Archon || type == UnitTypes::Terran_Vulture; }

        // Returns true for units that can transport
        bool isTransport() { return type == UnitTypes::Protoss_Shuttle || type == UnitTypes::Terran_Dropship || type == UnitTypes::Zerg_Overlord; }

        // Returns true for pure spellcaster types
        bool isSpellcaster()
        {
            return type == UnitTypes::Protoss_High_Templar || type == UnitTypes::Protoss_Dark_Archon || type == UnitTypes::Terran_Medic || type == UnitTypes::Terran_Science_Vessel ||
                   type == UnitTypes::Zerg_Defiler;
        }

        // Returns true for both tank types
        bool isSiegeTank() { return type == UnitTypes::Terran_Siege_Tank_Siege_Mode || type == UnitTypes::Terran_Siege_Tank_Tank_Mode; }

        bool isOccluded(BWAPI::Position);
        bool isHealthy();
        bool isRequestingPickup();
        bool isWithinEngage(UnitInfo &);
        bool isWithinAngle(UnitInfo &);
        bool isWithinBuildRange();
        bool canStartAttack();
        bool canStartCast(TechType);
        bool canStartCast(TechType, Position);
        bool canStartCast(TechType, UnitInfo &);
        bool canStartGather();
        bool canAttackGround();
        bool canAttackAir();
        bool canAttack(UnitInfo &);
        bool hasCollision();

        // Commander
        std::weak_ptr<UnitInfo> getCommander() { return commander; }
        void setCommander(UnitInfo *unit) { unit ? commander = unit->weak_from_this() : commander.reset(); }
        bool hasCommander() { return !commander.expired(); }

        bool canMirrorCommander(UnitInfo &otherUnit)
        {
            return gState != GlobalState::Retreat && !unit()->isIrradiated() && !isNearSuicide() && !attemptingRegroup() && !attemptingAvoidance() &&
                   (getType() == otherUnit.getType() || lState != LocalState::Attack);
        }

        // Commands
        bool isCommandable();
        std::string commandText;
        void setCommand(UnitCommandType, Position);
        void setCommand(UnitCommandType, UnitInfo &);
        void setCommand(UnitCommandType);
        void setCommand(TechType, Position);
        void setCommand(TechType, UnitInfo &);
        void setCommand(TechType);
        Position getCommandPosition() { return commandPosition; }
        UnitCommandType getCommandType() { return commandType; }

        // Goals
        Position getGoal() { return goal; }
        GoalType getGoalType() { return gType; }
        void setGoal(Position newPosition) { goal = newPosition; }
        void setGoalType(GoalType newGoalType) { gType = newGoalType; }

        // Paths
        void setMarchPath(BWEB::Path &newPath) { marchPath = newPath; }
        void setRetreatPath(BWEB::Path &newPath) { retreatPath = newPath; }
        BWEB::Path &getMarchPath() { return marchPath; }
        BWEB::Path &getRetreatPath() { return retreatPath; }
        bool hasSameMarchPath(Position source, Position target) { return marchPath.getSource() == TilePosition(source) && marchPath.getTarget() == TilePosition(target); }
        bool hasSameRetreatPath(Position source, Position target) { return retreatPath.getSource() == TilePosition(source) && retreatPath.getTarget() == TilePosition(target); }

        // Positions
        Position getPosition() { return position; }
        Position getFacingPosition() { return facingPosition; }
        Position getEngagePosition() { return engagePosition; }
        Position getDestination() { return destination; }
        Position getFormation() { return formation; }
        Position getNavigation() { return navigation; }
        Position getBuildPosition() { return buildPosition; }
        Position getInterceptPosition() { return interceptPosition; }
        Position getSurroundPosition() { return surroundPosition; }
        Position getTrapPosition() { return trapPosition; }
        Position getMarchPosition() { return marchPosition; }
        Position getRetreatPosition() { return retreatPosition; }
        void setEngagePosition(Position newPosition) { engagePosition = newPosition; }
        void setDestination(Position newPosition) { destination = newPosition; }
        void setFormation(Position newPosition) { formation = newPosition; }
        void setNavigation(Position newPosition) { navigation = newPosition; }
        void setBuildPosition(Position newPosition) { buildPosition = newPosition; }
        void setInterceptPosition(Position p) { interceptPosition = p; }
        void setSurroundPosition(Position p) { surroundPosition = p; }
        void setTrapPosition(Position p) { trapPosition = p; }
        void setMarchPosition(Position p) { marchPosition = p; }
        void setRetreatPosition(Position p) { retreatPosition = p; }
        WalkPosition getWalkPosition() { return walkPosition; }
        TilePosition getTilePosition() { return tilePosition; }
        TilePosition getBuildLocation() { return buildLocation; }
        void setBuildLocation(TilePosition newPosition) { buildLocation = newPosition; }

        // Proximity
        bool isNearSplash() { return nearSplash; }
        bool isNearSuicide() { return nearSuicide; }
        bool isNearHidden() { return nearHidden; }

        // Range
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInRangeOfThis() { return unitsInRangeOfThis; }
        bool isRangedByType(BWAPI::UnitType type);
        bool isWithinRange(UnitInfo &);

        // Reach
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInReachOfThis() { return unitsInReachOfThis; }
        bool isReachedByType(BWAPI::UnitType type);
        bool isWithinReach(UnitInfo &);

        // Resources
        std::weak_ptr<ResourceInfo> getResource() { return resource; }
        void setResource(ResourceInfo *unit);
        bool hasResource() { return !resource.expired(); }
        bool isWithinGatherRange();

        // Roles
        Role getRole() { return role; }
        void setRole(Role newRole) { role = newRole; }

        // Simulation
        std::weak_ptr<UnitInfo> getSimTarget() { return simTarget; }
        void setSimTarget(UnitInfo *unit) { unit ? simTarget = unit->weak_from_this() : simTarget.reset(); }
        bool hasSimTarget() { return !simTarget.expired(); }
        double getSimValue() { return simValue; }
        SimState getSimState() { return sState; }
        void setSimValue(double newValue) { simValue = newValue; }
        void setSimState(SimState newState) { sState = newState; }

        // States
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        void setLocalState(LocalState newState) { lState = newState; }

        // Strategy
        bool attemptingRunby();
        bool attemptingSurround();
        bool attemptingTrap();
        bool attemptingIntercept();
        bool attemptingHarass();
        bool attemptingRegroup();
        bool attemptingAvoidance();

        // Targets
        std::shared_ptr<UnitInfo> getTarget() { return target_.lock(); }
        void setTarget(UnitInfo *unit) { unit ? target_ = unit->weak_from_this() : target_.reset(); }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsTargetingThis() { return unitsTargetingThis; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInEngageOfThis() { return unitsInEngageOfThis; }
        bool isTargetedBySplash() { return targetedBySplash; }
        bool isTargetedBySuicide() { return targetedBySuicide; }
        bool isTargetedByHidden() { return targetedByHidden; }
        bool isTargetedByType(BWAPI::UnitType type);
        bool targetsFriendly();

        void addTargeter(UnitInfo &targeter)
        {
            unitsTargetingThis.push_back(targeter.weak_from_this());
            typesTargetingThis.insert(targeter.getType());
        }

        // Transport
        std::weak_ptr<UnitInfo> getTransport() { return transport; }
        void setTransport(UnitInfo *unit) { unit ? transport = unit->weak_from_this() : transport.reset(); }
        bool hasTransport() { return !transport.expired(); }
        TransportState getTransportState() { return tState; }
        void setTransportState(TransportState newState) { tState = newState; }
        std::vector<std::weak_ptr<UnitInfo>> &getAssignedCargo() { return assignedCargo; }

        double getCurrentSpeed() { return currentSpeed; }
        double getEngDist() { return engageDist; }
        double getEngageRadius() { return engageRadius; }
        double getRetreatRadius() { return retreatRadius; }
        double getDpsAgainst(UnitInfo &);

        bool isMarkedForDeath() { return markedForDeath; }
        bool isProxy() { return proxy; }
        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool isThreatening() { return threatening; }
        bool isStunned() { return stunned; }
        bool isHidden() { return hidden; }
        bool isCloaked() { return cloaked; }
        bool isInvincible() { return invincible; }

        void setAssumedLocation(Position p, WalkPosition w, TilePosition t)
        {
            position     = p;
            walkPosition = w;
            tilePosition = t;
        }
        void setMarkForDeath(bool newValue) { markedForDeath = newValue; }
        void setEngDist(double newValue) { engageDist = newValue; }
        void setBuildingType(UnitType newType) { buildingType = newType; }

        // Visuals
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
