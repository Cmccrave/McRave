#pragma once
#include <BWAPI.h>
#include "BWEB.h"

namespace McRave {

    class UnitInfo : public std::enable_shared_from_this<UnitInfo> {
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
        int shields = 0;
        int health = 0;
        int minStopFrame = 0;
        int energy = 0;

        bool burrowed = false;
        bool flying = false;

        BWAPI::Player player = nullptr;
        BWAPI::Unit thisUnit = nullptr;
        std::weak_ptr<UnitInfo> transport;
        std::weak_ptr<UnitInfo> target;
        std::weak_ptr<ResourceInfo> resource;

        std::vector<std::weak_ptr<UnitInfo>> assignedCargo;
        std::vector<std::weak_ptr<UnitInfo>> targetedBy;

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

        BWEB::Path attackPath;
        BWEB::Path retreatPath;
        BWEM::CPPath quickPath;

        void updateTarget();
        void updateStuckCheck();
    public:
        UnitInfo();

    #pragma region Utility
        bool hasResource() { return !resource.expired(); }
        bool hasTransport() { return !transport.expired(); }
        bool hasTarget() { return !target.expired(); }
        bool hasMoved() { return lastTile != tilePosition; }
        bool hasAttackedRecently() { return (BWAPI::Broodwar->getFrameCount() - lastAttackFrame < 50); }
        bool targetsFriendly() { return unitType == BWAPI::UnitTypes::Terran_Medic || unitType == BWAPI::UnitTypes::Terran_Science_Vessel || unitType == BWAPI::UnitTypes::Zerg_Defiler; }
        bool isStuck() { return (BWAPI::Broodwar->getFrameCount() - lastMoveFrame > 50); }        
        bool isSuicidal() { return unitType == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || unitType == BWAPI::UnitTypes::Zerg_Scourge || unitType == BWAPI::UnitTypes::Zerg_Infested_Terran; }
        bool isLightAir() { return unitType == BWAPI::UnitTypes::Protoss_Corsair || unitType == BWAPI::UnitTypes::Zerg_Mutalisk || unitType == BWAPI::UnitTypes::Terran_Wraith; }
        bool isCapitalShip() { return unitType == BWAPI::UnitTypes::Protoss_Carrier || unitType == BWAPI::UnitTypes::Terran_Battlecruiser || unitType == BWAPI::UnitTypes::Zerg_Guardian; }
        bool isHovering() { return unitType.isWorker() || unitType == BWAPI::UnitTypes::Protoss_Archon || unitType == BWAPI::UnitTypes::Protoss_Dark_Archon || unitType == BWAPI::UnitTypes::Terran_Vulture; }
        bool isTransport() { return unitType == BWAPI::UnitTypes::Protoss_Shuttle || unitType == BWAPI::UnitTypes::Terran_Dropship || unitType == BWAPI::UnitTypes::Zerg_Overlord; }
        bool isSpellcaster() { return unitType == BWAPI::UnitTypes::Protoss_High_Templar || unitType == BWAPI::UnitTypes::Protoss_Dark_Archon || unitType == BWAPI::UnitTypes::Terran_Medic || unitType == BWAPI::UnitTypes::Terran_Science_Vessel; }
        bool isSiegeTank() { return unitType == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode || unitType == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode; }
        bool isBurrowed() { return burrowed; }
        bool isFlying() { return flying; }
        bool isWithinReach(UnitInfo&);
        bool isWithinRange(UnitInfo&);
        bool isThreatening();

        bool isHealthy() {
            return (unitType.maxShields() > 0 && percentShield > LOW_SHIELD_PERCENT_LIMIT)
                || (unitType.isMechanical() && percentHealth > LOW_MECH_PERCENT_LIMIT);
        }
        bool isHidden() {
            auto detection = player->isEnemy(BWAPI::Broodwar->self()) ? Command::overlapsDetection(thisUnit, position, PlayerState::Self) : Command::overlapsDetection(thisUnit, position, PlayerState::Enemy);
            return (burrowed || (thisUnit->exists() && thisUnit->isCloaked())) && !detection;
        }        

        bool canStartAttack();
        bool canStartCast(BWAPI::TechType);
        bool canAttackGround();
        bool canAttackAir();

        template <typename P>
        bool canCreateAttackPath(P h) {
            BWAPI::TilePosition here(h);

            const auto shouldCreatePath =
                (attackPath.getTiles().empty() || attackPath.getTiles().front() != here || attackPath.getTiles().back() != this->getTilePosition());    // ...attack path is empty or not the same

            const auto canCreatePath =
                (!this->hasTransport()																							                        // ...unit has no transport
                    && this->hasTarget()                                                                                                                // ...unit has a target
                    && this->getPosition().isValid() && here.isValid()								                                                    // ...both TilePositions are valid
                    && !this->getType().isFlyer() && !this->getTarget().getType().isFlyer()											                    // ...neither units are flyers
                    && BWEB::Map::isUsed(here) == BWAPI::UnitTypes::None && BWEB::Map::isUsed(this->getTilePosition()) == BWAPI::UnitTypes::None		// ...neither TilePositions overlap buildings
                    && BWEB::Map::isWalkable(this->getTilePosition()) && BWEB::Map::isWalkable(here));	                                                // ...both TilePositions are on walkable tiles  

            return shouldCreatePath && canCreatePath;
        }

        template <typename P>
        bool canCreateRetreatPath(P h) {
            BWAPI::TilePosition here(h);

            const auto shouldCreatePath =
                (retreatPath.getTiles().empty() || retreatPath.getTiles().front() != here || retreatPath.getTiles().back() != this->getTilePosition());    // ...retreat path is empty or not the same

            const auto canCreatePath =
                (!this->hasTransport()																							                            // ...unit has no transport
                    && this->getPosition().isValid() && here.isValid()								                                                        // ...both TilePositions are valid
                    && !this->getType().isFlyer()											                                                                // ...unit is not a flyer
                    && BWEB::Map::isUsed(here) == UnitTypes::None && BWEB::Map::isUsed(here) == UnitTypes::None		                                        // ...neither TilePositions overlap buildings
                    && BWEB::Map::isWalkable(this->getTilePosition()) && BWEB::Map::isWalkable(here));	                                                    // ...both TilePositions are on walkable tiles   

            return shouldCreatePath && canCreatePath;
        }

        // Commanding the unit to prevent order/command spam
        bool command(BWAPI::UnitCommandType, BWAPI::Position, bool);
        bool command(BWAPI::UnitCommandType, UnitInfo&);

        // Information about frame timings
        int frameArrivesWhen() {
            return BWAPI::Broodwar->getFrameCount() + int(position.getDistance(Terrain::getDefendPosition()) / speed);
        }
        int frameCompletesWhen() {
            return BWAPI::Broodwar->getFrameCount() + int((1.0 - percentHealth) * double(unitType.buildTime()));
        }

        void update();
        void verifyPaths();
        void createDummy(BWAPI::UnitType);
    #pragma endregion

    #pragma region Drawing
        void circleRed() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Red); }
        void circleOrange() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Orange); }
        void circleYellow() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Yellow); }
        void circleGreen() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Green); }
        void circleBlue() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Blue); }
        void circlePurple() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Purple); }
        void circleBlack() { BWAPI::Broodwar->drawCircleMap(position, unitType.width(), BWAPI::Colors::Black); }
    #pragma endregion

    #pragma region Getters
        std::vector<std::weak_ptr<UnitInfo>>& getAssignedCargo() { return assignedCargo; }
        std::vector<std::weak_ptr<UnitInfo>>& getTargetedBy() { return targetedBy; }

        TransportState getTransportState() { return tState; }
        SimState getSimState() { return sState; }
        GlobalState getGlobalState() { return gState; }
        LocalState getLocalState() { return lState; }

        ResourceInfo &getResource() { return *resource.lock(); }
        UnitInfo &getTransport() { return *transport.lock(); }
        UnitInfo &getTarget() { return *target.lock(); }

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
        BWEB::Path& getAttackPath() { return attackPath; }
        BWEB::Path& getRetreatPath() { return retreatPath; }
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
        int getShields() { return shields; }
        int getHealth() { return health; }
        int getLastAttackFrame() { return lastAttackFrame; }
        int getMinStopFrame() { return minStopFrame; }
        int getLastVisibleFrame() { return lastVisibleFrame; }
        int getEnergy() { return energy; }
        int getRemainingTrainFrames() { return remainingTrainFrame; }
        int framesHoldingResource() { return resourceHeldFrames; }
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
        void setRole(Role newRole) { role = newRole; }
        void setUnit(BWAPI::Unit newUnit) { thisUnit = newUnit; }
        void setType(BWAPI::UnitType newType) { unitType = newType; }
        void setBuildingType(BWAPI::UnitType newType) { buildingType = newType; }
        void setSimPosition(BWAPI::Position newPosition) { simPosition = newPosition; }
        void setPosition(BWAPI::Position newPosition) { position = newPosition; }
        void setEngagePosition(BWAPI::Position newPosition) { engagePosition = newPosition; }
        void setDestination(BWAPI::Position newPosition) { destination = newPosition; }
        void setGoal(BWAPI::Position newPosition) { goal = newPosition; }
        void setWalkPosition(BWAPI::WalkPosition newPosition) { walkPosition = newPosition; }
        void setTilePosition(BWAPI::TilePosition newPosition) { tilePosition = newPosition; }
        void setBuildPosition(BWAPI::TilePosition newPosition) { buildPosition = newPosition; }
        void setAttackPath(BWEB::Path& newPath) { attackPath = newPath; }
        void setRetreatPath(BWEB::Path& newPath) { retreatPath = newPath; }
        void setQuickPath(BWEM::CPPath newPath) { quickPath = newPath; }
        void setLastPositions();
    #pragma endregion

    #pragma region Operators
        bool operator== (UnitInfo& p) {
            return thisUnit == p.unit();
        }

        bool operator!= (UnitInfo& p) {
            return thisUnit != p.unit();
        }

        bool operator< (UnitInfo& p) {
            return thisUnit < p.unit();
        }

        bool operator< (std::weak_ptr<UnitInfo>(unit)) {
            return thisUnit < unit.lock()->unit();
        }
    #pragma endregion
    };
}
