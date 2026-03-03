#pragma once
#include <BWAPI.h>

#include "BWEB.h"
#include "Info/Player/Players.h"
#include "Info/Resource/ResourceInfo.h"
#include "Info/Unit/UnitFrames.h"
#include "Info/Unit/UnitMath.h"
#include "Main/Common.h"
#include "Main/Helpers.h"
#include "UnitMath.h"

namespace McRave {

    class UnitData {
    protected:
        double visibleGroundStrength = 0.0;
        double visibleAirStrength    = 0.0;
        double maxGroundStrength     = 0.0;
        double maxAirStrength        = 0.0;
        double percentHealth         = 0.0;
        double percentShield         = 0.0;
        double percentTotal          = 0.0;
        double groundRange           = 0.0;
        double groundReach           = 0.0;
        double groundDamage          = 0.0;
        double airRange              = 0.0;
        double airReach              = 0.0;
        double airDamage             = 0.0;
        double speed                 = 0.0;
        double priority              = 0.0;

        int shields     = 0;
        int health      = 0;
        int armor       = 0;
        int shieldArmor = 0;
        int energy      = 0;
        int walkWidth   = 0;
        int walkHeight  = 0;

    public:
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

        int getArmor() { return armor; }
        int getShields() { return shields; }
        int getHealth() { return health; }
        int getEnergy() { return energy; }
        int getWalkWidth() { return walkWidth; }
        int getWalkHeight() { return walkHeight; }

        void updateUnitData(UnitInfo &unit);
    };

    class UnitInfo : public std::enable_shared_from_this<UnitInfo>, public UnitFrames, public UnitData {
        double engageDist    = 0.0;
        double simValue      = 0.0;
        double engageRadius  = 0.0;
        double retreatRadius = 0.0;
        double currentSpeed  = 0.0;
        float threat         = 0.0f;

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
        TransportState tState        = TransportState::None;
        LocalState lState            = LocalState::None;
        GlobalState gState           = GlobalState::None;
        SimState sState              = SimState::None;
        Role role                    = Role::None;
        GoalType gType               = GoalType::None;
        BWAPI::Player player         = nullptr;
        BWAPI::Unit bwUnit           = nullptr;
        BWAPI::UnitType type         = BWAPI::UnitTypes::None;
        BWAPI::UnitType buildingType = BWAPI::UnitTypes::None;

        BWAPI::Position position           = BWAPI::Positions::Invalid;
        BWAPI::Position engagePosition     = BWAPI::Positions::Invalid;
        BWAPI::Position destination        = BWAPI::Positions::Invalid;
        BWAPI::Position formation          = BWAPI::Positions::Invalid;
        BWAPI::Position navigation         = BWAPI::Positions::Invalid;
        BWAPI::Position lastPos            = BWAPI::Positions::Invalid;
        BWAPI::Position goal               = BWAPI::Positions::Invalid;
        BWAPI::Position commandPosition    = BWAPI::Positions::Invalid;
        BWAPI::Position surroundPosition   = BWAPI::Positions::Invalid;
        BWAPI::Position interceptPosition  = BWAPI::Positions::Invalid;
        BWAPI::WalkPosition walkPosition   = BWAPI::WalkPositions::Invalid;
        BWAPI::WalkPosition lastWalk       = BWAPI::WalkPositions::Invalid;
        BWAPI::TilePosition tilePosition   = BWAPI::TilePositions::Invalid;
        BWAPI::TilePosition buildPosition  = BWAPI::TilePositions::Invalid;
        BWAPI::TilePosition lastTile       = BWAPI::TilePositions::Invalid;
        BWAPI::UnitCommandType commandType = BWAPI::UnitCommandTypes::None;

        std::map<int, BWAPI::Position> positionHistory;
        std::map<int, BWAPI::UnitCommandType> commandHistory;
        std::map<int, std::pair<BWAPI::Order, BWAPI::Position>> orderHistory;
        BWEB::Path destinationPath;
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

        UnitInfo(BWAPI::Unit u) { bwUnit = u; }

        void update();

        // HACK: Hacky stuff that was added quickly for testing
        bool movedFlag = false;
        bool saveUnit  = false;
        bool stunned   = false;
        bool cloaked   = false;
        bool inDanger  = false;
        bool sacrifice = false;

        int commandFrame         = -999;
        int lastThreateningFrame = -999;
        int framesVisible        = -999;
        int framesCommitted      = 0;
        bool sharedCommand       = false;

        bool isValid() { return unit() && unit()->exists(); }
        bool isAvailable() { return !unit()->isLockedDown() && !unit()->isMaelstrommed() && !unit()->isStasised() && unit()->isCompleted(); }

        BWAPI::Position retreatPos = BWAPI::Positions::Invalid;
        BWAPI::Position marchPos   = BWAPI::Positions::Invalid;

        bool hasResource() { return !resource.expired(); }
        bool hasTransport() { return !transport.expired(); }
        bool hasTarget() { return !target_.expired(); }
        bool hasCommander() { return !commander.expired(); }
        bool hasSimTarget() { return !simTarget.expired(); }
        bool hasSamePath(BWAPI::Position source, BWAPI::Position target)
        {
            return destinationPath.getSource() == BWAPI::TilePosition(source) && destinationPath.getTarget() == BWAPI::TilePosition(target);
        }
        bool targetsFriendly();

        bool isSuicidal()
        {
            return type == BWAPI::UnitTypes::Protoss_Scarab || type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || type == BWAPI::UnitTypes::Zerg_Scourge ||
                   type == BWAPI::UnitTypes::Zerg_Infested_Terran;
        }
        bool isSplasher()
        {
            return type == BWAPI::UnitTypes::Protoss_Reaver || type == BWAPI::UnitTypes::Protoss_High_Templar || type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine ||
                   type == BWAPI::UnitTypes::Protoss_Archon || type == BWAPI::UnitTypes::Protoss_Corsair || type == BWAPI::UnitTypes::Terran_Valkyrie || type == BWAPI::UnitTypes::Zerg_Devourer;
        }
        bool isLightAir()
        {
            return type == BWAPI::UnitTypes::Protoss_Corsair || type == BWAPI::UnitTypes::Protoss_Scout || type == BWAPI::UnitTypes::Zerg_Mutalisk || type == BWAPI::UnitTypes::Terran_Wraith;
        }
        bool isToken() { return type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || type == BWAPI::UnitTypes::Protoss_Scarab || type == BWAPI::UnitTypes::Protoss_Interceptor; }
        bool isCapitalShip() { return type == BWAPI::UnitTypes::Protoss_Carrier || type == BWAPI::UnitTypes::Terran_Battlecruiser || type == BWAPI::UnitTypes::Zerg_Guardian; }
        bool isHovering() { return type.isWorker() || type == BWAPI::UnitTypes::Protoss_Archon || type == BWAPI::UnitTypes::Protoss_Dark_Archon || type == BWAPI::UnitTypes::Terran_Vulture; }
        bool isTransport() { return type == BWAPI::UnitTypes::Protoss_Shuttle || type == BWAPI::UnitTypes::Terran_Dropship || type == BWAPI::UnitTypes::Zerg_Overlord; }
        bool isSpellcaster()
        {
            return type == BWAPI::UnitTypes::Protoss_High_Templar || type == BWAPI::UnitTypes::Protoss_Dark_Archon || type == BWAPI::UnitTypes::Terran_Medic ||
                   type == BWAPI::UnitTypes::Terran_Science_Vessel || type == BWAPI::UnitTypes::Zerg_Defiler;
        }
        bool isSiegeTank() { return type == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode || type == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode; }
        bool isCompleted() { return completed; }
        bool isMelee() { return groundDamage > 0.0 && groundRange < 64.0; }
        bool isInvincible() { return invincible; }
        bool isCommandable();

        bool canMirrorCommander(UnitInfo &otherUnit)
        {
            return gState != GlobalState::Retreat && !isNearSuicide() && !isTargetedBySplash() && !attemptingRegroup() && (getType() == otherUnit.getType() || lState != LocalState::Attack);
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
        bool canStartCast(BWAPI::TechType, BWAPI::Position);
        bool canStartGather();
        bool canAttackGround();
        bool canAttackAir();

        double getDpsAgainst(UnitInfo &);

        // General commands that verify we aren't spamming the same command and sticking the unit
        void setCommand(BWAPI::UnitCommandType, BWAPI::Position);
        void setCommand(BWAPI::UnitCommandType, UnitInfo &);
        void setCommand(BWAPI::UnitCommandType);
        void setCommand(BWAPI::TechType, BWAPI::Position);
        void setCommand(BWAPI::TechType, UnitInfo &);
        void setCommand(BWAPI::TechType);
        BWAPI::Position getCommandPosition() { return commandPosition; }
        BWAPI::UnitCommandType getCommandType() { return commandType; }

        // Debug text
        std::string commandText;

        bool attemptingRunby();
        bool attemptingSurround();
        bool attemptingHarass();
        bool attemptingRegroup();

        std::vector<std::weak_ptr<UnitInfo>> &getAssignedCargo() { return assignedCargo; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsTargetingThis() { return unitsTargetingThis; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInReachOfThis() { return unitsInReachOfThis; }
        std::vector<std::weak_ptr<UnitInfo>> &getUnitsInRangeOfThis() { return unitsInRangeOfThis; }
        std::map<int, BWAPI::Position> &getPositionHistory() { return positionHistory; }
        std::map<int, BWAPI::UnitCommandType> &getCommandHistory() { return commandHistory; }
        std::map<int, std::pair<BWAPI::Order, BWAPI::Position>> &getOrderHistory() { return orderHistory; }

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
        BWAPI::Unit &unit() { return bwUnit; }
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
        BWEB::Path &getDestinationPath() { return destinationPath; }

        double getCurrentSpeed() { return currentSpeed; }
        double getEngDist() { return engageDist; }
        double getSimValue() { return simValue; }
        double getEngageRadius() { return engageRadius; }
        double getRetreatRadius() { return retreatRadius; }
        float getThreatAtUnit() { return threat; }

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

        void setAssumedLocation(BWAPI::Position p, BWAPI::WalkPosition w, BWAPI::TilePosition t)
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
        void setBuildingType(BWAPI::UnitType newType) { buildingType = newType; }
        void setEngagePosition(BWAPI::Position newPosition) { engagePosition = newPosition; }
        void setDestination(BWAPI::Position newPosition) { destination = newPosition; }
        void setFormation(BWAPI::Position newPosition) { formation = newPosition; }
        void setNavigation(BWAPI::Position newPosition) { navigation = newPosition; }
        void setGoal(BWAPI::Position newPosition) { goal = newPosition; }
        void setBuildPosition(BWAPI::TilePosition newPosition) { buildPosition = newPosition; }
        void setDestinationPath(BWEB::Path &newPath) { destinationPath = newPath; }

        void setInterceptPosition(BWAPI::Position p) { interceptPosition = p; }
        void setSurroundPosition(BWAPI::Position p) { surroundPosition = p; }

        void circle(BWAPI::Color color);
        void box(BWAPI::Color color);

        bool operator==(const UnitInfo &other) const { return bwUnit == other.bwUnit; }

        bool operator!=(const UnitInfo &other) const { return !(*this == other); }

        bool operator<(const UnitInfo &other) const { return bwUnit < other.bwUnit; }

        bool operator==(const std::weak_ptr<UnitInfo> &other) const
        {
            if (auto ptr = other.lock()) {
                return bwUnit == ptr->bwUnit;
            }
            return false;
        }

        bool operator!=(const std::weak_ptr<UnitInfo> &other) const { return !(*this == other); }

        bool operator<(const std::weak_ptr<UnitInfo> &other) const
        {
            if (auto ptr = other.lock()) {
                return bwUnit < ptr->bwUnit;
            }
            return false;
        }
    };

    inline bool operator==(std::weak_ptr<UnitInfo>(lunit), std::weak_ptr<UnitInfo>(runit)) { return lunit.lock()->unit() == runit.lock()->unit(); }

    inline bool operator<(std::weak_ptr<UnitInfo>(lunit), std::weak_ptr<UnitInfo>(runit)) { return lunit.lock()->unit() < runit.lock()->unit(); }
} // namespace McRave
