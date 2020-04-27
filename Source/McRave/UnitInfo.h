#pragma once
#include <BWAPI.h>
#include "BWEB.h"

namespace McRave {

    class UnitInfo : public std::enable_shared_from_this<UnitInfo>
    {
    #pragma region UnitData
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
        double simRadius = 0.0;

        int lastAttackFrame = -999;
        int lastVisibleFrame = -999;
        int lastPosMoveFrame = -999;
        int lastTileMoveFrame = -999;
        int lastThreateningFrame = 0;
        int resourceHeldFrames = -999;
        int remainingTrainFrame = 0;
        int shields = 0;
        int health = 0;
        int minStopFrame = 0;
        int energy = 0;
        int walkWidth = 0;
        int walkHeight = 0;

        bool burrowed = false;
        bool flying = false;
        bool threatening = false;
        bool hidden = false;
        bool nearSplash = false;
        bool targetedBySuicide = false;
    #pragma endregion

    #pragma region Targets
        std::weak_ptr<UnitInfo> transport;
        std::weak_ptr<UnitInfo> target;
        std::weak_ptr<UnitInfo> backupTarget;
        std::weak_ptr<ResourceInfo> resource;

        std::vector<std::weak_ptr<UnitInfo>> assignedCargo;
        std::vector<std::weak_ptr<UnitInfo>> targetedBy;
    #pragma endregion

    #pragma region States
        TransportState tState = TransportState::None;
        LocalState lState = LocalState::None;
        GlobalState gState = GlobalState::None;
        SimState sState = SimState::None;
        Role role = Role::None;
    #pragma endregion

    #pragma region BWAPIData
        BWAPI::Player player = nullptr;
        BWAPI::Unit bwUnit = nullptr;
        BWAPI::UnitType type = BWAPI::UnitTypes::None;
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
        std::map<BWAPI::TilePosition, int> lastTilesVisited;
    #pragma endregion

    #pragma region Paths
        BWEB::Path destinationPath;
        BWEB::Path targetPath;
        BWEM::CPPath quickPath;
    #pragma endregion

    #pragma region Updaters
        void updateTarget();
        void updateStuckCheck();
        void updateHidden();
        void updateThreatening();
    #pragma endregion

    public:

    #pragma region Constructors
        UnitInfo();

        UnitInfo(BWAPI::Unit u) {
            bwUnit = u;
        }
    #pragma endregion

    #pragma region Utility
        bool hasResource() { return !resource.expired(); }
        bool hasTransport() { return !transport.expired(); }
        bool hasTarget() { return !target.expired(); }
        bool hasBackupTarget() { return !backupTarget.expired(); }
        bool hasMovedArea() { return lastTile.isValid() && tilePosition.isValid() && BWEM::Map::Instance().GetArea(lastTile) != BWEM::Map::Instance().GetArea(tilePosition); }
        bool hasAttackedRecently() { return (BWAPI::Broodwar->getFrameCount() - lastAttackFrame < 50); }
        bool targetsFriendly() { return type == BWAPI::UnitTypes::Terran_Medic || type == BWAPI::UnitTypes::Terran_Science_Vessel || (type == BWAPI::UnitTypes::Zerg_Defiler && energy < 100); }
        bool isStuck() { return (BWAPI::Broodwar->getFrameCount() - lastTileMoveFrame > 12); }
        bool isSuicidal() { return type == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || type == BWAPI::UnitTypes::Zerg_Scourge || type == BWAPI::UnitTypes::Zerg_Infested_Terran; }
        bool isLightAir() { return type == BWAPI::UnitTypes::Protoss_Corsair || type == BWAPI::UnitTypes::Zerg_Mutalisk || type == BWAPI::UnitTypes::Terran_Wraith; }
        bool isCapitalShip() { return type == BWAPI::UnitTypes::Protoss_Carrier || type == BWAPI::UnitTypes::Terran_Battlecruiser || type == BWAPI::UnitTypes::Zerg_Guardian; }
        bool isHovering() { return type.isWorker() || type == BWAPI::UnitTypes::Protoss_Archon || type == BWAPI::UnitTypes::Protoss_Dark_Archon || type == BWAPI::UnitTypes::Terran_Vulture; }
        bool isTransport() { return type == BWAPI::UnitTypes::Protoss_Shuttle || type == BWAPI::UnitTypes::Terran_Dropship || type == BWAPI::UnitTypes::Zerg_Overlord; }
        bool isSpellcaster() { return type == BWAPI::UnitTypes::Protoss_High_Templar || type == BWAPI::UnitTypes::Protoss_Dark_Archon || type == BWAPI::UnitTypes::Terran_Medic || type == BWAPI::UnitTypes::Terran_Science_Vessel || type == BWAPI::UnitTypes::Zerg_Defiler; }
        bool isSiegeTank() { return type == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode || type == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode; }

        bool isHealthy();
        bool isRequestingPickup();
        bool isWithinReach(UnitInfo&);
        bool isWithinRange(UnitInfo&);
        bool isWithinBuildRange();
        bool isWithinGatherRange();
        bool canStartAttack();
        bool canStartCast(BWAPI::TechType);
        bool canStartGather();
        bool canAttackGround();
        bool canAttackAir();

        // General commands that verify we aren't spamming the same command and sticking the unit
        bool move(BWAPI::UnitCommandType, BWAPI::Position);
        bool click(BWAPI::UnitCommandType, UnitInfo&);
        bool cast(BWAPI::TechType, BWAPI::Position);

        // Information about frame timings
        int frameArrivesWhen() {
            return BWAPI::Broodwar->getFrameCount() + int(position.getDistance(Terrain::getDefendPosition()) / speed);
        }
        int frameCompletesWhen() {
            if (percentHealth >= 1.0 || bwUnit->isCompleted())
                return BWAPI::Broodwar->getFrameCount();
            auto ratio = (double(health) - (0.1 * double(type.maxHitPoints()))) / (0.9 * double(type.maxHitPoints()));
            return BWAPI::Broodwar->getFrameCount() + int(std::round((1.0 - ratio) * double(type.buildTime())));
        }
        Time timeArrivesWhen() {
            int arrival = frameArrivesWhen();
            return Time(arrival / 1440, (arrival / 24) % 60);
        }
        Time timeCompletesWhen() {
            int completes = frameCompletesWhen();
            return Time(completes / 1440, (completes / 24) % 60);
        }

        // Logic that dictates overriding simulation results
        bool localRetreat();
        bool localEngage();
        bool globalRetreat();
        bool globalEngage();

        void update();
        void verifyPaths();
        void createDummy(BWAPI::UnitType);
    #pragma endregion

    #pragma region Drawing
        void circleRed() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Red); }
        void circleOrange() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Orange); }
        void circleYellow() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Yellow); }
        void circleGreen() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Green); }
        void circleBlue() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Blue); }
        void circlePurple() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Purple); }
        void circleBlack() { BWAPI::Broodwar->drawCircleMap(position, type.width(), BWAPI::Colors::Black); }
    #pragma endregion

    #pragma region Getters
        std::vector<std::weak_ptr<UnitInfo>>& getAssignedCargo() { return assignedCargo; }
        std::vector<std::weak_ptr<UnitInfo>>& getTargetedBy() { return targetedBy; }
        std::map<BWAPI::TilePosition, int>& getLastTilesVisited() { return lastTilesVisited; }

        TransportState getTransportState() { return tState; }
        SimState getSimState() { return sState; }
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }

        ResourceInfo &getResource() { return *resource.lock(); }
        UnitInfo &getTransport() { return *transport.lock(); }
        UnitInfo &getTarget() { return *target.lock(); }
        UnitInfo &getBackupTarget() { return *backupTarget.lock(); }

        Role getRole() { return role; }
        BWAPI::Unit& unit() { return bwUnit; }
        BWAPI::UnitType getType() { return type; }
        BWAPI::UnitType getBuildType() { return buildingType; }
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
        BWEB::Path& getDestinationPath() { return destinationPath; }
        BWEB::Path& getTargetPath() { return targetPath; }
        BWEM::CPPath& getQuickPath() { return quickPath; }
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
        double getSimRadius() { return simRadius; }
        int getShields() { return shields; }
        int getHealth() { return health; }
        int getLastAttackFrame() { return lastAttackFrame; }
        int getMinStopFrame() { return minStopFrame; }
        int getLastVisibleFrame() { return lastVisibleFrame; }
        int getEnergy() { return energy; }
        int getRemainingTrainFrames() { return remainingTrainFrame; }
        int framesHoldingResource() { return resourceHeldFrames; }
        int getWalkWidth() { return walkWidth; }
        int getWalkHeight() { return walkHeight; }
        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool isThreatening() { return threatening; }
        bool isHidden() { return hidden; }
        bool isNearSplash() { return nearSplash; }
        bool isTargetedBySuicide() { return targetedBySuicide; }
    #pragma endregion      

    #pragma region Setters
        void setEngDist(double newValue) { engageDist = newValue; }
        void setSimValue(double newValue) { simValue = newValue; }
        void setLastAttackFrame(int newValue) { lastAttackFrame = newValue; }
        void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }
        void setTransportState(TransportState newState) { tState = newState; }
        void setSimState(SimState newState) { sState = newState; }
        void setGlobalState(GlobalState newState) { gState = newState; }
        void setLocalState(LocalState newState) { lState = newState; }
        void setResource(ResourceInfo* unit) { unit ? resource = unit->weak_from_this() : resource.reset(); }
        void setTransport(UnitInfo* unit) { unit ? transport = unit->weak_from_this() : transport.reset(); }
        void setTarget(UnitInfo* unit) { unit ? target = unit->weak_from_this() : target.reset(); }
        void setBackupTarget(UnitInfo* unit) { unit ? backupTarget = unit->weak_from_this() : backupTarget.reset(); }
        void setRole(Role newRole) { role = newRole; }
        void setType(BWAPI::UnitType newType) { type = newType; }
        void setBuildingType(BWAPI::UnitType newType) { buildingType = newType; }
        void setSimPosition(BWAPI::Position newPosition) { simPosition = newPosition; }
        void setPosition(BWAPI::Position newPosition) { position = newPosition; }
        void setEngagePosition(BWAPI::Position newPosition) { engagePosition = newPosition; }
        void setDestination(BWAPI::Position newPosition) { destination = newPosition; }
        void setGoal(BWAPI::Position newPosition) { goal = newPosition; }
        void setWalkPosition(BWAPI::WalkPosition newPosition) { walkPosition = newPosition; }
        void setTilePosition(BWAPI::TilePosition newPosition) { tilePosition = newPosition; }
        void setBuildPosition(BWAPI::TilePosition newPosition) { buildPosition = newPosition; }
        void setDestinationPath(BWEB::Path& newPath) { destinationPath = newPath; }
        void setTargetPath(BWEB::Path& newPath) { targetPath = newPath; }
        void setQuickPath(BWEM::CPPath newPath) { quickPath = newPath; }
        void setLastPositions();
    #pragma endregion

    #pragma region Operators
        bool operator== (UnitInfo& p) {
            return bwUnit == p.unit();
        }

        bool operator!= (UnitInfo& p) {
            return bwUnit != p.unit();
        }

        bool operator< (UnitInfo& p) {
            return bwUnit < p.unit();
        }

        bool operator< (std::weak_ptr<UnitInfo>(unit)) {
            return bwUnit < unit.lock()->unit();
        }
    #pragma endregion
    };
}
